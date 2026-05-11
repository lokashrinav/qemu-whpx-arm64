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
#include "android/emulation/compatibility_check.h"

#include <sstream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdlib.h>
#include "absl/base/call_once.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/log/log_sink_registry.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "android/base/logging/StudioLogSink.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "android/metrics/tests/MockMetricsReporter.h"
#include "android/metrics/tests/MockMetricsWriter.h"

namespace android {

namespace metrics {
extern void set_unittest_Reporter(MetricsReporter::Ptr newPtr);
}

namespace emulation {

using ::testing::EndsWith;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::StartsWith;

// Matcher to check if a string is present in a vector of string_views.
MATCHER_P(ContainsString, expected_string, "") {
    for (const auto& str_view : arg) {
        if (str_view == expected_string) {
            return true;
        }
    }
    return false;
}

struct CaptureLogSink : public absl::LogSink {
public:
    void Send(const absl::LogEntry& entry) override {
        captured_log_ += absl::StrFormat("%s\n", entry.text_message());
    }

    void clear() { captured_log_.clear(); }

    std::string captured_log_;
};

void initlogs_once() {
    absl::InitializeLog();
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kFatal);
}

static constexpr const char* kVersion = "version";
static constexpr const char* kFullVersion = "fullVersion";
static constexpr const char* kQemuVersion = "qemuVersion";
static constexpr const char* kSessionId = "session";

using EventInterceptor =
        std::function<void(android_studio::AndroidStudioEvent*)>;

static absl::once_flag initlogs;
class AvdCompatibilityCheckResultTest : public ::testing::Test {
protected:
    AvdCompatibilityCheckResultTest() {
        absl::call_once(initlogs, initlogs_once);
    }
    void SetUp() override {
        // Add the CaptureLogSink
        log_sink_ = std::make_unique<CaptureLogSink>();
        absl::AddLogSink(log_sink_.get());
        absl::SetVLogLevel("*", 2);
        base::studio_sink()->SetOutputStream(&std::cerr);
        oldCerr = std::cerr.rdbuf();
        std::cerr.rdbuf(buffer.rdbuf());
        AvdCompatibilityManager::instance().invalidate();
        createReporter();
    }

    void TearDown() override {
        // Remove the CaptureLogSink
        absl::RemoveLogSink(log_sink_.get());
        // Capture and restore stdout
        std::cerr.rdbuf(oldCerr);
        buffer.clear();
    }

    void createReporter(EventInterceptor event_interceptor =
                                [](android_studio::AndroidStudioEvent*) {}) {
        // auto reporter = [conditional](android_studio::AndroidStudioEvent* ev)
        // {
        //     conditional(ev);
        // };
        auto reporter =
                [event_interceptor = std::move(event_interceptor)](
                        metrics::MockMetricsReporter::ConditionalCallback
                                callback) {
                    android_studio::AndroidStudioEvent event;
                    callback(&event);
                    event_interceptor(&event);
                };
        android::metrics::set_unittest_Reporter(
                std::make_unique<metrics::MockMetricsReporter>(
                        true, mWriter, kVersion, kFullVersion, kQemuVersion,
                        reporter));
    }

