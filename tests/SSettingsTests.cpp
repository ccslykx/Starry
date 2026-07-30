#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QPixmap>
#include <QPointer>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

#include "SConfig.h"
#include "SPluginInfo.h"
#include "SPluginItem.h"
#include "SPopup.h"
#include "SPopupItem.h"
#include "SSettings.h"

class SSettingsTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void deletesTheRequestedPluginWithoutASelectedRow();
    void deletesPopupItems();
    void supportsQuotedPluginArguments();
    void synchronizesPopupOrder();
    void restoresMinimizedSettingsWindow();

private:
    static SPluginInfo *makePlugin(const QString &name);

    std::unique_ptr<QTemporaryDir> m_configDir;
    SConfig *m_config = nullptr;
    SSettings *m_settings = nullptr;
};

SPluginInfo *SSettingsTests::makePlugin(const QString &name)
{
    QPixmap icon(2, 2);
    icon.fill(Qt::black);
    return new SPluginInfo(name, "echo test", icon, 0, "test", true, true);
}

void SSettingsTests::initTestCase()
{
    m_configDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_configDir->isValid());
    QVERIFY(QDir().mkpath(m_configDir->filePath("icons")));
    m_config = SConfig::config(m_configDir->path());
    m_settings = SSettings::instance();
}

void SSettingsTests::deletesTheRequestedPluginWithoutASelectedRow()
{
    SPluginInfo *first = makePlugin("First");
    SPluginInfo *second = makePlugin("Second");
    QVERIFY(m_config->addPlugin(first, NewCreate));
    QVERIFY(m_config->addPlugin(second, NewCreate));
    m_settings->addPluginItem(first);
    m_settings->addPluginItem(second);

    QPointer<SPluginInfo> guardedFirst(first);
    QPointer<SPluginItem> guardedFirstItem(first->pluginItem);
    QPointer<SPluginItem> guardedSecondItem(second->pluginItem);
    emit first->needDelete(first);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    QVERIFY(guardedFirst.isNull());
    QVERIFY(guardedFirstItem.isNull());
    QVERIFY(!guardedSecondItem.isNull());
    QCOMPARE(m_config->getSPluginInfo("Second"), second);
    QCOMPARE(second->index, 0);
}

void SSettingsTests::deletesPopupItems()
{
    SPluginInfo *info = makePlugin("PopupOnly");
    QPointer<SPluginInfo> guardedInfo(info);
    QPointer<SPopupItem> item(SPopupItem::create(info));

    SPopupItem::remove(item);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(item.isNull());

    info->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(guardedInfo.isNull());
}

void SSettingsTests::supportsQuotedPluginArguments()
{
#ifndef Q_OS_UNIX
    QSKIP("This command smoke test currently targets Unix executables");
#else
    const QString outputPath = m_configDir->filePath("quoted output");
    SPluginInfo *info = makePlugin("QuotedCommand");
    info->script = QString("/usr/bin/touch \"%1\"").arg(outputPath);
    QPointer<SPluginInfo> guardedInfo(info);
    QPointer<SPopupItem> item(SPopupItem::create(info));

    item->exec();
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(outputPath), 2000);

    SPopupItem::remove(item);
    info->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(item.isNull());
    QVERIFY(guardedInfo.isNull());
#endif
}

void SSettingsTests::synchronizesPopupOrder()
{
    SPluginInfo *first = makePlugin("PopupFirst");
    SPluginInfo *second = makePlugin("PopupSecond");
    first->index = 0;
    second->index = 1;

    SPopup *popup = SPopup::instance();
    popup->addItem(first);
    popup->addItem(second);
    SPluginItem *firstSettingsItem = SPluginItem::create(first);
    SPluginItem *secondSettingsItem = SPluginItem::create(second);
    QHBoxLayout *layout = qobject_cast<QHBoxLayout *>(popup->layout());
    QVERIFY(layout);
    QCOMPARE(qobject_cast<SPopupItem *>(layout->itemAt(0)->widget())->pluginInfo(), first);
    QCOMPARE(qobject_cast<SPopupItem *>(layout->itemAt(1)->widget())->pluginInfo(), second);

    firstSettingsItem->setIndexToInfo(1);
    secondSettingsItem->setIndexToInfo(0);

    QCOMPARE(qobject_cast<SPopupItem *>(layout->itemAt(0)->widget())->pluginInfo(), second);
    QCOMPARE(qobject_cast<SPopupItem *>(layout->itemAt(1)->widget())->pluginInfo(), first);

    popup->deleteItem(first->popupItem);
    popup->deleteItem(second->popupItem);
    firstSettingsItem->deleteLater();
    secondSettingsItem->deleteLater();
    first->deleteLater();
    second->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void SSettingsTests::restoresMinimizedSettingsWindow()
{
    m_settings->setWindowState(Qt::WindowMinimized);
    m_settings->showAndActivate();
    QVERIFY(m_settings->isVisible());
    QVERIFY(!(m_settings->windowState() & Qt::WindowMinimized));
    QCoreApplication::processEvents();
}

QTEST_MAIN(SSettingsTests)

#include "SSettingsTests.moc"
