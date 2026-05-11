/*
 * QEMU Windows Hypervisor Platform accelerator (WHPX) - ARM64
 *
 * Copyright Microsoft Corp. 2017
 * Copyright 2024 - ARM64 port
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/address-spaces.h"
#include "exec/exec-all.h"
#include "exec/ram_addr.h"
#include "exec/memory-remap.h"
#include "qemu-common.h"
#include "sysemu/accel.h"
#include "sysemu/whpx.h"
#include "sysemu/sysemu.h"
#include "sysemu/cpus.h"
#include "qemu/abort.h"
#include "qemu/main-loop.h"
#include "hw/boards.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "internals.h"

#include <windows.h>
#include <WinHvPlatform.h>

#include "whpx-arm64.h"

/* ARM64 GIC interrupt controller types (not in SDK 10.0.22621.0) */
#ifndef WHvPartitionPropertyCodeArm64IcParameters
#define WHvPartitionPropertyCodeArm64IcParameters ((WHV_PARTITION_PROPERTY_CODE)0x00001012)

typedef UINT32 WHV_ARM64_INTERRUPT_VECTOR;

typedef enum {
    WHvArm64IcEmulationModeNone = 0,
    WHvArm64IcEmulationModeGicV3 = 1
} WHV_ARM64_IC_EMULATION_MODE;

typedef struct {
    WHV_GUEST_PHYSICAL_ADDRESS GicdBaseAddress;
    WHV_GUEST_PHYSICAL_ADDRESS GitsTranslaterBaseAddress;
    UINT32 Reserved;
    UINT32 GicLpiIntIdBits;
    WHV_ARM64_INTERRUPT_VECTOR GicPpiOverflowInterruptFromCntv;
    WHV_ARM64_INTERRUPT_VECTOR GicPpiPerformanceMonitorsInterrupt;
    UINT32 Reserved1[6];
} WHV_ARM64_IC_GIC_V3_PARAMETERS;

typedef struct {
    WHV_ARM64_IC_EMULATION_MODE EmulationMode;
    UINT32 Reserved;
    union {
        WHV_ARM64_IC_GIC_V3_PARAMETERS GicV3Parameters;
    };
} WHV_ARM64_IC_PARAMETERS;
#endif

/* ==================== State ==================== */

struct whpx_state {
    uint64_t mem_quota;
    WHV_PARTITION_HANDLE partition;
};

/* ARM64 exit context — reverse-engineered from Windows 11 26200 */
typedef struct WHV_ARM64_VP_EXIT_CONTEXT {
    uint32_t ExitReason;       /* offset 0: 0x80000000 = memory access */
    uint32_t Reserved0;        /* offset 4 */
    uint32_t Reserved1[3];     /* offset 8 */
    uint32_t InstructionLength;/* offset 20: instruction length (4 for ARM64) */
    uint64_t Pc;               /* offset 24: faulting PC */
    uint64_t Pstate;           /* offset 32: PSTATE at exit */
    uint32_t Reserved2;        /* offset 40 */
    uint32_t AccessInfo;       /* offset 44: access type info (1=write) */
    uint32_t Reserved3;        /* offset 48 */
    uint32_t Reserved4;        /* offset 52 */
    uint64_t Gva;              /* offset 56: guest virtual address */
    uint64_t Gpa;              /* offset 64: guest physical address (IPA) */
    uint32_t InstructionBytes; /* offset 72: the trapped instruction */
    uint8_t  Padding[4096 - 76];
} WHV_ARM64_VP_EXIT_CONTEXT;

#define WHV_ARM64_EXIT_REASON_MEMORY_ACCESS   0x80000000
#define WHV_ARM64_EXIT_REASON_CANCELED        0x00000002
#define WHV_ARM64_EXIT_REASON_HYPERCALL       0x80000003
#define WHV_ARM64_EXIT_REASON_SYSREG_ACCESS   0x80000001

struct whpx_vcpu {
    bool interruptable;
    bool interruption_pending;
    WHV_ARM64_VP_EXIT_CONTEXT exit_ctx;
};

volatile bool whpx_allowed;
static struct whpx_state whpx_global;


/* Periodic cancel thread — forces RunVP to return so timer interrupts
 * can be checked and injected when the hypervisor doesn't deliver them. */
static HANDLE whpx_cancel_thread_handle;
static volatile LONG whpx_cancel_thread_running;
static int whpx_num_cpus;

/* IRQ level tracking — skip redundant WHvRequestInterrupt calls.
 * Index by SPI number (0-based).  Only the UART SPI fires at high
 * frequency, but track all of them for correctness. */
#define WHPX_MAX_SPI 256
static volatile LONG whpx_spi_level[WHPX_MAX_SPI];

/* Instruction fetch cache — avoid WHvTranslateGva on repeated PCs */
typedef struct {
    uint64_t pc;
    uint32_t insn;
} WhpxInsnCache;
#define WHPX_INSN_CACHE_BITS 8
#define WHPX_INSN_CACHE_SIZE (1 << WHPX_INSN_CACHE_BITS)
#define WHPX_INSN_CACHE_MASK (WHPX_INSN_CACHE_SIZE - 1)
static __declspec(thread) WhpxInsnCache insn_cache[WHPX_INSN_CACHE_SIZE];

/* Function pointers loaded from WinHvPlatform.dll */
typedef HRESULT (WINAPI *WHvGetCapability_t)(WHV_CAPABILITY_CODE, VOID*, UINT32, UINT32*);
typedef HRESULT (WINAPI *WHvCreatePartition_t)(WHV_PARTITION_HANDLE*);
typedef HRESULT (WINAPI *WHvSetupPartition_t)(WHV_PARTITION_HANDLE);
typedef HRESULT (WINAPI *WHvDeletePartition_t)(WHV_PARTITION_HANDLE);
typedef HRESULT (WINAPI *WHvGetPartitionProperty_t)(WHV_PARTITION_HANDLE, WHV_PARTITION_PROPERTY_CODE, VOID*, UINT32, UINT32*);
typedef HRESULT (WINAPI *WHvSetPartitionProperty_t)(WHV_PARTITION_HANDLE, WHV_PARTITION_PROPERTY_CODE, const VOID*, UINT32);
typedef HRESULT (WINAPI *WHvMapGpaRange_t)(WHV_PARTITION_HANDLE, VOID*, WHV_GUEST_PHYSICAL_ADDRESS, UINT64, WHV_MAP_GPA_RANGE_FLAGS);
typedef HRESULT (WINAPI *WHvUnmapGpaRange_t)(WHV_PARTITION_HANDLE, WHV_GUEST_PHYSICAL_ADDRESS, UINT64);
typedef HRESULT (WINAPI *WHvCreateVirtualProcessor_t)(WHV_PARTITION_HANDLE, UINT32, UINT32);
typedef HRESULT (WINAPI *WHvDeleteVirtualProcessor_t)(WHV_PARTITION_HANDLE, UINT32);
typedef HRESULT (WINAPI *WHvRunVirtualProcessor_t)(WHV_PARTITION_HANDLE, UINT32, VOID*, UINT32);
typedef HRESULT (WINAPI *WHvCancelRunVirtualProcessor_t)(WHV_PARTITION_HANDLE, UINT32, UINT32);
typedef HRESULT (WINAPI *WHvGetVirtualProcessorRegisters_t)(WHV_PARTITION_HANDLE, UINT32, const WHV_REGISTER_NAME*, UINT32, WHV_REGISTER_VALUE*);
typedef HRESULT (WINAPI *WHvSetVirtualProcessorRegisters_t)(WHV_PARTITION_HANDLE, UINT32, const WHV_REGISTER_NAME*, UINT32, const WHV_REGISTER_VALUE*);
typedef HRESULT (WINAPI *WHvTranslateGva_t)(WHV_PARTITION_HANDLE, UINT32, WHV_GUEST_VIRTUAL_ADDRESS, WHV_TRANSLATE_GVA_FLAGS, WHV_TRANSLATE_GVA_RESULT*, WHV_GUEST_PHYSICAL_ADDRESS*);
typedef HRESULT (WINAPI *WHvRequestInterrupt_t)(WHV_PARTITION_HANDLE, const void*, UINT32);
typedef HRESULT (WINAPI *WHvWriteGpaRange_t)(WHV_PARTITION_HANDLE, WHV_GUEST_PHYSICAL_ADDRESS, const VOID*, UINT32);

