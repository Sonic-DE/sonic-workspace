/*
    SPDX-FileCopyrightText: 2026 SonicDE Contributors

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtTest
import "../ui" as NotificationsUi

TestCase {
    name: "NotificationTimeField"
    when: windowShown
    width: 300
    height: 100

    NotificationsUi.TimeField {
        id: field
    }

    SignalSpy {
        id: modifiedSpy
        target: field
        signalName: "valueModified"
    }

    function init() {
        field.locale = Qt.locale("en_GB");
        field.value = 0;
        modifiedSpy.clear();
    }

    function test_roundTrip_data() {
        return [
            { tag: "midnight", value: 0, text: "00:00" },
            { tag: "morning", value: 7 * 60 + 5, text: "07:05" },
            { tag: "noon", value: 12 * 60, text: "12:00" },
            { tag: "evening", value: 22 * 60 + 30, text: "22:30" },
            { tag: "last-minute", value: 1439, text: "23:59" }
        ];
    }

    function test_roundTrip(data) {
        field.value = data.value;
        compare(field.displayText, data.text);
        compare(field.valueFromText(data.text, field.locale), data.value);
        compare(modifiedSpy.count, 0); // Loading/defaults must not look like user edits.
    }

    function test_localeRoundTrip_data() {
        return [
            { tag: "12-hour-midnight", locale: "en_US", value: 0, expected: "12:00 AM" },
            { tag: "12-hour-noon", locale: "en_US", value: 12 * 60, expected: "12:00 PM" },
            { tag: "12-hour-evening", locale: "en_US", value: 22 * 60, expected: "10:00 PM" },
            { tag: "24-hour", locale: "de_DE", value: 22 * 60, expected: "22:00" },
            { tag: "localized-digits", locale: "ar_EG", value: 22 * 60 + 5 },
            { tag: "localized-meridiem", locale: "zh_TW", value: 7 * 60 }
        ];
    }

    function test_localeRoundTrip(data) {
        field.locale = Qt.locale(data.locale);
        field.value = data.value;
        const time = new Date(2000, 0, 15, Math.floor(data.value / 60), data.value % 60);
        compare(field.displayText, time.toLocaleTimeString(field.locale, Locale.ShortFormat));
        if (data.expected !== undefined) {
            compare(field.displayText.replace(/\s/g, " "), data.expected);
        }
        compare(field.valueFromText(field.displayText, field.locale), data.value);
        compare(modifiedSpy.count, 0);
    }

    function test_localeChange() {
        field.value = 22 * 60;
        const time = new Date(2000, 0, 15, 22, 0);
        field.locale = Qt.locale("en_US");
        compare(field.displayText, time.toLocaleTimeString(field.locale, Locale.ShortFormat));
        field.locale = Qt.locale("de_DE");
        compare(field.displayText, time.toLocaleTimeString(field.locale, Locale.ShortFormat));
        compare(field.value, 22 * 60);
        compare(modifiedSpy.count, 0);
    }

    function test_invalidInput() {
        field.value = 22 * 60;
        compare(field.valueFromText("not a time", field.locale), 22 * 60);
        compare(field.valueFromText("25:99", field.locale), 22 * 60);
    }

    function test_keyboardEdit() {
        field.contentItem.forceActiveFocus();
        keyClick(Qt.Key_A, Qt.ControlModifier);
        keyClick("2");
        keyClick("2");
        keyClick(":");
        keyClick("3");
        keyClick("0");
        keyClick(Qt.Key_Return);
        compare(field.value, 22 * 60 + 30);
        compare(modifiedSpy.count, 1);
    }

    function test_wrapAtMidnight() {
        field.value = 1439;
        field.increase();
        compare(field.value, 0);
        field.decrease();
        compare(field.value, 1439);
    }
}
