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

#include <vulkan/vulkan_core.h>
#include "host-common/FeatureControl.h"  // for isEnabled
#include "host-common/opengl/emugl_config.h"

namespace android {
namespace emulation {

using android::base::System;
using android_studio::EmulatorCompatibilityInfo;

// A check to make sure there is a enough GPU capabilities available
// for the given avd.
AvdCompatibilityCheckResult hasSufficientHwGpu(AvdInfo* avd) {
    EmulatorCompatibilityInfo metrics;
    if (avd == nullptr) {
        metrics.set_check(
                EmulatorCompatibilityInfo::AVD_COMPATIBILITY_CHECK_NO_AVD);
        return {.description =
                        "No avd present, cannot check for GPU capabilities",
                .status = AvdCompatibility::Warning,
                .metrics = metrics};
    }

    // Allow users and tests to skip compatibility checks
    if (System::get()->envGet("ANDROID_EMU_SKIP_GPU_CHECKS") == "1") {
        metrics.set_check(EmulatorCompatibilityInfo::
                                  AVD_COMPATIBILITY_CHECK_GPU_CHECK_SKIP);
        return {.description = "GPU compatibility checks are disabled",
                .status = AvdCompatibility::Warning,
                .metrics = metrics};
    }

    const char* name = avdInfo_getName(avd);
    const bool isXrAvd = (avdInfo_getAvdFlavor(avd) == AVD_DEV_2024);

    // TODO(b/373601997): Correctly determine if hardware gpu requested.
    // emuglConfig_get_current_renderer() == SELECTED_RENDERER_HOST won't
    // work as emuglconfig is not initialized when doing hw gpu checks.
    const bool hwGpuRequested = isXrAvd;
    if (!hwGpuRequested) {
        metrics.set_check(EmulatorCompatibilityInfo::
                                  AVD_COMPATIBILITY_CHECK_GPU_CHECK_SKIP);
        metrics.set_details(
                "Hardware GPU compatibility checks are not required");
        return {.description =
                        "Hardware GPU compatibility checks are not required",
                .status = AvdCompatibility::Ok,
                .metrics = metrics};
    }

    // Check XR specific compatibility issues
    // TODO(b/373601997): Improve supported platforms and configurations
    if (isXrAvd) {
        // Not supported on Mac Intel due to missing GPU features
#if defined(__APPLE__) && !defined(__arm64__)
        metrics.set_check(EmulatorCompatibilityInfo::AVD_COMPATIBILITY_CHECK_SYSTEM_CORE);
        metrics.set_details("MacIntel on Xr");
        return {.description =
                        absl::StrFormat("`%s` is not supported to run on "
                                        "Mac with Intel processors",
                                        name),
                .status = AvdCompatibility::Error,
                .metrics = metrics};
#endif
    }

    // Only apply the Vulkan related checks when GuestAngle is enabled
    namespace fc = android::featurecontrol;
    bool requiresHwGpuCheck = true;
#if defined(__APPLE__)
    requiresHwGpuCheck = false;
#else
    if (!fc::isEnabled(fc::GuestAngle)) {
        requiresHwGpuCheck = false;
    }
#endif

    if (!requiresHwGpuCheck) {
        return {.description = absl::StrFormat(
                        "Hardware GPU requirements to run avd: `%s` are passed",
                        name),
                .status = AvdCompatibility::Ok,
                .metrics = metrics};
    }

    char* vkVendor = nullptr;
    int vkMajor = 0;
    int vkMinor = 0;
    int vkPatch = 0;
    uint64_t vkDeviceMemBytes = 0;
    uint32_t vkDriverVersion = 0;
    uint64_t vkDeviceMaxAllocationCount = 0;

    emuglConfig_get_vulkan_hardware_gpu(&vkVendor, &vkMajor, &vkMinor, &vkPatch,
                                        &vkDeviceMemBytes, &vkDriverVersion,
                                        &vkDeviceMaxAllocationCount);
    if (!vkVendor) {
        // Could not properly detect the hardware parameters
        metrics.set_check(EmulatorCompatibilityInfo::
                                  AVD_COMPATIBILITY_CHECK_GPU_CHECK_NO_VULKAN);
        metrics.set_details("VulkanFail");
        return {.description = absl::StrFormat(
                        "Could not detect GPU for Vulkan compatibility "
                        "checks. Please try updating your GPU Drivers"),
                .status = AvdCompatibility::Error,
                .metrics = metrics};
    }

    // TODO(b/381540970): Use servers side flags and deny listings for filtering
    // GPU compatibility
    bool isAMD = (strncmp("AMD", vkVendor, 3) == 0);
    bool isIntel = (strncmp("Intel", vkVendor, 5) == 0);
    bool isNvidia = (strncmp("NVIDIA", vkVendor, 6) == 0);
    bool isUnsupportedGpuDriver = false;
    std::string driverVersionStr;
    if (isNvidia) {
        // Decode Nvidia driver version to make it meaningful to the users
        // Reference: VulkanDeviceInfo::getDriverVersion() at
        // https://github.com/SaschaWillems/VulkanCapsViewer/blob/master/vulkanDeviceInfo.cpp
        // 10 bits = major version (up to r1023)
        // 8 bits = minor version (up to 255)
        // 8 bits = secondary branch version/build version (up to 255)
        // 6 bits = tertiary branch/build version (up to 63)
        const uint32_t major = (vkDriverVersion >> 22) & 0x3ff;
        const uint32_t minor = (vkDriverVersion >> 14) & 0x0ff;

        driverVersionStr = std::to_string(major) + "." + std::to_string(minor);

        // Disallow driver versions below 553.35 as they may cause BSODs
        // (ref:b/379178011).
        if (major < 553 || (major == 553 && minor < 35)) {
            isUnsupportedGpuDriver = true;
        }
    } else {
        // Use regular VK_API_VERSION encoding to print the version.
        driverVersionStr =
                std::to_string(VK_API_VERSION_MAJOR(vkDriverVersion)) + "." +
                std::to_string(VK_API_VERSION_MINOR(vkDriverVersion)) + "." +
                std::to_string(VK_API_VERSION_PATCH(vkDriverVersion));

#if defined(_WIN32)
        // Based on androidEmuglConfigInit
        if (isAMD) {
            if (vkMajor == 1 && vkMinor < 3) {
                // on windows, amd gpu with api 1.2.x does not work
                // for vulkan, disable it
                isUnsupportedGpuDriver = true;
            }
        } else if (isIntel) {
            if (vkMajor == 1 &&
                ((vkMinor == 3 && vkPatch < 240) || (vkMinor < 3))) {
                // intel gpu with api < 1.3.240 does not work
                // for vulkan, disable it
                isUnsupportedGpuDriver = true;
            }
        }
#endif
#if defined(__linux__)
        if (isAMD &&
            strcmp("AMD Custom GPU 0405 (RADV VANGOGH)", vkVendor) == 0) {
            // on linux, this specific amd gpu does not work for vulkan
            // even with VulkanAllocateDeviceMemoryOnly, disable it
            // (b/225541819)
            isUnsupportedGpuDriver = true;
        }
#endif
    }

    const std::string vendorName = vkVendor;
    free(vkVendor);

    if (isUnsupportedGpuDriver) {
        metrics.set_check(
                EmulatorCompatibilityInfo::
                        AVD_COMPATIBILITY_CHECK_GPU_CHECK_UNSUPPORTED_VULKAN_VERSION);
        metrics.set_details(absl::StrFormat("GPU:%s, API: %d.%d.%d Driver: %s",
                                            vendorName, vkMajor,
                                            vkMinor, vkPatch,
                                            driverVersionStr));
        return {
                .description = absl::StrFormat(
                        "GPU driver is not supported to run avd: `%s`. "
                        "Your '%s' GPU has Vulkan API version `%d.%d.%d`, "
                        "driver version `%s` and is not supported for Vulkan",
                        name, vendorName, vkMajor, vkMinor, vkPatch,
                        driverVersionStr),
                .status = AvdCompatibility::Error,
                .metrics = metrics,
        };
    }

    // Check available GPU memory, use rounded MiB values to allow some driver margin.
    const uint64_t deviceMemMiB = vkDeviceMemBytes / (1024 * 1024);
    const uint64_t avdMinGpuMemMiB = isXrAvd ? (isAMD ? 4000 : 2000) : 0;
    if (deviceMemMiB < avdMinGpuMemMiB) {
        metrics.set_check(
                EmulatorCompatibilityInfo::
                        AVD_COMPATIBILITY_CHECK_GPU_CHECK_INSUFFICIENT_MEMORY);
        metrics.set_details(absl::StrFormat("GPU:%s, deviceMemMiB: %llu",
                                            vendorName, deviceMemMiB));
        return {
                .description = absl::StrFormat(
                        "Not enough GPU memory available to run avd: `%s`. "
                        "Available: %llu MB, minimum required: %llu MB",
                        name, deviceMemMiB, avdMinGpuMemMiB),
                .status = AvdCompatibility::Error,
                .metrics = metrics,
        };
    }
    const uint64_t avdSuggestedGpuMemMiB = isXrAvd ? (isAMD ? 8000 : 4000) : 0;
    if (deviceMemMiB < avdSuggestedGpuMemMiB) {
        metrics.set_check(
                EmulatorCompatibilityInfo::
                        AVD_COMPATIBILITY_CHECK_GPU_CHECK_INSUFFICIENT_MEMORY);
        metrics.set_details(absl::StrFormat("GPU:%s, deviceMemMiB: %llu",
                                            vendorName, deviceMemMiB));
        return {
                .description =
                        absl::StrFormat("GPU memory available (%llu MB) to run "
                                        "avd: `%s` is below "
                                        "the suggested level (%llu MB)",
                                        deviceMemMiB, name, avdSuggestedGpuMemMiB),
                .status = AvdCompatibility::Warning,
                .metrics = metrics};
    }

    // Check maxAllocationCount for GPU.
    // For XR we will request 8192 for AMD GPUs because in testing we've
    // encountered issues. Otherwise we require 4096 as per spec and Android Baseline.
    const uint64_t suggestedAllocationCountRequired =
            isXrAvd ? (isAMD ? 8192 : 4096) : 4096;
    if (vkDeviceMaxAllocationCount < suggestedAllocationCountRequired) {
        metrics.set_check(
                EmulatorCompatibilityInfo::
                        AVD_COMPATIBILITY_CHECK_GPU_CHECK_INSUFFICIENT_MEMORY);
        metrics.set_details(absl::StrFormat(
                "GPU:%s, driver:%s maxMemoryAllocationCount: %llu", vendorName,
                driverVersionStr, vkDeviceMaxAllocationCount));
        return {
                .description = absl::StrFormat(
                        "GPU `%s` does not support the memory properties required "
                        "to run avd: `%s`. Available allocations: %llu, suggested: %llu",
                        vendorName, name, vkDeviceMaxAllocationCount,
                        suggestedAllocationCountRequired),
                .status = AvdCompatibility::Warning,
                .metrics = metrics};
    }

    return {
            .description = absl::StrFormat(
                    "Hardware GPU requirements to run avd: `%s` are met", name),
            .status = AvdCompatibility::Ok,
            .metrics = metrics};
}

REGISTER_COMPATIBILITY_CHECK(hasSufficientHwGpu);

}  // namespace emulation
}  // namespace android
