/*
 * QEMU Windows Hypervisor Platform (WHPX) - ARM64 definitions
 *
 * ARM64 register names and structures for WHPX that are not yet in
 * the Windows SDK 10.0.22621.0. These values are from the Windows 11
 * 24H2 SDK (10.0.26100.0+) and mingw-w64 headers.
 */

#ifndef WHPX_ARM64_H
#define WHPX_ARM64_H

/* WinHvPlatform.h must be included before this header */

/* ARM64 General Purpose Registers */
#define WHvArm64RegisterX0      ((WHV_REGISTER_NAME)0x00020000)
#define WHvArm64RegisterX1      ((WHV_REGISTER_NAME)0x00020001)
#define WHvArm64RegisterX2      ((WHV_REGISTER_NAME)0x00020002)
#define WHvArm64RegisterX3      ((WHV_REGISTER_NAME)0x00020003)
#define WHvArm64RegisterX4      ((WHV_REGISTER_NAME)0x00020004)
#define WHvArm64RegisterX5      ((WHV_REGISTER_NAME)0x00020005)
#define WHvArm64RegisterX6      ((WHV_REGISTER_NAME)0x00020006)
#define WHvArm64RegisterX7      ((WHV_REGISTER_NAME)0x00020007)
#define WHvArm64RegisterX8      ((WHV_REGISTER_NAME)0x00020008)
#define WHvArm64RegisterX9      ((WHV_REGISTER_NAME)0x00020009)
#define WHvArm64RegisterX10     ((WHV_REGISTER_NAME)0x0002000A)
#define WHvArm64RegisterX11     ((WHV_REGISTER_NAME)0x0002000B)
#define WHvArm64RegisterX12     ((WHV_REGISTER_NAME)0x0002000C)
#define WHvArm64RegisterX13     ((WHV_REGISTER_NAME)0x0002000D)
#define WHvArm64RegisterX14     ((WHV_REGISTER_NAME)0x0002000E)
#define WHvArm64RegisterX15     ((WHV_REGISTER_NAME)0x0002000F)
#define WHvArm64RegisterX16     ((WHV_REGISTER_NAME)0x00020010)
#define WHvArm64RegisterX17     ((WHV_REGISTER_NAME)0x00020011)
#define WHvArm64RegisterX18     ((WHV_REGISTER_NAME)0x00020012)
#define WHvArm64RegisterX19     ((WHV_REGISTER_NAME)0x00020013)
#define WHvArm64RegisterX20     ((WHV_REGISTER_NAME)0x00020014)
#define WHvArm64RegisterX21     ((WHV_REGISTER_NAME)0x00020015)
#define WHvArm64RegisterX22     ((WHV_REGISTER_NAME)0x00020016)
#define WHvArm64RegisterX23     ((WHV_REGISTER_NAME)0x00020017)
#define WHvArm64RegisterX24     ((WHV_REGISTER_NAME)0x00020018)
#define WHvArm64RegisterX25     ((WHV_REGISTER_NAME)0x00020019)
#define WHvArm64RegisterX26     ((WHV_REGISTER_NAME)0x0002001A)
#define WHvArm64RegisterX27     ((WHV_REGISTER_NAME)0x0002001B)
#define WHvArm64RegisterX28     ((WHV_REGISTER_NAME)0x0002001C)
#define WHvArm64RegisterFp      ((WHV_REGISTER_NAME)0x0002001D)
#define WHvArm64RegisterLr      ((WHV_REGISTER_NAME)0x0002001E)
#define WHvArm64RegisterSp      ((WHV_REGISTER_NAME)0x0002001F)
#define WHvArm64RegisterSpEl0   ((WHV_REGISTER_NAME)0x00020020)
#define WHvArm64RegisterSpEl1   ((WHV_REGISTER_NAME)0x00020021)
#define WHvArm64RegisterPc      ((WHV_REGISTER_NAME)0x00020022)
#define WHvArm64RegisterPstate  ((WHV_REGISTER_NAME)0x00020023)

/* ARM64 SIMD/FP Registers (128-bit) */
#define WHvArm64RegisterQ0      ((WHV_REGISTER_NAME)0x00030000)
#define WHvArm64RegisterQ1      ((WHV_REGISTER_NAME)0x00030001)
#define WHvArm64RegisterQ2      ((WHV_REGISTER_NAME)0x00030002)
#define WHvArm64RegisterQ3      ((WHV_REGISTER_NAME)0x00030003)
#define WHvArm64RegisterQ4      ((WHV_REGISTER_NAME)0x00030004)
#define WHvArm64RegisterQ5      ((WHV_REGISTER_NAME)0x00030005)
#define WHvArm64RegisterQ6      ((WHV_REGISTER_NAME)0x00030006)
#define WHvArm64RegisterQ7      ((WHV_REGISTER_NAME)0x00030007)
#define WHvArm64RegisterQ8      ((WHV_REGISTER_NAME)0x00030008)
#define WHvArm64RegisterQ9      ((WHV_REGISTER_NAME)0x00030009)
#define WHvArm64RegisterQ10     ((WHV_REGISTER_NAME)0x0003000A)
#define WHvArm64RegisterQ11     ((WHV_REGISTER_NAME)0x0003000B)
#define WHvArm64RegisterQ12     ((WHV_REGISTER_NAME)0x0003000C)
#define WHvArm64RegisterQ13     ((WHV_REGISTER_NAME)0x0003000D)
#define WHvArm64RegisterQ14     ((WHV_REGISTER_NAME)0x0003000E)
#define WHvArm64RegisterQ15     ((WHV_REGISTER_NAME)0x0003000F)
#define WHvArm64RegisterQ16     ((WHV_REGISTER_NAME)0x00030010)
#define WHvArm64RegisterQ17     ((WHV_REGISTER_NAME)0x00030011)
#define WHvArm64RegisterQ18     ((WHV_REGISTER_NAME)0x00030012)
#define WHvArm64RegisterQ19     ((WHV_REGISTER_NAME)0x00030013)
#define WHvArm64RegisterQ20     ((WHV_REGISTER_NAME)0x00030014)
#define WHvArm64RegisterQ21     ((WHV_REGISTER_NAME)0x00030015)
#define WHvArm64RegisterQ22     ((WHV_REGISTER_NAME)0x00030016)
#define WHvArm64RegisterQ23     ((WHV_REGISTER_NAME)0x00030017)
#define WHvArm64RegisterQ24     ((WHV_REGISTER_NAME)0x00030018)
#define WHvArm64RegisterQ25     ((WHV_REGISTER_NAME)0x00030019)
#define WHvArm64RegisterQ26     ((WHV_REGISTER_NAME)0x0003001A)
#define WHvArm64RegisterQ27     ((WHV_REGISTER_NAME)0x0003001B)
#define WHvArm64RegisterQ28     ((WHV_REGISTER_NAME)0x0003001C)
#define WHvArm64RegisterQ29     ((WHV_REGISTER_NAME)0x0003001D)
#define WHvArm64RegisterQ30     ((WHV_REGISTER_NAME)0x0003001E)
#define WHvArm64RegisterQ31     ((WHV_REGISTER_NAME)0x0003001F)

