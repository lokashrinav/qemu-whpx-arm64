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
#ifndef XRINPUTMODEDIALOG_H
#define XRINPUTMODEDIALOG_H

#include <QDialog>

namespace Ui {
class XrInputModeDialog;
}

class XrInputModeDialog : public QDialog {
    Q_OBJECT

public:
    explicit XrInputModeDialog(QWidget* parent = nullptr);
    ~XrInputModeDialog();

signals:
    void onXrInputModeRequested(int control);

private slots:
    void on_btn_xr_input_keyboard_mouse_clicked();
    void on_btn_xr_input_hand_raycast_clicked();
    void on_btn_xr_input_eye_tacking_clicked();

private:
    Ui::XrInputModeDialog* ui;
    bool mShown = false;
};

#endif  // XRINPUTMODEDIALOG_H
