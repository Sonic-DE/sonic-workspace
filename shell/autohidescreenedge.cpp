/*
    SPDX-FileCopyrightText: 2023 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <config-plasma.h>

#include "autohidescreenedge.h"
#include "debug.h"
#include "panelview.h"

#include <KWindowSystem>

#include <QDebug>
#include <QWindow>
#include <qpa/qplatformwindow_p.h>

#include <xcb/xcb.h>

class X11AutoHideScreenEdge : public AutoHideScreenEdge
{
    Q_OBJECT

public:
    X11AutoHideScreenEdge(PanelView *view);
    ~X11AutoHideScreenEdge() override;

    void deactivate() override;
    void activate() override;

private:
    xcb_atom_t m_atom = XCB_ATOM_NONE;
};

X11AutoHideScreenEdge::X11AutoHideScreenEdge(PanelView *view)
    : AutoHideScreenEdge(view)
{
    xcb_connection_t *connection = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection();

    constexpr QByteArrayView atomName("_KDE_NET_WM_SCREEN_EDGE_SHOW");
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom_unchecked(connection, false, atomName.length(), atomName.constData());
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, nullptr);
    if (reply) {
        m_atom = reply->atom;
        free(reply);
    }
}

X11AutoHideScreenEdge::~X11AutoHideScreenEdge()
{
    if (!m_view) {
        return;
    }
    deactivate();
}

void X11AutoHideScreenEdge::deactivate()
{
    if (m_atom != XCB_ATOM_NONE) {
        xcb_delete_property(qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection(), m_view->winId(), m_atom);
    }
}

void X11AutoHideScreenEdge::activate()
{
    if (m_atom == XCB_ATOM_NONE) {
        return;
    }

    uint32_t value = 0;

    switch (m_view->location()) {
    case Plasma::Types::TopEdge:
        value = 0;
        break;
    case Plasma::Types::RightEdge:
        value = 1;
        break;
    case Plasma::Types::BottomEdge:
        value = 2;
        break;
    case Plasma::Types::LeftEdge:
        value = 3;
        break;
    case Plasma::Types::Floating:
    default:
        value = 4;
        break;
    }

    xcb_change_property(qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection(),
                        XCB_PROP_MODE_REPLACE,
                        m_view->winId(),
                        m_atom,
                        XCB_ATOM_CARDINAL,
                        32,
                        1,
                        &value);
}

AutoHideScreenEdge::AutoHideScreenEdge(PanelView *view)
    : QObject(view)
    , m_view(view)
{
}

AutoHideScreenEdge *AutoHideScreenEdge::create(PanelView *view)
{
        return new X11AutoHideScreenEdge(view);
}

#include "autohidescreenedge.moc"

#include "moc_autohidescreenedge.cpp"
