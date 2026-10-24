/*
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Effects

import org.kde.kirigami as Kirigami

Item {
    id: wallpaperFader
    property Item clock
    property Item mainStack
    property Item footer
    property alias source: wallpaperBlur.source
    property real factor: 0
    // The lockscreen's surrounding UI may use Complementary colors. The
    // wallpaper tint must always follow the user's actual Window background.
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    readonly property color tintColor: Kirigami.Theme.backgroundColor

    property bool alwaysShowClock: false

    state: "on"

    Behavior on factor {
        NumberAnimation {
            target: wallpaperFader
            property: "factor"
            duration: Kirigami.Units.veryLongDuration * 2
            easing.type: Easing.InOutQuad
        }
    }
    MultiEffect {
        id: wallpaperBlur
        anchors.fill: parent
        // This is a full-screen effect, not an item shadow. Automatic padding
        // changes the captured bounds and exposes the sharp wallpaper edges.
        autoPaddingEnabled: false
        blurEnabled: true
        blurMax: 50
        blur: wallpaperFader.factor
        blurMultiplier: 0
    }
    ShaderEffect {
        id: wallpaperShader
        anchors.fill: parent
        supportsAtlasTextures: true
        property var source: ShaderEffectSource {
            sourceItem: wallpaperBlur
            live: true
            hideSource: true
            textureMirroring: ShaderEffectSource.NoMirroring
        }

        readonly property color tintColor: wallpaperFader.tintColor
        readonly property real tintAmount: 0.35 * wallpaperFader.factor

        fragmentShader: "qrc:/qt/qml/org/kde/breeze/components/shaders/WallpaperFader.frag.qsb"
    }

    states: [
        State {
            name: "on"
            PropertyChanges {
                mainStack.opacity: 1
                footer.opacity: 1
                wallpaperFader.factor: 1
                clock.shadow.opacity: 0
                clock.opacity: 1
            }
        },
        State {
            name: "off"
            PropertyChanges {
                mainStack.opacity: 0
                footer.opacity: 0
                wallpaperFader.factor: 0
                clock.shadow.opacity: wallpaperFader.alwaysShowClock ? 1 : 0
                clock.opacity: wallpaperFader.alwaysShowClock ? 1 : 0
            }
        }
    ]
    transitions: [
        Transition {
            from: "off"
            to: "on"
            //Note: can't use animators as they don't play well with parallelanimations
            NumberAnimation {
                targets: [wallpaperFader.mainStack, wallpaperFader.footer, wallpaperFader.clock]
                property: "opacity"
                duration: Kirigami.Units.veryLongDuration
                easing.type: Easing.InOutQuad
            }
        },
        Transition {
            from: "on"
            to: "off"
            NumberAnimation {
                targets: [wallpaperFader.mainStack, wallpaperFader.footer, wallpaperFader.clock]
                property: "opacity"
                duration: Kirigami.Units.veryLongDuration
                easing.type: Easing.InOutQuad
            }
        }
    ]
}
