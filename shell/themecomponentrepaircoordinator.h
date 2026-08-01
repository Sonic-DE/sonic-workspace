/*
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QObject>
#include <QPointer>

class KNotification;

class ThemeComponentRepairCoordinator : public QObject {
    Q_OBJECT

public:
    explicit ThemeComponentRepairCoordinator(QObject *parent = nullptr);
    void start();

private:
    void checkComponents(bool retry);
    void applyRepairs();

    QPointer<KNotification> m_notification;
};
