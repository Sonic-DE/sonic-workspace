/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../historymodel.h"
#include "../historyitem.h"
#include "../klipper.h"
#include "systemclipboard.h"

#include <QAbstractItemModelTester>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QMimeData>
#include <QPushButton>
#include <QRadioButton>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTest>

#include <KConfigDialog>
#include <KConfigGroup>
#include <KCoreConfigSkeleton>
#include <KSharedConfig>

class HistoryModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testMigrateSelectionSettings_data();
    void testMigrateSelectionSettings();
    void testHistorySettings_data();
    void testHistorySettings();
    void testSetMaxSize();
    void testInsertRemove();
    void testClear();
    void testIndexOf();
    void testType_data();
    void testType();
    void testConfigurationDialog_data();
    void testConfigurationDialog();
    void testKeepClipboardContents();
};

void HistoryModelTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void HistoryModelTest::testMigrateSelectionSettings_data()
{
    QTest::addColumn<int>("legacyKeys");
    QTest::addColumn<int>("legacyValues");
    QTest::addColumn<bool>("actionsEnabled");
    QTest::addColumn<int>("newValues");

    for (int keys = 0; keys < 8; ++keys) {
        for (int values = 0; values < 8; ++values) {
            for (bool actions : {false, true}) {
                const auto name = QStringLiteral("keys-%1-values-%2-actions-%3").arg(keys).arg(values).arg(actions);
                QTest::newRow(qPrintable(name)) << keys << values << actions << -1;
            }
        }
    }
    for (int values = 0; values < 4; ++values) {
        const auto name = QStringLiteral("preserve-new-settings-%1").arg(values);
        QTest::newRow(qPrintable(name)) << 7 << values << true << values;
    }
}

void HistoryModelTest::testMigrateSelectionSettings()
{
    QFETCH(int, legacyKeys);
    QFETCH(int, legacyValues);
    QFETCH(bool, actionsEnabled);
    QFETCH(int, newValues);

    auto *settings = HistoryModel::settings();
    auto config = settings->sharedConfig();
    KConfigGroup general(config, QStringLiteral("General"));
    general.deleteGroup();
    const QStringList legacyNames{QStringLiteral("IgnoreSelection"), QStringLiteral("IgnoreImages"), QStringLiteral("SelectionTextOnly")};
    for (int bit = 0; bit < legacyNames.size(); ++bit) {
        if (legacyKeys & (1 << bit)) {
            general.writeEntry(legacyNames[bit], bool(legacyValues & (1 << bit)));
        }
    }
    if (newValues >= 0) {
        general.writeEntry("SaveSelection", bool(newValues & 1));
        general.writeEntry("SaveImages", bool(newValues & 2));
        general.writeEntry("ActionOnSelection", false);
    }
    general.writeEntry("URLGrabberEnabled", actionsEnabled);
    QVERIFY(general.sync());
    settings->read();

    const bool saveSelection = newValues >= 0 ? bool(newValues & 1) : (legacyKeys & 1) && !(legacyValues & 1);
    const bool saveImages = newValues >= 0 ? bool(newValues & 2) : (legacyKeys & 2) && !(legacyValues & 2);
    const bool actionOnSelection = newValues < 0 && (legacyKeys & 1) && saveSelection && actionsEnabled;

    // The migration must preserve the result on subsequent startups.
    for (int load = 0; load < 2; ++load) {
        auto history = HistoryModel::self();
        QVERIFY(history);
        config->reparseConfiguration();
        for (const auto &name : legacyNames) {
            QVERIFY(!general.hasKey(name));
        }
        QCOMPARE(general.readEntry("SaveSelection", false), saveSelection);
        QCOMPARE(general.readEntry("SaveImages", false), saveImages);
        QCOMPARE(general.readEntry("ActionOnSelection", false), actionOnSelection);
        QCOMPARE(history->m_bSaveSelection, saveSelection);
        QCOMPARE(history->m_bSaveImages, saveImages);
        settings->read();
    }
}

void HistoryModelTest::testHistorySettings_data()
{
    QTest::addColumn<bool>("saveSelection");
    QTest::addColumn<bool>("saveImages");
    QTest::addColumn<bool>("selectionMode");
    QTest::addColumn<int>("contentType");

    for (int flags = 0; flags < 8; ++flags) {
        for (int type = 0; type < 5; ++type) {
            const auto name = QStringLiteral("flags-%1-content-%2").arg(flags).arg(type);
            QTest::newRow(qPrintable(name)) << bool(flags & 1) << bool(flags & 2) << bool(flags & 4) << type;
        }
    }
}

