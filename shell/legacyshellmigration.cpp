/*
    SPDX-FileCopyrightText: 2026 SonicDE contributors

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "legacyshellmigration.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QUrl>

#include <cerrno>
#include <cstring>
#include <unistd.h>

namespace SonicShellMigration {
namespace {
bool fail(QString *error, const QString &message)
{
    if (error) {
        *error = message;
    }
    return false;
}

bool exists(const QString &path)
{
    const QFileInfo info(path);
    return info.exists() || info.isSymLink();
}

bool prepare(const Paths &paths, QString *error)
{
    for (const auto &path : {paths.config, paths.state, paths.data}) {
        if (!QDir::isAbsolutePath(path)) {
            return fail(error, QStringLiteral("Migration requires absolute XDG paths: %1").arg(path));
        }
    }
    if (!QDir().mkpath(paths.config) || !QDir().mkpath(paths.state)) {
        return fail(error, QStringLiteral("Cannot create Sonic Shell configuration/state directories"));
    }
    return true;
}

bool readConfig(const QString &path, QByteArray *data, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return fail(error, QStringLiteral("Cannot read %1: %2").arg(path, file.errorString()));
    }
    *data = file.readAll();
    if (file.error() != QFile::NoError) {
        return fail(error, QStringLiteral("Cannot read complete settings from %1").arg(path));
    }
    QByteArray text = *data;
    if (text.startsWith("\xef\xbb\xbf")) {
        text.remove(0, 3);
    }
    if (text.contains('\0') || QString::fromUtf8(text).toUtf8() != text) {
        return fail(error, QStringLiteral("Invalid UTF-8 configuration: %1").arg(path));
    }
    for (const auto &raw : text.split('\n')) {
        const auto line = raw.trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';')) {
            continue;
        }
        const auto option = line.lastIndexOf("[$");
        const bool deletion = option >= 0 && line.endsWith(']') && line.mid(option + 2).contains('d');
        if (line.startsWith('[') ? !line.endsWith(']') : (line.startsWith('=') || (!line.contains('=') && !deletion))) {
            return fail(error, QStringLiteral("Malformed configuration: %1").arg(path));
        }
    }
    return true;
}

struct Copy {
    QString destination;
    QByteArray content;
    QString source = {};
};

bool copyPreservingMetadata(const QString &source, const QString &destination, bool recursive, QString *error)
{
    QProcess process;
    // Let the copy operation retain metadata; do not apply permissions or
    // ownership separately. Existing user data is never a copy target.
    QStringList arguments{recursive ? QStringLiteral("-a") : QStringLiteral("--preserve=mode,ownership")};
    if (recursive) {
        arguments << QStringLiteral("--no-clobber");
    }
    arguments << QStringLiteral("--") << source << destination;
    process.start(QStringLiteral("cp"), arguments);
    if (!process.waitForStarted() || !process.waitForFinished(-1) || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return fail(error, QStringLiteral("Cannot copy %1 to %2: %3").arg(source, destination, QString::fromLocal8Bit(process.readAllStandardError())));
    }
    return true;
}

bool importData(const QString &source, const QString &destination, QString *error)
{
    if (!exists(source)) {
        return true;
    }
    if (exists(destination)) {
        // Merge only real directories. Existing files and symlinks always win.
        if (QFileInfo(source).isSymLink() || QFileInfo(destination).isSymLink() || !QFileInfo(source).isDir() || !QFileInfo(destination).isDir()) {
            return true;
        }
        const auto entries = QDir(source).entryList(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        for (const auto &entry : entries) {
            if (!importData(source + u'/' + entry, destination + u'/' + entry, error)) {
                return false;
            }
        }
        return true;
    }
    return copyPreservingMetadata(source, destination, true, error);
}

bool planCopy(const QString &source, const QString &destination, QList<Copy> *copies, QString *error, bool emptyIfMissing = false)
{
    if (exists(destination)) {
        if (!QFileInfo(destination).isFile()) {
            return fail(error, QStringLiteral("Existing destination is not a regular settings file: %1").arg(destination));
        }
        return true;
    }
    if (source.isEmpty() || !exists(source)) {
        if (emptyIfMissing) {
            copies->append({destination, {}});
        }
        return true;
    }
    Copy copy{destination, {}};
    if (!QFileInfo(source).isFile()) {
        return fail(error, QStringLiteral("Cannot import legacy settings: %1").arg(source));
    }
    if (!readConfig(source, &copy.content, error)) {
        return false;
    }
    copy.source = source;
    copies->append(copy);
    return true;
}

bool publish(const Copy &copy, QString *error)
{
    QTemporaryFile temporary(QFileInfo(copy.destination).absolutePath() + QStringLiteral("/.sonicshell-import-XXXXXX"));
    if (!temporary.open()) {
        return fail(error, QStringLiteral("Cannot stage imported settings for %1").arg(copy.destination));
    }
    if (!copy.source.isEmpty()) {
        temporary.close();
        if (!copyPreservingMetadata(copy.source, temporary.fileName(), false, error) || !temporary.open()) {
            return false;
        }
    }
    if (!temporary.resize(0) || temporary.write(copy.content) != copy.content.size() || !temporary.flush() || ::fsync(temporary.handle()) != 0) {
        return fail(error, QStringLiteral("Cannot stage imported settings for %1").arg(copy.destination));
    }
    // Same-directory hard-link publication is atomic and cannot replace a file
    // that appeared after preflight. QTemporaryFile removes only its own name.
    if (::link(QFile::encodeName(temporary.fileName()).constData(), QFile::encodeName(copy.destination).constData()) != 0) {
        const int errorNumber = errno;
        if (errorNumber != EEXIST || !QFileInfo(copy.destination).isFile()) {
            return fail(error, QStringLiteral("Cannot publish %1: %2").arg(copy.destination, QString::fromLocal8Bit(std::strerror(errorNumber))));
        }
    }
    return true;
}

QString oldState(const Paths &paths)
{
    const QString current = paths.state + QStringLiteral("/plasmashellstaterc");
    // Prefer the current XDG state location over the historical application-data
    // location. Never call openStateConfig on either legacy input: it can move it.
    return exists(current) ? current : paths.data + QStringLiteral("/plasmashell/plasmashellstaterc");
}

bool isGroup(const QByteArray &line, const QByteArray &name)
{
    QByteArray header = '[' + name + ']';
    if (line == header) {
        return true;
    }
    header.append('[');
    return line.startsWith(header);
}

bool completionRecord(const QString &path, const QString &shell, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return fail(error, QStringLiteral("Cannot read import completion record: %1").arg(path));
    }
    const auto object = QJsonDocument::fromJson(file.readAll()).object();
    return (object.value(QStringLiteral("version")).toInt() == 1 && (shell.isEmpty() || object.value(QStringLiteral("shell")).toString() == shell))
        || fail(error, QStringLiteral("Invalid import completion record: %1").arg(path));
}

bool isRootGroup(QByteArray line, const QByteArray &name)
{
    const QByteArray header = '[' + name + ']';
    if (!line.startsWith(header)) {
        return false;
    }
    line.remove(0, header.size());
    // KConfig metadata flags are not child groups. Do not rewrite values in a
    // nested [Shell][SomethingElse] group while migrating the root Shell group.
    while (!line.isEmpty()) {
        if (!line.startsWith("[$")) {
            return false;
        }
        const auto end = line.indexOf(']');
        if (end < 0) {
            return false;
        }
        line.remove(0, end + 1);
    }
    return true;
}

QByteArray shellPackageValue(const QByteArray &content)
{
    bool shell = false;
    QByteArray selected;
    QByteArray parsed = content;
    if (parsed.startsWith("\xef\xbb\xbf")) {
        parsed.remove(0, 3);
    }
    for (const auto &raw : parsed.split('\n')) {
        const auto line = raw.trimmed();
        if (line.startsWith('[')) {
            shell = isRootGroup(line, "Shell");
        } else if (shell) {
            const auto equals = line.indexOf('=');
            const auto key = line.left(equals).trimmed();
            if (equals >= 0 && (key == "ShellPackage" || key.startsWith("ShellPackage["))) {
                selected = line.mid(equals + 1).trimmed();
            }
        }
    }
    return selected;
}

QByteArray remapDefaultPackage(const QByteArray &content)
{
    QByteArray updated;
    bool shell = false;
    const auto lines = content.split('\n');
    for (qsizetype i = 0; i < lines.size(); ++i) {
        QByteArray raw = lines[i];
        QByteArray line = raw.trimmed();
        if (i == 0 && line.startsWith("\xef\xbb\xbf")) {
            line.remove(0, 3);
        }
        if (line.startsWith('[')) {
            shell = isRootGroup(line, "Shell");
        } else if (shell) {
            const auto equals = raw.indexOf('=');
            const auto key = raw.left(equals).trimmed();
            if (equals >= 0 && (key == "ShellPackage" || key.startsWith("ShellPackage[")) && raw.mid(equals + 1).trimmed() == "org.kde.plasma.desktop") {
                raw = raw.left(equals + 1) + "org.sonicde.desktop";
            }
        }
        updated += raw;
        if (i + 1 < lines.size()) {
            updated += '\n';
        }
    }
    return updated;
}

bool planShortcuts(const Paths &paths, QByteArray *updated, bool *changed, QString *error)
{
    const QString path = paths.config + QStringLiteral("/kglobalshortcutsrc");
    if (!exists(path)) {
        return true;
    }
    QByteArray original;
    if (!readConfig(path, &original, error)) {
        return false;
    }
    QByteArray parsed = original;
    if (parsed.startsWith("\xef\xbb\xbf")) {
        parsed.remove(0, 3);
    }
    const auto lines = parsed.split('\n');
    for (const auto &raw : lines) {
        if (isGroup(raw.trimmed(), "sonicshell")) {
            return true;
        }
    }
    QByteArray imported;
    bool inGroup = false;
    for (const auto &raw : lines) {
        const auto line = raw.trimmed();
        if (line.startsWith('[')) {
            inGroup = isGroup(line, "plasmashell");
            if (inGroup) {
                imported += "[sonicshell]" + line.mid(sizeof("[plasmashell]") - 1) + '\n';
            }
        } else if (inGroup) {
            imported += raw + '\n';
        }
    }
    if (!imported.isEmpty()) {
        *updated = original + (original.endsWith('\n') ? QByteArray() : QByteArray("\n")) + imported;
        *changed = true;
    }
    return true;
}
}

Paths userPaths()
{
    return {QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation),
            QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation),
            QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
}

QString layoutFile(const QString &shellPackage)
{
    return QStringLiteral("sonic-") + shellPackage + QStringLiteral("-appletsrc");
}

bool importState(const Paths &paths, QString *error, int lockTimeout)
{
    if (error) {
        error->clear();
    }
    if (!prepare(paths, error)) {
        return false;
    }
    QLockFile lock(paths.state + QStringLiteral("/.sonicshell-import.lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(lockTimeout)) {
        return fail(error, QStringLiteral("Another Sonic Shell settings import is active"));
    }
    const QString completed = paths.state + QStringLiteral("/.sonicshell-base-import.json");
    if (exists(completed)) {
        // Do not resurrect deliberately removed state after a completed import.
        return completionRecord(completed, {}, error);
    }
    QList<Copy> copies;
    if (!planCopy(oldState(paths), paths.state + QStringLiteral("/sonicshellstaterc"), &copies, error, true)) {
        return false;
    }
    for (const auto &copy : copies) {
        if (!publish(copy, error)) {
            return false;
        }
    }
    return true;
}

bool importSettings(const Paths &paths, const QString &requestedPackage, QString *error, int lockTimeout)
{
    if (error) {
        error->clear();
    }
    if (!prepare(paths, error)) {
        return false;
    }
    QLockFile lock(paths.state + QStringLiteral("/.sonicshell-import.lock"));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(lockTimeout)) {
        return fail(error, QStringLiteral("Another Sonic Shell settings import is active"));
    }
    const QString baseMarker = paths.state + QStringLiteral("/.sonicshell-base-import.json");
    const bool baseDone = exists(baseMarker);
    if (baseDone && !completionRecord(baseMarker, {}, error)) {
        return false;
    }
    QString shellPackage = requestedPackage;
    if (shellPackage.isEmpty()) {
        shellPackage = qEnvironmentVariable("SONIC_DEFAULT_SHELL", QStringLiteral("org.sonicde.desktop"));
        QString config = paths.config + QStringLiteral("/sonicshellrc");
        const bool newConfig = exists(config);
        if (!newConfig && !baseDone) {
            config = paths.config + QStringLiteral("/plasmashellrc");
        }
        if (exists(config)) {
            QByteArray contents;
            if (!readConfig(config, &contents, error)) {
                return false;
            }
            const auto selected = shellPackageValue(contents);
            if (!selected.isEmpty()) {
                shellPackage = !newConfig && selected == "org.kde.plasma.desktop" ? QStringLiteral("org.sonicde.desktop") : QString::fromUtf8(selected);
            }
        }
    }
    static const QRegularExpression validPackage(QStringLiteral("^[A-Za-z0-9_.-]+$"));
    if (!validPackage.match(shellPackage).hasMatch() || shellPackage == QStringLiteral("org.kde.plasma.desktop")) {
        return fail(error, QStringLiteral("Invalid or renamed shell package: %1").arg(shellPackage));
    }
    const QString marker = paths.state + QStringLiteral("/.sonicshell-import-")
        + QString::fromLatin1(QCryptographicHash::hash(shellPackage.toUtf8(), QCryptographicHash::Sha256).toHex()) + QStringLiteral(".json");
    if (exists(marker)) {
        return (baseDone || fail(error, QStringLiteral("Shell completion record exists without a completed base import")))
            && completionRecord(marker, shellPackage, error);
    }
    // Only the explicitly renamed default package changes ID. Custom/phone/TV
    // packages keep their selected IDs; no guessed org.kde -> org.sonicde mapping.
    const QString legacyPackage = shellPackage == QStringLiteral("org.sonicde.desktop") ? QStringLiteral("org.kde.plasma.desktop") : shellPackage;
    QList<Copy> copies;
    if (!baseDone
        && (!planCopy(paths.config + QStringLiteral("/plasmashellrc"), paths.config + QStringLiteral("/sonicshellrc"), &copies, error)
            || !planCopy(oldState(paths), paths.state + QStringLiteral("/sonicshellstaterc"), &copies, error, true))) {
        return false;
    }
    if (!planCopy(paths.config + QStringLiteral("/plasma-") + legacyPackage + QStringLiteral("-appletsrc"),
                  paths.config + QStringLiteral("/") + layoutFile(shellPackage),
                  &copies,
                  error)) {
        return false;
    }
    for (auto &copy : copies) {
        if (copy.destination == paths.config + QStringLiteral("/sonicshellrc")) {
            copy.content = remapDefaultPackage(copy.content);
        }
        // Quicklaunch and icon configuration can contain absolute local paths.
        // Redirect only imported references into this user's copied data tree.
        const QString oldData = paths.data + QStringLiteral("/plasmashell/");
        const QString newData = paths.data + QStringLiteral("/sonicshell/");
        copy.content.replace(QUrl::fromLocalFile(oldData).toEncoded(), QUrl::fromLocalFile(newData).toEncoded());
        copy.content.replace(oldData.toUtf8(), newData.toUtf8());
    }
    // This is the same lock used by KConfig's file backend. Read and append the
    // component while holding it, preserving every other group and old binding.
    QLockFile shortcutsLock(paths.config + QStringLiteral("/kglobalshortcutsrc.lock"));
    if (!shortcutsLock.tryLock(lockTimeout)) {
        return fail(error, QStringLiteral("Cannot lock shared shortcut configuration"));
    }
    QByteArray shortcuts;
    bool shortcutsChanged = false;
    if (!baseDone && !planShortcuts(paths, &shortcuts, &shortcutsChanged, error)) {
        return false;
    }
    // All legacy inputs have been validated before publishing any destination.
    // On interruption, already-published destinations are preserved on retry.
    if (!baseDone && exists(paths.data + QStringLiteral("/plasmashell"))) {
        if (!QDir().mkpath(paths.data) || !importData(paths.data + QStringLiteral("/plasmashell"), paths.data + QStringLiteral("/sonicshell"), error)) {
            return false;
        }
    }
    for (const auto &copy : copies) {
        if (!publish(copy, error)) {
            return false;
        }
    }
    if (shortcutsChanged) {
        QSaveFile file(paths.config + QStringLiteral("/kglobalshortcutsrc"));
        file.setDirectWriteFallback(false);
        if (!file.open(QIODevice::WriteOnly) || file.write(shortcuts) != shortcuts.size() || !file.commit()) {
            return fail(error, QStringLiteral("Cannot atomically import shell shortcuts"));
        }
    }
    if (!baseDone
        && (!publish({baseMarker, QJsonDocument(QJsonObject{{QStringLiteral("version"), 1}}).toJson()}, error) || !completionRecord(baseMarker, {}, error))) {
        return false;
    }
    const QByteArray completion = QJsonDocument(QJsonObject{{QStringLiteral("version"), 1}, {QStringLiteral("shell"), shellPackage}}).toJson();
    return publish({marker, completion}, error) && completionRecord(marker, shellPackage, error);
}
}
