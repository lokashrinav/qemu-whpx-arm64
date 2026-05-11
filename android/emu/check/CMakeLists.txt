android_add_library(
  TARGET android-emu-check-impl
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/compatibility_check.cpp
      android/disk_space_check.cpp
      android/hw_gpu_check.cpp
      android/system_check.cpp
      android/hypervisor_check.cpp
  DEPS absl::absl_log
       absl::str_format
       aemu-gl-init
       android-emu-avd
       android-emu-base
       android-emu-base-headers
       android-emu-base-logging
       android-emu-feature
       android-emu-files
       android-emu-metrics
       android-hw-config
       gfxstream_vulkan_headers
       protobuf::libprotobuf
       qemu-host-common-headers)
target_include_directories(android-emu-check-impl PUBLIC include)
android_link_complete_archive(TARGET android-emu-check-impl AS
                              android-emu-check)
target_include_directories(android-emu-check INTERFACE include)

if(NOT LINUX_AARCH64)
  # Mock reporting is not available on linux aarch64
  android_add_test(
    TARGET android-emu-check_unittests
    SOURCE_DIR test
    SRC compatibility_check_unittest.cpp
    DEPS absl::absl_log
         absl::log_initialize
         android-emu-avd
         android-emu-base
         android-emu-base-headers
         android-emu-check
         android-emu-files
         android-emu-metrics
         android-emu-metrics-mock-reporter
         android-hw-config
         gmock
         gmock_main
         gtest
         protobuf::libprotobuf)
  target_include_directories(android-emu-check_unittests PRIVATE src)
endif()
