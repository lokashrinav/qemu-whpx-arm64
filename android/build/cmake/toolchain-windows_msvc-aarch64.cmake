# Copyright 2026 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Native Windows ARM64 variant of the existing clang-cl/MSVC toolchain.

get_filename_component(ADD_PATH "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}/Modules")
get_filename_component(AOSP_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../.."
                       ABSOLUTE)
set(ANDROID_QEMU2_TOP_DIR "${AOSP_ROOT}/external/qemu")
include(toolchain)
include(toolchain-rust)

toolchain_configure_tags("windows_msvc-aarch64")

get_filename_component(ANDROID_QEMU2_TOP_DIR
                       "${CMAKE_CURRENT_LIST_FILE}/../../../../" ABSOLUTE)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ARM64)

if(WIN32)
  get_clang_version(CLANG_VER)
  message(
    STATUS "Configuring native Windows ARM64 build using clang-cl: ${CLANG_VER}"
  )
  get_filename_component(
    CLANG_DIR
    "${ANDROID_QEMU2_TOP_DIR}/../../prebuilts/clang/host/windows-x86/${CLANG_VER}"
    REALPATH)

  # The original clang-cl.exe was a broken Linux symlink; replaced with a real copy.
  set(CLANG_CL ${CLANG_DIR}/bin/clang-cl.exe)
  internal_get_env_cache(CC_EXE)
  if(OPTION_CCACHE)
    string(REPLACE "\\" "/" OPTION_CCACHE "${OPTION_CCACHE}")
    message(STATUS "Enabling ${OPTION_CCACHE} with ${CLANG_CL}")

    if(NOT CC_EXE)
      message(STATUS "Building flattener to handle @rsp files.")
      execute_process(
        COMMAND
          "${CLANG_CL}" "--driver-mode=cl" "--target=arm64-pc-windows-msvc"
          "${ANDROID_QEMU2_TOP_DIR}/android/build/win/flattenrsp.cpp"
          "/DCLANG_CL=${CLANG_CL}" "/DCCACHE=${OPTION_CCACHE}" "/DUNICODE" "/O2"
          "/std:c++17" "/Fe:${CMAKE_BINARY_DIR}/cc.exe" COMMAND_ECHO STDOUT
        OUTPUT_VARIABLE STD_OUT
        ERROR_VARIABLE STD_ERR
        RESULT_VARIABLE ERR_CODE)
      if(ERR_CODE)
        message(
          WARNING "Disabling sccache, will use ${CLANG_CL} as is instead.")
        internal_set_env_cache(CC_EXE "${CLANG_CL}")
      else()
        internal_set_env_cache(CC_EXE "${CMAKE_BINARY_DIR}/cc.exe")
      endif()
    endif()
  else()
    if(NOT CC_EXE)
      internal_set_env_cache(CC_EXE "${CLANG_CL}")
    endif()
  endif()

  set(CMAKE_C_COMPILER "${CC_EXE}")
  set(CMAKE_CXX_COMPILER "${CC_EXE}")
  set(CMAKE_C_COMPILER_TARGET arm64-pc-windows-msvc)
  set(CMAKE_CXX_COMPILER_TARGET arm64-pc-windows-msvc)
  set(ANDROID_LLVM_SYMBOLIZER "${CLANG_DIR}/bin/llvm-symbolizer.exe")

  set(CMAKE_CXX_FLAGS_DEBUG "/MD /Z7")
  set(CMAKE_C_FLAGS_DEBUG "/MD /Z7")
  set(CMAKE_C_FLAGS_RELEASE "/MD /Zi")
  set(CMAKE_CXX_FLAGS_RELEASE "/MD /Zi")

  set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 1)
  set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES 1)
  set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 1)
  set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)

  set(CMAKE_C_RESPONSE_FILE_LINK_FLAG "@")
  set(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")
  set(CMAKE_NINJA_FORCE_RESPONSE_FILE 1 CACHE INTERNAL "")

  add_definitions("-D_WIN32_WINNT=0x0A00")
  add_definitions("-DWINVER=0x0A00")
  add_definitions("-DNTDDI_VERSION=0x0A000000")

  set(CMAKE_LINKER "${CLANG_DIR}/bin/lld-link.exe" CACHE STRING "" FORCE)
  set(CMAKE_SHARED_LINKER_FLAGS_RELEASE
      "/MACHINE:ARM64 /IGNORE:4099 /DEBUG /NODEFAULTLIB:LIBCMT /OPT:REF /OPT:ICF"
      CACHE STRING "" FORCE)
  set(CMAKE_EXE_LINKER_FLAGS
      "/MACHINE:ARM64 /IGNORE:4099 /DEBUG /NODEFAULTLIB:LIBCMT")

  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
  set(CMAKE_MODULE_LINKER_FLAGS
      "${CMAKE_MODULE_LINKER_FLAGS} /MACHINE:ARM64 /MANIFEST:NO")
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} /MACHINE:ARM64 /MANIFEST:NO")

  add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_USE_MATH_DEFINES
                  -DWIN32_LEAN_AND_MEAN
                  -D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
else()
  message(FATAL_ERROR "windows-aarch64 is currently wired for native Windows builds only.")
endif()

set(ANDROID_ASM_TYPE win64)
set(CMAKE_SHARED_LIBRARY_PREFIX "lib")

# Explicitly point at ninja so the vcvars PATH override doesn't hide it.
find_program(_NINJA_EXE ninja ninja.exe
  PATHS
    "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja"
    "$ENV{PATH}"
  NO_DEFAULT_PATH
)
if(_NINJA_EXE)
  set(CMAKE_MAKE_PROGRAM "${_NINJA_EXE}" CACHE FILEPATH "" FORCE)
endif()

if(EXISTS "${AOSP_ROOT}/prebuilts/rust/windows-x86/")
  message(
    STATUS
      "Skipping Rust toolchain configuration for Windows ARM64; no AOSP Windows ARM64 Rust target is wired."
  )
endif()
