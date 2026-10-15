/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "virtualkeyboardcontroller.h"

#include <QtTest>

class FakeKeyboardBackend : public VirtualKeyboardBackend {
public:
    bool available = true;
    bool visible = false;
    int shows = 0;
    int hides = 0;

    bool isAvailable() const override
    {
        return available;
    }
    bool isVisible() const override
    {
        return visible;
    }
    void show() override
    {
        ++shows;
        setVisible(true);
    }
    void hide() override
    {
        ++hides;
        setVisible(false);
    }

    void setVisible(bool value)
    {
        if (visible != value) {
            visible = value;
            Q_EMIT visibleChanged();
        }
    }
};

class VirtualKeyboardControllerTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void activationAndHide()
    {
        auto backend = std::make_unique<FakeKeyboardBackend>();
        auto *fake = backend.get();
        VirtualKeyboardController controller(std::move(backend));
        QSignalSpy visibility(&controller, &VirtualKeyboardController::visibleChanged);
        QVERIFY(controller.isAvailable());
        QVERIFY(!controller.isActive());
        controller.forceActivate();
        QVERIFY(controller.isActive());
        QVERIFY(controller.isVisible());
        QCOMPARE(fake->shows, 1);
        controller.setActive(false);
        QVERIFY(!controller.isActive());
        QVERIFY(!controller.isVisible());
        QCOMPARE(fake->hides, 1);
        QCOMPARE(visibility.count(), 2);
    }

    void unavailableBackendCannotActivate()
    {
        auto backend = std::make_unique<FakeKeyboardBackend>();
        auto *fake = backend.get();
        fake->available = false;
        VirtualKeyboardController controller(std::move(backend));
        controller.forceActivate();
        QVERIFY(!controller.isActive());
        QVERIFY(!controller.isVisible());
        QCOMPARE(fake->shows, 0);
    }

    void focusDoesNotShowUnrequestedKeyboard()
    {
        auto backend = std::make_unique<FakeKeyboardBackend>();
        auto *fake = backend.get();
        VirtualKeyboardController controller(std::move(backend));
        fake->setVisible(true);
        QVERIFY(!controller.isVisible());
        QVERIFY(!controller.isActive());
        QCOMPARE(fake->hides, 1);
    }

    void losingInputFocusReleasesActiveState()
    {
        auto backend = std::make_unique<FakeKeyboardBackend>();
        auto *fake = backend.get();
        VirtualKeyboardController controller(std::move(backend));
        controller.forceActivate();
        fake->setVisible(false);
        QVERIFY(!controller.isActive());
        QVERIFY(!controller.isVisible());
        controller.forceActivate();
        QVERIFY(controller.isVisible());
        QCOMPARE(fake->shows, 2);
    }

    void backendDisappearanceHidesKeyboard()
    {
        auto backend = std::make_unique<FakeKeyboardBackend>();
        auto *fake = backend.get();
        VirtualKeyboardController controller(std::move(backend));
        controller.forceActivate();
        fake->available = false;
        Q_EMIT fake->availabilityChanged();
        QVERIFY(!controller.isAvailable());
        QVERIFY(!controller.isVisible());
        QVERIFY(!controller.isActive());
    }

    void applicationShutdownHidesKeyboard()
    {
        auto backend = std::make_unique<FakeKeyboardBackend>();
        VirtualKeyboardController controller(std::move(backend));
        controller.forceActivate();
        QVERIFY(QMetaObject::invokeMethod(QCoreApplication::instance(), "aboutToQuit", Qt::DirectConnection));
        QVERIFY(!controller.isActive());
        QVERIFY(!controller.isVisible());
    }
};

QTEST_GUILESS_MAIN(VirtualKeyboardControllerTest)

#include "virtualkeyboardcontrollertest.moc"
