/*
    localegeneratorubuntu.cpp
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QStandardPaths>

#include <KLocalizedString>

#include "kcm_regionandlang_debug.h"
#include "localegeneratorubuntu.h"

void LocaleGeneratorUbuntu::localesGenerate(const QStringList &list)
{
    ubuntuInstall(list);
}

void LocaleGeneratorUbuntu::ubuntuInstall(const QStringList &locales)
{
    m_packageIDs.clear();
    if (!m_proc) {
        m_proc = new QProcess(this);
    }
    QStringList args;
    args.reserve(locales.size());
    for (const auto &locale : locales) {
        // fallback Locale C does not have language support packages
        if (locale == QStringLiteral("C")) {
            continue;
        }
        auto localeStripped = locale;
        localeStripped.remove(QStringLiteral(".UTF-8"));
        args.append(QStringLiteral("--language=%1").arg(localeStripped));
    }
    const QString binaryPath = QStandardPaths::findExecutable(QStringLiteral("check-language-support"));
    if (!binaryPath.isEmpty()) {
        m_proc->setProgram(binaryPath);
        m_proc->setArguments(args);
        connect(m_proc, &QProcess::finished, this, &LocaleGeneratorUbuntu::ubuntuLangCheck);
        m_proc->start();
    } else {
        Q_EMIT userHasToGenerateManually(i18nc("@info:warning", "Can't locate executable `check-language-support`"));
    }
}

void LocaleGeneratorUbuntu::ubuntuLangCheck(int statusCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    if (statusCode != 0) {
        // Something wrong with this Ubuntu, don't try further
        Q_EMIT userHasToGenerateManually(i18nc("the arg is the output of failed check-language-support call",
                                               "check-language-support failed, output: %1 %2",
                                               QString::fromUtf8(m_proc->readAllStandardOutput()),
                                               QString::fromUtf8(m_proc->readAllStandardError())));
        return;
    }
    const QString output = QString::fromUtf8(m_proc->readAllStandardOutput().simplified());
    QStringList packages = output.split(QLatin1Char(' '));
    packages.erase(std::remove_if(packages.begin(),
                                  packages.end(),
                                  [](QString &i) {
                                      return i.isEmpty();
                                  }),
                   packages.end());

    if (packages.isEmpty()) {
        Q_EMIT success();
    }
}

#include "moc_localegeneratorubuntu.cpp"