static struct {
    WHvGetCapability_t WHvGetCapability;
    WHvCreatePartition_t WHvCreatePartition;
    WHvSetupPartition_t WHvSetupPartition;
    WHvDeletePartition_t WHvDeletePartition;
    WHvGetPartitionProperty_t WHvGetPartitionProperty;
    WHvSetPartitionProperty_t WHvSetPartitionProperty;
    WHvMapGpaRange_t WHvMapGpaRange;
    WHvUnmapGpaRange_t WHvUnmapGpaRange;
    WHvCreateVirtualProcessor_t WHvCreateVirtualProcessor;
    WHvDeleteVirtualProcessor_t WHvDeleteVirtualProcessor;
    WHvRunVirtualProcessor_t WHvRunVirtualProcessor;
    WHvCancelRunVirtualProcessor_t WHvCancelRunVirtualProcessor;
    WHvGetVirtualProcessorRegisters_t WHvGetVirtualProcessorRegisters;
    WHvSetVirtualProcessorRegisters_t WHvSetVirtualProcessorRegisters;
    WHvTranslateGva_t WHvTranslateGva;
    WHvRequestInterrupt_t WHvRequestInterrupt;
    WHvWriteGpaRange_t WHvWriteGpaRange;
} whp_dispatch;

static bool whp_dispatch_initialized;
static HMODULE hWinHvPlatform;

/* ==================== Dispatch Init ==================== */

static bool init_whp_dispatch(void)
{
    if (whp_dispatch_initialized) {
        return true;
    }

    hWinHvPlatform = LoadLibraryW(L"WinHvPlatform.dll");
    if (!hWinHvPlatform) {
        error_report("WHPX: Could not load WinHvPlatform.dll");
        return false;
    }

#define LOAD_FUNC(name) \
    whp_dispatch.name = (name ## _t)GetProcAddress(hWinHvPlatform, #name); \
    if (!whp_dispatch.name) { \
        error_report("WHPX: Could not find %s in WinHvPlatform.dll", #name); \
        goto error; \
    }

    LOAD_FUNC(WHvGetCapability)
    LOAD_FUNC(WHvCreatePartition)
    LOAD_FUNC(WHvSetupPartition)
    LOAD_FUNC(WHvDeletePartition)
    LOAD_FUNC(WHvGetPartitionProperty)
    LOAD_FUNC(WHvSetPartitionProperty)
    LOAD_FUNC(WHvMapGpaRange)
    LOAD_FUNC(WHvUnmapGpaRange)
    LOAD_FUNC(WHvCreateVirtualProcessor)
    LOAD_FUNC(WHvDeleteVirtualProcessor)
    LOAD_FUNC(WHvRunVirtualProcessor)
    LOAD_FUNC(WHvCancelRunVirtualProcessor)
    LOAD_FUNC(WHvGetVirtualProcessorRegisters)
    LOAD_FUNC(WHvSetVirtualProcessorRegisters)
    LOAD_FUNC(WHvTranslateGva)
    LOAD_FUNC(WHvRequestInterrupt)
    LOAD_FUNC(WHvWriteGpaRange)

#undef LOAD_FUNC

    whp_dispatch_initialized = true;
    return true;

error:
    FreeLibrary(hWinHvPlatform);
    hWinHvPlatform = NULL;
    return false;
}

/* ==================== Register Names ==================== */

/* GP regs X0..X28, FP, LR, SP_EL0, SP_EL1, PC, PSTATE */
#define WHPX_ARM64_GP_REG_COUNT  31  /* X0-X28, FP(X29), LR(X30) */
#define WHPX_ARM64_REG_COUNT     (WHPX_ARM64_GP_REG_COUNT + 4 + 32 + 2 + 10 + 5)
/* 31 GP + SP/SpEl0/PC/PSTATE + 32 Q regs + FPCR/FPSR + 10 sysregs + 5 timer */

static const WHV_REGISTER_NAME whpx_arm64_gp_regs[] = {
    WHvArm64RegisterX0,  WHvArm64RegisterX1,  WHvArm64RegisterX2,
    WHvArm64RegisterX3,  WHvArm64RegisterX4,  WHvArm64RegisterX5,
    WHvArm64RegisterX6,  WHvArm64RegisterX7,  WHvArm64RegisterX8,
    WHvArm64RegisterX9,  WHvArm64RegisterX10, WHvArm64RegisterX11,
    WHvArm64RegisterX12, WHvArm64RegisterX13, WHvArm64RegisterX14,
    WHvArm64RegisterX15, WHvArm64RegisterX16, WHvArm64RegisterX17,
    WHvArm64RegisterX18, WHvArm64RegisterX19, WHvArm64RegisterX20,
    WHvArm64RegisterX21, WHvArm64RegisterX22, WHvArm64RegisterX23,
    WHvArm64RegisterX24, WHvArm64RegisterX25, WHvArm64RegisterX26,
    WHvArm64RegisterX27, WHvArm64RegisterX28,
    WHvArm64RegisterFp,  WHvArm64RegisterLr,
};

static const WHV_REGISTER_NAME whpx_arm64_core_regs[] = {
    WHvArm64RegisterSp,
    WHvArm64RegisterSpEl0,
    WHvArm64RegisterPc,
    WHvArm64RegisterPstate,
};

static const WHV_REGISTER_NAME whpx_arm64_simd_regs[] = {
    WHvArm64RegisterQ0,  WHvArm64RegisterQ1,  WHvArm64RegisterQ2,
    WHvArm64RegisterQ3,  WHvArm64RegisterQ4,  WHvArm64RegisterQ5,
    WHvArm64RegisterQ6,  WHvArm64RegisterQ7,  WHvArm64RegisterQ8,
    WHvArm64RegisterQ9,  WHvArm64RegisterQ10, WHvArm64RegisterQ11,
    WHvArm64RegisterQ12, WHvArm64RegisterQ13, WHvArm64RegisterQ14,
    WHvArm64RegisterQ15, WHvArm64RegisterQ16, WHvArm64RegisterQ17,
    WHvArm64RegisterQ18, WHvArm64RegisterQ19, WHvArm64RegisterQ20,
    WHvArm64RegisterQ21, WHvArm64RegisterQ22, WHvArm64RegisterQ23,
    WHvArm64RegisterQ24, WHvArm64RegisterQ25, WHvArm64RegisterQ26,
    WHvArm64RegisterQ27, WHvArm64RegisterQ28, WHvArm64RegisterQ29,
    WHvArm64RegisterQ30, WHvArm64RegisterQ31,
};

static const WHV_REGISTER_NAME whpx_arm64_fp_ctrl_regs[] = {
    WHvArm64RegisterFpcr,
    WHvArm64RegisterFpsr,
};

static const WHV_REGISTER_NAME whpx_arm64_sys_regs[] = {
    WHvArm64RegisterSctlrEl1,
    WHvArm64RegisterTtbr0El1,
    WHvArm64RegisterTtbr1El1,
    WHvArm64RegisterTcrEl1,
    WHvArm64RegisterEsrEl1,
    WHvArm64RegisterFarEl1,
    WHvArm64RegisterMairEl1,
    WHvArm64RegisterVbarEl1,
    WHvArm64RegisterElrEl1,
    WHvArm64RegisterSpsrEl1,
};

/* ==================== VP support ==================== */

static struct whpx_vcpu *get_whpx_vcpu(CPUState *cpu)
{
    return (struct whpx_vcpu *)cpu->hax_vcpu;
}

static void whpx_set_registers(CPUState *cpu)
{
    struct whpx_state *whpx = &whpx_global;
    CPUARMState *env = (CPUARMState *)cpu->env_ptr;
    HRESULT hr;
    int i;

    /* Set GP registers X0-X28, FP(X29), LR(X30) */
    WHV_REGISTER_VALUE gp_values[31];
    for (i = 0; i < 31; i++) {
        gp_values[i].Reg64 = env->xregs[i];
    }
    hr = whp_dispatch.WHvSetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        whpx_arm64_gp_regs, 31, gp_values);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to set GP registers, hr=%08lx", hr);
    }

    /* Set core registers: SP, SP_EL0, PC, PSTATE */
    WHV_REGISTER_VALUE core_values[4];
    core_values[0].Reg64 = env->sp_el[1];
    core_values[1].Reg64 = env->sp_el[0];
    core_values[2].Reg64 = env->pc;
    core_values[3].Reg64 = pstate_read(env);
    hr = whp_dispatch.WHvSetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        whpx_arm64_core_regs, 4, core_values);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to set core registers, hr=%08lx", hr);
    }

    /* Set SIMD/FP registers Q0-Q31 (128-bit each) */
    WHV_REGISTER_VALUE simd_values[32];
    for (i = 0; i < 32; i++) {
        simd_values[i].Reg128.Low64 = env->vfp.zregs[i].d[0];
        simd_values[i].Reg128.High64 = env->vfp.zregs[i].d[1];
    }
    hr = whp_dispatch.WHvSetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        whpx_arm64_simd_regs, 32, simd_values);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to set SIMD registers, hr=%08lx", hr);
    }

    /* Set FPCR/FPSR */
    WHV_REGISTER_VALUE fp_ctrl_values[2];
    fp_ctrl_values[0].Reg64 = vfp_get_fpcr(env);
    fp_ctrl_values[1].Reg64 = vfp_get_fpsr(env);
    hr = whp_dispatch.WHvSetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        whpx_arm64_fp_ctrl_regs, 2, fp_ctrl_values);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to set FP ctrl registers, hr=%08lx", hr);
    }

    /* Set system registers */
    WHV_REGISTER_VALUE sys_values[10];
    sys_values[0].Reg64 = env->cp15.sctlr_el[1];
    sys_values[1].Reg64 = env->cp15.ttbr0_el[1];
    sys_values[2].Reg64 = env->cp15.ttbr1_el[1];
    sys_values[3].Reg64 = env->cp15.tcr_el[1].raw_tcr;
    sys_values[4].Reg64 = env->cp15.esr_el[1];
    sys_values[5].Reg64 = env->cp15.far_el[1];
    sys_values[6].Reg64 = env->cp15.mair_el[1];
    sys_values[7].Reg64 = env->cp15.vbar_el[1];
    sys_values[8].Reg64 = env->elr_el[1];
    sys_values[9].Reg64 = env->banked_spsr[aarch64_banked_spsr_index(1)];
    hr = whp_dispatch.WHvSetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        whpx_arm64_sys_regs, 10, sys_values);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to set system registers, hr=%08lx", hr);
    }
}