void HistoryModelTest::testHistorySettings()
{
    QFETCH(bool, saveSelection);
    QFETCH(bool, saveImages);
    QFETCH(bool, selectionMode);
    QFETCH(int, contentType);

    auto *settings = HistoryModel::settings();
    KConfigGroup general(settings->sharedConfig(), QStringLiteral("General"));
    general.writeEntry("SaveSelection", saveSelection);
    general.writeEntry("SaveImages", saveImages);
    general.writeEntry("SyncClipboards", false);
    QVERIFY(general.sync());
    settings->read();
    auto history = HistoryModel::self();
    history->clear();
    QTRY_COMPARE(history->pendingJobs(), 0);

    QMimeData data;
    if (contentType == 0 || contentType == 2 || contentType == 4) {
        data.setText(QStringLiteral("clipboard settings regression"));
    }
    if (contentType >= 1 && contentType <= 3) {
        QImage image(2, 2, QImage::Format_ARGB32);
        image.fill(Qt::red);
        data.setImageData(image);
    }
    if (contentType == 3) {
        data.setData(QStringLiteral("x-kde-force-image-copy"), QByteArrayLiteral("1"));
    } else if (contentType == 4) {
        data.setData(QStringLiteral("x-kde-passwordManagerHint"), QByteArrayLiteral("secret"));
    }

    SystemClipboard::self()->checkClipData(selectionMode ? QClipboard::Selection : QClipboard::Clipboard, &data);
    const bool expected = (!selectionMode || saveSelection) && contentType != 4 && (contentType != 1 || saveImages);
    QCOMPARE(history->rowCount(), expected ? 1 : 0);
    QTRY_COMPARE(history->pendingJobs(), 0);
    history->clear();
    QTRY_COMPARE(history->pendingJobs(), 0);
}

void HistoryModelTest::testSetMaxSize()
{
    SystemClipboard::self()->clear();
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.get()));
    history->setMaxSize(0);

    QCOMPARE(history->rowCount(), 0);
    QCOMPARE(history->maxSize(), 0); // Default value

    // insert an item - should still be empty
    QVERIFY(!history->insert(QStringLiteral("foo")));
    QCOMPARE(history->rowCount(), 0);

    // now it should insert again
    history->setMaxSize(1);
    QCOMPARE(history->maxSize(), 1);
    QVERIFY(history->insert(QStringLiteral("foo")));
    QCOMPARE(history->rowCount(), 1);

    // insert another item, foo should get removed
    history->insert(QStringLiteral("bar"));
    QCOMPARE(history->rowCount(), 1);
    QCOMPARE(history->data(history->index(0, 0)).toString(), QLatin1String("bar"));

    // I don't trust the model, add more items
    history->setMaxSize(10);
    QVERIFY(history->insert(QStringLiteral("foo")));
    QVERIFY(history->insert(QStringLiteral("foobar")));
    QCOMPARE(history->rowCount(), 3);
    QCOMPARE(history->data(history->index(0, 0)).toString(), QLatin1String("foobar"));
    QCOMPARE(history->data(history->index(1, 0)).toString(), QLatin1String("foo"));
    QCOMPARE(history->data(history->index(2, 0)).toString(), QLatin1String("bar"));

    // setting to 0 should clear again
    history->setMaxSize(0);
    QCOMPARE(history->maxSize(), 0);
    QCOMPARE(history->rowCount(), 0);
    QTRY_COMPARE(history->pendingJobs(), 0);
}

void HistoryModelTest::testInsertRemove()
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.get()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);

    const QString fooText = QStringLiteral("foo");
    const QString barText = QStringLiteral("bar");
    const QString fooBarText = QStringLiteral("foobar");
    const QString fooUuid = QString::fromLatin1(QCryptographicHash::hash(fooText.toUtf8(), QCryptographicHash::Sha1).toHex());
    const QString barUuid = QString::fromLatin1(QCryptographicHash::hash(barText.toUtf8(), QCryptographicHash::Sha1).toHex());
    const QString foobarUuid = QString::fromLatin1(QCryptographicHash::hash(fooBarText.toUtf8(), QCryptographicHash::Sha1).toHex());

    // let's insert a few items
    history->insert(fooText);
    QModelIndex index = history->index(0, 0);
    QCOMPARE(index.data().toString(), fooText);

    history->insert(barText);
    QCOMPARE(index.data().toString(), barText);

    history->insert(fooBarText);
    QCOMPARE(history->data(history->index(0, 0)).toString(), fooBarText);
    index = history->index(0, 0);

    // insert one again - it should be moved to top
    history->insert(barText);
    QCOMPARE(history->data(history->index(0, 0)).toString(), barText);

    // move one to top using the slot
    // already on top, shouldn't change anything
    history->moveToTop(barUuid);
    QCOMPARE(history->data(history->index(0, 0)).toString(), barText);

    // another one should change, though
    history->moveToTop(foobarUuid);
    QCOMPARE(history->data(history->index(0, 0)).toString(), fooBarText);

    // remove them again
    QVERIFY(history->remove(foobarUuid));
    QCOMPARE(history->data(history->index(0, 0)).toString(), barText);

    QVERIFY(history->remove(barUuid));
    QCOMPARE(history->data(history->index(0, 0)).toString(), fooText);

    QVERIFY(history->remove(fooUuid));
    QCOMPARE(history->rowCount(), 0);
    QTRY_COMPARE(history->pendingJobs(), 0);
}

