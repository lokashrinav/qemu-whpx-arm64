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

#include "android/physics/XrDeviceModel.h"

#include "aemu/base/logging/Log.h"
#include "aemu/base/misc/StringUtils.h"
#include "android/console.h"
#include "android/emulation/control/adb/AdbInterface.h"
#include "android/utils/debug.h"
#include "host-common/FeatureControl.h"
#include "host-common/hw-config.h"
#include "xr_emulator_conn.pb.h"

#define D(...) VERBOSE_PRINT(sensors, __VA_ARGS__)
#define I(...) dinfo(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define E(...) derror(__VA_ARGS__)

using xr_emulator_proto::EmulatorRequest;
using xr_emulator_proto::EmulatorResponse;
using xr_emulator_proto::EnvironmentMode;
using xr_emulator_proto::InputMode;
using xr_emulator_proto::MsgType;
using xr_emulator_proto::ViewportControlMode;
using xr_emulator_proto::XrOptions_Environment;

namespace android {
namespace physics {

namespace {
static QemudClient* _xr_client_connect(void* opaque,
                                       QemudService* service,
                                       int channel,
                                       const char* client_param) {
    D("XR emulator client connected");
    XrDeviceModel* device_model = static_cast<XrDeviceModel*>(opaque);
    return device_model->initializeQemudClient(channel, client_param);
}

static void _xr_client_recv(void* opaque,
                            uint8_t* msg,
                            int msglen,
                            QemudClient* client) {
    D("%s: msg length %d", __func__, msglen);
    XrDeviceModel* device_model = static_cast<XrDeviceModel*>(opaque);
    device_model->qemudClientRecv(msg, msglen);
}

static void _xr_client_close(void* opaque) {
    D("Client Closed");
    XrDeviceModel* device_model = static_cast<XrDeviceModel*>(opaque);
    device_model->qemudClientClose();
}
}  // namespace

XrDeviceModel::XrDeviceModel() {
    if (qemud_service == nullptr) {
        qemud_service = qemud_service_register("xr_service", 0, this,
                                               _xr_client_connect, NULL, NULL);
        D("%s: qemud service initialized", __func__);
    } else {
        D("%s: qemud service already initialized", __func__);
    }
}

void XrDeviceModel::setXrInputMode(float value,
                                   PhysicalInterpolation /*mode*/) {
    enum XrInputMode m = (enum XrInputMode)value;
    sendXrInputMode(m);
    mLastInputModeRequested = m;
}

float XrDeviceModel::getXrInputMode(
        ParameterValueType /*parameterValueType*/) const {
    return mLastInputModeRequested;
}

void XrDeviceModel::setXrEnvironmentMode(float value,
                                         PhysicalInterpolation /*mode*/) {
    const enum XrEnvironmentMode m = (enum XrEnvironmentMode)value;
    sendXrEnvironmentMode(m);
    mLastEnvironmentModeRequested = m;
}

float XrDeviceModel::getXrEnvironmentMode(
        ParameterValueType /*parameterValueType*/) const {
    return mLastInputModeRequested;
}

void XrDeviceModel::setXrScreenRecenter(float value,
                                        PhysicalInterpolation /*mode*/) {
    sendXrScreenRecenter();
}

float XrDeviceModel::getXrScreenRecenter(
        ParameterValueType /*parameterValueType*/) const {
    return 0;
}

void XrDeviceModel::setXrViewportControlMode(float value,
                                             PhysicalInterpolation /*mode*/) {
    const enum XrViewportControlMode m = (enum XrViewportControlMode)value;
    sendXrViewportControlMode(m);
    mLastViewportControlModeRequested = m;
}

float XrDeviceModel::getXrViewportControlMode(
        ParameterValueType /*parameterValueType*/) const {
    return mLastViewportControlModeRequested;
}

void XrDeviceModel::setXrHeadRotation(float x,
                            float y,
                            float z,
                            PhysicalInterpolation mode) {
    D("XrDeviceModel::setXrHeadRotation %f %f %f", x, y, z);
    EmulatorRequest request;
    request.set_msg_type(MsgType::MSG_TYPE_SET_HEAD_ROTATION);
    auto rotation_param = request.mutable_xr_head_rotation_event();
    rotation_param->set_x(x);
    rotation_param->set_y(y);
    rotation_param->set_z(z);
    qemudClientSend(request);
}

void XrDeviceModel::setXrHeadMovement(float x,
                                      float y,
                                      float z,
                                      PhysicalInterpolation mode) {
    D("XrDeviceModel::setXrHeadMovement %f %f %f", x, y, z);
    EmulatorRequest request;
    request.set_msg_type(MsgType::MSG_TYPE_SET_HEAD_MOVEMENT);
    auto translation_param = request.mutable_xr_head_movement_event();
    translation_param->set_delta_x(x);
    translation_param->set_delta_y(y);
    translation_param->set_delta_z(z);
    qemudClientSend(request);
}

void XrDeviceModel::setXrHeadAngularVelocity(float omega_x,
                              float omega_y,
                              float omega_z,
                              PhysicalInterpolation mode) {
    D("XrDeviceModel::setXrHeadAngularVelocity %f %f %f",
        omega_x, omega_y, omega_z);
    EmulatorRequest request;
    request.set_msg_type(MsgType::MSG_TYPE_SET_HEAD_ANGULAR_VELOCITY);
    auto angular_velocity_param = request.mutable_xr_head_angular_velocity_event();
    angular_velocity_param->set_omega_x(omega_x);
    angular_velocity_param->set_omega_y(omega_y);
    angular_velocity_param->set_omega_z(omega_z);
    qemudClientSend(request);
}

void XrDeviceModel::setXrHeadVelocity(float x,
                            float y,
                            float z,
                            PhysicalInterpolation mode) {
    D("XrDeviceModel::setXrHeadVelocity %f %f %f", x, y, z);
    EmulatorRequest request;
    request.set_msg_type(MsgType::MSG_TYPE_SET_HEAD_VELOCITY);
    auto velocity_param = request.mutable_xr_head_velocity_event();
    velocity_param->set_x(x);
    velocity_param->set_y(y);
    velocity_param->set_z(z);
    qemudClientSend(request);
}

void XrDeviceModel::setXrOptions(int environment,
                                float passthroughCoefficient,
                                PhysicalInterpolation mode) {
    D("XrDeviceModel::setXrOptions %f %f", environment, passthroughCoefficient);
    EmulatorRequest request;
    request.set_msg_type(MsgType::MSG_TYPE_SET_OPTIONS);
    auto xr_options_param = request.mutable_xr_options();
    xr_options_param->set_environment(static_cast<XrOptions_Environment>(environment));
    xr_options_param->set_passthrough_coefficient(passthroughCoefficient);
    qemudClientSend(request);

    vec3 currentOptions = getXrOptions(PARAMETER_VALUE_TYPE_CURRENT);
    int currentEnvironment = static_cast<int>(currentOptions.x);
    float currentPassthroughCoefficient = currentOptions.y;

    if (currentEnvironment != environment || 
        currentPassthroughCoefficient != passthroughCoefficient) {
        xr_emulator_proto::XrOptions options;
        options.set_environment(static_cast<xr_emulator_proto::XrOptions_Environment>(environment));
        options.set_passthrough_coefficient(passthroughCoefficient);

        mXrOptionsPublisher.fireEvent(options);
    }
}

vec3 XrDeviceModel::getXrOptions(
        ParameterValueType parameterValueType) const {
    // TODO(b/396418192): implement get environment mode handling in guest OS
    return {1, 0.5, 0};
}

void XrDeviceModel::sendXrInputMode(enum XrInputMode mode) {
    D("XrDeviceModel::sendXrInputMode");
    EmulatorRequest request;
    request.set_msg_type(MsgType::MSG_TYPE_SET_INPUT_MODE);
    auto proto_mode = InputMode::INPUT_MODE_MOUSE_UNKNOWN;
    switch (mode) {
        case XR_INPUT_MODE_MOUSE_KEYBOARD:
            proto_mode = InputMode::INPUT_MODE_MOUSE_KEYBOARD;
            break;
        case XR_INPUT_MODE_HAND_RAYCAST:
            proto_mode = InputMode::INPUT_MODE_HAND_RAYCAST;
            break;
        case XR_INPUT_MODE_EYE_TRACKING:
            proto_mode = InputMode::INPUT_MODE_EYE_TRACKING;
            break;
        default:
            W("Unknown XR input mode requested: %d, ignored.\n", mode);
            break;
    }
    request.set_input_mode(proto_mode);
    qemudClientSend(request);
}

void XrDeviceModel::sendXrEnvironmentMode(enum XrEnvironmentMode mode) {
    D("XrDeviceModel::sendXrEnvironmentMode");
    EmulatorRequest request;
    request.set_msg_type(MsgType::MSG_TYPE_SET_ENVIRONMENT_MODE);
    auto proto_mode = EnvironmentMode::ENVIRONMENT_MODE_UNKNOWN;
    switch (mode) {
        case XR_ENVIRONMENT_MODE_PASSTHROUGH_ON:
            proto_mode = EnvironmentMode::ENVIRONMENT_MODE_PASSTHROUGH_ON;
            break;
        case XR_ENVIRONMENT_MODE_PASSTHROUGH_OFF:
            proto_mode = EnvironmentMode::ENVIRONMENT_MODE_PASSTHROUGH_OFF;
            break;
        case XR_ENVIRONMENT_MODE_LIVING_ROOM_DAY:
            proto_mode = EnvironmentMode::ENVIRONMENT_MODE_LIVING_ROOM_DAY;
            break;
        case XR_ENVIRONMENT_MODE_LIVING_ROOM_NIGHT:
            proto_mode = EnvironmentMode::ENVIRONMENT_MODE_LIVING_ROOM_NIGHT;
            break;
        default:
            W("Unknown XR environment mode requested: %d, ignored.\n", mode);
            return;
    }
    request.set_environment_mode(proto_mode);
    qemudClientSend(request);
}

void XrDeviceModel::sendXrScreenRecenter() {
    D("XrDeviceModel::sendXrScreenRecenter");
    EmulatorRequest request;
    request.set_msg_type(MsgType::MSG_TYPE_RECENTER_SCREEN);
    qemudClientSend(request);
}

void XrDeviceModel::sendXrViewportControlMode(enum XrViewportControlMode mode) {
    D("XrDeviceModel::sendXrViewportControlMode");
    EmulatorRequest request;
    request.set_msg_type(MsgType::MSG_TYPE_SET_VIEWPORT_CONTROL);
    ViewportControlMode proto_mode =
            ViewportControlMode::VIEWPORT_CONTROL_MODE_UNKNOWN;
    switch (mode) {
        case VIEWPORT_CONTROL_MODE_PAN:
            proto_mode = ViewportControlMode::VIEWPORT_CONTROL_MODE_PAN;
            break;
        case VIEWPORT_CONTROL_MODE_ZOOM:
            proto_mode = ViewportControlMode::VIEWPORT_CONTROL_MODE_ZOOM;
            break;
        case VIEWPORT_CONTROL_MODE_ROTATE:
            proto_mode = ViewportControlMode::VIEWPORT_CONTROL_MODE_ROTATE;
            break;
        default:
            W("Unknown XR viewport mode requested: %d, ignored.\n", mode);
            return;
    }
    request.set_viewport_control_mode(proto_mode);
    qemudClientSend(request);
}

QemudClient* XrDeviceModel::initializeQemudClient(int channel,
                                                  const char* client_param) {
    D("XrDeviceModel::initializeQemudClient");
    qemud_client =
            qemud_client_new(qemud_service, channel, client_param, this,
                             _xr_client_recv, _xr_client_close, NULL, NULL);
    qemud_client_set_framing(qemud_client, 1);
    return qemud_client;
}

void XrDeviceModel::qemudClientRecv(uint8_t* msg, int msglen) {
    D("XrDeviceModel::qemudClientRecv");
    EmulatorResponse response;
    if (response.ParseFromArray(msg, msglen)) {
        switch (response.response_case()) {
            case EmulatorResponse::kStatus: {
                D("Status: %d", static_cast<int>(response.status()));
                break;
            }

            case EmulatorResponse::kXrOptions: {
                const auto& xr_options = response.xr_options();
                I("XrOptions received: %s", xr_options.ShortDebugString().c_str());
                if (xr_options.has_passthrough_coefficient()) {
                    I("Received passthrough coefficient: %f",
                      xr_options.passthrough_coefficient());
                    mXrOptionsPublisher.fireEvent(xr_options);
                }
                break;
            }

            case EmulatorResponse::RESPONSE_NOT_SET: {
                E("No response type set");
                break;
            }
        }
    } else {
        E("Cannot parse raw string: %s", std::string_view(reinterpret_cast<const char*>(msg), msglen));
    }
}

void XrDeviceModel::qemudClientClose() {
    D("XrDeviceModel::qemudClientClose");
}

void XrDeviceModel::qemudClientSend(
        const xr_emulator_proto::EmulatorRequest& request) {
    if (qemud_client == nullptr) {
        W("Client not connected yet. Ignoring message!");
        return;
    }
    std::string serialized_request;
    request.SerializeToString(&serialized_request);
    qemud_client_send(
            qemud_client,
            reinterpret_cast<const uint8_t*>(serialized_request.c_str()),
            serialized_request.length());
}

}  // namespace physics
}  // namespace android
