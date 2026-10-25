/*
    SPDX-FileCopyrightText: 2026 SonicDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "legacyshellmigration.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <cstdio>
#include <unistd.h>

using namespace SonicShellMigration;

class LegacyShellMigrationTest : public QObject {
    Q_OBJECT
private:
    static Paths paths(const QString &root)
    {
        return {root + QStringLiteral("/config"), root + QStringLiteral("/state"), root + QStringLiteral("/data")};
    }

    static bool write(const QString &path, const QByteArray &content)
    {
        if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
            return false;
        }
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(content) == content.size();
    }

    static QByteArray read(const QString &path)
    {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

    static bool seed(const Paths &p)
    {
        return write(p.config + QStringLiteral("/plasmashellrc"), "[PlasmaViews][Panel 7]\nheight=42\n[ScreenConnectors]\n0=DP-1\n")
            && write(p.config + QStringLiteral("/plasma-org.kde.plasma.desktop-appletsrc"), "[Containments][7]\nplugin=org.kde.panel\nlastScreen=0\n")
            && write(p.state + QStringLiteral("/plasmashellstaterc"), "[General]\nSystemInstallDate=2020-01-01\n")
            && write(p.config + QStringLiteral("/kglobalshortcutsrc"), "[other-app]\ncustom=Meta+X,none,Other\n[plasmashell]\ncustom=Meta+Y,none,Shell\n");
    }

private Q_SLOTS:
    void freshProfile()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto p = paths(directory.path());
        QString error;
        QVERIFY2(importSettings(p, QStringLiteral("org.sonicde.desktop"), &error), qPrintable(error));
        QVERIFY(QFileInfo::exists(p.state + QStringLiteral("/sonicshellstaterc")));
        QVERIFY(!QFileInfo::exists(p.config + QStringLiteral("/sonicshellrc")));
        QVERIFY(!QFileInfo::exists(p.config + QStringLiteral("/") + layoutFile(QStringLiteral("org.sonicde.desktop"))));
        QCOMPARE(QDir(p.state).entryList({QStringLiteral(".sonicshell-import-*.json")}, QDir::Files | QDir::Hidden).size(), 1);
    }

    void importsWithoutMovingOrChangingOriginals()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(seed(p));
        const auto source = read(p.config + QStringLiteral("/plasmashellrc"));
        const auto layout = read(p.config + QStringLiteral("/plasma-org.kde.plasma.desktop-appletsrc"));
        const auto state = read(p.state + QStringLiteral("/plasmashellstaterc"));
        const auto shortcuts = read(p.config + QStringLiteral("/kglobalshortcutsrc"));
        QVERIFY(QFile::setPermissions(p.config + QStringLiteral("/plasmashellrc"), QFileDevice::ReadOwner | QFileDevice::WriteOwner));
        QString error;
        QVERIFY2(importSettings(p, QStringLiteral("org.sonicde.desktop"), &error), qPrintable(error));
        QCOMPARE(read(p.config + QStringLiteral("/sonicshellrc")), source);
        QCOMPARE(read(p.config + QStringLiteral("/") + layoutFile(QStringLiteral("org.sonicde.desktop"))), layout);
        QCOMPARE(read(p.state + QStringLiteral("/sonicshellstaterc")), state);
        QCOMPARE(read(p.config + QStringLiteral("/plasmashellrc")), source);
        QCOMPARE(read(p.config + QStringLiteral("/plasma-org.kde.plasma.desktop-appletsrc")), layout);
        QCOMPARE(read(p.state + QStringLiteral("/plasmashellstaterc")), state);
        const auto updated = read(p.config + QStringLiteral("/kglobalshortcutsrc"));
        QVERIFY(updated.startsWith(shortcuts));
        QVERIFY(updated.contains("[sonicshell]\ncustom=Meta+Y,none,Shell"));
        QVERIFY(!(QFile::permissions(p.config + QStringLiteral("/sonicshellrc")) & QFileDevice::ReadOther));
        QCOMPARE(QFile::permissions(p.config + QStringLiteral("/sonicshellrc")), QFile::permissions(p.config + QStringLiteral("/plasmashellrc")));
        QCOMPARE(QFileInfo(p.config + QStringLiteral("/sonicshellrc")).ownerId(), QFileInfo(p.config + QStringLiteral("/plasmashellrc")).ownerId());
        QCOMPARE(QFileInfo(p.config + QStringLiteral("/sonicshellrc")).groupId(), QFileInfo(p.config + QStringLiteral("/plasmashellrc")).groupId());
        QVERIFY2(importSettings(p, QStringLiteral("org.sonicde.desktop"), &error), qPrintable(error));
        QCOMPARE(read(p.config + QStringLiteral("/kglobalshortcutsrc")), updated);
    }

    void preservesExistingDestinations()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(seed(p));
        QVERIFY(write(p.config + QStringLiteral("/sonicshellrc"), "[New]\ncustom=keep\n"));
        QVERIFY(write(p.state + QStringLiteral("/sonicshellstaterc"), "[New]\ncustom=keep\n"));
        QVERIFY(write(p.config + QStringLiteral("/") + layoutFile(QStringLiteral("org.sonicde.desktop")), "[New]\ncustom=keep\n"));
        QVERIFY(write(p.config + QStringLiteral("/kglobalshortcutsrc"), "[sonicshell]\ncustom=Meta+Z\n[plasmashell]\ncustom=Meta+Y\n"));
        QString error;
        QVERIFY2(importSettings(p, QStringLiteral("org.sonicde.desktop"), &error), qPrintable(error));
        QCOMPARE(read(p.config + QStringLiteral("/sonicshellrc")), QByteArray("[New]\ncustom=keep\n"));
        QCOMPARE(read(p.state + QStringLiteral("/sonicshellstaterc")), QByteArray("[New]\ncustom=keep\n"));
        QCOMPARE(read(p.config + QStringLiteral("/") + layoutFile(QStringLiteral("org.sonicde.desktop"))), QByteArray("[New]\ncustom=keep\n"));
        QCOMPARE(read(p.config + QStringLiteral("/kglobalshortcutsrc")), QByteArray("[sonicshell]\ncustom=Meta+Z\n[plasmashell]\ncustom=Meta+Y\n"));
    }

    void importsQuicklaunchIconsAndApplicationData()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        const QString oldData = p.data + QStringLiteral("/plasmashell");
        const QString newData = p.data + QStringLiteral("/sonicshell");
        const QString launcher = QStringLiteral("/quicklaunch/custom launcher.desktop");
        const QByteArray desktop("[Desktop Entry]\nType=Link\nName=Custom\nURL=https://example.org\n");
        const QByteArray image("\x89PNG\0binary", 11);
        QVERIFY(write(oldData + launcher, desktop));
        QVERIFY(write(oldData + QStringLiteral("/icons/custom.png"), image));
        QVERIFY(write(oldData + QStringLiteral("/.custom-data"), "keep application data"));
        const QByteArray layout = "[Containments][7][Applets][8][Configuration][General]\nlaunchers=" + QUrl::fromLocalFile(oldData + launcher).toEncoded()
            + "\nicon=" + (oldData + QStringLiteral("/icons/custom.png")).toUtf8() + '\n';
        QVERIFY(write(p.config + QStringLiteral("/plasma-org.kde.plasma.desktop-appletsrc"), layout));
        const auto permissions = QFile::permissions(oldData + launcher);
        const auto owner = QFileInfo(oldData + launcher).ownerId();
        const auto group = QFileInfo(oldData + launcher).groupId();
        QString error;
        QVERIFY2(importSettings(p, QStringLiteral("org.sonicde.desktop"), &error), qPrintable(error));
        QCOMPARE(read(newData + launcher), desktop);
        QCOMPARE(QFile::permissions(newData + launcher), permissions);
        QCOMPARE(QFileInfo(newData + launcher).ownerId(), owner);
        QCOMPARE(QFileInfo(newData + launcher).groupId(), group);
        QCOMPARE(read(newData + QStringLiteral("/icons/custom.png")), image);
        QCOMPARE(read(newData + QStringLiteral("/.custom-data")), QByteArray("keep application data"));
        const auto imported = read(p.config + QStringLiteral("/sonic-org.sonicde.desktop-appletsrc"));
        QVERIFY(imported.contains(QUrl::fromLocalFile(newData + launcher).toEncoded()));
        QVERIFY(imported.contains((newData + QStringLiteral("/icons/custom.png")).toUtf8()));
        QCOMPARE(read(p.config + QStringLiteral("/plasma-org.kde.plasma.desktop-appletsrc")), layout);
        QCOMPARE(read(oldData + launcher), desktop);
        QVERIFY(QFile::remove(newData + launcher));
        QVERIFY2(importSettings(p, QStringLiteral("example.custom"), &error), qPrintable(error));
        QVERIFY(!QFileInfo::exists(newData + launcher));
    }

    void dataMergePreservesExistingFilesAndSymlinks()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        const QString oldData = p.data + QStringLiteral("/plasmashell");
        const QString newData = p.data + QStringLiteral("/sonicshell");
        QVERIFY(write(oldData + QStringLiteral("/quicklaunch/existing.desktop"), "legacy"));
        QVERIFY(write(oldData + QStringLiteral("/quicklaunch/absent.desktop"), "imported"));
        QVERIFY(write(oldData + QStringLiteral("/icons/custom.png"), "legacy image"));
        QVERIFY(write(newData + QStringLiteral("/quicklaunch/existing.desktop"), "destination wins"));
        const QString iconTarget = directory.path() + QStringLiteral("/user-icons");
        QVERIFY(QDir().mkpath(iconTarget));
        QVERIFY(QFile::link(iconTarget, newData + QStringLiteral("/icons")));
        const auto permissions = QFile::permissions(newData + QStringLiteral("/quicklaunch/existing.desktop"));
        QString error;
        QVERIFY2(importSettings(p, QStringLiteral("org.sonicde.desktop"), &error), qPrintable(error));
        QCOMPARE(read(newData + QStringLiteral("/quicklaunch/existing.desktop")), QByteArray("destination wins"));
        QCOMPARE(QFile::permissions(newData + QStringLiteral("/quicklaunch/existing.desktop")), permissions);
        QCOMPARE(read(newData + QStringLiteral("/quicklaunch/absent.desktop")), QByteArray("imported"));
        QVERIFY(QFileInfo(newData + QStringLiteral("/icons")).isSymLink());
        QVERIFY(!QFileInfo::exists(iconTarget + QStringLiteral("/custom.png")));
        QCOMPARE(read(oldData + QStringLiteral("/quicklaunch/existing.desktop")), QByteArray("legacy"));
    }

    void validatesAllInputsBeforePublication()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(seed(p));
        QVERIFY(write(p.config + QStringLiteral("/plasma-org.kde.plasma.desktop-appletsrc"), "not an ini configuration"));
        QString error;
        QVERIFY(!importSettings(p, QStringLiteral("org.sonicde.desktop"), &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(!QFileInfo::exists(p.config + QStringLiteral("/sonicshellrc")));
        QVERIFY(!QFileInfo::exists(p.state + QStringLiteral("/sonicshellstaterc")));
        QCOMPARE(QDir(p.state).entryList({QStringLiteral(".sonicshell-import-*.json")}, QDir::Files | QDir::Hidden).size(), 0);
        QVERIFY(write(p.config + QStringLiteral("/plasma-org.kde.plasma.desktop-appletsrc"), "[Containments][7]\nlastScreen=0\n"));
        QVERIFY2(importSettings(p, QStringLiteral("org.sonicde.desktop"), &error), qPrintable(error));
    }

    void resumesPartialImport()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(seed(p));
        QVERIFY(write(p.config + QStringLiteral("/sonicshellrc"), "[Existing]\ncustom=keep\n"));
        QString error;
        QVERIFY2(importSettings(p, QStringLiteral("org.sonicde.desktop"), &error), qPrintable(error));
        QCOMPARE(read(p.config + QStringLiteral("/sonicshellrc")), QByteArray("[Existing]\ncustom=keep\n"));
        QVERIFY(QFileInfo::exists(p.config + QStringLiteral("/") + layoutFile(QStringLiteral("org.sonicde.desktop"))));
        // Once completed, intentionally deleted new settings are not reimported.
        QVERIFY(QFile::remove(p.config + QStringLiteral("/sonicshellrc")));
        QVERIFY2(importSettings(p, QStringLiteral("org.sonicde.desktop"), &error), qPrintable(error));
        QVERIFY(!QFileInfo::exists(p.config + QStringLiteral("/sonicshellrc")));
        QVERIFY2(importSettings(p, QStringLiteral("example.other-shell"), &error), qPrintable(error));
        QVERIFY(!QFileInfo::exists(p.config + QStringLiteral("/sonicshellrc")));
    }

    void importsStateBeforeKdedWrites()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        const auto legacy = p.data + QStringLiteral("/plasmashell/plasmashellstaterc");
        QVERIFY(write(legacy, "[Old]\nvalue=legacy\n"));
        QString error;
        QVERIFY2(importState(p, &error), qPrintable(error));
        QCOMPARE(read(p.state + QStringLiteral("/sonicshellstaterc")), QByteArray("[Old]\nvalue=legacy\n"));
        QCOMPARE(read(legacy), QByteArray("[Old]\nvalue=legacy\n"));
        QVERIFY(write(p.state + QStringLiteral("/sonicshellstaterc"), "[Donation]\ncustom=keep\n"));
        QVERIFY2(importSettings(p, QStringLiteral("org.sonicde.desktop"), &error), qPrintable(error));
        QCOMPARE(read(p.state + QStringLiteral("/sonicshellstaterc")), QByteArray("[Donation]\ncustom=keep\n"));
    }

    void prefersCanonicalState()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(write(p.data + QStringLiteral("/plasmashell/plasmashellstaterc"), "[Old]\nvalue=data\n"));
        QVERIFY(write(p.state + QStringLiteral("/plasmashellstaterc"), "[Old]\nvalue=state\n"));
        QString error;
        QVERIFY2(importState(p, &error), qPrintable(error));
        QCOMPARE(read(p.state + QStringLiteral("/sonicshellstaterc")), QByteArray("[Old]\nvalue=state\n"));
        QVERIFY(QFileInfo::exists(p.data + QStringLiteral("/plasmashell/plasmashellstaterc")));
        QVERIFY(QFileInfo::exists(p.state + QStringLiteral("/plasmashellstaterc")));
    }

    void customShellUsesExplicitIdentityMapping()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(write(p.config + QStringLiteral("/plasma-example.custom-appletsrc"), "[Panel]\nvalue=custom\n"));
        QString error;
        QVERIFY2(importSettings(p, QStringLiteral("example.custom"), &error), qPrintable(error));
        QCOMPARE(read(p.config + QStringLiteral("/sonic-example.custom-appletsrc")), QByteArray("[Panel]\nvalue=custom\n"));
        QVERIFY(!importSettings(p, QStringLiteral("../escape"), &error));
        QVERIFY(!importSettings(p, QStringLiteral("org.kde.plasma.desktop"), &error));
    }

    void remapsOnlyImportedDefaultPackage()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(seed(p));
        const QByteArray original("[Unrelated]\nShellPackage=org.kde.plasma.desktop\n[Shell]\nShellPackage=org.kde.plasma.desktop\n");
        QVERIFY(write(p.config + QStringLiteral("/plasmashellrc"), original));
        QString error;
        QVERIFY2(importSettings(p, {}, &error), qPrintable(error));
        QCOMPARE(read(p.config + QStringLiteral("/sonicshellrc")),
                 QByteArray("[Unrelated]\nShellPackage=org.kde.plasma.desktop\n[Shell]\nShellPackage=org.sonicde.desktop\n"));
        QCOMPARE(read(p.config + QStringLiteral("/plasmashellrc")), original);
        QVERIFY(QFileInfo::exists(p.config + QStringLiteral("/sonic-org.sonicde.desktop-appletsrc")));
    }

    void honorsConfiguredCustomShell()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(write(p.config + QStringLiteral("/plasmashellrc"), "[Shell]\nShellPackage=example.custom\n"));
        QVERIFY(write(p.config + QStringLiteral("/plasma-example.custom-appletsrc"), "[Panel]\nvalue=custom\n"));
        QString error;
        QVERIFY2(importSettings(p, {}, &error), qPrintable(error));
        QCOMPARE(read(p.config + QStringLiteral("/sonicshellrc")), QByteArray("[Shell]\nShellPackage=example.custom\n"));
        QCOMPARE(read(p.config + QStringLiteral("/sonic-example.custom-appletsrc")), QByteArray("[Panel]\nvalue=custom\n"));
        QVERIFY(!QFileInfo::exists(p.config + QStringLiteral("/sonic-org.sonicde.desktop-appletsrc")));
    }

    void preservesKConfigDeletionDirectives()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        const QByteArray source("[Shell]\nOldSetting[$d]\nShellPackage=example.custom\n");
        QVERIFY(write(p.config + QStringLiteral("/plasmashellrc"), source));
        QString error;
        QVERIFY2(importSettings(p, {}, &error), qPrintable(error));
        QCOMPARE(read(p.config + QStringLiteral("/sonicshellrc")), source);
        QCOMPARE(read(p.config + QStringLiteral("/plasmashellrc")), source);
    }

    void leavesNestedShellSettingsUntouched()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        const QByteArray source("[Shell][$i]\nShellPackage=org.kde.plasma.desktop\n[Shell][Unrelated]\nShellPackage=org.kde.plasma.desktop\n");
        QVERIFY(write(p.config + QStringLiteral("/plasmashellrc"), source));
        QString error;
        QVERIFY2(importSettings(p, {}, &error), qPrintable(error));
        QCOMPARE(read(p.config + QStringLiteral("/sonicshellrc")),
                 QByteArray("[Shell][$i]\nShellPackage=org.sonicde.desktop\n[Shell][Unrelated]\nShellPackage=org.kde.plasma.desktop\n"));
        QCOMPARE(read(p.config + QStringLiteral("/plasmashellrc")), source);
    }

    void preservesUtf8BomAndShortcutGroups()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        const QByteArray source("\xef\xbb\xbf[Shell]\nShellPackage=org.kde.plasma.desktop\n");
        const QByteArray shortcuts("\xef\xbb\xbf[plasmashell]\ncustom=Meta+Y\n[plasmashell][Extra]\nkeep=yes\n");
        QVERIFY(write(p.config + QStringLiteral("/plasmashellrc"), source));
        QVERIFY(write(p.config + QStringLiteral("/kglobalshortcutsrc"), shortcuts));
        QString error;
        QVERIFY2(importSettings(p, {}, &error), qPrintable(error));
        QCOMPARE(read(p.config + QStringLiteral("/sonicshellrc")), QByteArray("\xef\xbb\xbf[Shell]\nShellPackage=org.sonicde.desktop\n"));
        const auto result = read(p.config + QStringLiteral("/kglobalshortcutsrc"));
        QVERIFY(result.startsWith(shortcuts));
        QVERIFY(result.contains("[sonicshell]\ncustom=Meta+Y"));
        QVERIFY(result.contains("[sonicshell][Extra]\nkeep=yes"));
        QCOMPARE(read(p.config + QStringLiteral("/plasmashellrc")), source);
    }

    void unreadableInputDoesNotPublishDefaults()
    {
        if (::geteuid() == 0) {
            QSKIP("Root bypasses file read permissions");
        }
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(seed(p));
        const QString original = p.config + QStringLiteral("/plasmashellrc");
        const QByteArray contents = read(original);
        QVERIFY(QFile::setPermissions(original, {}));
        QString error;
        const bool imported = importSettings(p, QStringLiteral("org.sonicde.desktop"), &error);
        QVERIFY(QFile::setPermissions(original, QFileDevice::ReadOwner | QFileDevice::WriteOwner));
        QVERIFY(!imported);
        QVERIFY(!QFileInfo::exists(p.config + QStringLiteral("/sonicshellrc")));
        QVERIFY(!QFileInfo::exists(p.state + QStringLiteral("/sonicshellstaterc")));
        QCOMPARE(read(original), contents);
    }

    void preservesExistingDestinationSymlink()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(seed(p));
        const QString destination = p.config + QStringLiteral("/sonicshellrc");
        const QString target = directory.path() + QStringLiteral("/explicit-new-settings");
        QVERIFY(write(target, "[Custom]\nkeep=yes\n"));
        QVERIFY(QFile::link(target, destination));
        QString error;
        QVERIFY2(importSettings(p, QStringLiteral("org.sonicde.desktop"), &error), qPrintable(error));
        QVERIFY(QFileInfo(destination).isSymLink());
        QCOMPARE(read(target), QByteArray("[Custom]\nkeep=yes\n"));
    }

    void invalidUtf8FailsBeforePublication()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        const QByteArray original("[General]\nvalue=\xff\n");
        QVERIFY(write(p.config + QStringLiteral("/plasmashellrc"), original));
        QString error;
        QVERIFY(!importSettings(p, QStringLiteral("org.sonicde.desktop"), &error));
        QVERIFY(!QFileInfo::exists(p.config + QStringLiteral("/sonicshellrc")));
        QVERIFY(!QFileInfo::exists(p.state + QStringLiteral("/sonicshellstaterc")));
        QCOMPARE(read(p.config + QStringLiteral("/plasmashellrc")), original);
    }

    void respectsMigrationAndConfigLocks()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(seed(p));
        QString error;
        QLockFile lock(p.state + QStringLiteral("/.sonicshell-import.lock"));
        QVERIFY(lock.tryLock(0));
        QVERIFY(!importSettings(p, QStringLiteral("org.sonicde.desktop"), &error, 0));
        QVERIFY(!QFileInfo::exists(p.config + QStringLiteral("/sonicshellrc")));
        lock.unlock();
        QLockFile shortcuts(p.config + QStringLiteral("/kglobalshortcutsrc.lock"));
        QVERIFY(shortcuts.tryLock(0));
        QVERIFY(!importSettings(p, QStringLiteral("org.sonicde.desktop"), &error, 0));
        QVERIFY(!QFileInfo::exists(p.config + QStringLiteral("/sonicshellrc")));
    }

    void destinationFailureDoesNotInitializeDefaults()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(seed(p));
        QVERIFY(QDir().mkdir(p.config + QStringLiteral("/sonicshellrc")));
        QString error;
        QVERIFY(!importSettings(p, QStringLiteral("org.sonicde.desktop"), &error));
        QVERIFY(!QFileInfo::exists(p.state + QStringLiteral("/sonicshellstaterc")));
        QVERIFY(QFileInfo(p.config + QStringLiteral("/sonicshellrc")).isDir());
    }

    void corruptCompletionRecordFailsClosed()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(seed(p));
        QVERIFY(write(p.state + QStringLiteral("/.sonicshell-base-import.json"), "incomplete record"));
        QString error;
        QVERIFY(!importSettings(p, QStringLiteral("org.sonicde.desktop"), &error));
        QVERIFY(!importState(p, &error));
        QVERIFY(!QFileInfo::exists(p.config + QStringLiteral("/sonicshellrc")));
    }

    void concurrentStartsBeforeQApplication()
    {
        QTemporaryDir directory;
        const auto p = paths(directory.path());
        QVERIFY(seed(p));
        QProcess first;
        QProcess second;
        const QStringList arguments{QStringLiteral("--migration-child"), directory.path()};
        first.start(QCoreApplication::applicationFilePath(), arguments);
        second.start(QCoreApplication::applicationFilePath(), arguments);
        QVERIFY(first.waitForFinished(15000));
        QVERIFY(second.waitForFinished(15000));
        QCOMPARE(first.exitCode(), 0);
        QCOMPARE(second.exitCode(), 0);
        QCOMPARE(read(p.config + QStringLiteral("/kglobalshortcutsrc")).count("[sonicshell]"), 1);
    }
};

int main(int argc, char **argv)
{
    if (argc == 3 && QByteArray(argv[1]) == "--migration-child") {
        const QString root = QString::fromLocal8Bit(argv[2]);
        qputenv("XDG_CONFIG_HOME", (root + QStringLiteral("/config")).toLocal8Bit());
        qputenv("XDG_STATE_HOME", (root + QStringLiteral("/state")).toLocal8Bit());
        qputenv("XDG_DATA_HOME", (root + QStringLiteral("/data")).toLocal8Bit());
        QString error;
        const bool imported = importSettings(userPaths(), QStringLiteral("org.sonicde.desktop"), &error);
        if (!imported) {
            std::fprintf(stderr, "%s\n", qPrintable(error));
        }
        return imported ? 0 : 1;
    }
    QCoreApplication application(argc, argv);
    LegacyShellMigrationTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "legacyshellmigrationtest.moc"
