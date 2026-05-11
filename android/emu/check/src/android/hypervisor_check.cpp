// Copyright 2024 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "absl/strings/str_format.h"
#include "android/avd/info.h"
#include "android/base/system/System.h"
#include "android/emulation/compatibility_check.h"
#include "android/cpu_accelerator.h"

#ifdef _WIN32
namespace android {
namespace emulation {

using android::base::System;
using android_studio::EmulatorCompatibilityInfo;

// A check to make sure there is a enough disk space available
// for the given avd.
AvdCompatibilityCheckResult hasCompatibleHypervisor(AvdInfo* avd) {
    android_studio::EmulatorCompatibilityInfo metrics;
    if (avd == nullptr) {
        metrics.set_check(
                EmulatorCompatibilityInfo::AVD_COMPATIBILITY_CHECK_NO_AVD);
        return {
                .description = "No avd present, cannot check hypervisor compatibility",
                .status = AvdCompatibility::Warning,
                .metrics = metrics};
    }

    // Allow users and tests to skip compatibility checks
    if (System::get()->envGet("ANDROID_EMU_SKIP_HYP_CHECKS") == "1") {
        return {
                .description = "Hypervisor compatibility checks are disabled",
                .status = AvdCompatibility::Warning,
        };
    }

    const char* name = avdInfo_getName(avd);
    const bool isXrAvd = (avdInfo_getAvdFlavor(avd) == AVD_DEV_2024);
    AndroidCpuAccelerator accelerator = androidCpuAcceleration_getAccelerator();

    if (isXrAvd && (accelerator == ANDROID_CPU_ACCELERATOR_AEHD ||
                    accelerator == ANDROID_CPU_ACCELERATOR_HAX)) {
        metrics.set_check(EmulatorCompatibilityInfo::AVD_COMPATIBILITY_CHECK_GPU_CHECK_NO_VULKAN);
        metrics.set_details("Accelerator for Xr");
        return {
                .description = absl::StrFormat(
                        "Your current hypervisor (AEHD or HAXM) is not compatible with Android XR AVD %s. "
                        "Please install WHPX instead. "
                        "Refer to https://developer.android.com/studio/run/emulator-acceleration#vm-windows-whpx",
                        name),
                .status = AvdCompatibility::Warning,
                .metrics = metrics};
    }

    return {
            .description = absl::StrFormat(
                    "Hypervisor compatibility to run avd: `%s` are met", name),
            .status = AvdCompatibility::Ok,
    };
};

REGISTER_COMPATIBILITY_CHECK(hasCompatibleHypervisor);

}  // namespace emulation
}  // namespace android
#endif
