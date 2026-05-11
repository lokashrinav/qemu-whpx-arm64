#
# Copyright 2025 - The Android Open Source Project
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
#
#
# --- Docs ---
# Dockerfile for building lavapipe with the following options:
#  - MESA upstream main: --build-arg CHECKOUT=upstream
#
#  - MESA upstream versioned: --build-arg CHECKOUT=upstream --build-arg MESA_TAG="24.1.3"
#
#  - MESA aosp(default): --build-arg CHECKOUT=aosp (or default by skipping the CHECKOUT arg)
#
#  - --output type=local,dest=./prebuilts/android-emulator-build/common/vulkan/linux-x86_64/icds
#   - this is the location where you want the binary to be placed (.so and icd)
#
# Docker context: ensure you are in your local AOSP checkout (emu-master-dev). This is a docker requirement;
#
# Example command:
# DOCKER_BUILDKIT=1 docker build --output type=local,dest=./prebuilts/android-emulator-build/common/vulkan/linux-x86_64/icds -t my-mesa-image -f android/build/docker/linux/lavapipe/Dockerfile .
# ---


ARG CHECKOUT=aosp
FROM debian:testing AS install
RUN apt-get update

RUN apt-get install -y \
       ca-certificates \
    && update-ca-certificates

# Install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends --no-install-suggests \
        git \
        python3-pip\
        build-essential \
        cmake \
        python3 \
        zlib1g-dev \
        glslang-tools \
        python3-venv\
        libdrm-dev \
        libdrm-amdgpu1 \
        libdrm-intel1\
        libdrm-nouveau2\
        libdrm-radeon1\
        libdrm2\
        libclc-18 \
        libclc-18-dev \
        llvm-18 \
        llvm-18-dev\
        libllvmspirvlib-18-dev \
        libclang-18-dev \
        bison \
        flex \
        libelf-dev \
        bindgen \
        cbindgen \
        libwayland-dev \
        libwayland-egl-backend-dev \
        libxcb-randr0-dev \
        libx11-xcb-dev \
        libx11-dev \
        libxext6 \
        libxfixes3 \
        libxcb-glx0-dev \
        libxcb-shm0-dev \
        libxcb-dri2-0-dev \
        libxcb-dri3-dev \
        libxcb-present-dev \
        libxshmfence-dev \
        libxxf86vm-dev \
        libxrandr-dev \
        pkg-config \
        libxfixes-dev \
        ninja-build

RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"
RUN pip install --upgrade mako meson ply pyyaml setuptools

FROM install AS mesa_upstream
ARG MESA_TAG=main
RUN --mount=type=cache,target=/tmp/git_cache/ \
     git clone --depth=1 --branch=${MESA_TAG} https://gitlab.freedesktop.org/mesa/mesa.git /tmp/git_cache/mesa_${MESA_TAG}/; \
     cd /tmp/git_cache/mesa_${MESA_TAG}/ && git pull && cp -r ./ /tmp/mesa/

FROM install AS mesa_aosp
COPY ./external/mesa3d /tmp/mesa/
WORKDIR /tmp/mesa
RUN apt-get update && apt-get install -y --no-install-recommends --no-install-suggests rustc
RUN meson setup --wipe build_docker/

#this can be passed in as --build-arg CHECKOUT=upstream/aosp, by default AOSP
FROM mesa_${CHECKOUT} AS build
WORKDIR /tmp/mesa

# Configure Mesa build with lavapipe
RUN meson setup build_docker -Dvulkan-drivers=swrast -Dgallium-drivers=llvmpipe

# Build Mesa
RUN ninja -C build_docker

ARG CACHEBUST=1
RUN ldd -v /tmp/mesa/build_docker/src/gallium/targets/lavapipe/libvulkan_lvp.so
RUN apt-get install -y --no-install-recommends --no-install-suggests jq
RUN jq '.ICD.library_path = "./libvulkan_lvp.so"' /tmp/mesa/build_docker/src/gallium/targets/lavapipe/lvp_icd.x86_64.json > /tmp/mesa/build_docker/src/gallium/targets/lavapipe/lvp_icd.x86_64.json_tmp && mv /tmp/mesa/build_docker/src/gallium/targets/lavapipe/lvp_icd.x86_64.json_tmp /tmp/mesa/build_docker/src/gallium/targets/lavapipe/lvp_icd.x86_64.json

FROM scratch AS final
COPY --from=build /tmp/mesa/build_docker/src/gallium/targets/lavapipe/libvulkan_lvp.so ./
COPY --from=build /tmp/mesa/build_docker/src/gallium/targets/lavapipe/lvp_icd.x86_64.json ./
