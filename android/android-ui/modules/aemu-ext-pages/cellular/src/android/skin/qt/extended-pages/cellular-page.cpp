// Copyright (C) 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/skin/qt/extended-pages/cellular-page.h"

#include <qsettings.h>  // for QSettings::Ini...
#include <qstring.h>    // for operator+

#include <QComboBox>  // for QComboBox
#include <QSettings>  // for QSettings
#include <QVariant>   // for QVariant

#include "aemu/base/async/Looper.h"
#include "aemu/base/async/ThreadLooper.h"
#include "android/avd/util.h"  // for path_getAvdCon...
#include "android/console.h"   // for android_hw
#include "android/console.h"
#include "android/emulation/control/cellular_agent.h"  // for QAndroidCellul...
#include "android/emulator-window.h"                   // for emulator_windo...
#include "android/main-common.h"                       // for emulator_has_n...
#include "android/metrics/UiEventTracker.h"
#include "android/skin/qt/qt-settings.h"  // for PER_AVD_SETTIN...
#include "android/utils/debug.h"
#include "host-common/VmLock.h"  // for RecursiveScope...
#include "ui_cellular-page.h"    // for CellularPage

class QWidget;

// Must be protected by the BQL!
static const QAndroidCellularAgent* sCellularAgent = nullptr;
static void saveDataStatus(int status);
static void saveNetworkType(int type);
static void saveSignalStrength(int strength);
static void saveVoiceStatus(int status);
static void saveMeterStatus(int status);
static int getSavedDataStatus();
static int getSavedMeterStatus();
static int getSavedNetworkType();
static int getSavedSignalStrength();
static int getSavedVoiceStatus();

#define STATE(p) \
    case (p):    \
        s = #p;  \
        break;


static std::string translate_idx(CellularStatus value) {
    std::string s = "";
    switch (value) {
        STATE(Cellular_Stat_Home);
        STATE(Cellular_Stat_Roaming);
        STATE(Cellular_Stat_Searching);
        STATE(Cellular_Stat_Denied);
        STATE(Cellular_Stat_Unregistered);
        default:
            derror("%s: Unseen value for cellular status: 0x%x", __func__, value);
            return "Unknown";
    }
    // Chop off "Cellular_"
    return s.substr(9);
}

static std::string translate_idx(CellularStandard value) {
    std::string s = "";
    switch (value) {
        STATE(Cellular_Std_GSM);
        STATE(Cellular_Std_HSCSD);
        STATE(Cellular_Std_GPRS);
        STATE(Cellular_Std_EDGE);
        STATE(Cellular_Std_UMTS);
        STATE(Cellular_Std_HSDPA);
        STATE(Cellular_Std_LTE);
        STATE(Cellular_Std_full);
        STATE(Cellular_Std_5G);
        default:
            derror("%s: Unseen value for cellular standasrd: 0x%x", __func__, value);
            return "Unknown";
    }
    // Chop off "Cellular_"
    return s.substr(9);
}

static std::string translate_idx(CellularSignal value) {
    std::string s = "";
    switch (value) {
        STATE(Cellular_Signal_None);
        STATE(Cellular_Signal_Poor);
        STATE(Cellular_Signal_Moderate);
        STATE(Cellular_Signal_Good);
        STATE(Cellular_Signal_Great);
        default:
            derror("%s: Unseen value for cellular signal: 0x%x", __func__, value);
            return "Unknown";
    }
    // Chop off "Cellular_"
    return s.substr(9);
}

static std::string translate_idx(CellularMeterStatus value) {
    std::string s = "";
    switch (value) {
        STATE(Cellular_Metered);
        STATE(Cellular_Temporarily_Not_Metered);
    }
    return s;
}

CellularPage::CellularPage(QWidget* parent)
    : QWidget(parent),
      mUi(new Ui::CellularPage()),
      mDropDownTracker(new UiEventTracker(
              android_studio::EmulatorUiEvent::OPTION_SELECTED,
              android_studio::EmulatorUiEvent::EXTENDED_CELLULAR_TAB)) {
    mUi->setupUi(this);
    // Restore previous setting values to the UI widgets

    // Network type
    mUi->cell_standardBox->setCurrentIndex(getSavedNetworkType());

    // Signal strength
    mUi->cell_signalStatusBox->setCurrentIndex(getSavedSignalStrength());

    // Voice status
    mUi->cell_voiceStatusBox->setCurrentIndex(getSavedVoiceStatus());

    // Data status
    mUi->cell_dataStatusBox->setCurrentIndex(getSavedDataStatus());

    // Meter status
    mUi->cell_meterStatusBox->setCurrentIndex(
            getSavedMeterStatus());  // always metered
}

// static
void CellularPage::setCellularAgent(const QAndroidCellularAgent* agent) {
    if (!agent)
        return;

    sCellularAgent = agent;

    // Network parameters

    if (emulator_has_network_option) {
        // The user specified network parameters on the command line.
        // Do not override the command-line values.
        return;
    }

    android::base::ThreadLooper::runOnMainLooper([agent]() {
        // Get the settings that were previously saved. Give them
        // to the device.

        // Network type
        if (agent->setStandard) {
            agent->setStandard((CellularStandard)getSavedNetworkType());
        }

        // Signal strength
        if (agent->setSignalStrengthProfile) {
            agent->setSignalStrengthProfile(
                    (CellularSignal)getSavedSignalStrength());
        }

        // Voice status
        if (agent->setVoiceStatus) {
            agent->setVoiceStatus((CellularStatus)getSavedVoiceStatus());
        }

        // Data status
        if (agent->setDataStatus) {
            agent->setDataStatus((CellularStatus)getSavedDataStatus());
        }

        // Meter status
        if (agent->setMeterStatus) {
            agent->setMeterStatus((CellularMeterStatus)getSavedMeterStatus());
        }
    });
}

