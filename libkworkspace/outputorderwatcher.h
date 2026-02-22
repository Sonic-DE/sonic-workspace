/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

#include "kworkspace_export.h"

class QScreen;
class QTimer;

/**
 * This class watches for output ordering changes from
 * the relevant backend
 */
class KWORKSPACE_EXPORT OutputOrderWatcher : public QObject
{
    Q_OBJECT
public:
    /**
     * Create the correct OutputOrderWatcher
     */
    static OutputOrderWatcher *instance(QObject *parent);

    /**
     * Returns the list of outputs in order
     *
     * At the time of change signalling all Screens are
     * guaranteed to exist in QGuiApplication.
     *
     * After screen removal this can temporarily contain
     * dead entries.
     */
    QStringList outputOrder() const;

    /**
     * @internal
     * For X11 we know libkscreen takes a server grab whilst changing properties.
     * This means we know at the time of any runtime screen addition and removal the priorities will
     * be up-to-date when we poll them.
     *
     * For dynamic ordering changes without screen changes we will get the change property multiple times.
     * A timer is used to batch them.
     *
     *  A caveat on X11 is the initial startup where plasmashell may query screen information before
     * kscreen has set properties.
     * This should resolve itself as a dynamic re-ordering when kscreen does start.
     */
    virtual void refresh();
Q_SIGNALS:
    void outputOrderChanged(const QStringList &outputOrder);

protected:
    OutputOrderWatcher(QObject *parent);
    /**
     * Backend failed, use QScreen based implementation
     */
    void useFallback(bool fallback, const char *reason = nullptr);

    QStringList m_outputOrder;
    bool m_orderProtocolPresent = false;

private:
};

