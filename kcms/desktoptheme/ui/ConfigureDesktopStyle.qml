/*
    SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami

// Settings for desktop styles that support them, laid out like Window Decorations'
// Configure Titlebar page. Transparency: [DesktopThemeOpacity] in kdeglobals.
KCM.SimpleKCM {
    title: i18nc("@title", "Configure Desktop Style")

    // One opacity, 0–100 %: a percentage box between "Transparent" and "Opaque" over a
    // slider, both editing the same value.
    component OpacitySlider: ColumnLayout {
        id: opacity
        required property string settingName
        Layout.fillWidth: true
        Layout.minimumWidth: Kirigami.Units.gridUnit * 20

        RowLayout {
            QQC2.Label {
                text: i18nc("@label:slider end", "Transparent")
            }
            Item {
                Layout.fillWidth: true
            }
            QQC2.SpinBox {
                from: 0
                to: 100
                value: kcm.transparency[opacity.settingName]
                textFromValue: (value, locale) => i18nc("opacity percentage", "%1 %", value)
                valueFromText: (text, locale) => parseInt(text)
                editable: true
                onValueModified: kcm.transparency[opacity.settingName] = value
            }
            Item {
                Layout.fillWidth: true
            }
            QQC2.Label {
                text: i18nc("@label:slider end", "Opaque")
            }
        }
        QQC2.Slider {
            Layout.fillWidth: true
            from: 0
            to: 100
            stepSize: 1
            value: kcm.transparency[opacity.settingName]
            onMoved: kcm.transparency[opacity.settingName] = value

            KCM.SettingStateBinding {
                configObject: kcm.transparency
                settingName: opacity.settingName
            }
        }
    }

    ColumnLayout {
        Kirigami.InlineMessage {
            Layout.fillWidth: true
            type: Kirigami.MessageType.Information
            visible: !kcm.desktopThemeModel.selectedThemeFollowsDesktopThemeOpacity
            text: i18nc("@info", "The selected desktop style uses its own settings. These settings apply to desktop styles that support them, such as Silver Light and Silver Dark.")
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            Kirigami.Separator {
                Kirigami.FormData.isSection: true
                Kirigami.FormData.label: i18nc("@title:group", "Transparency")
            }

            QQC2.CheckBox {
                text: i18nc("@option:check", "Use these settings instead of the desktop style's own")
                checked: kcm.transparency.enabled
                onToggled: kcm.transparency.enabled = checked

                KCM.SettingStateBinding {
                    configObject: kcm.transparency
                    settingName: "enabled"
                }
            }

            OpacitySlider {
                Kirigami.FormData.label: i18nc("@label:slider", "Panels:")
                settingName: "panels"
            }
            OpacitySlider {
                Kirigami.FormData.label: i18nc("@label:slider", "Pop-ups:")
                settingName: "popups"
            }
            OpacitySlider {
                Kirigami.FormData.label: i18nc("@label:slider", "Tooltips:")
                settingName: "tooltips"
            }
            OpacitySlider {
                Kirigami.FormData.label: i18nc("@label:slider", "Desktop widgets:")
                settingName: "widgets"
            }
        }
    }
}
