/*
    SPDX-FileCopyrightText: 2026 SonicDE Contributors

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Controls as QtControls

// A daily local time, represented as minutes after midnight.
QtControls.SpinBox {
    id: root

    from: 0
    to: 24 * 60 - 1
    editable: true
    wrap: true
    inputMethodHints: Qt.ImhTime

    // Time input may contain localized digits, separators, and AM/PM text.
    // Parse the complete input with the same locale used to display it.
    validator: null
    textFromValue: function(value, locale) {
        const time = new Date(2000, 0, 15, Math.floor(value / 60), value % 60);
        return time.toLocaleTimeString(locale, Locale.ShortFormat);
    }
    valueFromText: function(text, locale) {
        const time = Date.fromLocaleTimeString(locale, text, Locale.ShortFormat);
        return isNaN(time.getTime()) ? root.value : time.getHours() * 60 + time.getMinutes();
    }
}
