/*
    SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami

// Settings a selected theme doesn't use are greyed out; their values stay saved.
KCM.SimpleKCM {
    id: root

    actions: [
        Kirigami.Action {
            icon.name: "configure"
            text: i18nc("@action:button", "Exceptions…")
            enabled: kcm.decorationSupported
            onTriggered: kcm.push("Exceptions.qml")
        }
    ]

    component SettingCheckBox: QQC2.CheckBox {
        id: box
        required property QtObject configObject
        required property string settingName
        property bool supported: true
        checked: configObject[settingName]
        onToggled: configObject[settingName] = checked

        KCM.SettingStateBinding {
            configObject: box.configObject
            settingName: box.settingName
            extraEnabledConditions: box.supported
        }
    }

    // A section's note, shown while the selected theme doesn't use the section.
    // A preferred width: a wrapped label's natural width is its unwrapped text, which
    // would switch the form to its narrow layout.
    component UnsupportedNote: QQC2.Label {
        Layout.preferredWidth: Kirigami.Units.gridUnit * 20
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        font: Kirigami.Theme.smallFont
    }

    Kirigami.FormLayout {
        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "Desktop")
        }

        UnsupportedNote {
            visible: !kcm.desktopStyleSupported
            text: i18nc("@info", "The selected desktop style uses its own transparency. Desktop styles such as Silver Light and Silver Dark use these settings.")
        }

        SettingCheckBox {
            text: i18nc("@option:check", "Use these settings instead of the desktop style's own")
            configObject: kcm.desktopStyle
            settingName: "enabled"
            supported: kcm.desktopStyleSupported
        }

        OpacitySlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Panels:")
            configObject: kcm.desktopStyle
            settingName: "panels"
            supported: kcm.desktopStyleSupported
        }
        OpacitySlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Pop-ups:")
            configObject: kcm.desktopStyle
            settingName: "popups"
            supported: kcm.desktopStyleSupported
        }
        OpacitySlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Tooltips:")
            configObject: kcm.desktopStyle
            settingName: "tooltips"
            supported: kcm.desktopStyleSupported
        }
        OpacitySlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Desktop widgets:")
            configObject: kcm.desktopStyle
            settingName: "widgets"
            supported: kcm.desktopStyleSupported
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "Menus")
        }

        UnsupportedNote {
            visible: !kcm.applicationStyleSupported
            text: i18nc("@info", "The selected application style draws opaque menus. The Silver and Breeze application styles use this setting.")
        }

        OpacitySlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Menus:")
            configObject: kcm.menus
            settingName: "opacity"
            supported: kcm.applicationStyleSupported
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            Kirigami.FormData.label: i18nc("@title:group", "Titlebars")
        }

        UnsupportedNote {
            visible: !kcm.decorationSupported
            text: i18nc("@info", "The selected window decoration draws opaque titlebars. The Silver window decoration uses these settings.")
        }

        OpacitySlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Active window:")
            configObject: kcm.toolsArea
            settingName: "activeOpacity"
            supported: kcm.decorationSupported
        }
        OpacitySlider {
            Kirigami.FormData.label: i18nc("@label:slider", "Inactive window:")
            configObject: kcm.toolsArea
            settingName: "inactiveOpacity"
            supported: kcm.decorationSupported
        }

        SettingCheckBox {
            text: i18nc("checkbox label", "Always make maximized titlebars opaque")
            configObject: kcm.toolsArea
            settingName: "opaqueWhenMaximized"
            supported: kcm.decorationSupported
        }
        SettingCheckBox {
            text: i18nc("checkbox label", "Blur titlebars that have transparency")
            configObject: kcm.toolsArea
            settingName: "blurBehind"
            supported: kcm.decorationSupported
        }
        SettingCheckBox {
            text: i18nc("checkbox label", "Apply these opacity settings to the entire header area (toolbars and menu bars)")
            configObject: kcm.toolsArea
            settingName: "applyOpacityToHeader"
            supported: kcm.decorationSupported
        }
        SettingCheckBox {
            text: i18nc("checkbox label", "Match the titlebar to the application colour")
            configObject: kcm.toolsArea
            settingName: "matchApplicationColor"
            supported: kcm.decorationSupported
        }
    }
}