static void whpx_get_registers(CPUState *cpu)
{
    struct whpx_state *whpx = &whpx_global;
    CPUARMState *env = (CPUARMState *)cpu->env_ptr;
    HRESULT hr;
    int i;

    /* Get GP registers */
    WHV_REGISTER_VALUE gp_values[31];
    hr = whp_dispatch.WHvGetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        whpx_arm64_gp_regs, 31, gp_values);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to get GP registers, hr=%08lx", hr);
        return;
    }
    for (i = 0; i < 31; i++) {
        env->xregs[i] = gp_values[i].Reg64;
    }

    /* Get core registers */
    WHV_REGISTER_VALUE core_values[4];
    hr = whp_dispatch.WHvGetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        whpx_arm64_core_regs, 4, core_values);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to get core registers, hr=%08lx", hr);
        return;
    }
    env->sp_el[1] = core_values[0].Reg64;  /* SP_EL1 */
    env->sp_el[0] = core_values[1].Reg64;  /* SP_EL0 */
    env->pc = core_values[2].Reg64;
    pstate_write(env, (uint32_t)core_values[3].Reg64);

    /* Get SIMD registers */
    WHV_REGISTER_VALUE simd_values[32];
    hr = whp_dispatch.WHvGetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        whpx_arm64_simd_regs, 32, simd_values);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to get SIMD registers, hr=%08lx", hr);
        return;
    }
    for (i = 0; i < 32; i++) {
        env->vfp.zregs[i].d[0] = simd_values[i].Reg128.Low64;
        env->vfp.zregs[i].d[1] = simd_values[i].Reg128.High64;
    }

    /* Get FPCR/FPSR */
    WHV_REGISTER_VALUE fp_ctrl_values[2];
    hr = whp_dispatch.WHvGetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        whpx_arm64_fp_ctrl_regs, 2, fp_ctrl_values);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to get FP ctrl registers, hr=%08lx", hr);
        return;
    }
    vfp_set_fpcr(env, (uint32_t)fp_ctrl_values[0].Reg64);
    vfp_set_fpsr(env, (uint32_t)fp_ctrl_values[1].Reg64);

    /* Get system registers */
    WHV_REGISTER_VALUE sys_values[10];
    hr = whp_dispatch.WHvGetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index,
        whpx_arm64_sys_regs, 10, sys_values);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to get system registers, hr=%08lx", hr);
        return;
    }
    env->cp15.sctlr_el[1] = sys_values[0].Reg64;
    env->cp15.ttbr0_el[1] = sys_values[1].Reg64;
    env->cp15.ttbr1_el[1] = sys_values[2].Reg64;
    env->cp15.tcr_el[1].raw_tcr = sys_values[3].Reg64;
    env->cp15.esr_el[1] = sys_values[4].Reg64;
    env->cp15.far_el[1] = sys_values[5].Reg64;
    env->cp15.mair_el[1] = sys_values[6].Reg64;
    env->cp15.vbar_el[1] = sys_values[7].Reg64;
    env->elr_el[1] = sys_values[8].Reg64;
    env->banked_spsr[aarch64_banked_spsr_index(1)] = sys_values[9].Reg64;
}

/* ==================== GICv3 Redistributor (mapped as RAM) ==================== */

#define WHPX_GICR_BASE      0x080A0000ULL
#define WHPX_GICR_STRIDE    0x20000ULL

static uint8_t *gicr_mem = NULL;
static uint64_t gicr_mem_size = 0;

static void whpx_init_gicr_memory(struct whpx_state *whpx, int ncpus)
{
    gicr_mem_size = WHPX_GICR_STRIDE * ncpus;

    gicr_mem = (uint8_t *)VirtualAlloc(NULL, gicr_mem_size,
                                        MEM_COMMIT | MEM_RESERVE,
                                        PAGE_READWRITE);
    if (!gicr_mem) {
        error_report("WHPX: Failed to allocate GICR memory (%llu bytes)",
                     (unsigned long long)gicr_mem_size);
        return;
    }
    memset(gicr_mem, 0, gicr_mem_size);

    for (int i = 0; i < ncpus; i++) {
        uint8_t *rd_base = gicr_mem + i * WHPX_GICR_STRIDE;
        uint8_t *sgi_base = rd_base + 0x10000;

        /* GICR_CTLR (offset 0x0000) = 0 */
        /* GICR_IIDR (offset 0x0004) */
        *(uint32_t *)(rd_base + 0x0004) = 0x0100043B;

        /* GICR_TYPER (offset 0x0008, 64-bit):
         * [23:8] Processor_Number, [4] Last, [63:32] Affinity (Aff0=cpu_idx) */
        uint64_t typer = 0;
        typer |= ((uint64_t)i << 8);   /* Processor_Number */
        typer |= ((uint64_t)i << 32);  /* Aff0 */
        if (i == ncpus - 1) {
            typer |= (1ULL << 4);       /* Last */
        }
        *(uint32_t *)(rd_base + 0x0008) = (uint32_t)(typer & 0xFFFFFFFF);
        *(uint32_t *)(rd_base + 0x000C) = (uint32_t)(typer >> 32);

        /* GICR_WAKER (offset 0x0014) = 0 (processor active) */
        *(uint32_t *)(rd_base + 0x0014) = 0;

        /* GICR_PIDR2 (offset 0xFFE8) = 0x3B (GICv3) */
        *(uint32_t *)(rd_base + 0xFFE8) = 0x3B;

        /* SGI_base: GICR_IGROUPR0 (offset 0x0080) = all group 1 */
        *(uint32_t *)(sgi_base + 0x0080) = 0xFFFFFFFF;

        /* SGI_base: GICR_ISENABLER0 (offset 0x0100) = all SGIs + PPIs enabled */
        *(uint32_t *)(sgi_base + 0x0100) = 0xFFFFFFFF;

        /* SGI_base: GICR_IPRIORITYR0-7 (offset 0x0400, 32 bytes): default 0xA0 */
        memset(sgi_base + 0x0400, 0xA0, 32);
    }

    HRESULT hr = whp_dispatch.WHvMapGpaRange(
        whpx->partition, gicr_mem, WHPX_GICR_BASE, gicr_mem_size,
        WHvMapGpaRangeFlagRead | WHvMapGpaRangeFlagWrite);

    if (FAILED(hr)) {
        error_report("WHPX: Failed to map GICR at GPA 0x%llx, hr=%08lx",
                     (unsigned long long)WHPX_GICR_BASE, hr);
        VirtualFree(gicr_mem, 0, MEM_RELEASE);
        gicr_mem = NULL;
    }
}

/* ==================== Interrupt Injection ==================== */

/* ARM64 WHV_INTERRUPT_CONTROL is 32 bytes (not the 16-byte x86 version).
 * The SDK 10.0.22621.0 only has the x86 definition; these come from the
 * 10.0.26100.0 SDK and mingw-w64 headers. */
typedef union {
    UINT64 AsUINT64;
    struct {
        UINT32 InterruptType;  /* 0 = Fixed */
        UINT32 Reserved1 : 2;
        UINT32 Asserted  : 1;
        UINT32 Retarget  : 1;
        UINT32 Reserved2 : 28;
    };
} WHV_ARM64_INTERRUPT_CONTROL2;

typedef struct {
    UINT64 TargetPartition;
    WHV_ARM64_INTERRUPT_CONTROL2 InterruptControl;
    UINT64 DestinationAddress;
    UINT32 RequestedVector;
    UINT8  TargetVtl;
    UINT8  ReservedZ0;
    UINT16 ReservedZ1;
} WHV_ARM64_INTERRUPT_CONTROL;

