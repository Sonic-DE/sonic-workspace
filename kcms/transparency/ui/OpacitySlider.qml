/*
    SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami

// One opacity, 0–100: a box and a slider editing the same setting. The % sits outside the box:
// the desktop style's spin box commits each keystroke, so a suffix would move the cursor.
ColumnLayout {
    id: opacity

    required property QtObject configObject
    required property string settingName
    property bool supported: true

    Layout.fillWidth: true
    Layout.minimumWidth: Kirigami.Units.gridUnit * 20

    RowLayout {
        enabled: slider.enabled
        QQC2.Label {
            text: i18nc("@label:slider end", "Transparent")
        }
        Item {
            Layout.fillWidth: true
        }
        QQC2.SpinBox {
            from: 0
            to: 100
            value: opacity.configObject[opacity.settingName]
            editable: true
            onValueModified: opacity.configObject[opacity.settingName] = value
        }
        QQC2.Label {
            text: i18nc("@label percent sign after an opacity value", "%")
        }
        Item {
            Layout.fillWidth: true
        }
        QQC2.Label {
            text: i18nc("@label:slider end", "Opaque")
        }
    }
    QQC2.Slider {
        id: slider
        Layout.fillWidth: true
        from: 0
        to: 100
        stepSize: 1
        value: opacity.configObject[opacity.settingName]
        onMoved: opacity.configObject[opacity.settingName] = value

        KCM.SettingStateBinding {
            configObject: opacity.configObject
            settingName: opacity.settingName
            extraEnabledConditions: opacity.supported
        }
    }
}