/* ARM64 FP Control/Status (in system register space, not SIMD) */
#define WHvArm64RegisterFpcr    ((WHV_REGISTER_NAME)0x00040009)
#define WHvArm64RegisterFpsr    ((WHV_REGISTER_NAME)0x0004000A)

/* ARM64 System Registers - EL1 */
#define WHvArm64RegisterSctlrEl1   ((WHV_REGISTER_NAME)0x00040002)
#define WHvArm64RegisterTtbr0El1   ((WHV_REGISTER_NAME)0x00040004)
#define WHvArm64RegisterTtbr1El1   ((WHV_REGISTER_NAME)0x00040005)
#define WHvArm64RegisterTcrEl1     ((WHV_REGISTER_NAME)0x00040006)
#define WHvArm64RegisterEsrEl1     ((WHV_REGISTER_NAME)0x00040007)
#define WHvArm64RegisterFarEl1     ((WHV_REGISTER_NAME)0x00040008)
#define WHvArm64RegisterMairEl1    ((WHV_REGISTER_NAME)0x0004000B)
#define WHvArm64RegisterVbarEl1    ((WHV_REGISTER_NAME)0x0004000C)
#define WHvArm64RegisterElrEl1     ((WHV_REGISTER_NAME)0x0004000E)
#define WHvArm64RegisterSpsrEl1    ((WHV_REGISTER_NAME)0x00040010)

/* Timer registers */
#define WHvArm64RegisterCntkvctEl1  ((WHV_REGISTER_NAME)0x00040100)
#define WHvArm64RegisterCntvCtlEl0  ((WHV_REGISTER_NAME)0x00040101)
#define WHvArm64RegisterCntvCvalEl0 ((WHV_REGISTER_NAME)0x00040102)
#define WHvArm64RegisterCntpCtlEl0  ((WHV_REGISTER_NAME)0x00040103)
#define WHvArm64RegisterCntpCvalEl0 ((WHV_REGISTER_NAME)0x00040104)

/* GIC Redistributor base address (per-vCPU) */
#define WHvArm64RegisterGicrBaseGpa ((WHV_REGISTER_NAME)0x00063000)

/* ARM64 Exit Reasons (from WHV_RUN_VP_EXIT_REASON) */
#define WHvRunVpExitReasonNone                  0x00000000
#define WHvRunVpExitReasonMemoryAccess          0x00000001
#define WHvRunVpExitReasonUnmappedMemory        0x00000001
#define WHvRunVpExitReasonX64IoPortAccess       0x00000002
#define WHvRunVpExitReasonUnrecoverableException 0x00000004
#define WHvRunVpExitReasonInvalidVpRegisterValue 0x00000005
#define WHvRunVpExitReasonUnsupportedFeature    0x00000006
#define WHvRunVpExitReasonX64InterruptWindow    0x00000007
#define WHvRunVpExitReasonArm64Reset            0x0000000C
#define WHvRunVpExitReasonCanceled              0x00000002

/* ARM64-specific exit context structures */
typedef struct WHV_ARM64_MEMORY_ACCESS_INFO {
    UINT32 AccessType : 2;  /* 0=read, 1=write */
    UINT32 Reserved : 30;
} WHV_ARM64_MEMORY_ACCESS_INFO;

typedef struct WHV_ARM64_MEMORY_ACCESS_CONTEXT {
    UINT8 InstructionByteCount;
    UINT8 Reserved[3];
    UINT8 InstructionBytes[4];
    WHV_ARM64_MEMORY_ACCESS_INFO AccessInfo;
    UINT64 Gpa;
    UINT64 Gva;
} WHV_ARM64_MEMORY_ACCESS_CONTEXT;

/* Processor features for ARM64 partition setup */
#define WHvPartitionPropertyCodeProcessorFeatures  ((WHV_PARTITION_PROPERTY_CODE)0x00001001)
#define WHvPartitionPropertyCodeProcessorCount     ((WHV_PARTITION_PROPERTY_CODE)0x00001fff)

#endif /* WHPX_ARM64_H */