void whpx_arm64_inject_irq(void *opaque, int spi_num, int level)
{
    struct whpx_state *whpx = &whpx_global;
    (void)opaque;

    if (spi_num >= 0 && spi_num < WHPX_MAX_SPI) {
        LONG old = InterlockedExchange(&whpx_spi_level[spi_num], level);
        if (old == level) {
            return;
        }
    }

    uint32_t intid = spi_num + 32;

    WHV_ARM64_INTERRUPT_CONTROL ic = {0};
    ic.InterruptControl.InterruptType = 0; /* Fixed */
    ic.InterruptControl.Asserted = level ? 1 : 0;
    ic.RequestedVector = intid;

    whp_dispatch.WHvRequestInterrupt(
        whpx->partition, (const WHV_INTERRUPT_CONTROL *)&ic, sizeof(ic));

    if (level) {
        whp_dispatch.WHvCancelRunVirtualProcessor(whpx->partition, 0, 0);
    }
}

/* ==================== Instruction Fetch Helper ==================== */

static uint32_t whpx_fetch_insn(CPUState *cpu, uint64_t pc)
{
    uint32_t idx = (uint32_t)(pc >> 2) & WHPX_INSN_CACHE_MASK;
    if (insn_cache[idx].pc == pc) {
        return insn_cache[idx].insn;
    }

    struct whpx_state *whpx = &whpx_global;
    WHV_TRANSLATE_GVA_RESULT xlat_result = {0};
    WHV_GUEST_PHYSICAL_ADDRESS insn_gpa = 0;
    uint32_t insn = 0;

    HRESULT hr = whp_dispatch.WHvTranslateGva(
        whpx->partition, cpu->cpu_index, pc,
        WHvTranslateGvaFlagValidateRead, &xlat_result, &insn_gpa);

    if (FAILED(hr) || xlat_result.ResultCode != 0) {
        return 0;
    }

    cpu_physical_memory_read(insn_gpa, &insn, 4);

    insn_cache[idx].pc = pc;
    insn_cache[idx].insn = insn;
    return insn;
}

/* ==================== PSCI Handler ==================== */

#define PSCI_0_2_FN_BASE         0x84000000
#define PSCI_0_2_FN64_BASE       0xC4000000
#define PSCI_0_2_FN_CPU_SUSPEND  0x84000001
#define PSCI_0_2_FN64_CPU_SUSPEND 0xC4000001
#define PSCI_0_2_FN_CPU_OFF      0x84000002
#define PSCI_0_2_FN_CPU_ON       0x84000003
#define PSCI_0_2_FN64_CPU_ON     0xC4000003
#define PSCI_0_2_FN_AFFINITY_INFO 0x84000004
#define PSCI_0_2_FN64_AFFINITY_INFO 0xC4000004
#define PSCI_0_2_FN_SYSTEM_OFF   0x84000008
#define PSCI_0_2_FN_SYSTEM_RESET 0x84000009
#define PSCI_0_2_FN_PSCI_VERSION 0x84000000
#define PSCI_0_2_FN_MIGRATE_INFO_TYPE 0x84000006
#define PSCI_0_2_FN_PSCI_FEATURES 0x8400000A

/* PSCI v0.1 function IDs (legacy, vendor-specific) */
#define PSCI_0_1_FN_CPU_SUSPEND  0x95c1ba5e
#define PSCI_0_1_FN_CPU_OFF      0x95c1ba5f
#define PSCI_0_1_FN_CPU_ON       0x95c1ba60
#define PSCI_0_1_FN_MIGRATE      0x95c1ba61

#define PSCI_RET_SUCCESS          0
#define PSCI_RET_NOT_SUPPORTED    ((int32_t)-1)
#define PSCI_RET_INVALID_PARAMS   ((int32_t)-2)
#define PSCI_RET_DENIED           ((int32_t)-3)
#define PSCI_RET_ALREADY_ON       ((int32_t)-4)
#define PSCI_RET_ON_PENDING       ((int32_t)-5)

static int whpx_handle_psci(CPUState *cpu, WHV_ARM64_VP_EXIT_CONTEXT *ctx)
{
    struct whpx_state *whpx = &whpx_global;
    HRESULT hr;

    /* Read X0 (function ID) and X1-X3 (arguments) */
    WHV_REGISTER_NAME arg_regs[4] = {
        WHvArm64RegisterX0, WHvArm64RegisterX1,
        WHvArm64RegisterX2, WHvArm64RegisterX3
    };
    WHV_REGISTER_VALUE arg_vals[4] = {0};
    hr = whp_dispatch.WHvGetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index, arg_regs, 4, arg_vals);
    if (FAILED(hr)) {
        return -1;
    }

    uint32_t fn = (uint32_t)arg_vals[0].Reg64;
    uint64_t arg1 = arg_vals[1].Reg64;
    uint64_t arg2 = arg_vals[2].Reg64;
    uint64_t arg3 = arg_vals[3].Reg64;

    int64_t ret_val = PSCI_RET_NOT_SUPPORTED;

    switch (fn) {
    case PSCI_0_2_FN_PSCI_VERSION:
        ret_val = 0x00010000; /* PSCI 1.0 */
        break;

    case PSCI_0_1_FN_CPU_ON:
    case PSCI_0_2_FN_CPU_ON:
    case PSCI_0_2_FN64_CPU_ON: {
        uint64_t target_affinity = arg1;
        uint64_t entry_point = arg2;
        uint64_t context_id = arg3;
        int target_cpu = (int)(target_affinity & 0xFF);

        CPUState *target_cs = qemu_get_cpu(target_cpu);
        if (!target_cs) {
            ret_val = PSCI_RET_INVALID_PARAMS;
            break;
        }

        /* Set target CPU registers: PC=entry_point, X0=context_id */
        WHV_REGISTER_NAME boot_regs[3] = {
            WHvArm64RegisterPc, WHvArm64RegisterX0, WHvArm64RegisterPstate
        };
        WHV_REGISTER_VALUE boot_vals[3] = {0};
        boot_vals[0].Reg64 = entry_point;
        boot_vals[1].Reg64 = context_id;
        boot_vals[2].Reg64 = 0x3C5; /* EL1h, all interrupts masked */
        hr = whp_dispatch.WHvSetVirtualProcessorRegisters(
            whpx->partition, target_cpu, boot_regs, 3, boot_vals);
        if (FAILED(hr)) {
            ret_val = PSCI_RET_DENIED;
            break;
        }

        target_cs->halted = false;
        target_cs->exception_index = -1;
        qemu_cpu_kick(target_cs);

        ret_val = PSCI_RET_SUCCESS;
        break;
    }

    case PSCI_0_1_FN_CPU_OFF:
    case PSCI_0_2_FN_CPU_OFF:
        cpu->halted = true;
        cpu->exception_index = EXCP_HLT;
        /* Return X0=success, though the CPU won't read it */
        ret_val = PSCI_RET_SUCCESS;
        break;

    case PSCI_0_1_FN_CPU_SUSPEND:
    case PSCI_0_2_FN_CPU_SUSPEND:
    case PSCI_0_2_FN64_CPU_SUSPEND:
        /* Treat as WFI - halt until next interrupt */
        cpu->halted = true;
        cpu->exception_index = EXCP_HLT;
        ret_val = PSCI_RET_SUCCESS;
        break;

    case PSCI_0_2_FN_AFFINITY_INFO:
    case PSCI_0_2_FN64_AFFINITY_INFO: {
        int target_cpu = (int)(arg1 & 0xFF);
        CPUState *target_cs = qemu_get_cpu(target_cpu);
        if (!target_cs) {
            ret_val = PSCI_RET_INVALID_PARAMS;
        } else {
            ret_val = target_cs->halted ? 1 : 0; /* 0=ON, 1=OFF */
        }
        break;
    }

    case PSCI_0_2_FN_SYSTEM_OFF:
        qemu_system_shutdown_request(SHUTDOWN_CAUSE_GUEST_SHUTDOWN);
        ret_val = PSCI_RET_SUCCESS;
        break;

    case PSCI_0_2_FN_SYSTEM_RESET:
        qemu_system_reset_request(SHUTDOWN_CAUSE_GUEST_RESET);
        ret_val = PSCI_RET_SUCCESS;
        break;

    case PSCI_0_2_FN_MIGRATE_INFO_TYPE:
        ret_val = 2; /* TOS is not present */
        break;

    case PSCI_0_2_FN_PSCI_FEATURES:
        /* Report all implemented functions as supported */
        switch ((uint32_t)arg1) {
        case PSCI_0_2_FN_PSCI_VERSION:
        case PSCI_0_2_FN_CPU_ON:
        case PSCI_0_2_FN64_CPU_ON:
        case PSCI_0_2_FN_CPU_OFF:
        case PSCI_0_2_FN_CPU_SUSPEND:
        case PSCI_0_2_FN64_CPU_SUSPEND:
        case PSCI_0_2_FN_AFFINITY_INFO:
        case PSCI_0_2_FN64_AFFINITY_INFO:
        case PSCI_0_2_FN_SYSTEM_OFF:
        case PSCI_0_2_FN_SYSTEM_RESET:
        case PSCI_0_2_FN_MIGRATE_INFO_TYPE:
            ret_val = PSCI_RET_SUCCESS;
            break;
        default:
            ret_val = PSCI_RET_NOT_SUPPORTED;
            break;
        }
        break;

    default:
        ret_val = PSCI_RET_NOT_SUPPORTED;
        break;
    }

    /* Write return value to X0 */
    WHV_REGISTER_NAME ret_name = WHvArm64RegisterX0;
    WHV_REGISTER_VALUE ret_reg = {0};
    ret_reg.Reg64 = (uint64_t)(int64_t)ret_val;
    whp_dispatch.WHvSetVirtualProcessorRegisters(
        whpx->partition, cpu->cpu_index, &ret_name, 1, &ret_reg);

    /* Advance PC past HVC/SMC (4 bytes) */
    if (fn != PSCI_0_2_FN_CPU_OFF) {
        WHV_REGISTER_NAME pc_name = WHvArm64RegisterPc;
        WHV_REGISTER_VALUE pc_val = {0};
        pc_val.Reg64 = ctx->Pc + 4;
        whp_dispatch.WHvSetVirtualProcessorRegisters(
            whpx->partition, cpu->cpu_index, &pc_name, 1, &pc_val);
    }

    return (fn == PSCI_0_2_FN_CPU_OFF) ? 1 : 0;
}

