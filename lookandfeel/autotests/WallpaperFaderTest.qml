/*
 * SPDX-FileCopyrightText: 2026 SonicDE Community
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
import QtQuick
import org.kde.kirigami as Kirigami
import "../components" as Components

Rectangle {
    id: root
    width: 640
    height: 400
    color: "magenta"
    property url testImage
    property bool sourceLayered: true
    property bool blurred: false
    property alias fader: fader
    property color windowBackground: "#2a2e32"
    Kirigami.Theme.backgroundColor: "magenta"
    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary

    Image {
        id: wallpaper
        anchors.fill: parent
        source: root.testImage
        smooth: false
        layer.enabled: root.sourceLayered
    }
    Item { id: mainStack }
    Item { id: footer }
    Item {
        id: clock
        property Item shadow: Item {}
    }
    Components.WallpaperFader {
        id: fader
        anchors.fill: parent
        source: wallpaper
        mainStack: mainStack
        footer: footer
        clock: clock
        Kirigami.Theme.backgroundColor: root.windowBackground
        state: root.blurred ? "on" : "off"
    }
}
