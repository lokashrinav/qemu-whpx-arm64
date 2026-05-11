/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <mutex>
#include <vector>

#include "aemu/base/EventNotificationSupport.h"  // for EventNotifi...
#include "android/emulation/android_qemud.h"
#include "android/hw-sensors.h"
#include "android/physics/Physics.h"
#include "xr_emulator_conn.pb.h"

namespace android {
namespace physics {

class XrDeviceModel {
public:
    // Define a nested publisher class with friend access for XrDeviceModel
    // XrDeviceModel needs to access the protected method of XrOptionsPublisher
    class XrOptionsPublisher
        : public base::EventNotificationSupport<xr_emulator_proto::XrOptions> {
        friend class XrDeviceModel;
    };

    XrDeviceModel();

    // Send and receive Input Mode (Mouse-KB, hand tracking, eye gaze, etc.) to
    // the guest operating system.
    void setXrInputMode(float value, PhysicalInterpolation mode);
    float getXrInputMode(ParameterValueType parameterValueType) const;

    // Send and receive Environment Mode to the guest operating system. This is
    // currently used in standalone emulator path.
    void setXrEnvironmentMode(float value, PhysicalInterpolation mode);
    float getXrEnvironmentMode(ParameterValueType parameterValueType) const;

    // Send and receive Screen Recenter event state to the guest operating
    // system.
    void setXrScreenRecenter(float value, PhysicalInterpolation mode);
    float getXrScreenRecenter(ParameterValueType parameterValueType) const;

    // Send  and receive Viewport Control Mode to the guest operating system.
    void setXrViewportControlMode(float value, PhysicalInterpolation mode);
    float getXrViewportControlMode(ParameterValueType parameterValueType) const;

    // Send Head Rotation state to the guest operating system.
    void setXrHeadRotation(float x,
                                float y,
                                float z,
                                PhysicalInterpolation mode);

    // Send Head Movement to the guest operating system.
    void setXrHeadMovement(float x,
                                float y,
                                float z,
                                PhysicalInterpolation mode);

    // Send Head Angular Velocity to the guest operating system.
    void setXrHeadAngularVelocity(float omega_x,
                                       float omega_y,
                                       float omega_z,
                                       PhysicalInterpolation mode);

    // Send Head Velocity to the guest operating system.
    void setXrHeadVelocity(float x,
                                float y,
                                float z,
                                PhysicalInterpolation mode);

    // Send and receive Passthrough state to the guest operating system.
    // Passthrough state can also be set in guest OS, thus requiring both getter and setter.
    // This is currently used in Android Studio integrated emulator path.
    void setXrOptions(int environment,
                      float passthroughCoefficient,
                      PhysicalInterpolation mode);
    vec3 getXrOptions(
        ParameterValueType parameterValueType) const;

    XrOptionsPublisher* getXrOptionsPublisher() {
        return &mXrOptionsPublisher;
    }

    QemudClient* initializeQemudClient(int channel, const char* client_param);
    void qemudClientRecv(uint8_t* msg, int msglen);
    void qemudClientClose();
    void qemudClientSend(const xr_emulator_proto::EmulatorRequest& request);

private:
    XrInputMode mLastInputModeRequested =
            XrInputMode::XR_INPUT_MODE_MOUSE_KEYBOARD;
    XrEnvironmentMode mLastEnvironmentModeRequested =
            XrEnvironmentMode::XR_ENVIRONMENT_MODE_PASSTHROUGH_OFF;
    XrViewportControlMode mLastViewportControlModeRequested =
            XrViewportControlMode::VIEWPORT_CONTROL_MODE_UNKNOWN;

    void sendXrInputMode(enum XrInputMode mode);
    void sendXrEnvironmentMode(enum XrEnvironmentMode mode);
    void sendXrScreenRecenter();
    void sendXrViewportControlMode(enum XrViewportControlMode mode);

    XrOptionsPublisher mXrOptionsPublisher;

    QemudService* qemud_service = nullptr;
    QemudClient* qemud_client = nullptr;
};

}  // namespace physics
}  // namespace android
