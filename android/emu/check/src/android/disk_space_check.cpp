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

namespace android {
namespace emulation {

using android::base::System;
using namespace android_studio;

// A check to make sure there is a enough disk space available
// for the given avd.
AvdCompatibilityCheckResult hasSufficientDiskSpace(AvdInfo* avd) {
    android_studio::EmulatorCompatibilityInfo metrics;
    metrics.set_check(EmulatorCompatibilityInfo::
                              AVD_COMPATIBILITY_CHECK_INSUFFICIENT_DISK_SPACE);
    if (avd == nullptr) {
        metrics.set_check(
                EmulatorCompatibilityInfo::AVD_COMPATIBILITY_CHECK_NO_AVD);
        return {.description = "No avd present, cannot check for disk space",
                .status = AvdCompatibility::Warning,
                .metrics = metrics};
    }
    const char* name = avdInfo_getName(avd);
    System::FileSize freeDisk;
    bool underPressure =
            System::isUnderDiskPressure(avdInfo_getContentPath(avd), &freeDisk);
    if (underPressure) {
        metrics.set_check(
                EmulatorCompatibilityInfo::
                        AVD_COMPATIBILITY_CHECK_INSUFFICIENT_DISK_SPACE);
        metrics.set_details(std::to_string(freeDisk));
        return {.description =
                        absl::StrFormat("Your device does not have enough disk "
                                        "space to run avd: `%s`",
                                        name),
                .status = AvdCompatibility::Error,
                .metrics = metrics};
    }

    return {.description = absl::StrFormat(
                    "Disk space requirements to run avd: `%s` are met", name),
            .status = AvdCompatibility::Ok,
            .metrics = metrics};
};

REGISTER_COMPATIBILITY_CHECK(hasSufficientDiskSpace);

}  // namespace emulation
}  // namespace android
