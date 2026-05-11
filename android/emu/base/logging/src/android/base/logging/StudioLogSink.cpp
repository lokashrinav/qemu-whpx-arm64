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
#include "android/base/logging/StudioLogSink.h"
#include "android/base/logging//StudioMessage.h"
#include <iostream>
#include "absl/log/absl_log.h"

namespace android {
namespace base {

std::string_view StudioLogSink::TranslateSeverity(
        const absl::LogEntry& entry) const {
    switch (entry.log_severity()) {
        case absl::LogSeverity::kInfo:
            return "USER_INFO   ";
        case absl::LogSeverity::kWarning:
            return "USER_WARNING";
        case absl::LogSeverity::kError:
            return "USER_ERROR  ";
        case absl::LogSeverity::kFatal:
            return "FATAL       ";
    }
}


void StudioLogSink::Send(const absl::LogEntry& entry) {
    ColorLogSink::Send(entry);
    if (entry.log_severity() == absl::LogSeverity::kFatal) {
        exit(EXIT_FAILURE);
    }
}

StudioLogSink* studio_sink() {
    static StudioLogSink studioLog(&std::cout);
    return &studioLog;
}

[[noreturn]] void EXIT_WITH_FATAL_MESSAGE(const std::string& message) {
    ABSL_LOG(FATAL).ToSinkOnly(android::base::studio_sink()) << message;
}


}  // namespace base
}  // namespace android