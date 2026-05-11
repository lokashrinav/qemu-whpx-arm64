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

#include "android/emulation/control/xr/XrInputEventSender.h"

#include "aemu/base/async/ThreadLooper.h"
#include "aemu/base/Log.h"  // for LogStream, LOG
#include "android/console.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

void XrInputEventSender::sendXrCommand(const XrCommand command) {
    android::base::ThreadLooper::runOnMainLooper([this, command] {
        doSendXrCommand(command);
    });
}

void XrInputEventSender::sendXrHeadRotation(const RotationRadian rotation) {
    android::base::ThreadLooper::runOnMainLooper([this, rotation] {
        doSendXrHeadRotation(rotation.x(), rotation.y(), rotation.z());
    });
}

void XrInputEventSender::sendXrHeadMovement(const Translation translation) {
    android::base::ThreadLooper::runOnMainLooper([this, translation] {
        doSendXrHeadMovement(translation.delta_x(),
                             translation.delta_y(),
                             translation.delta_z());
    });
}

void XrInputEventSender::sendXrHeadAngularVelocity(
        const AngularVelocity angular_velocity) {
    android::base::ThreadLooper::runOnMainLooper([this, angular_velocity] {
        doSendXrHeadAngularVelocity(angular_velocity.omega_x(),
                                    angular_velocity.omega_y(),
                                    angular_velocity.omega_z());
    });
}

void XrInputEventSender::sendXrHeadVelocity(const Velocity velocity) {
    android::base::ThreadLooper::runOnMainLooper([this, velocity] {
        doSendXrHeadVelocity(velocity.x(), velocity.y(), velocity.z());
    });
}

//TODO(b/396429645): extract a template function for this and the other doSendXrEvent functions
void XrInputEventSender::doSendXrCommand(const XrCommand command) {
    auto agent = mAgents->emu;
    switch (command.action()) {
        case XrCommand::RECENTER:
            agent->setXrScreenRecenter();
            break;
        default:
            LOG(WARNING) << "Unknown XrCommand action: " << command.action();
            break;
    }
}

void XrInputEventSender::doSendXrHeadRotation(float x, float y, float z) {
    auto agent = mAgents->emu;
    if (agent->sendXrHeadRotationEvent(x, y, z) == false) {
        LOG(ERROR) << "Unable to set XrHeadRotationEvent.";
    }
}

void XrInputEventSender::doSendXrHeadMovement(float delta_x,
                                              float delta_y,
                                              float delta_z) {
    auto agent = mAgents->emu;
    if (agent->sendXrHeadMovementEvent(delta_x, delta_y, delta_z) == false) {
        LOG(ERROR) << "Unable to set XrHeadMovementEvent.";
    }
}

void XrInputEventSender::doSendXrHeadAngularVelocity(float omega_x,
                                                     float omega_y,
                                                     float omega_z) {
    auto agent = mAgents->emu;
    if (agent->sendXrHeadAngularVelocityEvent(
            omega_x, omega_y, omega_z) == false) {
        LOG(ERROR) << "Unable to set XrHeadAngularVelocityEvent.";
    }
}

void XrInputEventSender::doSendXrHeadVelocity(float x, float y, float z) {
    auto agent = mAgents->emu;
    if (agent->sendXrHeadVelocityEvent(x, y, z) == false) {
        LOG(ERROR) << "Unable to set XrHeadVelocityEvent.";
    }
}

}  // namespace control
}  // namespace emulation
}  // namespace android
