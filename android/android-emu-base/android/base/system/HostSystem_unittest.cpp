// Copyright (C) 2015 The Android Open Source Project
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

#include "android/base/system/System.h"

#include "aemu/base/EintrWrapper.h"
#include "aemu/base/files/PathUtils.h"
#include "aemu/base/Log.h"
#include "aemu/base/misc/FileUtils.h"
#include "System.cpp"

#include <gtest/gtest.h>

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include <fcntl.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#define ARRAYLEN(x)  (sizeof(x)/sizeof(x[0]))

namespace android {
namespace base {

#if defined(__x86_64__)
TEST(System, getCpuBrandName) {
    // BrandName returned by CPUID can have 48 bytes at most
    // including the NULL terminator. Some CPUs seem not to follow
    // the specification. So we purposefully enlarge the string
    // buffer here to check whether the brand name returned by
    // CPUID can be longer than 47 bytes.
    char name1[100] = { 0 };
    char name2[100] = { 0 };

#ifdef _WIN32
    uint32_t core_count = 0;
    uint32_t lp_count = 0;

    HostSystem::getCpuBrandNameAndCoreCountWMI((char *)name1, &core_count, &lp_count);
#elif defined(__APPLE__) && defined (__MACH__)
    HostSystem::getCpuBrandNameSysctl((char *)name1);
#elif defined(__linux__)
    HostSystem::getCpuBrandNameProcfs((char *)name1);
#endif

    HostSystem::getCpuBrandNameCpuid((char *)name2);

    LOG(INFO) << "CPU brand name from the OS: " << name1;
    LOG(INFO) << "CPU brand name from CPUID: " << name2;

    EXPECT_LT(0, strlen(name1));
    EXPECT_LT(0, strlen(name2));
    EXPECT_GT(48, strlen(name2));
    EXPECT_EQ(0, strcmp(name1, name2));
}
#endif

}  // namespace base
}  // namespace android