void CellularPage::on_cell_standardBox_currentIndexChanged(int index) {
    saveNetworkType(index);

    if (sCellularAgent && sCellularAgent->setStandard) {
        CellularStandard cStandard = (CellularStandard)index;
        const auto* agent = sCellularAgent;
        android::base::ThreadLooper::runOnMainLooper(
                [agent, cStandard]() { agent->setStandard(cStandard); });
        mDropDownTracker->increment(translate_idx(cStandard));
    }
}

void CellularPage::on_cell_voiceStatusBox_currentIndexChanged(int index) {
    saveVoiceStatus(index);

    if (sCellularAgent && sCellularAgent->setVoiceStatus) {
        CellularStatus vStatus = (CellularStatus)index;
        const auto* agent = sCellularAgent;
        android::base::ThreadLooper::runOnMainLooper(
                [agent, vStatus]() { agent->setVoiceStatus(vStatus); });
        mDropDownTracker->increment(translate_idx(vStatus) + "_VOICE");
    }
}

void CellularPage::on_cell_meterStatusBox_currentIndexChanged(int index) {
    saveMeterStatus(index);

    if (sCellularAgent && sCellularAgent->setMeterStatus) {
        CellularMeterStatus mStatus = (CellularMeterStatus)index;
        const auto* agent = sCellularAgent;
        android::base::ThreadLooper::runOnMainLooper(
                [agent, mStatus]() { agent->setMeterStatus(mStatus); });
        mDropDownTracker->increment(translate_idx(mStatus));
    }
}

void CellularPage::on_cell_dataStatusBox_currentIndexChanged(int index) {
    saveDataStatus(index);

    if (sCellularAgent && sCellularAgent->setDataStatus) {
        CellularStatus dStatus = (CellularStatus)index;
        const auto* agent = sCellularAgent;
        android::base::ThreadLooper::runOnMainLooper(
                [agent, dStatus]() { agent->setDataStatus(dStatus); });
        mDropDownTracker->increment(translate_idx(dStatus) + "_DATA");
    }
}

void CellularPage::on_cell_signalStatusBox_currentIndexChanged(int index) {
    saveSignalStrength(index);

    if (sCellularAgent && sCellularAgent->setSignalStrengthProfile) {
        const auto* agent = sCellularAgent;
        CellularSignal signal = (CellularSignal)index;
        android::base::ThreadLooper::runOnMainLooper(
                [agent, signal]() { agent->setSignalStrengthProfile(signal); });
        mDropDownTracker->increment(translate_idx(signal));
    }
}

//////////////
//
// Local static functions to save and retrieve settings

static void saveDataStatus(int status) {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_CELLULAR_DATA_STATUS,
                                     status);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::CELLULAR_DATA_STATUS, status);
    }
}

static void saveNetworkType(int type) {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(
                Ui::Settings::PER_AVD_CELLULAR_NETWORK_TYPE, type);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::CELLULAR_NETWORK_TYPE, type);
    }
}

static void saveSignalStrength(int strength) {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(
                Ui::Settings::PER_AVD_CELLULAR_SIGNAL_STRENGTH, strength);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::CELLULAR_SIGNAL_STRENGTH, strength);
    }
}

static void saveVoiceStatus(int status) {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(
                Ui::Settings::PER_AVD_CELLULAR_VOICE_STATUS, status);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::CELLULAR_VOICE_STATUS, status);
    }
}

static void saveMeterStatus(int status) {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(
                Ui::Settings::PER_AVD_CELLULAR_METER_STATUS, status);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::CELLULAR_METER_STATUS, status);
    }
}

static int getSavedMeterStatus() {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings
                .value(Ui::Settings::PER_AVD_CELLULAR_METER_STATUS,
                       Cellular_Metered)
                .toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings
                .value(Ui::Settings::CELLULAR_METER_STATUS, Cellular_Metered)
                .toInt();
    }
}

static int getSavedDataStatus() {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings
                .value(Ui::Settings::PER_AVD_CELLULAR_DATA_STATUS,
                       Cellular_Stat_Home)
                .toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings
                .value(Ui::Settings::CELLULAR_DATA_STATUS, Cellular_Stat_Home)
                .toInt();
    }
}

static int getSavedNetworkType() {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings
                .value(Ui::Settings::PER_AVD_CELLULAR_NETWORK_TYPE,
                       Cellular_Std_full)
                .toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings
                .value(Ui::Settings::CELLULAR_NETWORK_TYPE, Cellular_Std_full)
                .toInt();
    }
}

static int getSavedSignalStrength() {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings
                .value(Ui::Settings::PER_AVD_CELLULAR_SIGNAL_STRENGTH,
                       Cellular_Signal_Moderate)
                .toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings
                .value(Ui::Settings::CELLULAR_SIGNAL_STRENGTH,
                       Cellular_Signal_Moderate)
                .toInt();
    }
}

static int getSavedVoiceStatus() {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return avdSpecificSettings
                .value(Ui::Settings::PER_AVD_CELLULAR_VOICE_STATUS,
                       Cellular_Stat_Home)
                .toInt();
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return settings
                .value(Ui::Settings::CELLULAR_VOICE_STATUS, Cellular_Stat_Home)
                .toInt();
    }
}
