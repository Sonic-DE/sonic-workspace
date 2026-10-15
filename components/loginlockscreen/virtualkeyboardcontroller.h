/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <memory>

// A backend can be replaced without changing the greeter or shared footer API.
class VirtualKeyboardBackend : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;
    virtual bool isAvailable() const = 0;
    virtual bool isVisible() const = 0;
    virtual void show() = 0;
    virtual void hide() = 0;

Q_SIGNALS:
    void availabilityChanged();
    void visibleChanged();
};

class VirtualKeyboardController : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(VirtualKeyboard)
    QML_SINGLETON
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)
    Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)

public:
    explicit VirtualKeyboardController(QObject *parent = nullptr);
    explicit VirtualKeyboardController(std::unique_ptr<VirtualKeyboardBackend> backend, QObject *parent = nullptr);
    ~VirtualKeyboardController() override;

    bool isAvailable() const;
    bool isVisible() const;
    bool isActive() const;
    void setActive(bool active);
    Q_INVOKABLE void forceActivate();

Q_SIGNALS:
    void availableChanged();
    void visibleChanged();
    void activeChanged();

private:
    struct ActivationState {
        bool active = false;
    };
    std::unique_ptr<VirtualKeyboardBackend> m_backend;
    std::shared_ptr<ActivationState> m_activation;
};
