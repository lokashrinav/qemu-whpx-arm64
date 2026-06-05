# QEMU WHPX ARM64 — Hardware-Accelerated Android Emulation on Windows

Run ARM64 Android virtual devices at near-native speed on Windows using the Windows Hypervisor Platform (WHPX). No more slow software emulation — this QEMU fork uses Hyper-V to hardware-accelerate ARM64 vCPUs directly.

## Why This Exists

The stock Android Emulator uses KVM on Linux and HAXM/Hyper-V on Windows, but only for x86 guests. If you want to run ARM64 Android images on Windows — whether for native ARM app testing, ARM64 Windows development, or accurate device emulation — you're stuck with software translation (TCG), which is painfully slow.

This fork adds a complete WHPX accelerator backend for ARM64, giving you hardware-accelerated ARM64 Android emulation on Windows 11. It works on both x86_64 and ARM64 Windows hosts.

## What's Under the Hood

| Component | Details |
|---|---|
| **Hypervisor** | Windows Hypervisor Platform (WHPX) via `WinHvPlatform.dll` |
| **CPU** | ARM64 vCPU with full GP (X0–X30), SIMD (Q0–Q31), and system register mapping |
| **Interrupt Controller** | GICv3 with Redistributor emulation |
| **Multi-core** | PSCI (Power State Coordination Interface) for SMP boot |
| **Virtual Board** | Ranchu — Android's virtual board with Goldfish + Virtio devices |
| **Graphics** | Virtio-GPU with gfxstream, OpenGL/EGL passthrough |
| **Display** | SDL2, GTK, VNC backends |

### Architecture

```
┌──────────────────────────────────────────────────┐
│                  Android Guest                    │
│              (ARM64 kernel + system)              │
├──────────────────────────────────────────────────┤
│              Ranchu Virtual Board                 │
│  ┌──────────┬──────────┬───────────┬───────────┐ │
│  │ GICv3    │ PL011    │ Goldfish  │ Virtio    │ │
│  │ (IRQ)    │ (UART)   │ (devices) │ (GPU/net) │ │
│  └──────────┴──────────┴───────────┴───────────┘ │
├──────────────────────────────────────────────────┤
│           WHPX ARM64 Accelerator                  │
│     WHvRunVirtualProcessor · Register sync        │
│     Memory mapping · Instruction emulation        │
├──────────────────────────────────────────────────┤
│         Windows Hypervisor Platform               │
│              (Hyper-V partition)                   │
└──────────────────────────────────────────────────┘
```

## Requirements

- **OS:** Windows 11 (build 22621+, 26100+ recommended for full ARM64 WHPX support)
- **Hyper-V:** Enabled in Windows Features → "Windows Hypervisor Platform"
- **Build tools:** Visual Studio 2022 with C++ desktop workload
- **Python:** 3.x
- **Git:** With symlinks enabled (`git config --global core.symlinks true`)

### Enable Hyper-V

```powershell
# Run as Administrator
Enable-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform -All
Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All -All
# Reboot required
```

## Quick Start

### Building

```bat
:: Clone with full repo tooling
cd \ && mkdir emu-master-dev && cd emu-master-dev
repo init -u https://android.googlesource.com/platform/manifest -b emu-master-dev
repo sync

:: Build the emulator
cd external\qemu
android\rebuild
```

Binaries land in `objs/`. For incremental builds:

```bat
ninja -C objs
```

### Running

```bat
:: List available virtual devices
objs\emulator -list-avds

:: Launch with WHPX acceleration
objs\emulator -avd <avd_name> -accel whpx

:: Launch with the GUI launcher (see below)
cd launcher && python launcher.py
```

### Using the Launcher

A web-based launcher UI is included for easy configuration and management:

```bash
cd launcher
pip install -r requirements.txt
python launcher.py
```

This opens a local dashboard where you can configure AVDs, check system compatibility, and launch emulator instances with one click. See [launcher/README.md](launcher/) for details.

## Project Structure

