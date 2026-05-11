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
#pragma once

#include <string_view>
#include "absl/log/absl_log.h"
#include "absl/strings/str_format.h"
#include "android/base/logging/StudioLogSink.h"

namespace android {
namespace base {
/**
 * @def USER_MESSAGE
 * @brief Logs a message specifically formatted for Android Studio recognition.
 *
 * This macro simplifies the process of logging messages that Android Studio can
 * identify and display with appropriate UI elements based on the log level.
 * It utilizes the `StudioLogSink` to ensure the messages are correctly
 * formatted.
 *
 * Use these messages sparingly to avoid overwhelming the user.
 *
 * Android Studio determines how these messages are presented in its UI. Do
 * not overuse these messages, and certainly avoid repeating the same message
 * frequently.
 *
 * @param verbose_level The desired verbosity level for the log message.
 * Supported levels include:
 * - `INFO`   : Informational message for the user.
 * - `WARNING`: Warning message for the user.
 * - `ERROR`  : Error message for the user.
 *
 * Example usage:
 *
 * USER_MESSAGE(INFO) << "Hello this is an info message for the user";
 * USER_MESSAGE(WARNING) << "Hello this is a warning message for the user";
 * USER_MESSAGE(ERROR) << "Hello this is an error message for the user";
 *
 * Messages will appear on the console log as follows:
 *
 * USER_INFO    | Hello this is an info message for the user
 * USER_WARNING | Hello this is a warning message for the user
 * USER_ERROR   | Hello this is an error message for the user
 */
#define USER_MESSAGE(verbose_level)                                          \
    static_assert(std::string_view(#verbose_level) != "FATAL",               \
                  "USER_MESSAGE cannot use FATAL log level, use "            \
                  "EXIT_WITH_FATAL_MESSAGE(printf_format_string) instead."); \
    ABSL_LOG_INTERNAL_LOG_IMPL(_##verbose_level)                             \
            .ToSinkOnly(android::base::studio_sink())

/**
 * @brief Terminates the program with a fatal error message.
 *
 * This function logs a fatal error message and then exits the program with
 * `EXIT_FAILURE` code. Android Studio will recognize the
 * fatal message and display it to the user.
 *
 * It does **not** generate a crash report. Use dfatal from
 * `aemu/base/logging/Log.h` for fatal messages that should create a
 * crashreport.
 *
 * @param message The fatal error message to log.
 *
 * @note `FATAL` messages will be logged with the `FATAL` prefix, not
 *       `USER_FATAL`.
 *
 * @example
 *   EXIT_WITH_FATAL_MESSAGE("The emulator process will now terminate");
 *
 *   The message will appear on the console log as:
 *   FATAL        | The emulator process will now terminate
 *
 * @see hardware/google/aemu/base/include/aemu/base/logging/Log.h
 */
[[noreturn]] LOGGING_API void EXIT_WITH_FATAL_MESSAGE(
        const std::string& message);

/**
 * @brief Terminates the program with a formatted fatal error message.
 *
 * This function overload allows you to provide a format string and arguments
 * to create a formatted fatal error message. It then logs the message and
 * exits the program with EXIT_FAILURE code. Android Studio will recognize the
 * fatal message and display it to the user.
 *
 * It does **not** generate a crash report. Use dfatal from
 * `aemu/base/logging/Log.h` for fatal messages that should create a
 * crashreport.
 *
 * @tparam Args  Types of arguments for formatting.
 * @param format The format string for the error message.
 * @param args   Arguments to be formatted into the message.
 *
 * @note This function uses `absl::str_format` for formatting.
 *
 * @example
 *   EXIT_WITH_FATAL_MESSAGE("Error code: %d", errorCode);
 *
 * @see hardware/google/aemu/base/include/aemu/base/logging/Log.h
 */
template <typename... Args>
[[noreturn]] inline void EXIT_WITH_FATAL_MESSAGE(
        const absl::FormatSpec<Args...>& format,
        const Args&... args) {
    EXIT_WITH_FATAL_MESSAGE(absl::StrFormat(format, args...));
}
}  // namespace base
}  // namespace android