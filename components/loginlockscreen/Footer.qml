/*
    SPDX-FileCopyrightText: 2026 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.breeze.components as BreezeComponents
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.workspace.components as WorkspaceComponents

/*!
  A footer for the login and lock screens that contains components for e.g.
  switching keyboard layouts and showing the on-screen keyboard.
*/
RowLayout {
    // Note: The containment masks on the buttons stretch their clickable area to
    // the screen edges, essentially making them adhere to Fitts's law.
    //
    // Due to virtual keyboard button having an icon, buttons may have
    // different heights, so fillHeight is required.

    id: root

    // Consumers may provide another backend with the same control contract.
    property QtObject virtualKeyboard: VirtualKeyboard

    readonly property bool virtualKeyboardVisible: virtualKeyboard.visible

    function hideVirtualKeyboard() {
        virtualKeyboard.active = false;
    }

    /*!
      Signal emitted when the on-screen keyboard button is clicked.

      This signal is used to notify the login and lock screens that the on-screen
      keyboard is being activated, so they can ensure focus is set on the input field.
    */
    signal oskActivated

    anchors {
        bottom: parent.bottom
        left: parent.left
        right: parent.right
        margins: Kirigami.Units.smallSpacing
    }

    spacing: Kirigami.Units.smallSpacing

    PlasmaComponents.ToolButton {
        id: onScreenKeyboardButton

        focusPolicy: Qt.TabFocus
        text: i18ndc("plasma_shell_org.kde.plasma.desktop", "Button to show/hide virtual keyboard", "Virtual Keyboard")
        enabled: root.virtualKeyboard.available
        icon.name: root.virtualKeyboard.visible ? "input-keyboard-virtual-on" : "input-keyboard-virtual-off"

        onClicked: {
            if (root.virtualKeyboard.visible) {
                root.virtualKeyboard.active = false;
            } else {
                root.oskActivated();
                root.virtualKeyboard.forceActivate();
            }
        }

        Layout.fillHeight: true

        containmentMask: Item {
            parent: onScreenKeyboardButton
            anchors.fill: parent
            anchors.leftMargin: -root.anchors.margins
            anchors.bottomMargin: -root.anchors.margins
        }
    }

    PlasmaComponents.ToolButton {
        id: keyboardButton

        focusPolicy: Qt.TabFocus
        Accessible.description: i18ndc("plasma_shell_org.kde.plasma.desktop", "Button to change keyboard layout", "Switch layout")
        icon.name: "input-keyboard"

        WorkspaceComponents.KeyboardLayoutSwitcher {
            id: keyboardLayoutSwitcher

            anchors.fill: parent
            acceptedButtons: Qt.NoButton
        }

        text: keyboardLayoutSwitcher.layoutNames.longName
        onClicked: keyboardLayoutSwitcher.keyboardLayout.switchToNextLayout()

        visible: keyboardLayoutSwitcher.hasMultipleKeyboardLayouts

        Layout.fillHeight: true

        containmentMask: Item {
            parent: keyboardButton
            anchors.fill: parent
            anchors.leftMargin: onScreenKeyboardButton.visible ? 0 : -root.anchors.margins
            anchors.bottomMargin: -root.anchors.margins
        }
    }

    Item {
        Layout.fillWidth: true
    }

    BreezeComponents.Battery {}
}
