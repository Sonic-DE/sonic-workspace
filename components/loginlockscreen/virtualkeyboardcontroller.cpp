/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "virtualkeyboardcontroller.h"

#include <QCoreApplication>
#include <QDebug>
#include <QGuiApplication>
#include <QInputMethod>
#include <QPointer>
#include <private/qguiapplication_p.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformintegration.h>
#include <utility>

namespace {
class QtVirtualKeyboardBackend : public VirtualKeyboardBackend {
public:
    QtVirtualKeyboardBackend()
        : m_inputMethod(QGuiApplication::inputMethod())
    {
        auto *integration = QGuiApplicationPrivate::platformIntegration();
        auto *context = integration ? integration->inputContext() : nullptr;
        m_available = context && context->isValid() && QByteArray(context->metaObject()->className()).contains("QtVirtualKeyboard");
        if (!m_available) {
            qWarning() << "Qt Virtual Keyboard input context is unavailable; install the matching Qt Virtual Keyboard runtime";
        }
        connect(m_inputMethod, &QInputMethod::visibleChanged, this, &VirtualKeyboardBackend::visibleChanged);
    }

    bool isAvailable() const override
    {
        return m_available;
    }
    bool isVisible() const override
    {
        return m_inputMethod && m_inputMethod->isVisible();
    }
    void show() override
    {
        if (m_inputMethod) {
            m_inputMethod->show();
        }
    }
    void hide() override
    {
        if (m_inputMethod) {
            m_inputMethod->hide();
        }
    }

private:
    QPointer<QInputMethod> m_inputMethod;
    bool m_available = false;
};
}

VirtualKeyboardController::VirtualKeyboardController(QObject *parent)
    : VirtualKeyboardController(std::make_unique<QtVirtualKeyboardBackend>(), parent)
{
    // All QML engines in an application use the same Qt input method. One
    // footer must not hide a keyboard requested through another engine.
    static std::weak_ptr<ActivationState> applicationActivation;
    if (auto activation = applicationActivation.lock()) {
        m_activation = std::move(activation);
    } else {
        applicationActivation = m_activation;
    }
}

VirtualKeyboardController::VirtualKeyboardController(std::unique_ptr<VirtualKeyboardBackend> backend, QObject *parent)
    : QObject(parent)
    , m_backend(std::move(backend))
    , m_activation(std::make_shared<ActivationState>())
{
    Q_ASSERT(m_backend);
    connect(m_backend.get(), &VirtualKeyboardBackend::visibleChanged, this, [this] {
        // Physical-keyboard users must not get an unsolicited keyboard on focus.
        if (m_backend->isVisible() && !m_activation->active) {
            m_backend->hide();
            return;
        }
        if (!m_backend->isVisible() && m_activation->active) {
            m_activation->active = false;
        }
        Q_EMIT activeChanged();
        Q_EMIT visibleChanged();
    });
    connect(m_backend.get(), &VirtualKeyboardBackend::availabilityChanged, this, [this] {
        if (!isAvailable()) {
            setActive(false);
        }
        Q_EMIT availableChanged();
    });
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this] {
        setActive(false);
    });
}

VirtualKeyboardController::~VirtualKeyboardController()
{
    if (m_activation.use_count() == 1) {
        setActive(false);
    }
}

bool VirtualKeyboardController::isAvailable() const
{
    return m_backend->isAvailable();
}
bool VirtualKeyboardController::isVisible() const
{
    return m_backend->isVisible();
}
bool VirtualKeyboardController::isActive() const
{
    return m_activation->active;
}

void VirtualKeyboardController::setActive(bool active)
{
    if (active && !isAvailable()) {
        return;
    }
    if (m_activation->active != active) {
        m_activation->active = active;
        Q_EMIT activeChanged();
    }
    if (active) {
        m_backend->show();
    } else {
        m_backend->hide();
    }
}

void VirtualKeyboardController::forceActivate()
{
    setActive(true);
}