```
├── target/arm/
│   ├── whpx-all.c            # WHPX ARM64 accelerator (core implementation)
│   └── whpx-arm64.h          # ARM64 register definitions for WHPX
├── hw/arm/
│   └── ranchu.c              # Android Ranchu virtual board
├── android-qemu2-glue/
│   ├── main.cpp              # Emulator entry point
│   ├── display.cpp           # Display system bridge
│   └── emulation/            # Device emulation (Goldfish, Virtio-GPU)
├── ui/                        # Display backends (SDL2, GTK, VNC)
├── accel/                     # Accelerator backends (WHPX, KVM, TCG)
├── launcher/                  # Web-based launcher UI
└── android/
    └── docs/                  # Platform build guides
```

## Key Implementation Details

The WHPX ARM64 backend (`target/arm/whpx-all.c`) handles:

- **Partition lifecycle** — Creates a Hyper-V partition, configures GICv3 interrupt controller parameters, maps guest physical memory
- **vCPU execution loop** — Calls `WHvRunVirtualProcessor`, handles exits for memory access, system register traps, PSCI calls, and interrupts
- **Register synchronization** — Bidirectional mapping between QEMU's `CPUARMState` and WHPX register arrays (GP, SIMD, system registers)
- **GICv3 emulation** — Configures distributor/redistributor base addresses, handles interrupt routing, periodic timer interrupt injection via a dedicated thread
- **PSCI support** — Implements `CPU_ON`, `CPU_OFF`, `SYSTEM_RESET` for multi-core boot and power management

Register definitions in `whpx-arm64.h` are backported from Windows SDK 10.0.26100.0+ to support builds with older SDK versions.

## Verified Boot (ARM64 Windows on Snapdragon X Elite)

Successfully booted Android 14 Linux kernel with WHPX hardware acceleration:

```
Windows Hypervisor Platform accelerator is operational (ARM64)
[    0.000000] Booting Linux on physical CPU 0x0000000000 [0x511f0011]
[    0.000000] Linux version 6.1.23-android14-4-00257-g7e35917775b8
[    0.000000] Machine model: linux,ranchu
...
[    3.384645] Freeing unused kernel memory: 1344K
```

- Kernel: Android 14, Linux 6.1.23 (ARM64)
- Host: Windows 11 ARM64 (Snapdragon X Elite, 32GB)
- Accelerator: WHPX (Windows Hypervisor Platform)
- Guest RAM: 2GB, vCPU: Cortex-A57
- Boot time: ~3.4s to kernel init complete

### Boot Command

```bat
qemu-system-aarch64.exe ^
  -machine virt -cpu cortex-a57 -accel whpx -m 2048 ^
  -nographic -serial stdio -net none ^
  -kernel path\to\kernel-ranchu ^
  -initrd path\to\ramdisk.img ^
  -drive if=none,id=system,file=path\to\system.img,format=raw,readonly=on ^
  -device virtio-blk-device,drive=system ^
  -append "console=ttyAMA0 androidboot.hardware=ranchu"
```

## ARM64 Windows Build Notes

Building on ARM64 Windows required several patches to the AOSP emulator source:

1. **Cross-compilation toolchain** — ARM64 clang-cl with `--target=arm64-pc-windows-msvc`, ARM64 MSVC linker
2. **gRPC code generation** — Built `grpc_cpp_plugin` from source (fixed absl::string_view ABI mismatch with `/std:c++14`)
3. **x86 intrinsic guards** — Wrapped 92 x86 headers with `#if !defined(__aarch64__)` guards
4. **WHPX accelerator** — Compiled `target/arm/whpx-all.c` with `-DCONFIG_WHPX -DTARGET_AARCH64`
5. **Stub DLLs** — Created ARM64 stub DLLs for metrics, logging, and agent subsystems
6. **Data symbol fixes** — Redirected global variable alternatenames to zero-filled data stubs (not function code)

## Platform Build Guides

- [Windows](android/docs/WINDOWS-DEV.md)
- [Linux](android/docs/LINUX-DEV.md)
- [macOS](android/docs/DARWIN-DEV.md)

## License

GNU General Public License v2. See [LICENSE](LICENSE) for details. QEMU is a trademark of Fabrice Bellard.
