#include <QCoreApplication>
#include <QAbstractButton>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
#include <QPointer>
#include <QProgressBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

#include <memory>

#include "SButton.h"
#include "SConfig.h"
#include "SPluginEditor.h"
#include "SPluginInfo.h"
#include "SPluginItem.h"
#include "SPluginTaskManager.h"
#include "SPopup.h"
#include "SPopupItem.h"
#include "SSettings.h"
#include "SSwitcher.h"

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
    void nativeControlsSupportKeyboard();
    void buttonRolesAndNavigationSelection();
    void darkModeUsesBrighterOrangeBackgrounds();
    void pluginDeleteButtonFollowsEditButton();
    void pluginItemRespondsToThemeChanges();
    void pluginSelectionUsesOrangeBackground();
    void editorUsesInlineValidationInsideSettings();
    void popupItemsExposeTipsAndElideLongNames();
    void popupHonorsVisibilityAndEscapeRules();
    void tracksPopupTasksAndAllowsConfirmedForceStop();

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
    QSignalSpy executionSpy(item, &SPopupItem::executionFinished);

    item->exec();
    QTRY_COMPARE_WITH_TIMEOUT(executionSpy.count(), 1, 2000);
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(outputPath), 2000);
    QVERIFY(executionSpy.constFirst().constFirst().toBool());

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

void SSettingsTests::nativeControlsSupportKeyboard()
{
    SButton button("Keyboard button");
    QSignalSpy clickSpy(&button, &QPushButton::clicked);
    QTest::keyClick(&button, Qt::Key_Space);
    QCOMPARE(clickSpy.count(), 1);

    SSwitcher switcher("On", "Off", false);
    QSignalSpy switchOnSpy(&switcher, &SSwitcher::switchOn);
    QTest::keyClick(&switcher, Qt::Key_Space);
    QVERIFY(switcher.isOn());
    QCOMPARE(switchOnSpy.count(), 1);
    QCOMPARE(switcher.text(), QString("✓ On"));
    QCOMPARE(switcher.accessibleDescription(), QString("On"));
    QVERIFY(switcher.styleSheet().contains("border-color: #59C837"));
    QVERIFY(!switcher.styleSheet().contains("#F97316"));
}

void SSettingsTests::buttonRolesAndNavigationSelection()
{
    SButton primary("Primary");
    primary.setRole(SButton::Role::Primary);
    QCOMPARE(primary.role(), SButton::Role::Primary);
    QCOMPARE(primary.property("buttonRole").toString(), QString("primary"));
    QVERIFY(primary.styleSheet().contains("QPushButton:hover"));
    QVERIFY(primary.styleSheet().contains("QPushButton:pressed"));
    QVERIFY(primary.styleSheet().contains("QPushButton:focus"));
    QVERIFY(primary.styleSheet().contains("QPushButton:disabled"));
    QVERIFY(primary.styleSheet().contains("#F97316"));

    SButton navigation("Navigation");
    navigation.setRole(SButton::Role::Navigation);
    navigation.setSelected(true);
    QVERIFY(navigation.isCheckable());
    QVERIFY(navigation.isChecked());
    QVERIFY(navigation.isSelected());
    QCOMPARE(navigation.property("selected").toBool(), true);
    navigation.click();
    navigation.setSelected(true);
    QVERIFY(navigation.isChecked());

    QListWidget *menu = m_settings->findChild<QListWidget *>("settingsMenuList");
    QVERIFY(menu);
    QCOMPARE(menu->count(), 4);
    SButton *pluginsButton = qobject_cast<SButton *>(menu->itemWidget(menu->item(0)));
    SButton *tasksButton = qobject_cast<SButton *>(menu->itemWidget(menu->item(1)));
    QVERIFY(pluginsButton);
    QVERIFY(tasksButton);
    QVERIFY(pluginsButton->isSelected());
    QCOMPARE(pluginsButton->role(), SButton::Role::Navigation);

    tasksButton->click();
    QVERIFY(tasksButton->isSelected());
    QVERIFY(!pluginsButton->isSelected());
    QCOMPARE(menu->currentRow(), 1);
}