    std::unique_ptr<CaptureLogSink> log_sink_;
    std::streambuf* oldCerr;
    std::stringstream buffer;
    std::shared_ptr<metrics::MockMetricsWriter> mWriter{
            std::make_shared<metrics::MockMetricsWriter>(kSessionId)};
};

struct AvdCompatibilityManagerTest {
    static void clear() { AvdCompatibilityManager::instance().mChecks.clear(); }
};

AvdCompatibilityCheckResult sampleOkayCheck(AvdInfo* foravd) {
    return {
            .description = "Sample Ok Verification",
            .status = AvdCompatibility::Ok,
    };
};

AvdCompatibilityCheckResult alwaysErrorBar(AvdInfo* foravd) {
    android_studio::EmulatorCompatibilityInfo metrics;
    metrics.set_check(android_studio::EmulatorCompatibilityInfo::
                              AVD_COMPATIBILITY_CHECK_UNKNOWN);
    metrics.set_details("more bar");

    return {.description = "You need more bar",
            .status = AvdCompatibility::Error,
            .metrics = metrics};
};

AvdCompatibilityCheckResult alwaysErrorFoo(AvdInfo* foravd) {
    return {
            .description = "You need more foo",
            .status = AvdCompatibility::Error,
    };
};

AvdCompatibilityCheckResult alwaysWarnBaz(AvdInfo* foravd) {
    android_studio::EmulatorCompatibilityInfo metrics;
    metrics.set_check(android_studio::EmulatorCompatibilityInfo::
                              AVD_COMPATIBILITY_CHECK_UNKNOWN);
    metrics.set_details("more baz");

    return {.description = "You are low on baz, more baz would be good for you",
            .status = AvdCompatibility::Warning,
            .metrics = metrics};
};

static int gRunCount = 0;

AvdCompatibilityCheckResult runCounter(AvdInfo* foravd) {
    gRunCount++;
    return {
            .description = absl::StrFormat("You ran me: %d times", gRunCount),
            .status = AvdCompatibility::Ok,
    };
};

REGISTER_COMPATIBILITY_CHECK(sampleOkayCheck);
REGISTER_COMPATIBILITY_CHECK(alwaysErrorBar);
REGISTER_COMPATIBILITY_CHECK(alwaysErrorFoo);
REGISTER_COMPATIBILITY_CHECK(runCounter);

TEST_F(AvdCompatibilityCheckResultTest, auto_registration_should_work) {
    EXPECT_THAT(AvdCompatibilityManager::instance().registeredChecks(),
                ContainsString("sampleOkayCheck"));
    EXPECT_THAT(AvdCompatibilityManager::instance().registeredChecks(),
                ContainsString("alwaysErrorBar"));
    EXPECT_THAT(AvdCompatibilityManager::instance().registeredChecks(),
                ContainsString("alwaysErrorFoo"));
}

TEST_F(AvdCompatibilityCheckResultTest, ensureAvdCompatibility_will_fatal) {
    // Remove our std:err redirect, we are going to exit so we can only grab
    // stderr!
    std::cerr.rdbuf(oldCerr);

    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(alwaysErrorBar,
                                                      "alwaysErrorBar");

    EXPECT_DEATH(
            AvdCompatibilityManager::instance().ensureAvdCompatibility(nullptr),
            ".*You need more bar.*")
            << "We did not exit the system with the message we expected to "
               "see.";
}

TEST_F(AvdCompatibilityCheckResultTest,
       ensureAvdCompatibility_will_log_warning) {
    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(alwaysWarnBaz,
                                                      "alwaysWarnbaz");
    AvdCompatibilityManager::instance().ensureAvdCompatibility(nullptr);
    EXPECT_THAT(buffer.str(), testing::StartsWith("USER_WARNING |"));
    EXPECT_THAT(buffer.str(),
                testing::HasSubstr(
                        "You are low on baz, more baz would be good for you."));
}

TEST_F(AvdCompatibilityCheckResultTest, actually_calls_registered_tests) {
    bool wascalled = false;
    AvdCompatibilityManager::instance().registerCheck(
            [&wascalled](AvdInfo* info) {
                wascalled = true;
                return AvdCompatibilityCheckResult{
                        "This check always fails with an error",
                        AvdCompatibility::Error};
            },
            "dynamic_test");
    auto results = AvdCompatibilityManager::instance().check(nullptr);
    EXPECT_THAT(wascalled, testing::Eq(true));
}

TEST_F(AvdCompatibilityCheckResultTest, checks_are_run_only_once) {
    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(runCounter, "runCounter");
    gRunCount = 0;
    AvdCompatibilityManager::instance().check(nullptr);
    AvdCompatibilityManager::instance().check(nullptr);
    EXPECT_THAT(gRunCount, testing::Eq(1));
}

TEST_F(AvdCompatibilityCheckResultTest, checks_again_invalidate) {
    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(runCounter, "runCounter");
    gRunCount = 0;
    AvdCompatibilityManager::instance().check(nullptr);
    AvdCompatibilityManager::instance().invalidate();
    AvdCompatibilityManager::instance().check(nullptr);
    EXPECT_THAT(gRunCount, testing::Eq(2));
}

TEST_F(AvdCompatibilityCheckResultTest, constructIssueString_error_one) {
    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(alwaysErrorBar,
                                                      "alwaysErrorBar");
    AvdCompatibilityManager::instance().registerCheck(alwaysErrorFoo,
                                                      "alwaysErrorFoo");
    auto results = AvdCompatibilityManager::instance().check(nullptr);
    auto errorString = AvdCompatibilityManager::instance().constructIssueString(
            results, AvdCompatibility::Error);
    EXPECT_EQ(errorString, "You need more bar, You need more foo.");
}

TEST_F(AvdCompatibilityCheckResultTest,
       constructIssueString_error_two_injects_a_comma) {
    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(alwaysErrorBar,
                                                      "alwaysErrorBar");
    AvdCompatibilityManager::instance().registerCheck(alwaysErrorFoo,
                                                      "alwaysErrorFoo");
    auto results = AvdCompatibilityManager::instance().check(nullptr);
    auto errorString = AvdCompatibilityManager::instance().constructIssueString(
            results, AvdCompatibility::Error);
    EXPECT_EQ(errorString, "You need more bar, You need more foo.");
}

TEST_F(AvdCompatibilityCheckResultTest,
       constructIssueString_error_to_many_adds_message) {
    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(alwaysErrorBar,
                                                      "alwaysErrorBar");
    AvdCompatibilityManager::instance().registerCheck(alwaysErrorFoo,
                                                      "alwaysErrorFoo");

    AvdCompatibilityManager::instance().registerCheck(alwaysErrorFoo,
                                                      "alwaysErrorFoo2");
    auto results = AvdCompatibilityManager::instance().check(nullptr);
    auto errorString = AvdCompatibilityManager::instance().constructIssueString(
            results, AvdCompatibility::Error);
    EXPECT_EQ(errorString, "You need more bar, You need more foo, and more.");
}

TEST_F(AvdCompatibilityCheckResultTest, constructIssueString_warning) {
    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(alwaysWarnBaz,
                                                      "alwaysWarnBaz");
    auto results = AvdCompatibilityManager::instance().check(nullptr);
    auto warningString =
            AvdCompatibilityManager::instance().constructIssueString(
                    results, AvdCompatibility::Warning);
    EXPECT_EQ(warningString,
              "You are low on baz, more baz would be good for you.");
}

TEST_F(AvdCompatibilityCheckResultTest,
       constructIssueString_noIssues_has_empty_strings) {
    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(sampleOkayCheck,
                                                      "sampleOkayCheck");
    auto results = AvdCompatibilityManager::instance().check(nullptr);
    auto errorString = AvdCompatibilityManager::instance().constructIssueString(
            results, AvdCompatibility::Error);
    auto warningString =
            AvdCompatibilityManager::instance().constructIssueString(
                    results, AvdCompatibility::Warning);
    EXPECT_EQ(errorString, "");
    EXPECT_EQ(warningString, "");
}

TEST_F(AvdCompatibilityCheckResultTest, no_issues_no_logging) {
    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(sampleOkayCheck,
                                                      "sampleOkayCheck");
    auto results = AvdCompatibilityManager::instance().check(nullptr);

    AvdCompatibilityManager::instance().ensureAvdCompatibility(nullptr);
    EXPECT_THAT(buffer.str(), testing::IsEmpty());
}

TEST_F(AvdCompatibilityCheckResultTest, no_issues_no_reporting) {
    createReporter([](auto e) {
        EXPECT_FALSE(true)
                << "No metrics should have been reported for the ok case.";
    });
    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(sampleOkayCheck,
                                                      "sampleOkayCheck");
    auto results = AvdCompatibilityManager::instance().check(nullptr);
    AvdCompatibilityManager::instance().reportMetrics(results);
    AvdCompatibilityManager::instance().ensureAvdCompatibility(nullptr);
}

TEST_F(AvdCompatibilityCheckResultTest, report_warning_metrics) {
    bool report_called = false;
    createReporter([&report_called](android_studio::AndroidStudioEvent* e) {
        EXPECT_TRUE(e->emulator_details().has_emu_compat_info());
        EXPECT_EQ(e->emulator_details().emu_compat_info().status(),
                  android_studio::EmulatorCompatibilityInfo::
                          AVD_COMPATIBILITY_STATUS_WARNING);

        EXPECT_EQ(e->emulator_details().emu_compat_info().check(),
                  android_studio::EmulatorCompatibilityInfo::
                          AVD_COMPATIBILITY_CHECK_UNKNOWN);
        EXPECT_EQ(e->emulator_details().emu_compat_info().details(),
                  "more baz");
        report_called = true;
    });
    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(alwaysWarnBaz,
                                                      "alwaysWarnBaz");
    auto results = AvdCompatibilityManager::instance().check(nullptr);
    AvdCompatibilityManager::instance().reportMetrics(results);

    EXPECT_TRUE(report_called) << "The reporter callback was not invoked, so "
                                  "metrics were not generated.";
}

TEST_F(AvdCompatibilityCheckResultTest, report_error_metrics) {
    bool report_called = false;
    createReporter([&report_called](android_studio::AndroidStudioEvent* e) {
        EXPECT_TRUE(e->emulator_details().has_emu_compat_info());
        EXPECT_EQ(e->emulator_details().emu_compat_info().status(),
                  android_studio::EmulatorCompatibilityInfo::
                          AVD_COMPATIBILITY_STATUS_ERROR);
        EXPECT_EQ(e->emulator_details().emu_compat_info().check(),
                  android_studio::EmulatorCompatibilityInfo::
                          AVD_COMPATIBILITY_CHECK_UNKNOWN);
        EXPECT_EQ(e->emulator_details().emu_compat_info().details(),
                  "more bar");
        report_called = true;
    });
    AvdCompatibilityManagerTest::clear();
    AvdCompatibilityManager::instance().registerCheck(alwaysErrorBar,
                                                      "alwaysErrorBar");
    auto results = AvdCompatibilityManager::instance().check(nullptr);
    AvdCompatibilityManager::instance().reportMetrics(results);

    EXPECT_TRUE(report_called) << "The reporter callback was not invoked, so "
                                  "metrics were not generated.";
}

}  // namespace emulation
}  // namespace android
