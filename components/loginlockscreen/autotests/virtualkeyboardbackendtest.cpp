/*
 * SPDX-FileCopyrightText: 2026 SonicDE Community
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "virtualkeyboardcontroller.h"

#include <QDir>
#include <QGuiApplication>
#include <QInputMethod>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

class VirtualKeyboardBackendTest : public QObject {
    Q_OBJECT
    QQmlEngine m_engine;
    std::unique_ptr<VirtualKeyboardController> m_controller;
    std::unique_ptr<QObject> m_host;

private Q_SLOTS:
    void existingFooterContract()
    {
        if (qgetenv("SONIC_KEYBOARD_TEST_ISOLATED") != "1") {
            QSKIP("This input integration test requires a private Xvfb display");
        }
        m_controller = std::make_unique<VirtualKeyboardController>();
        QVERIFY2(m_controller->isAvailable(), "Qt-compatible Sonic keyboard input context was not found");
        QQmlComponent component(&m_engine);
        component.setData(
            "import QtQuick\nimport QtQuick.Window\nWindow { visible: true; width: 800; height: 400;"
            " TextInput { objectName: 'plain'; x: 20; y: 20; width: 700; height: 40; focus: true }"
            " TextInput { objectName: 'password'; x: 20; y: 80; width: 700; height: 40; echoMode: TextInput.Password;"
            " inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText } }",
            QUrl());
        m_host.reset(component.create());
        QVERIFY2(m_host, qPrintable(component.errorString()));
        auto *window = qobject_cast<QQuickWindow *>(m_host.get());
        auto *plain = m_host->findChild<QQuickItem *>(QStringLiteral("plain"));
        auto *password = m_host->findChild<QQuickItem *>(QStringLiteral("password"));
        QVERIFY(window);
        QVERIFY(plain);
        QVERIFY(password);
        window->requestActivate();
        QVERIFY(QTest::qWaitForWindowExposed(window));
        plain->forceActiveFocus();
        QTRY_COMPARE(QGuiApplication::focusObject(), plain);
        QTRY_VERIFY(!m_controller->isVisible());

        // These are the existing Footer.qml calls, with no new integration API.
        m_controller->forceActivate();
        QTRY_VERIFY(m_controller->isActive());
        QTRY_VERIFY(m_controller->isVisible());
        QQuickWindow *keyboard = nullptr;
        for (auto *candidate : QGuiApplication::topLevelWindows()) {
            if (candidate->objectName() == QLatin1String("sonicKeyboardWindow")) {
                keyboard = qobject_cast<QQuickWindow *>(candidate);
            }
        }
        QVERIFY(keyboard);
        QTRY_VERIFY(keyboard->isVisible());
        QVERIFY(keyboard->flags() & Qt::WindowDoesNotAcceptFocus);
        QVERIFY(keyboard->findChild<QQuickItem *>(QStringLiteral("sonicInputPanel")));
        QCOMPARE(QGuiApplication::focusWindow(), window);
        QCOMPARE(QGuiApplication::focusObject(), plain);

        password->forceActiveFocus();
        QTRY_COMPARE(QGuiApplication::focusObject(), password);
        m_controller->forceActivate();
        QTRY_VERIFY(m_controller->isVisible());
        for (const auto character : QStringLiteral("not-a-real-password")) {
            QTest::keyClick(window, static_cast<Qt::Key>(character.toUpper().unicode()));
        }
        QTRY_COMPARE(password->property("text").toString(), QStringLiteral("not-a-real-password"));
        QVERIFY(!password->property("displayText").toString().contains(QStringLiteral("password")));
        QCOMPARE(QGuiApplication::focusObject(), password);

        m_controller->setActive(false);
        QTRY_VERIFY(!m_controller->isVisible());
        QTRY_VERIFY(!keyboard->isVisible());
        QCOMPARE(QGuiApplication::focusObject(), password);
    }
};

int main(int argc, char **argv)
{
    QTemporaryDir home;
    if (!home.isValid()) {
        return 1;
    }
    qputenv("HOME", home.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", (home.path() + QStringLiteral("/config")).toUtf8());
    qputenv("XDG_CONFIG_DIRS", (home.path() + QStringLiteral("/empty-config")).toUtf8());
    qputenv("XDG_DATA_HOME", (home.path() + QStringLiteral("/data")).toUtf8());
    qputenv("XDG_CACHE_HOME", (home.path() + QStringLiteral("/cache")).toUtf8());
    qputenv("QT_QPA_PLATFORMTHEME", "generic");
    qputenv("QT_IM_MODULE", "sonicvirtualkeyboard");
    qunsetenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE");
    qputenv("QT_NO_XDG_DESKTOP_PORTAL", "1");
    qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/sonic-keyboard-footer-bus");
    qputenv("DBUS_SYSTEM_BUS_ADDRESS", "unix:path=/nonexistent/sonic-keyboard-footer-system-bus");
    QGuiApplication app(argc, argv);
    VirtualKeyboardBackendTest test;
    int result = 1;
    QTimer::singleShot(0, &app, [&] {
        result = QTest::qExec(&test, argc, argv);
        app.exit(result);
    });
    app.exec();
    return result;
}

#include "virtualkeyboardbackendtest.moc"