void SSettingsTests::darkModeUsesBrighterOrangeBackgrounds()
{
    struct PaletteRestorer
    {
        QPalette palette;
        ~PaletteRestorer()
        {
            QGuiApplication::setPalette(palette);
            QCoreApplication::processEvents();
        }
    } restorer{QGuiApplication::palette()};

    QPalette darkPalette = restorer.palette;
    darkPalette.setColor(QPalette::Window, QColor("#101828"));
    QGuiApplication::setPalette(darkPalette);
    QCoreApplication::processEvents();

    SButton primary("Primary");
    primary.setRole(SButton::Role::Primary);
    QVERIFY(primary.styleSheet().contains("background: #FB923C"));
    QVERIFY(primary.styleSheet().contains("background: #9A3412"));
    QVERIFY(!primary.styleSheet().contains("background: #431407"));

    SSwitcher switcher("On", "Off", true);
    QVERIFY(switcher.styleSheet().contains("background: #214E2D"));
    QVERIFY(switcher.styleSheet().contains("border-color: #59C837"));
    QVERIFY(!switcher.styleSheet().contains("#F97316"));

    QListWidget *pluginList = m_settings->findChild<QListWidget *>("pluginList");
    QVERIFY(pluginList);
    QVERIFY(pluginList->styleSheet().contains("background: #9A3412"));
}

void SSettingsTests::pluginDeleteButtonFollowsEditButton()
{
    SPluginInfo *info = makePlugin("ActionOrderPlugin");
    QPointer<SPluginItem> item(SPluginItem::create(info));
    QHBoxLayout *layout = qobject_cast<QHBoxLayout *>(item->layout());
    SButton *editButton = item->findChild<SButton *>("editPluginButton");
    SButton *deleteButton = item->findChild<SButton *>("deletePluginButton");
    QVERIFY(layout);
    QVERIFY(editButton);
    QVERIFY(deleteButton);
    QCOMPARE(layout->indexOf(deleteButton), layout->indexOf(editButton) + 1);

    item->deleteLater();
    info->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(item.isNull());
}

void SSettingsTests::pluginItemRespondsToThemeChanges()
{
    struct PaletteRestorer
    {
        QPalette palette;
        ~PaletteRestorer()
        {
            QGuiApplication::setPalette(palette);
            QCoreApplication::processEvents();
        }
    } restorer{QGuiApplication::palette()};

    QPalette lightPalette = restorer.palette;
    lightPalette.setColor(QPalette::Window, Qt::white);
    QGuiApplication::setPalette(lightPalette);
    QCoreApplication::processEvents();

    SPluginInfo *info = makePlugin("TransparentTipPlugin");
    QPointer<SPluginItem> item(SPluginItem::create(info));
    QLabel *tipLabel = item->findChild<QLabel *>("pluginTipLabel");
    SButton *deleteButton = item->findChild<SButton *>("deletePluginButton");
    QVERIFY(tipLabel);
    QVERIFY(deleteButton);
    QVERIFY(tipLabel->styleSheet().contains("background-color: transparent"));
    QVERIFY(tipLabel->styleSheet().contains("color: #000000"));
    QVERIFY(tipLabel->styleSheet().contains("border: none"));
    QVERIFY(!tipLabel->styleSheet().contains("palette(base)"));

    const qint64 lightIconKey = deleteButton->icon().cacheKey();
    const QImage deleteIcon = deleteButton->icon().pixmap(QSize(18, 18)).toImage();
    bool hasDarkPixel = false;
    for (int y = 0; y < deleteIcon.height() && !hasDarkPixel; ++y)
    {
        for (int x = 0; x < deleteIcon.width(); ++x)
        {
            const QColor pixel = deleteIcon.pixelColor(x, y);
            if (pixel.alpha() > 128 && pixel.value() < 128)
            {
                hasDarkPixel = true;
                break;
            }
        }
    }
    QVERIFY(hasDarkPixel);

    QPalette darkPalette = restorer.palette;
    darkPalette.setColor(QPalette::Window, QColor("#101828"));
    QGuiApplication::setPalette(darkPalette);
    QCoreApplication::processEvents();

    QVERIFY(tipLabel->styleSheet().contains("color: #FFFFFF"));
    QVERIFY(deleteButton->icon().cacheKey() != lightIconKey);
    const QImage darkDeleteIcon = deleteButton->icon().pixmap(QSize(18, 18)).toImage();
    bool hasLightPixel = false;
    for (int y = 0; y < darkDeleteIcon.height() && !hasLightPixel; ++y)
    {
        for (int x = 0; x < darkDeleteIcon.width(); ++x)
        {
            const QColor pixel = darkDeleteIcon.pixelColor(x, y);
            if (pixel.alpha() > 128 && pixel.value() > 220)
            {
                hasLightPixel = true;
                break;
            }
        }
    }
    QVERIFY(hasLightPixel);

    item->deleteLater();
    info->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(item.isNull());
}