/* ==================== Exit Handling ==================== */

static int whpx_handle_mmio(CPUState *cpu, WHV_ARM64_VP_EXIT_CONTEXT *ctx)
{
    struct whpx_state *whpx = &whpx_global;
    uint64_t gpa = ctx->Gpa;
    uint8_t buf[16] = {0};
    uint64_t next_pc = ctx->Pc + 4;
    CPUARMState *env = (CPUARMState *)cpu->env_ptr;

    uint32_t insn = whpx_fetch_insn(cpu, ctx->Pc);

    int access_size = 4;
    int rt = 0;
    int rt2 = -1;
    bool is_write = false;
    bool sign_extend = false;
    bool extend_to_64 = false;
    int rs = -1;

    if (insn != 0) {
        int group = (insn >> 27) & 0x7;
        rt = insn & 0x1f;

        switch (group) {
        case 7: /* LDR/STR single register (all addressing modes) */
        case 6: {
            int size_bits = (insn >> 30) & 0x3;
            int V = (insn >> 26) & 1;
            int opc = (insn >> 22) & 0x3;
            /* opc=00: STR, opc=01: LDR, opc=10: LDRS* (64), opc=11: LDRS* (32)
             * STR is the only store — all others are loads */
            is_write = (opc == 0);
            if (!V) {
                switch (size_bits) {
                case 0: access_size = 1; break;
                case 1: access_size = 2; break;
                case 2: access_size = 4; break;
                case 3: access_size = 8; break;
                }
                if (!is_write && opc >= 2) {
                    sign_extend = true;
                    extend_to_64 = (opc == 2);
                }
            } else {
                switch (size_bits) {
                case 0: access_size = 4; break;
                case 1: access_size = 8; break;
                default: access_size = 16; break;
                }
            }
            break;
        }
        case 5: { /* LDP/STP */
            int opc_pair = (insn >> 30) & 0x3;
            int V = (insn >> 26) & 1;
            is_write = !((insn >> 22) & 1);
            rt2 = (insn >> 10) & 0x1f;
            if (!V) {
                switch (opc_pair) {
                case 0: access_size = 4; break;
                case 1: access_size = 4; sign_extend = true;
                        extend_to_64 = true; break;
                case 2: access_size = 8; break;
                default: access_size = 4; break;
                }
            } else {
                switch (opc_pair) {
                case 0: access_size = 4; break;
                case 1: access_size = 8; break;
                case 2: access_size = 16; break;
                default: access_size = 4; break;
                }
            }
            break;
        }
        case 1: { /* LDXR/STXR/LDAR/STLR */
            int size_bits = (insn >> 30) & 0x3;
            is_write = !((insn >> 22) & 1);
            switch (size_bits) {
            case 0: access_size = 1; break;
            case 1: access_size = 2; break;
            case 2: access_size = 4; break;
            case 3: access_size = 8; break;
            }
            if (is_write && !((insn >> 21) & 1)) {
                rs = (insn >> 16) & 0x1f;
            }
            break;
        }
        default: { /* Unknown — best-effort size decode */
            int size_bits = (insn >> 30) & 0x3;
            is_write = ((insn >> 22) & 1) == 0;
            switch (size_bits) {
            case 0: access_size = 1; break;
            case 1: access_size = 2; break;
            case 2: access_size = 4; break;
            case 3: access_size = 8; break;
            }
            break;
        }
        }
    }

    if (is_write) {
        if (rt2 >= 0) {
            /* STP — store pair */
            uint64_t v1 = 0, v2 = 0;
            int n = 0;
            WHV_REGISTER_NAME rnames[2];
            WHV_REGISTER_VALUE rvals[2] = {{0}};
            if (rt < 31) { rnames[n++] = WHvArm64RegisterX0 + rt; }
            if (rt2 < 31) { rnames[n++] = WHvArm64RegisterX0 + rt2; }
            if (n) {
                whp_dispatch.WHvGetVirtualProcessorRegisters(
                    whpx->partition, cpu->cpu_index, rnames, n, rvals);
            }
            int idx = 0;
            v1 = (rt < 31) ? rvals[idx++].Reg64 : 0;
            v2 = (rt2 < 31) ? rvals[idx].Reg64 : 0;
            memcpy(buf, &v1, access_size);
            memcpy(buf + access_size, &v2, access_size);
            qemu_mutex_lock_iothread();
            address_space_write(&address_space_memory, gpa,
                                MEMTXATTRS_UNSPECIFIED, buf, access_size);
            address_space_write(&address_space_memory, gpa + access_size,
                                MEMTXATTRS_UNSPECIFIED,
                                buf + access_size, access_size);
            qemu_mutex_unlock_iothread();
        } else {
            uint64_t write_val = 0;
            if (rt < 31) {
                WHV_REGISTER_NAME reg_name = WHvArm64RegisterX0 + rt;
                WHV_REGISTER_VALUE reg_val = {0};
                whp_dispatch.WHvGetVirtualProcessorRegisters(
                    whpx->partition, cpu->cpu_index, &reg_name, 1, &reg_val);
                write_val = reg_val.Reg64;
            }
            memcpy(buf, &write_val, access_size);
            qemu_mutex_lock_iothread();
            address_space_write(&address_space_memory, gpa,
                                MEMTXATTRS_UNSPECIFIED, buf, access_size);
            qemu_mutex_unlock_iothread();
        }

        if (rs >= 0 && rs < 31) {
            WHV_REGISTER_NAME names[2];
            WHV_REGISTER_VALUE vals[2] = {{0}};
            names[0] = WHvArm64RegisterX0 + rs;
            vals[0].Reg64 = 0;
            names[1] = WHvArm64RegisterPc;
            vals[1].Reg64 = next_pc;
            whp_dispatch.WHvSetVirtualProcessorRegisters(
                whpx->partition, cpu->cpu_index, names, 2, vals);
        } else {
            WHV_REGISTER_NAME pc_name = WHvArm64RegisterPc;
            WHV_REGISTER_VALUE pc_val = {0};
            pc_val.Reg64 = next_pc;
            whp_dispatch.WHvSetVirtualProcessorRegisters(
                whpx->partition, cpu->cpu_index, &pc_name, 1, &pc_val);
        }
    } else {
        if (rt2 >= 0) {
            /* LDP — load pair */
            uint8_t buf2[16] = {0};
            qemu_mutex_lock_iothread();
            address_space_read(&address_space_memory, gpa,
                               MEMTXATTRS_UNSPECIFIED, buf, access_size);
            address_space_read(&address_space_memory, gpa + access_size,
                               MEMTXATTRS_UNSPECIFIED, buf2, access_size);
            qemu_mutex_unlock_iothread();

            uint64_t v1 = 0, v2 = 0;
            memcpy(&v1, buf, access_size);
            memcpy(&v2, buf2, access_size);
            if (sign_extend && access_size == 4) {
                v1 = (uint64_t)(int64_t)(int32_t)(uint32_t)v1;
                v2 = (uint64_t)(int64_t)(int32_t)(uint32_t)v2;
            }

            int n = 0;
            WHV_REGISTER_NAME names[3];
            WHV_REGISTER_VALUE vals[3] = {{0}};
            if (rt < 31) {
                names[n] = WHvArm64RegisterX0 + rt;
                vals[n].Reg64 = v1;
                env->xregs[rt] = v1;
                n++;
            }
            if (rt2 < 31) {
                names[n] = WHvArm64RegisterX0 + rt2;
                vals[n].Reg64 = v2;
                env->xregs[rt2] = v2;
                n++;
            }
            names[n] = WHvArm64RegisterPc;
            vals[n].Reg64 = next_pc;
            n++;
            whp_dispatch.WHvSetVirtualProcessorRegisters(
                whpx->partition, cpu->cpu_index, names, n, vals);
        } else {
            qemu_mutex_lock_iothread();
            address_space_read(&address_space_memory, gpa,
                               MEMTXATTRS_UNSPECIFIED, buf, access_size);
            qemu_mutex_unlock_iothread();

            uint64_t read_val = 0;
            memcpy(&read_val, buf, access_size);

            if (sign_extend) {
                switch (access_size) {
                case 1:
                    read_val = extend_to_64
                        ? (uint64_t)(int64_t)(int8_t)(uint8_t)read_val
                        : (uint64_t)(uint32_t)(int32_t)(int8_t)(uint8_t)read_val;
                    break;
                case 2:
                    read_val = extend_to_64
                        ? (uint64_t)(int64_t)(int16_t)(uint16_t)read_val
                        : (uint64_t)(uint32_t)(int32_t)(int16_t)(uint16_t)read_val;
                    break;
                case 4:
                    read_val = (uint64_t)(int64_t)(int32_t)(uint32_t)read_val;
                    break;
                }
            }

            if (rt < 31) {
                WHV_REGISTER_NAME names[2];
                WHV_REGISTER_VALUE vals[2] = {{0}};
                names[0] = WHvArm64RegisterX0 + rt;
                vals[0].Reg64 = read_val;
                names[1] = WHvArm64RegisterPc;
                vals[1].Reg64 = next_pc;
                whp_dispatch.WHvSetVirtualProcessorRegisters(
                    whpx->partition, cpu->cpu_index, names, 2, vals);
                env->xregs[rt] = read_val;
            } else {
                WHV_REGISTER_NAME pc_name = WHvArm64RegisterPc;
                WHV_REGISTER_VALUE pc_val = {0};
                pc_val.Reg64 = next_pc;
                whp_dispatch.WHvSetVirtualProcessorRegisters(
                    whpx->partition, cpu->cpu_index, &pc_name, 1, &pc_val);
            }
        }
    }

    env->pc = next_pc;
    return 0;
}

