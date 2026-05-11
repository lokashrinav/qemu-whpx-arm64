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
#ifndef XRENVIRONMENTMODEDIALOG_H
#define XRENVIRONMENTMODEDIALOG_H

#include <QDialog>

namespace Ui {
class XrEnvironmentModeDialog;
}

class XrEnvironmentModeDialog : public QDialog {
    Q_OBJECT

public:
    explicit XrEnvironmentModeDialog(QWidget* parent = nullptr);
    ~XrEnvironmentModeDialog();

signals:
    void onXrEnvironmentModeRequested(int control);

private slots:
    void on_btn_xr_environment_passthrough_on_clicked();
    void on_btn_xr_environment_passthrough_off_clicked();
    void on_btn_xr_environment_living_room_day_clicked();
    void on_btn_xr_environment_living_room_night_clicked();

private:
    Ui::XrEnvironmentModeDialog* ui;
    bool mShown = false;
};

#endif  // XRENVIRONMENTMODEDIALOG_H
