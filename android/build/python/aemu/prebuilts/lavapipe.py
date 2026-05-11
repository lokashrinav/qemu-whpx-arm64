#!/usr/bin/env python
#
# Copyright 2024 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import atexit
import logging
import os
import shutil
import subprocess
import sys
import tempfile
from aemu.prebuilts.deps.common import EXE_SUFFIX
import aemu.prebuilts.deps.common as deps_common
from pathlib import Path
import platform
import json

AOSP_ROOT = Path(__file__).resolve().parents[7]
HOST_OS = platform.system().lower()
HOST_ARCH = platform.machine().lower()

AOSP_MESA_SRC_PATH = os.path.join(AOSP_ROOT, "external", "mesa3d")
MESON_PATH = os.path.join(AOSP_ROOT, "prebuilts", "meson")
NINJA_PATH = os.path.join(AOSP_ROOT, "prebuilts", "ninja", HOST_OS + "-x86")

def checkDependencies():
    logging.info("Checking for required build dependencies..")
    # - Python >= 3.6.x
    logging.info(">> Checking for Python >= 3.6.x (%s)", sys.version)
    deps_common.checkPythonVersion(min_vers=(3, 6))

    logging.info(">> Checking CMake >= 3.19")
    deps_common.checkCmakeVersion(min_vers=(3, 19))

    logging.info(">> Checking Ninja >= 1.8.2")
    deps_common.checkNinjaVersion(min_vers=(1, 8, 2))

    logging.info(">> Checking for bison")
    deps_common.checkBisonVersion()
    logging.info(">> Checking for flex")
    deps_common.checkFlexVersion()

    logging.info("Library dependencies will be verified by meson");
    return True

def configureLavapipeBuild(srcdir, builddir):
    if Path(builddir).exists():
        shutil.rmtree(builddir)
    old_umask = os.umask(0o027)
    os.makedirs(builddir)
    os.umask(old_umask)

    config_script = "meson"
    conf_args = [config_script, "setup", builddir, "-Dvulkan-drivers=swrast", "-Dgallium-drivers=llvmpipe", "-Dshared-llvm=false"]
    toolchain_dir = Path(builddir) / "toolchain"
    os.makedirs(Path(builddir) / "toolchain")

    meson_dir = deps_common.getMesonDirectory()
    with open(toolchain_dir / "meson", 'x') as f:
        f.write(f"#!/bin/sh\n{meson_dir}/meson.py $@\n")
    os.chmod(toolchain_dir / "meson", 0o777)

    deps_common.addToSearchPath(str(toolchain_dir))
    logging.info("[%s] Running %s in %s", builddir, config_script, srcdir)
    logging.info(conf_args)
    subprocess.run(args=conf_args, stderr=subprocess.STDOUT, check=True, cwd=srcdir, env=os.environ.copy())
    logging.info("%s succeeded", config_script)

def buildLavapipe(srcdir, builddir):
    ninja_build_cmd = ["ninja" + EXE_SUFFIX, "-C", builddir]
    logging.info(ninja_build_cmd)
    subprocess.run(args=ninja_build_cmd, stderr=subprocess.STDOUT, check=True, cwd=srcdir, env=os.environ.copy())
    logging.info("Build succeeded")

def installLavapipe(builddir, installdir):

    def retargetICDFile(icdFile):
        try:
            with open(icdFile, 'r') as f:
                data = json.load(f)

            data["ICD"]["library_path"] = "./libvulkan_lvp.so"

            with open(icdFile, 'w') as f:
                json.dump(data, f, indent=4)
            logging.info(f"Successfully modified {icdFile}")
        except FileNotFoundError:
            logging.error(f"Error: File not found at {icdFile}")
        except json.JSONDecodeError:
            logging.error(f"Error: Invalid JSON format in {icdFile}")
        except Exception as e:
            logging.error(f"An unexpected error occurred: {e}")

    os.makedirs(installdir,exist_ok=True)
    LAVAPIPE_PREFIX= os.path.join(builddir, "src/gallium/targets/lavapipe")
    LAVAPIPE_SO_SRC = os.path.join(LAVAPIPE_PREFIX, "libvulkan_lvp.so")
    LAVAPIPE_SO_DST = os.path.join(installdir, "libvulkan_lvp.so")
    LAVAPIPE_ICD_SRC = os.path.join(LAVAPIPE_PREFIX, "lvp_icd.x86_64.json")
    LAVAPIPE_ICD_DST = os.path.join(installdir, "lvp_icd.x86_64.json")
    logging.info("Installing Lavapipe to %s", installdir)
    # The ICD file built in MESA expects the library to be installed in system path.
    # We will be shipping icd and .so in the same dir so rewrite to point locally.
    retargetICDFile(LAVAPIPE_ICD_SRC)
    shutil.copyfile(LAVAPIPE_ICD_SRC, LAVAPIPE_ICD_DST)
    shutil.copyfile(LAVAPIPE_SO_SRC, LAVAPIPE_SO_DST)
    logging.info("Installation succeeded")


def buildPrebuilt(args, prebuilts_out_dir):
    # Use meson from our prebuilts
    deps_common.addToSearchPath(MESON_PATH)
    # Use ninja from our prebuilts
    deps_common.addToSearchPath(NINJA_PATH)

    logging.info(os.environ)

    if not os.path.isdir(AOSP_MESA_SRC_PATH):
        logging.fatal("%s does not exist", AOSP_MESA_SRC_PATH)
    logging.info("MESA source: %s", AOSP_MESA_SRC_PATH)

    mesa_src_path = AOSP_MESA_SRC_PATH
    with tempfile.TemporaryDirectory() as mesa_build_path:
        logging.info("Building Lavapipe")
        lavapipe_install_dir =  os.path.join(prebuilts_out_dir, "lavapipe")
        configureLavapipeBuild(mesa_src_path, mesa_build_path)
        buildLavapipe(mesa_src_path, mesa_build_path)
        installLavapipe(mesa_build_path, lavapipe_install_dir)

        logging.info("Successfully built Lavapipe!")
