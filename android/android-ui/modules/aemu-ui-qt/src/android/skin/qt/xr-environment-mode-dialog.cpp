// Copyright (C) 2024 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "xr-environment-mode-dialog.h"
#include "ui_xr-environment-mode-dialog.h"

#include "android/hw-sensors.h"

XrEnvironmentModeDialog::XrEnvironmentModeDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::XrEnvironmentModeDialog) {
    ui->setupUi(this);
    setWindowFlags(Qt::Popup);
}

void XrEnvironmentModeDialog::on_btn_xr_environment_passthrough_on_clicked() {
    emit onXrEnvironmentModeRequested(XR_ENVIRONMENT_MODE_PASSTHROUGH_ON);
    accept();  // hides dialog
}

void XrEnvironmentModeDialog::on_btn_xr_environment_passthrough_off_clicked() {
    emit onXrEnvironmentModeRequested(XR_ENVIRONMENT_MODE_PASSTHROUGH_OFF);
    accept();  // hides dialog
}

void XrEnvironmentModeDialog::on_btn_xr_environment_living_room_day_clicked() {
    emit onXrEnvironmentModeRequested(XR_ENVIRONMENT_MODE_LIVING_ROOM_DAY);
    accept();  // hides dialog
}

void XrEnvironmentModeDialog::
        on_btn_xr_environment_living_room_night_clicked() {
    emit onXrEnvironmentModeRequested(XR_ENVIRONMENT_MODE_LIVING_ROOM_NIGHT);
    accept();  // hides dialog
}

XrEnvironmentModeDialog::~XrEnvironmentModeDialog() {
    delete ui;
}