static int whpx_handle_halt(CPUState *cpu)
{
    qemu_mutex_lock_iothread();
    if (!cpu->interrupt_request && !cpu->exit_request) {
        cpu->exception_index = EXCP_HLT;
        cpu->halted = true;
        qemu_mutex_unlock_iothread();
        return 1;
    }
    qemu_mutex_unlock_iothread();
    return 0;
}

/* ==================== vCPU Run Loop ==================== */

static void whpx_vcpu_pre_run(CPUState *cpu)
{
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);

    qemu_mutex_lock_iothread();

    /* Check for pending interrupts */
    if (!vcpu->interruption_pending && vcpu->interruptable) {
        if (cpu->interrupt_request & CPU_INTERRUPT_HARD) {
            cpu->interrupt_request &= ~CPU_INTERRUPT_HARD;
            /* ARM64 IRQ injection is handled by the hypervisor's virtual
             * interrupt controller. Signal it here if needed. */
        }
    }

    if (cpu->interrupt_request & CPU_INTERRUPT_HALT) {
        cpu->interrupt_request &= ~CPU_INTERRUPT_HALT;
        cpu->halted = true;
        cpu->exception_index = EXCP_HLT;
    }

    qemu_mutex_unlock_iothread();
}

static void whpx_vcpu_post_run(CPUState *cpu)
{
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);
    (void)vcpu;
}

static void whpx_vcpu_process_async_events(CPUState *cpu)
{
    if ((cpu->interrupt_request & CPU_INTERRUPT_HARD)) {
        cpu->halted = false;
    }
}

static int whpx_vcpu_run(CPUState *cpu)
{
    HRESULT hr;
    struct whpx_state *whpx = &whpx_global;
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);
    int ret;

    whpx_vcpu_process_async_events(cpu);
    if (cpu->halted && cpu->cpu_index == 0) {
        cpu->exception_index = EXCP_HLT;
        atomic_set(&cpu->exit_request, false);
        return 0;
    }

    qemu_mutex_unlock_iothread();
    cpu_exec_start(cpu);

    if (cpu->halted) {
        cpu->vcpu_dirty = false;
    }
    ULONGLONG last_break_time = GetTickCount64();
    do {
        if (cpu->vcpu_dirty) {
            whpx_set_registers(cpu);
            cpu->vcpu_dirty = false;
        }

        whpx_vcpu_pre_run(cpu);

        if (atomic_read(&cpu->exit_request)) {
            whpx_vcpu_kick(cpu);
        }

        hr = whp_dispatch.WHvRunVirtualProcessor(
            whpx->partition, cpu->cpu_index,
            &vcpu->exit_ctx, sizeof(vcpu->exit_ctx));

        if (FAILED(hr)) {
            error_report("WHPX: Failed to exec a virtual processor,"
                         " hr=%08lx", hr);
            ret = -1;
            break;
        }

        whpx_vcpu_post_run(cpu);

        uint32_t exit_reason = vcpu->exit_ctx.ExitReason;

        switch (exit_reason) {
        case WHV_ARM64_EXIT_REASON_MEMORY_ACCESS:
            ret = whpx_handle_mmio(cpu, &vcpu->exit_ctx);
            break;

        case WHV_ARM64_EXIT_REASON_CANCELED:
            if (atomic_read(&cpu->exit_request)) {
                cpu->exception_index = EXCP_INTERRUPT;
                ret = 1;
            } else {
                ret = 0;
            }
            break;

        case WHV_ARM64_EXIT_REASON_HYPERCALL:
            qemu_mutex_lock_iothread();
            ret = whpx_handle_psci(cpu, &vcpu->exit_ctx);
            qemu_mutex_unlock_iothread();
            if (ret) {
                cpu->exception_index = EXCP_HLT;
            }
            break;

        case 0x00000000:
            if (cpu->halted && vcpu->exit_ctx.Pc != 0) {
                cpu->halted = false;
                cpu->vcpu_dirty = false;
            }
            ret = 0;
            break;

        case 0x0000000C:
            if (cpu->halted) {
                cpu->halted = false;
                cpu->vcpu_dirty = false;
            }
            ret = 0;
            break;

        case 0xFFFFFFFF:
            if (cpu->halted && vcpu->exit_ctx.Pc != 0) {
                cpu->halted = false;
                cpu->vcpu_dirty = false;
            }
            if (atomic_read(&cpu->exit_request)) {
                cpu->exception_index = EXCP_INTERRUPT;
                ret = 1;
            } else {
                ret = 0;
            }
            break;

        default:
            /* Try treating as hypercall (PSCI) for unknown ARM64 exit types */
            if ((exit_reason & 0x80000000) != 0 && exit_reason != WHV_ARM64_EXIT_REASON_MEMORY_ACCESS) {
                qemu_mutex_lock_iothread();
                ret = whpx_handle_psci(cpu, &vcpu->exit_ctx);
                qemu_mutex_unlock_iothread();
                if (ret) {
                    cpu->exception_index = EXCP_HLT;
                }
            } else {
                error_report("WHPX: Unexpected VP exit code 0x%08x", exit_reason);
                whpx_get_registers(cpu);
                qemu_mutex_lock_iothread();
                qemu_system_guest_panicked(cpu_get_crash_info(cpu));
                qemu_mutex_unlock_iothread();
                ret = -1;
            }
            break;
        }

        if (!ret && (GetTickCount64() - last_break_time >= 5000)) {
            cpu->exception_index = EXCP_INTERRUPT;
            ret = 1;
        }

    } while (!ret);

    cpu_exec_end(cpu);
    qemu_mutex_lock_iothread();
    current_cpu = cpu;

    atomic_set(&cpu->exit_request, false);

    return ret < 0;
}

/* ==================== CPU Synchronization ==================== */

static void do_whpx_cpu_synchronize_state(CPUState *cpu, run_on_cpu_data arg)
{
    whpx_get_registers(cpu);
    cpu->vcpu_dirty = true;
}

static void do_whpx_cpu_synchronize_post_reset(CPUState *cpu,
                                               run_on_cpu_data arg)
{
    whpx_set_registers(cpu);
    cpu->vcpu_dirty = false;
}