void SSettingsTests::pluginSelectionUsesOrangeBackground()
{
    struct PaletteRestorer
    {
        QPalette palette;
        ~PaletteRestorer()
        {
            QGuiApplication::setPalette(palette);
            QCoreApplication::processEvents();
        }
    } restorer{QGuiApplication::palette()};

    QPalette lightPalette = restorer.palette;
    lightPalette.setColor(QPalette::Window, Qt::white);
    QGuiApplication::setPalette(lightPalette);
    QCoreApplication::processEvents();

    QListWidget *pluginList = m_settings->findChild<QListWidget *>("pluginList");
    QVERIFY(pluginList);
    QVERIFY(pluginList->styleSheet().contains("QListWidget#pluginList::item:selected"));
    QVERIFY(pluginList->styleSheet().contains("background: #FFF7ED"));

    SPluginInfo *info = makePlugin("SelectedPlugin");
    SPluginItem *item = SPluginItem::create(info);
    m_settings->addPluginItem(item);
    QListWidgetItem *listItem = pluginList->item(pluginList->count() - 1);
    QVERIFY(listItem);

    pluginList->clearSelection();
    pluginList->setCurrentItem(nullptr);
    QMetaObject::invokeMethod(item, "selectionRequested", Qt::DirectConnection);
    QCOMPARE(pluginList->currentItem(), listItem);
    QVERIFY(listItem->isSelected());

    m_settings->deletePluginItem(item);
    info->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void SSettingsTests::editorUsesInlineValidationInsideSettings()
{
    SPluginEditor *editor = SPluginEditor::editor();
    editor->create();

    QStackedWidget *content = m_settings->findChild<QStackedWidget *>();
    QVERIFY(content);
    QCOMPARE(content->currentWidget(), editor);
    QCOMPARE(editor->parentWidget(), content);

    QLineEdit *nameEdit = editor->findChild<QLineEdit *>("pluginNameEdit");
    QLineEdit *scriptEdit = editor->findChild<QLineEdit *>("pluginScriptEdit");
    QPushButton *createButton = editor->findChild<QPushButton *>("createPluginButton");
    QPushButton *cancelButton = editor->findChild<QPushButton *>("cancelPluginButton");
    QVERIFY(nameEdit);
    QVERIFY(scriptEdit);
    QVERIFY(createButton);
    QVERIFY(cancelButton);
    QVERIFY(!createButton->isEnabled());

    nameEdit->setText("ValidatedPlugin");
    scriptEdit->setText("/usr/bin/true");
    QVERIFY(createButton->isEnabled());

    nameEdit->setText("Invalid/Plugin");
    QVERIFY(!createButton->isEnabled());

    cancelButton->click();
    QVERIFY(content->currentWidget() != editor);
}

void SSettingsTests::popupItemsExposeTipsAndElideLongNames()
{
    const QString longName(80, QLatin1Char('N'));
    SPluginInfo *info = makePlugin(longName);
    info->tip = "Helpful plugin tip";
    QPointer<SPopupItem> item(SPopupItem::create(info));

    QVERIFY(item->toolTip().contains(longName));
    QVERIFY(item->toolTip().contains(info->tip));

    bool foundElidedName = false;
    for (QLabel *label : item->findChildren<QLabel *>())
    {
        if (label->text().contains(QChar(0x2026)))
        {
            foundElidedName = true;
            break;
        }
    }
    QVERIFY(foundElidedName);

    SPopupItem::remove(item);
    info->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void SSettingsTests::popupHonorsVisibilityAndEscapeRules()
{
    SPopup *popup = SPopup::instance();
    SPluginInfo *disabled = makePlugin("DisabledPopupPlugin");
    disabled->iconEnabled = false;
    disabled->nameEnabled = false;
    popup->addItem(disabled);

    popup->showPopup();
    QVERIFY(!popup->isVisible());
    popup->deleteItem(disabled->popupItem);
    disabled->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    SPluginInfo *enabled = makePlugin("EnabledPopupPlugin");
    popup->addItem(enabled);
    popup->showPopup();
    QVERIFY(popup->isVisible());
    QTest::keyClick(popup, Qt::Key_Escape);
    QVERIFY(!popup->isVisible());

    popup->deleteItem(enabled->popupItem);
    enabled->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void SSettingsTests::tracksPopupTasksAndAllowsConfirmedForceStop()
{
#ifndef Q_OS_UNIX
    QSKIP("This process lifecycle test currently targets Unix executables");
#else
    SPopup *popup = SPopup::instance();
    SPluginInfo *info = makePlugin("LongRunningPlugin");
    info->script = "/bin/sleep 5";
    popup->addItem(info);
    popup->showPopup();

    QLabel *statusLabel = popup->findChild<QLabel *>("popupExecutionStatus");
    QProgressBar *loadingIndicator = popup->findChild<QProgressBar *>("popupLoadingIndicator");
    QWidget *statusWidget = popup->findChild<QWidget *>("popupStatusWidget");
    QListWidget *taskList = m_settings->findChild<QListWidget *>("pluginTaskList");
    QVERIFY(statusLabel);
    QVERIFY(loadingIndicator);
    QVERIFY(statusWidget);
    QVERIFY(taskList);

    QSignalSpy executionSpy(info->popupItem, &SPopupItem::executionFinished);
    info->popupItem->exec();
    QCOMPARE(statusLabel->text(), QString("启动中…"));
    QVERIFY(loadingIndicator->isVisible());
    QVERIFY(!info->popupItem->isVisible());
    QCOMPARE(popup->size(), popup->layout()->sizeHint().expandedTo(popup->minimumSize()));

    QTRY_COMPARE_WITH_TIMEOUT(executionSpy.count(), 1, 2000);
    QVERIFY(executionSpy.constFirst().constFirst().toBool());
    QCOMPARE(statusLabel->text(), QString("已启动"));
    QVERIFY(!loadingIndicator->isVisible());
    QCOMPARE(popup->size(), popup->layout()->sizeHint().expandedTo(popup->minimumSize()));
    QTRY_COMPARE_WITH_TIMEOUT(taskList->count(), 1, 2000);

    QWidget *taskRow = taskList->itemWidget(taskList->item(0));
    QVERIFY(taskRow);
    QPushButton *forceStopButton = taskRow->findChild<QPushButton *>("forceStopTaskButton");
    QVERIFY(forceStopButton);
    bool confirmationAccepted = false;
    QTimer confirmationTimer;
    confirmationTimer.setInterval(10);
    QObject::connect(&confirmationTimer, &QTimer::timeout, [&confirmationTimer, &confirmationAccepted] {
        for (QWidget *widget : QApplication::allWidgets())
        {
            QMessageBox *messageBox = qobject_cast<QMessageBox *>(widget);
            if (!messageBox || !messageBox->isVisible())
            {
                continue;
            }
            QAbstractButton *yesButton = messageBox->button(QMessageBox::Yes);
            if (!yesButton)
            {
                continue;
            }
            confirmationAccepted = true;
            confirmationTimer.stop();
            yesButton->click();
            return;
        }
    });
    confirmationTimer.start();
    forceStopButton->click();
    QVERIFY(confirmationAccepted);

    QTRY_VERIFY_WITH_TIMEOUT(SPluginTaskManager::instance()->activeTasks().isEmpty(), 2000);
    QTRY_COMPARE_WITH_TIMEOUT(taskList->count(), 0, 2000);

    popup->deleteItem(info->popupItem);
    info->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    SPluginInfo *failedInfo = makePlugin("FailedPlugin");
    failedInfo->script = "/path/that/does/not/exist";
    popup->addItem(failedInfo);
    popup->showPopup();
    QSignalSpy failedSpy(failedInfo->popupItem, &SPopupItem::executionFinished);
    failedInfo->popupItem->exec();
    QCOMPARE(statusLabel->text(), QString("启动中…"));
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 2000);
    QVERIFY(!failedSpy.constFirst().constFirst().toBool());
    QCOMPARE(statusLabel->text(), QString("启动失败"));
    QVERIFY(SPluginTaskManager::instance()->activeTasks().isEmpty());
    QTest::qWait(500);
    QVERIFY(popup->isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(!popup->isVisible(), 700);

    popup->deleteItem(failedInfo->popupItem);
    failedInfo->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
#endif
}

QTEST_MAIN(SSettingsTests)

#include "SSettingsTests.moc"