void HistoryModelTest::testClear()
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.get()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);

    history->clear();
    QCOMPARE(history->rowCount(), 0);

    // insert some items
    QVERIFY(history->insert(QStringLiteral("foo")));
    QVERIFY(history->insert(QStringLiteral("bar")));
    QVERIFY(history->insert(QStringLiteral("foobar")));
    history->moveToTop(QString::fromLatin1(QCryptographicHash::hash(QByteArrayLiteral("bar"), QCryptographicHash::Sha1).toHex()));
    QCOMPARE(history->rowCount(), 3);

    // and clear
    history->clear();
    QTRY_COMPARE(history->pendingJobs(), 0);
    QCOMPARE(history->rowCount(), 0);
}

void HistoryModelTest::testIndexOf()
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    std::unique_ptr<QAbstractItemModelTester> modelTest(new QAbstractItemModelTester(history.get()));
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);
    QVERIFY(history->indexOf(QStringLiteral("whatever")) < 0);
    QVERIFY(history->indexOf(QString()) < 0);

    // insert some items
    history->insert(QStringLiteral("foo"));
    QVERIFY(history->indexOf(QStringLiteral("whatever")) < 0);
    QVERIFY(history->indexOf(QString()) < 0);
    const QString fooUuid = QString::fromLatin1(QCryptographicHash::hash(QByteArrayLiteral("foo"), QCryptographicHash::Sha1).toHex());
    QVERIFY(history->indexOf(fooUuid) >= 0);
    QCOMPARE(history->index(history->indexOf(fooUuid)).data(HistoryModel::UuidRole).toString(), fooUuid);

    history->clear();
    QTRY_COMPARE(history->pendingJobs(), 0);
    QVERIFY(history->indexOf(fooUuid) < 0);
}

void HistoryModelTest::testType_data()
{
    QTest::addColumn<std::shared_ptr<QMimeData>>("item");
    QTest::addColumn<HistoryItemType>("expectedType");

    auto item = std::make_shared<QMimeData>();
    item->setText(QStringLiteral("foo"));
    QTest::newRow("text") << item << HistoryItemType::Text;
    item = std::make_shared<QMimeData>();
    item->setImageData(QImage());
    QTest::newRow("image") << item << HistoryItemType::Image;
    item = std::make_shared<QMimeData>();
    item->setText(QStringLiteral("foo"));
    item->setUrls({QUrl(QStringLiteral("file:///home/"))});
    QTest::newRow("url") << item << HistoryItemType::Url;
}

void HistoryModelTest::testType()
{
    std::shared_ptr<HistoryModel> history = HistoryModel::self();
    QAbstractItemModelTester modelTest(history.get());
    history->setMaxSize(10);
    QCOMPARE(history->rowCount(), 0);

    QFETCH(std::shared_ptr<QMimeData>, item);
    QFETCH(HistoryItemType, expectedType);
    history->insert(item.get());
    QCOMPARE(history->index(0).data(HistoryModel::TypeRole).value<HistoryItemType>(), expectedType);
    history->clear();
    QTRY_COMPARE(history->pendingJobs(), 0);
}

