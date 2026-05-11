// Copyright (C) 2014 The Android Open Source Project
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
#include "aemu/base/async/ThreadLooper.h"
#include "android/console.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

// Class for sending XR input events to the emulator.
// It handles sending commands and head tracking data like rotation, movement,
// angular velocity and velocity over the UI thread.
class XrInputEventSender {
public:
    XrInputEventSender(const AndroidConsoleAgents* const agents)
        : mAgents(agents) {}

    // Sends the current event to the emulator over the UI thread.
    void sendXrCommand(const XrCommand command);
    void sendXrHeadRotation(const RotationRadian rotation);
    void sendXrHeadMovement(const Translation translation);
    void sendXrHeadAngularVelocity(const AngularVelocity angular_velocity);
    void sendXrHeadVelocity(const Velocity velocity);

private:
    void doSendXrCommand(const XrCommand command);
    void doSendXrHeadRotation(float x, float y, float z);
    void doSendXrHeadMovement(float delta_x, float delta_y, float delta_z);
    void doSendXrHeadAngularVelocity(float omega_x, float omega_y, float omega_z);
    void doSendXrHeadVelocity(float x, float y, float z);

    const AndroidConsoleAgents* const mAgents;
};

}  // namespace control
}  // namespace emulation
}  // namespace android
