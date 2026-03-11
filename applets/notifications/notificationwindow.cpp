/*
  SPDX-FileCopyrightText: 2024 David Edmundson <davidedmundson@kde.org>
  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "notificationwindow.h"

#include <KWindowSystem>
#include <KX11Extras>

NotificationWindow::NotificationWindow()
    : PlasmaQuick::PlasmaWindow()
{
    KX11Extras::setOnAllDesktops(winId(), true);
    KX11Extras::setType(winId(), NET::Notification);
}

NotificationWindow::~NotificationWindow() = default;

bool NotificationWindow::takeFocus() const
{
    return m_takeFocus;
}

void NotificationWindow::setTakeFocus(bool takeFocus)
{
    if (m_takeFocus == takeFocus) {
        return;
    }
    QWindow::setFlag(Qt::WindowDoesNotAcceptFocus, !takeFocus);

    m_takeFocus = takeFocus;
    Q_EMIT takeFocusChanged();
}

bool NotificationWindow::isCritical() const
{
    return m_critial;
}

void NotificationWindow::setIsCritical(bool critical)
{
    if (m_critial == critical) {
        return;
    }

    m_critial = critical;

    KX11Extras::setType(winId(), critical ? NET::CriticalNotification : NET::Notification);

    Q_EMIT isCriticalChanged();
}

bool NotificationWindow::event(QEvent *e)
{
    return PlasmaWindow::event(e);
}

void NotificationWindow::moveEvent(QMoveEvent *me)
{
    PlasmaWindow::moveEvent(me);
    update();
}

#include "moc_notificationwindow.cpp"