static void do_whpx_cpu_synchronize_post_init(CPUState *cpu,
                                              run_on_cpu_data arg)
{
    whpx_set_registers(cpu);
    cpu->vcpu_dirty = false;
}

static void do_whpx_cpu_synchronize_pre_loadvm(CPUState *cpu,
                                               run_on_cpu_data arg)
{
    cpu->vcpu_dirty = true;
}

void whpx_cpu_synchronize_state(CPUState *cpu)
{
    if (!cpu->vcpu_dirty) {
        run_on_cpu(cpu, do_whpx_cpu_synchronize_state, RUN_ON_CPU_NULL);
    }
}

void whpx_cpu_synchronize_post_reset(CPUState *cpu)
{
    run_on_cpu(cpu, do_whpx_cpu_synchronize_post_reset, RUN_ON_CPU_NULL);
}

void whpx_cpu_synchronize_post_init(CPUState *cpu)
{
    run_on_cpu(cpu, do_whpx_cpu_synchronize_post_init, RUN_ON_CPU_NULL);
}

void whpx_cpu_synchronize_pre_loadvm(CPUState *cpu)
{
    run_on_cpu(cpu, do_whpx_cpu_synchronize_pre_loadvm, RUN_ON_CPU_NULL);
}

/* ==================== vCPU Management ==================== */

int whpx_init_vcpu(CPUState *cpu)
{
    HRESULT hr;
    struct whpx_state *whpx = &whpx_global;
    struct whpx_vcpu *vcpu;

    vcpu = g_malloc0(sizeof(struct whpx_vcpu));
    if (!vcpu) {
        error_report("WHPX: Failed to allocate VCPU context.");
        return -ENOMEM;
    }

    hr = whp_dispatch.WHvCreateVirtualProcessor(
        whpx->partition, cpu->cpu_index, 0);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to create a virtual processor,"
                     " hr=%08lx", hr);
        g_free(vcpu);
        return -EINVAL;
    }

    /* Set per-vCPU GICR base address so the hypervisor can manage PPIs */
    {
        WHV_REGISTER_NAME gicr_reg = WHvArm64RegisterGicrBaseGpa;
        WHV_REGISTER_VALUE gicr_val = {0};
        gicr_val.Reg64 = WHPX_GICR_BASE + (cpu->cpu_index * WHPX_GICR_STRIDE);
        hr = whp_dispatch.WHvSetVirtualProcessorRegisters(
            whpx->partition, cpu->cpu_index, &gicr_reg, 1, &gicr_val);
        if (FAILED(hr)) {
            error_report("WHPX: Failed to set GICR base for cpu%d, hr=%08lx",
                         cpu->cpu_index, hr);
        }
    }

    vcpu->interruptable = true;
    cpu->vcpu_dirty = true;
    cpu->hax_vcpu = (struct hax_vcpu_state *)vcpu;

    return 0;
}

int whpx_vcpu_exec(CPUState *cpu)
{
    int ret;
    int fatal;

    for (;;) {
        if (cpu->exception_index >= EXCP_INTERRUPT) {
            ret = cpu->exception_index;
            cpu->exception_index = -1;
            break;
        }

        fatal = whpx_vcpu_run(cpu);

        if (fatal) {
            error_report("WHPX: Failed to exec a virtual processor");
            abort();
        }
    }

    return ret;
}

void whpx_destroy_vcpu(CPUState *cpu)
{
    struct whpx_state *whpx = &whpx_global;
    struct whpx_vcpu *vcpu = get_whpx_vcpu(cpu);

    whp_dispatch.WHvDeleteVirtualProcessor(whpx->partition, cpu->cpu_index);
    g_free(vcpu);
    cpu->hax_vcpu = NULL;
}

void whpx_vcpu_kick(CPUState *cpu)
{
    struct whpx_state *whpx = &whpx_global;
    whp_dispatch.WHvCancelRunVirtualProcessor(
        whpx->partition, cpu->cpu_index, 0);
}

/* ==================== Memory Management ==================== */

#define WHPX_MAX_SLOTS 64

struct whpx_slot {
    int present;
    uint64_t size;
    uint64_t gpa_start;
    void *hva;
};

static struct whpx_slot whpx_slots[WHPX_MAX_SLOTS];

void *whpx_gpa2hva(uint64_t gpa, bool *found)
{
    uint32_t i;
    *found = false;

    for (i = 0; i < WHPX_MAX_SLOTS; i++) {
        struct whpx_slot *slot = &whpx_slots[i];
        if (!slot->size) {
            continue;
        }
        if (gpa >= slot->gpa_start && gpa < slot->gpa_start + slot->size) {
            *found = true;
            return (void *)((char *)(slot->hva) + (gpa - slot->gpa_start));
        }
    }

    return NULL;
}

static struct whpx_slot *whpx_next_free_slot(void)
{
    int i;
    for (i = 0; i < WHPX_MAX_SLOTS; i++) {
        if (!whpx_slots[i].size) {
            return &whpx_slots[i];
        }
    }
    return NULL;
}

static struct whpx_slot *whpx_find_overlap_slot(uint64_t start, uint64_t end,
                                                 bool *coincident)
{
    int i;
    if (coincident) {
        *coincident = false;
    }

    for (i = 0; i < WHPX_MAX_SLOTS; i++) {
        struct whpx_slot *slot = &whpx_slots[i];
        if (slot->size && start < (slot->gpa_start + slot->size) &&
            end > slot->gpa_start) {
            if ((start == slot->gpa_start) &&
                (end == slot->gpa_start + slot->size)) {
                if (coincident) {
                    *coincident = true;
                }
            }
            return slot;
        }
    }

    return NULL;
}

static HRESULT whpx_set_ram(void *src, WHV_GUEST_PHYSICAL_ADDRESS gpa,
                            UINT64 size, WHV_MAP_GPA_RANGE_FLAGS flags,
                            int add)
{
    struct whpx_state *whpx = &whpx_global;
    HRESULT hr;

    if (add) {
        hr = whp_dispatch.WHvMapGpaRange(whpx->partition, src, gpa, size, flags);
    } else {
        hr = whp_dispatch.WHvUnmapGpaRange(whpx->partition, gpa, size);
    }

    if (FAILED(hr)) {
        error_report("WHPX: Failed to %s GPA range PA:%p, Size:%p bytes,"
                     " Host:%p, hr=%08lx",
                     (add ? "MAP" : "UNMAP"),
                     (void *)(uintptr_t)gpa, (void *)(uintptr_t)size, src, hr);
        return hr;
    }

    bool coincident;
    struct whpx_slot *overlap_slot =
        whpx_find_overlap_slot(gpa, gpa + size, &coincident);

    if (overlap_slot && coincident) {
        if (!add) {
            overlap_slot->size = 0;
        }
        return hr;
    }

    if (overlap_slot) {
        overlap_slot->size = 0;
    }

    if (add) {
        struct whpx_slot *new_slot = whpx_next_free_slot();
        if (!new_slot) {
            qemu_abort("%s: no free slots\n", __func__);
        }
        new_slot->size = size;
        new_slot->gpa_start = gpa;
        new_slot->hva = src;
    }

    return hr;
}

static void whpx_update_mapping(hwaddr start_pa, ram_addr_t size,
                                void *host_va, int add, int rom,
                                const char *name)
{
    whpx_set_ram(host_va, start_pa, size,
                 (WHvMapGpaRangeFlagRead |
                  WHvMapGpaRangeFlagExecute |
                  (rom ? 0 : WHvMapGpaRangeFlagWrite)),
                 add);
}

static void whpx_process_section(MemoryRegionSection *section, int add)
{
    MemoryRegion *mr = section->mr;
    hwaddr start_pa = section->offset_within_address_space;
    ram_addr_t size = int128_get64(section->size);
    unsigned int delta;
    uint64_t host_va;

    if (!memory_region_is_ram(mr)) {
        return;
    }

    if (memory_region_is_user_backed(mr)) {
        if (add && mr->ram_block && !mr->ram_block->host) {
            uint64_t block_size = mr->ram_block->used_length;
            void *backing = VirtualAlloc(NULL, (SIZE_T)block_size,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);
            if (!backing) {
                fprintf(stderr,
                        "WHPX: VirtualAlloc failed for user-backed region"
                        " %s (%llu MB)\n",
                        mr->name,
                        (unsigned long long)(block_size >> 20));
                return;
            }
            mr->ram_block->host = backing;
        }
        if (!mr->ram_block || !mr->ram_block->host) {
            return;
        }
    }

    delta = qemu_real_host_page_size - (start_pa & ~qemu_real_host_page_mask);
    delta &= ~qemu_real_host_page_mask;
    if (delta > size) {
        return;
    }
    start_pa += delta;
    size -= delta;
    size &= qemu_real_host_page_mask;
    if (!size || (start_pa & ~qemu_real_host_page_mask)) {
        return;
    }

    host_va = (uintptr_t)memory_region_get_ram_ptr(mr)
            + section->offset_within_region + delta;

    whpx_update_mapping(start_pa, size, (void *)(uintptr_t)host_va, add,
                        memory_region_is_rom(mr), mr->name);
}

