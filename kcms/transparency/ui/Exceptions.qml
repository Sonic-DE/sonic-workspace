/*
    SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami

// Per-application exceptions to the titlebar settings ([ToolsArea] exceptions in kdeglobals).
KCM.SimpleKCM {
    title: i18nc("@title", "Titlebar Exceptions")

    // An exception's opacity: overridden (checked, with its value) or not (the titlebar
    // setting applies).
    component OpacityOverride: RowLayout {
        id: opacityOverride
        required property var exceptionModel
        required property string role
        required property string label
        readonly property bool overridden: exceptionModel[role] !== undefined && exceptionModel[role] !== null
        QQC2.CheckBox {
            text: opacityOverride.label
            checked: opacityOverride.overridden
            onToggled: opacityOverride.exceptionModel[opacityOverride.role] = checked ? kcm.toolsArea[opacityOverride.role] : null
        }
        // The % stays outside the box, as in OpacitySlider.qml.
        QQC2.SpinBox {
            id: overrideValue
            from: 0
            to: 100
            enabled: opacityOverride.overridden
            value: opacityOverride.overridden ? opacityOverride.exceptionModel[opacityOverride.role] : kcm.toolsArea[opacityOverride.role]
            editable: true
            onValueModified: opacityOverride.exceptionModel[opacityOverride.role] = value
        }
        QQC2.Label {
            enabled: overrideValue.enabled
            text: i18nc("@label percent sign after an opacity value", "%")
        }
    }

    // An exception's on/off setting: overridden or not, and its value.
    component SwitchOverride: RowLayout {
        id: switchOverride
        required property var exceptionModel
        required property string role
        required property string label
        readonly property bool overridden: exceptionModel[role] !== undefined && exceptionModel[role] !== null
        QQC2.CheckBox {
            text: i18nc("@option:check override a setting", "Override:")
            checked: switchOverride.overridden
            onToggled: switchOverride.exceptionModel[switchOverride.role] = checked ? kcm.toolsArea[switchOverride.role] : null
        }
        QQC2.CheckBox {
            text: switchOverride.label
            enabled: switchOverride.overridden
            checked: switchOverride.overridden ? switchOverride.exceptionModel[switchOverride.role] : kcm.toolsArea[switchOverride.role]
            onToggled: switchOverride.exceptionModel[switchOverride.role] = checked
        }
    }

    // One exception: the applications it matches, and each titlebar setting it overrides
    // (unchecked: the titlebar setting applies).
    component ExceptionEditor: ColumnLayout {
        id: exception
        required property int index
        required property var model
        // A built-in default exception: it can only be switched off.
        readonly property bool isDefault: model.isDefault === true
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 30

        RowLayout {
            QQC2.CheckBox {
                checked: exception.model.exceptionEnabled
                onToggled: exception.model.exceptionEnabled = checked
                QQC2.ToolTip.text: i18nc("@info:tooltip", "Use this exception")
                QQC2.ToolTip.visible: hovered
            }
            QQC2.TextField {
                Layout.fillWidth: true
                readOnly: exception.isDefault
                placeholderText: i18nc("@info:placeholder", "Application name or window class (regular expression)")
                text: exception.model.pattern
                onTextEdited: exception.model.pattern = text
            }
            QQC2.Label {
                visible: exception.isDefault
                text: i18nc("@label a built-in exception", "Default")
            }
            QQC2.Button {
                visible: !exception.isDefault
                icon.name: "list-remove"
                display: QQC2.AbstractButton.IconOnly
                text: i18nc("@action:button", "Remove Exception")
                onClicked: kcm.toolsAreaExceptions.remove(exception.index)
                QQC2.ToolTip.text: text
                QQC2.ToolTip.visible: hovered
            }
        }

        OpacityOverride {
            enabled: !exception.isDefault
            exceptionModel: exception.model
            role: "activeOpacity"
            label: i18nc("@option:check", "Active window opacity:")
        }
        OpacityOverride {
            enabled: !exception.isDefault
            exceptionModel: exception.model
            role: "inactiveOpacity"
            label: i18nc("@option:check", "Inactive window opacity:")
        }
        SwitchOverride {
            enabled: !exception.isDefault
            exceptionModel: exception.model
            role: "opaqueWhenMaximized"
            label: i18nc("checkbox label", "Always make maximized titlebars opaque")
        }
        SwitchOverride {
            enabled: !exception.isDefault
            exceptionModel: exception.model
            role: "blurBehind"
            label: i18nc("checkbox label", "Blur titlebars that have transparency")
        }
        SwitchOverride {
            enabled: !exception.isDefault
            exceptionModel: exception.model
            role: "applyOpacityToHeader"
            label: i18nc("checkbox label", "Apply these opacity settings to the entire header area")
        }
        SwitchOverride {
            enabled: !exception.isDefault
            exceptionModel: exception.model
            role: "matchApplicationColor"
            label: i18nc("checkbox label", "Match the titlebar to the application colour")
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }
    }

    ColumnLayout {
        QQC2.Label {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            wrapMode: Text.WordWrap
            text: i18nc("@info", "An exception changes only the titlebar settings, for the applications it matches.")
        }

        Repeater {
            model: kcm.toolsAreaExceptions
            delegate: ExceptionEditor {}
        }

        QQC2.Button {
            icon.name: "list-add"
            text: i18nc("@action:button", "Add Exception")
            onClicked: kcm.toolsAreaExceptions.add()
        }
    }
}
