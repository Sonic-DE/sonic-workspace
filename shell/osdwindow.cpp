/*
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "osdwindow.h"
#include "config-X11.h"

#include <KWindowSystem>
#include <KX11Extras>

OsdWindow::OsdWindow()
{
    setFlag(Qt::WindowDoesNotAcceptFocus, true);
    setFlag(Qt::WindowTransparentForInput, true);

    KX11Extras::setOnAllDesktops(winId(), true);
    KX11Extras::setType(winId(), NET::OnScreenDisplay);
}