static void whpx_user_backed_ram_map(uint64_t gpa, void *hva,
                                     uint64_t size, int flags)
{
    struct whpx_state *whpx = &whpx_global;
    WHV_MAP_GPA_RANGE_FLAGS whpx_flags = 0;
    if (flags & USER_BACKED_RAM_FLAGS_READ) {
        whpx_flags |= WHvMapGpaRangeFlagRead;
    }
    if (flags & USER_BACKED_RAM_FLAGS_WRITE) {
        whpx_flags |= WHvMapGpaRangeFlagWrite;
    }
    if (flags & USER_BACKED_RAM_FLAGS_EXEC) {
        whpx_flags |= WHvMapGpaRangeFlagExecute;
    }

    whp_dispatch.WHvUnmapGpaRange(whpx->partition, gpa, size);

    HRESULT hr = whpx_set_ram(hva, gpa, size, whpx_flags, 1);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to map GPA range [0x%llx 0x%llx] to HVA %p",
                     (unsigned long long)gpa,
                     (unsigned long long)(gpa + size), hva);
    }
}

static void whpx_user_backed_ram_unmap(uint64_t gpa, uint64_t size)
{
    HRESULT hr = whpx_set_ram(NULL, gpa, size, 0, 0);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to unmap GPA range [0x%llx 0x%llx]",
                     (unsigned long long)gpa,
                     (unsigned long long)(gpa + size));
    }
}

static void whpx_region_add(MemoryListener *listener,
                           MemoryRegionSection *section)
{
    memory_region_ref(section->mr);
    whpx_process_section(section, 1);
}

static void whpx_region_del(MemoryListener *listener,
                           MemoryRegionSection *section)
{
    whpx_process_section(section, 0);
    memory_region_unref(section->mr);
}

static void whpx_transaction_begin(MemoryListener *listener)
{
}

static void whpx_transaction_commit(MemoryListener *listener)
{
}

static void whpx_log_sync(MemoryListener *listener,
                         MemoryRegionSection *section)
{
    MemoryRegion *mr = section->mr;
    if (!memory_region_is_ram(mr)) {
        return;
    }
    memory_region_set_dirty(mr, 0, int128_get64(section->size));
}

static MemoryListener whpx_memory_listener = {
    .begin = whpx_transaction_begin,
    .commit = whpx_transaction_commit,
    .region_add = whpx_region_add,
    .region_del = whpx_region_del,
    .log_sync = whpx_log_sync,
    .priority = 10,
};

static void whpx_memory_init(void)
{
    memory_listener_register(&whpx_memory_listener, &address_space_memory);
    qemu_set_user_backed_mapping_funcs(
        whpx_user_backed_ram_map,
        whpx_user_backed_ram_unmap);
}

/* ==================== Interrupt Handling ==================== */

static void whpx_handle_interrupt(CPUState *cpu, int mask)
{
    cpu->interrupt_request |= mask;

    if (!qemu_cpu_is_self(cpu)) {
        qemu_cpu_kick(cpu);
    }
}

/* ==================== Periodic Cancel Thread ==================== */

static DWORD WINAPI whpx_cancel_thread_func(LPVOID param)
{
    struct whpx_state *whpx = (struct whpx_state *)param;

    Sleep(1000);

    while (InterlockedCompareExchange(&whpx_cancel_thread_running, 1, 1)) {
        Sleep(2);

        for (int i = 0; i < whpx_num_cpus; i++) {
            whp_dispatch.WHvCancelRunVirtualProcessor(
                whpx->partition, i, 0);
        }
    }

    return 0;
}

/* ==================== Partition Init ==================== */

static int whpx_accel_init(MachineState *ms)
{
    struct whpx_state *whpx;
    int ret;
    HRESULT hr;
    WHV_CAPABILITY whpx_cap;
    UINT32 whpx_cap_size;
    WHV_PARTITION_PROPERTY prop;

    whpx = &whpx_global;

    if (!init_whp_dispatch()) {
        ret = -ENOSYS;
        goto error;
    }

    memset(whpx, 0, sizeof(struct whpx_state));
    memset(whpx_slots, 0, sizeof(whpx_slots));

    whpx->mem_quota = ms->ram_size;

    hr = whp_dispatch.WHvGetCapability(
        WHvCapabilityCodeHypervisorPresent, &whpx_cap,
        sizeof(whpx_cap), &whpx_cap_size);
    if (FAILED(hr) || !whpx_cap.HypervisorPresent) {
        error_report("WHPX: No accelerator found, hr=%08lx", hr);
        ret = -ENOSPC;
        goto error;
    }

    hr = whp_dispatch.WHvCreatePartition(&whpx->partition);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to create partition, hr=%08lx", hr);
        ret = -EINVAL;
        goto error;
    }

    memset(&prop, 0, sizeof(WHV_PARTITION_PROPERTY));
    prop.ProcessorCount = smp_cpus ? smp_cpus : 1;
    hr = whp_dispatch.WHvSetPartitionProperty(
        whpx->partition,
        WHvPartitionPropertyCodeProcessorCount,
        &prop,
        sizeof(WHV_PARTITION_PROPERTY));

    if (FAILED(hr)) {
        error_report("WHPX: Failed to set partition processor count to %d,"
                     " hr=%08lx", smp_cpus, hr);
        ret = -EINVAL;
        goto error;
    }

    /* ARM64: Configure GICv3 interrupt controller (required before SetupPartition) */
    {
        WHV_ARM64_IC_PARAMETERS ic_params;
        memset(&ic_params, 0, sizeof(ic_params));
        ic_params.EmulationMode = WHvArm64IcEmulationModeGicV3;
        ic_params.GicV3Parameters.GicdBaseAddress = 0x08000000;
        ic_params.GicV3Parameters.GitsTranslaterBaseAddress = 0x08080000;
        ic_params.GicV3Parameters.GicLpiIntIdBits = 0;
        ic_params.GicV3Parameters.GicPpiPerformanceMonitorsInterrupt = 23;
        ic_params.GicV3Parameters.GicPpiOverflowInterruptFromCntv = 27;

        hr = whp_dispatch.WHvSetPartitionProperty(
            whpx->partition,
            WHvPartitionPropertyCodeArm64IcParameters,
            &ic_params,
            sizeof(ic_params));
        if (FAILED(hr)) {
            error_report("WHPX: Failed to set ARM64 IC parameters, hr=%08lx", hr);
            ret = -EINVAL;
            goto error;
        }
    }

    hr = whp_dispatch.WHvSetupPartition(whpx->partition);
    if (FAILED(hr)) {
        error_report("WHPX: Failed to setup partition, hr=%08lx", hr);
        ret = -EINVAL;
        goto error;
    }

    /* GICR is managed by the hypervisor via WHvArm64RegisterGicrBaseGpa.
     * Don't map GICR as RAM — that would shadow the hypervisor's GICR. */

    whpx_memory_init();

    cpu_interrupt_handler = whpx_handle_interrupt;

    whpx_num_cpus = smp_cpus > 0 ? smp_cpus : 1;

    InterlockedExchange(&whpx_cancel_thread_running, 1);
    whpx_cancel_thread_handle = CreateThread(
        NULL, 0, whpx_cancel_thread_func, whpx, 0, NULL);

    printf("Windows Hypervisor Platform accelerator is operational (ARM64)\n");
    whpx_allowed = true;
    return 0;

error:
    if (whpx->partition) {
        whp_dispatch.WHvDeletePartition(whpx->partition);
        whpx->partition = NULL;
    }

    return ret;
}

__declspec(noinline) int whpx_enabled(void)
{
    return whpx_allowed;
}

/* ==================== Type Registration ==================== */

static void whpx_accel_class_init(ObjectClass *oc, void *data)
{
    AccelClass *ac = ACCEL_CLASS(oc);
    ac->name = "WHPX";
    ac->init_machine = whpx_accel_init;
    ac->allowed = &whpx_allowed;
}

static const TypeInfo whpx_accel_type = {
    .name = ACCEL_CLASS_NAME("whpx"),
    .parent = TYPE_ACCEL,
    .class_init = whpx_accel_class_init,
};

static bool whpx_type_registered = false;

void whpx_arm64_register_types(void)
{
    if (!whpx_type_registered) {
        whpx_type_registered = true;
        type_register_static(&whpx_accel_type);
    }
}

static void whpx_type_init(void)
{
    whpx_arm64_register_types();
}

type_init(whpx_type_init);
