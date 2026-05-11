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
#include <string>
#include "absl/strings/str_format.h"
#include "android/avd/info.h"
#include "android/base/system/System.h"
#include "android/emulation/compatibility_check.h"

#include "host-common/FeatureControl.h"  // for isEnabled

namespace android {
namespace emulation {

using android::base::System;
using namespace android_studio;

// A check to make sure various system properties (OS, CPU, RAM) are
// supported for the target AVD
AvdCompatibilityCheckResult hasSufficientSystem(AvdInfo* avd) {
    android_studio::EmulatorCompatibilityInfo metrics;
    if (avd == nullptr) {
        metrics.set_check(
                EmulatorCompatibilityInfo::AVD_COMPATIBILITY_CHECK_NO_AVD);
        return {.description =
                        "No avd present, cannot check for system capabilities",
                .status = AvdCompatibility::Error,
                .metrics = metrics};
    }

    auto sys = System::get();
    // Allow users and tests to skip compatibility checks
    if (sys->envGet("ANDROID_EMU_SKIP_SYSTEM_CHECKS") == "1") {
        metrics.set_check(
                EmulatorCompatibilityInfo::AVD_COMPATIBILITY_CHECK_SYSTEM_SKIP);
        return {.description = "System compatibility checks are disabled",
                .status = AvdCompatibility::Warning,
                .metrics = metrics};
    }
    if (sys->envGet("ANDROID_EMU_ABORT_SYSTEM_CHECKS") == "1") {
        metrics.set_check(EmulatorCompatibilityInfo::
                                  AVD_COMPATIBILITY_CHECK_SYSTEM_ABORT);
        return {.description =
                        "The user forced a compatibility error, unset "
                        "ANDROID_EMU_ABORT_SYSTEM_CHECKS environment variable "
                        "to launch the emulator",
                .status = AvdCompatibility::Error,
                .metrics = metrics};
    }

    const char* avdName = avdInfo_getName(avd);
    const bool isXrAvd = (avdInfo_getAvdFlavor(avd) == AVD_DEV_2024);

    // Check number of cores
    const int numCores = System::get()->getCpuCoreCount();
    const int minNumCores = isXrAvd ? 4 : 2;
    const int idealMinNumCores = isXrAvd ? 8 : 4;
    if (numCores < minNumCores) {
        // < 0.1% of our users as of November 2024
        metrics.set_check(
                EmulatorCompatibilityInfo::AVD_COMPATIBILITY_CHECK_SYSTEM_CORE);
        metrics.set_details(absl::StrFormat("numCores: %d", numCores));
        return {.description =
                        absl::StrFormat("AVD '%s' requires %d CPU cores to "
                                        "run. Only %d cores are available.",
                                        avdName, minNumCores, numCores),
                .status = AvdCompatibility::Error,
                .metrics = metrics};
    } else if (numCores < idealMinNumCores) {
        // < 2% of our users as of November 2024
        metrics.set_check(
                EmulatorCompatibilityInfo::AVD_COMPATIBILITY_CHECK_SYSTEM_CORE);
        metrics.set_details(absl::StrFormat("numCores: %d", numCores));

        return {.description =
                        absl::StrFormat("AVD '%s' will run more smoothly with "
                                        "%d CPU cores (currently using %d)",
                                        avdName, idealMinNumCores, numCores),
                .status = AvdCompatibility::Warning,
                .metrics = metrics};
    }

    // Check system RAM
    const android::base::MemUsage memUsage = System::get()->getMemUsage();
    if (memUsage.total_phys_memory == 0) {
        metrics.set_check(EmulatorCompatibilityInfo::
                                  AVD_COMPATIBILITY_CHECK_SYSTEM_MEMORY);
        metrics.set_details("MemFail");
        return {.description = absl::StrFormat(
                        "Unable to determine available system memory"),
                .status = AvdCompatibility::Warning,
                .metrics = metrics};
    }
    const uint64_t ramMB = (memUsage.total_phys_memory / (1024 * 1024));
    const uint64_t minRamMB = 2048;
    const uint64_t idealMinRamMB = isXrAvd ? 16384 : 4096;
    // < 5% of our users as of November 2024
    // TODO(b/376873919): Improve the reporting to account for avd requirements.
    if (ramMB < minRamMB) {
        metrics.set_check(EmulatorCompatibilityInfo::
                                  AVD_COMPATIBILITY_CHECK_SYSTEM_MEMORY);
        metrics.set_details(std::to_string(ramMB));
        return {.description = absl::StrFormat(
                        "Available system RAM is not enough to run "
                        "avd: '%s'. Available: %d MiB, minimum "
                        "required: %d MiB",
                        avdName, ramMB, minRamMB),
                .status = AvdCompatibility::Error,
                .metrics = metrics};
    } else if (ramMB < idealMinRamMB) {
        metrics.set_check(EmulatorCompatibilityInfo::
                                  AVD_COMPATIBILITY_CHECK_SYSTEM_MEMORY);
        metrics.set_details(std::to_string(ramMB));
        return {
                .description = absl::StrFormat(
                        "Suggested minimum system RAM to run "
                        "avd '%s' is %d MiB (available: %d MiB)",
                        avdName, idealMinRamMB, ramMB),
                .status = AvdCompatibility::Warning,
                .metrics = metrics};
    }

    return {
            .description = absl::StrFormat(
                    "System requirements to run avd: `%s` are met", avdName),
            .status = AvdCompatibility::Ok,
    };
}

REGISTER_COMPATIBILITY_CHECK(hasSufficientSystem);
}  // namespace emulation
}  // namespace android