void HistoryModelTest::testKeepClipboardContents()
{
    auto setKeepClipboardContents = [](bool value) {
        std::shared_ptr<HistoryModel> history = HistoryModel::self();
        auto settings = history->settings()->sharedConfig();
        KConfigGroup group = settings->group(QStringLiteral("General"));
        QVERIFY(group.isValid());
        group.writeEntry(QStringLiteral("KeepClipboardContents"), value);
        settings->sync();
        history->settings()->read();
        history->loadSettings();
    };

    {
        std::shared_ptr<HistoryModel> history = HistoryModel::self();
        setKeepClipboardContents(true);
        QVERIFY(history->insert(QStringLiteral("foo")));
        QCOMPARE(history->rowCount(), 1);
        QCOMPARE(history->pendingJobs(), 1);
        QTRY_COMPARE(history->pendingJobs(), 0);
    }

    {
        qDebug() << "Stage 2";
        std::shared_ptr<HistoryModel> history = HistoryModel::self();
        QCOMPARE(history->rowCount(), 1);

        setKeepClipboardContents(false);
    }

    qDebug() << "Stage 3";
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/klipper/data");
    QVERIFY(!dataDir.exists());

    {
        std::shared_ptr<HistoryModel> history = HistoryModel::self();
        QCOMPARE(history->rowCount(), 0);
        dataDir.refresh();
        QVERIFY(dataDir.exists());
        QCOMPARE(dataDir.entryList(QDir::NoDotAndDotDot).size(), 0);

        QVERIFY(history->insert(QStringLiteral("foobar")));
        QCOMPARE(history->rowCount(), 1);
        QCOMPARE(history->pendingJobs(), 1);
        QTRY_COMPARE(history->pendingJobs(), 0);
        dataDir.refresh();
        qDebug() << dataDir.entryList();
        QCOMPARE(dataDir.entryList().size(), 3); // QList(".", "..", "8843d7f92416211de9ebb963ff4ce28125932878")

        setKeepClipboardContents(true);
    }

    qDebug() << "Stage 4";
    dataDir.refresh();
    QVERIFY(dataDir.exists());
    QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/klipper").removeRecursively();
}

void HistoryModelTest::testConfigurationDialog_data()
{
    QTest::addColumn<bool>("syncClipboards");
    QTest::addColumn<bool>("saveSelection");
    QTest::addColumn<bool>("saveImages");
    for (int flags = 0; flags < 8; ++flags) {
        QTest::newRow(qPrintable(QString::number(flags))) << bool(flags & 1) << bool(flags & 2) << bool(flags & 4);
    }
}

void HistoryModelTest::testConfigurationDialog()
{
    QFETCH(bool, syncClipboards);
    QFETCH(bool, saveSelection);
    QFETCH(bool, saveImages);

    auto *settings = HistoryModel::settings();
    KConfigGroup general(settings->sharedConfig(), QStringLiteral("General"));
    general.writeEntry("SyncClipboards", syncClipboards);
    general.writeEntry("SaveSelection", saveSelection);
    general.writeEntry("SaveImages", saveImages);
    QVERIFY(general.sync());
    settings->read();

    Klipper klipper;
    klipper.slotConfigure();
    auto *dialog = KConfigDialog::exists(QStringLiteral("preferences"));
    QVERIFY(dialog);
    const auto cleanup = qScopeGuard([dialog] {
        delete dialog;
    });

    auto *sync = dialog->findChild<QCheckBox *>(QStringLiteral("kcfg_SyncClipboards"));
    auto *selection = dialog->findChild<QCheckBox *>(QStringLiteral("kcfg_SaveSelection"));
    auto *images = dialog->findChild<QCheckBox *>(QStringLiteral("kcfg_SaveImages"));
    QVERIFY(sync);
    QVERIFY(selection);
    QVERIFY(images);
    QCOMPARE(selection->isChecked(), saveSelection);
    QCOMPARE(images->isChecked(), saveImages);
    QVERIFY(dialog->findChildren<QRadioButton *>().isEmpty());
    sync->setChecked(!syncClipboards);
    QVERIFY(selection->isEnabled());
    QVERIFY(images->isEnabled());
    QCOMPARE(selection->isChecked(), saveSelection);
    QCOMPARE(images->isChecked(), saveImages);

    selection->setChecked(!saveSelection);
    images->setChecked(!saveImages);
    auto *buttonBox = dialog->findChild<QDialogButtonBox *>();
    QVERIFY(buttonBox);
    auto *apply = buttonBox->button(QDialogButtonBox::Apply);
    QVERIFY(apply);
    QTRY_VERIFY(apply->isEnabled());
    apply->click();
    settings->sharedConfig()->reparseConfiguration();
    QCOMPARE(general.readEntry("SaveSelection", false), !saveSelection);
    QCOMPARE(general.readEntry("SaveImages", false), !saveImages);
    auto history = HistoryModel::self();
    QCOMPARE(history->m_bSaveSelection, !saveSelection);
    QCOMPARE(history->m_bSaveImages, !saveImages);
}

QTEST_MAIN(HistoryModelTest)
#include "historymodeltest.moc"
