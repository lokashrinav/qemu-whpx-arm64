// Copyright (C) 2024 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "absl/strings/str_format.h"
#include "android/avd/info.h"
#include "studio_stats.pb.h"

namespace android {
namespace emulation {
/**
 * @enum AvdCompatibility
 * @brief Represents the compatibility status of an AVD (Android Virtual
 * Device).
 */
enum class AvdCompatibility : uint8_t {
    /**
     * @brief The check succeeded; the AVD is fully compatible.
     */
    Ok = 0,

    /**
     * @brief The AVD can run, but with limited functionality. User should be
     * informed.
     */
    Warning,

    /**
     * @brief The AVD cannot run with the current configuration.
     */
    Error,
};

/**
 * @struct AvdCompatibilityCheckResult
 * @brief Stores the result of an AVD compatibility check, including a
 * description and status.
 */
struct AvdCompatibilityCheckResult {
    /**
     * @brief A description of the check performed and its outcome.
     * do not include a period, as the framework will add one.
     */
    std::string description;

    /**
     * @brief The AvdCompatibility status indicating the level of compatibility.
     */
    AvdCompatibility status;

    /**
     * @brief Metrics that should be reported. Only warning and error status
     * will be reported. Please make sure to fill out the details of this protobuf
     * message when creating new checks.
     */
    android_studio::EmulatorCompatibilityInfo metrics;
};

/**
 * @typedef CompatibilityCheck
 * @brief A function that checks the compatibility of a given AVD with the
 * system.
 * @param avd A pointer to the AvdInfo struct containing information about the
 * AVD.
 * @return An AvdCompatibilityCheckResult struct containing the check outcome.
 */
using CompatibilityCheck = std::function<AvdCompatibilityCheckResult(AvdInfo*)>;

/**
 * @class AvdCompatibilityManager
 * @brief A singleton class managing and executing checks to validate AVD
 * compatibility with the device configuration.
 */
class AvdCompatibilityManager final {
public:
    /**
     * @brief Runs all registered compatibility checks on the specified AVD.
     *
     * This function executes all registered compatibility checks on the given
     * AVD. Results are cached, so subsequent calls will return the cached
     * results unless the checks are explicitly invalidated.
     *
     * @param avd A pointer to the AvdInfo struct representing the AVD to check.
     * @return A vector of AvdCompatibilityCheckResult structs, one for each
     *         compatibility check performed.
     */
    std::vector<AvdCompatibilityCheckResult> check(AvdInfo* avd);

    /**
     * @brief Checks the results of AVD compatibility checks for any errors.
     * @param results A vector of AvdCompatibilityCheckResult structs containing
     * the check outcomes.
     * @return true if any of the checks resulted in an AvdCompatibility::Error
     * status, false otherwise.
     */
    bool hasCompatibilityErrors(
            const std::vector<AvdCompatibilityCheckResult>& results);


    /**
     * @brief Report metrics for the collected compatbility checks.
     *
     * This will collect all gathered metrics during the checking phase and report
     * them to our metrics endpoint. Only warning and errors will be collected.
     *
     * @param results A vector of AvdCompatibilityCheckResult structs containing
     * the check outcomes.

     */
    void reportMetrics(const std::vector<AvdCompatibilityCheckResult>& results);

    /**
     * @brief Constructs an issue string (error or warning) from the given
     * compatibility check results.
     *
     * This function iterates through the provided results and constructs a
     * comma-separated string of issues with the specified status (Error or
     * Warning). To maintain readability, only the first two issues are included
     * in the string. If more issues exist, a ", and more.." suffix is appended.
     *
     * NOTE: You want to use the USER_MESSAGE(WARNING) logging macro for warning
     * strings and LOG(FATAL) macro for error strings.
     *
     * @param results A vector of AvdCompatibilityCheckResult structs containing
     * the check outcomes.
     * @param status The AvdCompatibility status (Error or Warning) to filter
     * issues by.
     * @return A string containing the concatenated issue descriptions.
     */
    std::string constructIssueString(
            const std::vector<AvdCompatibilityCheckResult>& results,
            AvdCompatibility status);
    /**
     * @brief Retrieves the singleton instance of the
     * AvdCompatibilityManager.
     * @return A reference to the single AvdCompatibilityManager instance.
     */
    static AvdCompatibilityManager& instance();

    /**
     * @brief Registers a new compatibility check function with the manager.
     * @param checkFn The compatibility check function to register.
     * @param name A string representing the name of the check.
     */
    void registerCheck(CompatibilityCheck checkFn, const char* name);

    /**
     * @brief Returns a list of the names of all registered compatibility
     * checks
     * @return A vector of string_views, each representing the name of a
     * registered check.
     */
    std::vector<std::string_view> registeredChecks();

    /**
     * @brief Invalidates the cached compatibility check results for the
     * specified AVD.
     *
     * This function clears the cached compatibility check results, forcing
     * the next call to `check()` to re-run all the checks.
     */
    void invalidate() { mRanChecks = false; }

    /**
     * @brief Ensures the compatibility of an AVD with the current system.
     *
     * This function performs a series of compatibility checks on the given AVD.
     * If any errors are found, the program terminates with a fatal error
     * message. Warnings are logged to the console.
     *
     * Example log lines:
     *
     * USER_WARNING | Suggested minimum number of CPU cores to run avd 'x'
     * is 4 (available: 2).
     * FATAL        | Your device does not have enough disk
     * space to run: `x`.
     *
     * @param avd A pointer to the AvdInfo struct representing the AVD to be
     * checked.
     * @param reportMetrics A boolean indicating whether to report metrics for
     * the compatibility checks. If true, metrics will be reported for warnings
     * and errors. If false, no metrics will be reported. Defaults to false.
     */
    static void ensureAvdCompatibility(AvdInfo* avd,
                                       bool reportMetrics = false);

private:
    AvdCompatibilityManager() = default;

    std::vector<std::pair<std::string_view, CompatibilityCheck>> mChecks;
    bool mRanChecks{false};
    std::vector<AvdCompatibilityCheckResult> mResults;

    // Testing..
    friend class AvdCompatibilityManagerTest;
};

/**
 * @macro REGISTER_COMPATIBILITY_CHECK
 * @brief A macro to conveniently register a compatibility check function at
 * compile time
 *
 * Note: Make sure to define your check in this library, or make sure that
 * the library that uses this macro is compiled with the whole archive flag.
 * @param check_name The name of the check function to be registered
 */
#define REGISTER_COMPATIBILITY_CHECK(check_name)                        \
    __attribute__((constructor)) void register##check_name() {          \
        AvdCompatibilityManager::instance().registerCheck(check_name,   \
                                                          #check_name); \
    }

template <typename Sink>
void AbslStringify(Sink& sink, AvdCompatibility status) {
    switch (status) {
        case AvdCompatibility::Ok:
            absl::Format(&sink, "Ok");
            break;
        case AvdCompatibility::Warning:
            absl::Format(&sink, "Warning");
            break;
        case AvdCompatibility::Error:
            absl::Format(&sink, "Error");
            break;
        default:
            ABSL_UNREACHABLE();
    }
}

}  // namespace emulation
}  // namespace android