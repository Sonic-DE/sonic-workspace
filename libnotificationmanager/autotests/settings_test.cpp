/*
    SPDX-FileCopyrightText: 2026 SonicDE Contributors

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <QTemporaryDir>
#include <QTest>

#include <KConfigGroup>
#include <KSharedConfig>

#include "donotdisturbsettings.h"
#include "notification.h"
#include "server.h"
#include "settings.h"

using namespace NotificationManager;

class SettingsTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void timePeriodDefaults();
    void timePeriodInhibition_data();
    void timePeriodInhibition();
    void timePeriodPersistence();
    void timePeriodReload();
    void notificationReceivedBeforeDelivery();

private:
    void writeTimePeriod(bool enabled, int start, int end);
};

void SettingsTest::writeTimePeriod(bool enabled, int start, int end)
{
    const auto config = KSharedConfig::openConfig(QStringLiteral("plasmanotifyrc"));
    KConfigGroup group(config, QStringLiteral("DoNotDisturb"));
    group.writeEntry("DuringTimePeriod", enabled);
    group.writeEntry("TimePeriodStart", start);
    group.writeEntry("TimePeriodEnd", end);
    // Do not start window/screen trackers in this headless test.
    group.writeEntry("WhenFullscreen", false);
    group.writeEntry("WhenScreensMirrored", false);
    config->sync();
}

void SettingsTest::timePeriodDefaults()
{
    writeTimePeriod(false, 0, 0);
    Settings settings;
    settings.setLive(false);
    settings.defaults();
    settings.save();

    DoNotDisturbSettings defaults;
    QCOMPARE(defaults.duringTimePeriod(), false);
    QCOMPARE(defaults.timePeriodStart(), 22 * 60);
    QCOMPARE(defaults.timePeriodEnd(), 7 * 60);
    QVERIFY(!settings.notificationsInhibitedByTimePeriod(QDateTime(QDate(2026, 10, 18), QTime(23, 0))));
}

void SettingsTest::timePeriodInhibition_data()
{
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("end");
    QTest::addColumn<QTime>("now");
    QTest::addColumn<bool>("expected");

    QTest::newRow("disabled") << false << 9 * 60 << 17 * 60 << QTime(12, 0) << false;
    QTest::newRow("before-start") << true << 9 * 60 << 17 * 60 << QTime(8, 59, 59) << false;
    QTest::newRow("at-start") << true << 9 * 60 << 17 * 60 << QTime(9, 0) << true;
    QTest::newRow("daytime") << true << 9 * 60 << 17 * 60 << QTime(12, 0) << true;
    QTest::newRow("at-end") << true << 9 * 60 << 17 * 60 << QTime(17, 0) << true;
    QTest::newRow("end-minute") << true << 9 * 60 << 17 * 60 << QTime(17, 0, 59) << true;
    QTest::newRow("after-end") << true << 9 * 60 << 17 * 60 << QTime(17, 1) << false;
    QTest::newRow("overnight-before-start") << true << 22 * 60 << 7 * 60 << QTime(21, 59, 59) << false;
    QTest::newRow("overnight-at-start") << true << 22 * 60 << 7 * 60 << QTime(22, 0) << true;
    QTest::newRow("overnight-evening") << true << 22 * 60 << 7 * 60 << QTime(23, 30) << true;
    QTest::newRow("overnight-midnight") << true << 22 * 60 << 7 * 60 << QTime(0, 0) << true;
    QTest::newRow("overnight-before-end") << true << 22 * 60 << 7 * 60 << QTime(6, 59, 59) << true;
    QTest::newRow("overnight-at-end") << true << 22 * 60 << 7 * 60 << QTime(7, 0) << true;
    QTest::newRow("overnight-after-end") << true << 22 * 60 << 7 * 60 << QTime(7, 1) << false;
    QTest::newRow("overnight-daytime") << true << 22 * 60 << 7 * 60 << QTime(12, 0) << false;
    QTest::newRow("end-at-midnight") << true << 23 * 60 << 0 << QTime(23, 59) << true;
    QTest::newRow("end-at-midnight-inclusive") << true << 23 * 60 << 0 << QTime(0, 0) << true;
    QTest::newRow("start-at-midnight") << true << 0 << 60 << QTime(0, 0) << true;
    QTest::newRow("equal-times-match") << true << 600 << 600 << QTime(10, 0) << true;
    QTest::newRow("equal-times-outside") << true << 600 << 600 << QTime(12, 0) << false;
}

void SettingsTest::timePeriodInhibition()
{
    QFETCH(bool, enabled);
    QFETCH(int, start);
    QFETCH(int, end);
    QFETCH(QTime, now);
    QFETCH(bool, expected);

    writeTimePeriod(enabled, start, end);
    Settings settings;
    settings.setLive(false);
    settings.load();

    const QDate date(2026, 10, 18);
    const QDateTime dateTime(date, now);
    QCOMPARE(settings.notificationsInhibitedByTimePeriod(dateTime), expected);
    // Incoming timestamps must be evaluated in local wall-clock time, not UTC.
    QCOMPARE(settings.notificationsInhibitedByTimePeriod(dateTime.toUTC()), expected);
    QVERIFY(!settings.notificationsInhibitedByTimePeriod(QDateTime()));
}

void SettingsTest::timePeriodPersistence()
{
    writeTimePeriod(false, 22 * 60, 7 * 60);
    DoNotDisturbSettings config;
    config.setDuringTimePeriod(true);
    config.setTimePeriodStart(21 * 60 + 30);
    config.setTimePeriodEnd(8 * 60 + 15);
    QVERIFY(config.save());

    DoNotDisturbSettings restored;
    QVERIFY(restored.duringTimePeriod());
    QCOMPARE(restored.timePeriodStart(), 21 * 60 + 30);
    QCOMPARE(restored.timePeriodEnd(), 8 * 60 + 15);

    Settings settings;
    settings.setLive(false);
    QVERIFY(settings.notificationsInhibitedByTimePeriod(QDateTime(QDate(2026, 10, 18), QTime(21, 30))));

    restored.setDuringTimePeriod(false);
    QVERIFY(restored.save());
    settings.load();
    QVERIFY(!settings.notificationsInhibitedByTimePeriod(QDateTime(QDate(2026, 10, 18), QTime(21, 30))));
    QCOMPARE(restored.timePeriodStart(), 21 * 60 + 30);
    QCOMPARE(restored.timePeriodEnd(), 8 * 60 + 15);
}

void SettingsTest::timePeriodReload()
{
    const QDateTime now(QDate(2026, 10, 18), QTime(23, 0));
    writeTimePeriod(false, 22 * 60, 7 * 60);
    Settings settings;
    settings.setLive(false);
    QVERIFY(!settings.notificationsInhibitedByTimePeriod(now));

    writeTimePeriod(true, 22 * 60, 7 * 60);
    settings.load();
    QVERIFY(settings.notificationsInhibitedByTimePeriod(now));

    writeTimePeriod(true, 9 * 60, 17 * 60);
    settings.load();
    QVERIFY(!settings.notificationsInhibitedByTimePeriod(now));

    writeTimePeriod(true, 22 * 60, 7 * 60);
    settings.load();
    settings.defaults();
    QVERIFY(!settings.notificationsInhibitedByTimePeriod(now));
}

void SettingsTest::notificationReceivedBeforeDelivery()
{
    auto &server = Server::self();
    QStringList events;
    QObject context;
    connect(&server, &Server::notificationReceived, &context, [&events] {
        events.append(QStringLiteral("received"));
    });
    connect(&server, &Server::notificationAdded, &context, [&events] {
        events.append(QStringLiteral("added"));
    });
    connect(&server, &Server::notificationReplaced, &context, [&events] {
        events.append(QStringLiteral("replaced"));
    });
    Notification notification;
    server.add(notification);
    QCOMPARE(events, QStringList({QStringLiteral("received"), QStringLiteral("added")}));
    events.clear();
    server.add(notification);
    QCOMPARE(events, QStringList({QStringLiteral("received"), QStringLiteral("replaced")}));
}

int main(int argc, char **argv)
{
    QTemporaryDir configDir;
    if (!configDir.isValid()) {
        return 1;
    }
    qputenv("XDG_CONFIG_HOME", configDir.path().toUtf8());
    QCoreApplication app(argc, argv);
    SettingsTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "settings_test.moc"
