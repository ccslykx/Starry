#include <QCoreApplication>
#include <QAbstractButton>
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QEnterEvent>
#include <QEvent>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPointer>
#include <QProgressBar>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleHints>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

#include <memory>

#include "SButton.h"
#include "SConfig.h"
#include "SLanguageManager.h"
#include "SPluginEditor.h"
#include "SPluginInfo.h"
#include "SPluginItem.h"
#include "SPluginTaskManager.h"
#include "SPopup.h"
#include "SPopupItem.h"
#include "SSettings.h"
#include "SSwitcher.h"

namespace
{
QSettings::Format configFileFormat()
{
#ifdef Q_OS_WIN
    return QSettings::IniFormat;
#else
    return QSettings::NativeFormat;
#endif
}
}

class SSettingsTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void deletesTheRequestedPluginWithoutASelectedRow();
    void deletesPopupItems();
    void supportsQuotedPluginArguments();
    void runsWindowsCommandBuiltins();
    void synchronizesPopupOrder();
    void restoresMinimizedSettingsWindow();
    void nativeControlsSupportKeyboard();
    void synchronizesSelectionPopupSetting();
    void buttonRolesAndNavigationSelection();
    void detailPagesShowTitlesAtTop();
    void supportsRuntimeLanguageSwitching();
    void aboutPageUsesCMakeVersion();
    void showsAccessibilityPermissionOnlyOnMac();
    void generalSettingsFollowRuntimeThemeSwitch();
    void darkModeUsesBrighterOrangeBackgrounds();
    void editorRefreshesDuringRuntimeThemeSwitch();
    void pluginDeleteButtonFollowsEditButton();
    void pluginItemRespondsToThemeChanges();
    void pluginSelectionUsesOrangeBackground();
    void taskSelectionMatchesPluginSelection();
    void editorUsesInlineValidationInsideSettings();
    void popupItemsExposeTipsAndElideLongNames();
    void popupHonorsVisibilityAndEscapeRules();
    void popupStatusTimeoutIgnoresHover();
    void reportsNonZeroPluginExit();
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
    m_config->setLanguageCode(QStringLiteral("en"));
    QVERIFY(SLanguageManager::instance()->setLanguage(QStringLiteral("en")));
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

void SSettingsTests::runsWindowsCommandBuiltins()
{
#ifndef Q_OS_WIN
    QSKIP("This command-interpreter test targets Windows");
#else
    SPluginTask *task = SPluginTaskManager::instance()->startTask(
        QStringLiteral("WindowsBuiltin"),
        QStringLiteral("echo"),
        QStringList({QStringLiteral("test")}),
        QStringLiteral("echo \"test\""));
    QVERIFY(task);

    QSignalSpy startedSpy(task, &SPluginTask::started);
    QSignalSpy failedSpy(task, &SPluginTask::failed);
    QSignalSpy finishedSpy(task, &SPluginTask::finished);

    QTRY_COMPARE_WITH_TIMEOUT(startedSpy.count(), 1, 2000);
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 2000);
    QCOMPARE(failedSpy.count(), 0);
    QCOMPARE(finishedSpy.constFirst().at(1).toInt(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(
        SPluginTaskManager::instance()->activeTasks().isEmpty(),
        2000);

    SPluginTask *startTask = SPluginTaskManager::instance()->startTask(
        QStringLiteral("WindowsStartBuiltin"),
        QStringLiteral("start"),
        QStringList({
            QStringLiteral("/b"),
            QStringLiteral("/wait"),
            QString(),
            QStringLiteral("cmd.exe"),
            QStringLiteral("/d"),
            QStringLiteral("/c"),
            QStringLiteral("exit"),
            QStringLiteral("0"),
        }),
        QStringLiteral("start /b /wait \"\" cmd.exe /d /c exit 0"));
    QVERIFY(startTask);

    QSignalSpy startStartedSpy(startTask, &SPluginTask::started);
    QSignalSpy startFailedSpy(startTask, &SPluginTask::failed);
    QSignalSpy startFinishedSpy(startTask, &SPluginTask::finished);

    QTRY_COMPARE_WITH_TIMEOUT(startStartedSpy.count(), 1, 2000);
    QTRY_COMPARE_WITH_TIMEOUT(startFinishedSpy.count(), 1, 2000);
    QCOMPARE(startFailedSpy.count(), 0);
    QCOMPARE(startFinishedSpy.constFirst().at(1).toInt(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(
        SPluginTaskManager::instance()->activeTasks().isEmpty(),
        2000);
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

void SSettingsTests::synchronizesSelectionPopupSetting()
{
    QFrame *selectionPopupCard =
        m_settings->findChild<QFrame *>("selectionPopupCard");
    SSwitcher *selectionPopupSwitcher =
        m_settings->findChild<SSwitcher *>("selectionPopupSwitcher");
    QVERIFY(selectionPopupCard);
    QVERIFY(selectionPopupSwitcher);
    QVERIFY(m_config->selectionPopupEnabled());
    QVERIFY(selectionPopupSwitcher->isOn());
    QCOMPARE(
        selectionPopupSwitcher->accessibleName(),
        QStringLiteral("Enable selection popup"));

    selectionPopupSwitcher->click();
    QVERIFY(!selectionPopupSwitcher->isOn());
    QVERIFY(!m_config->selectionPopupEnabled());
    QSettings savedSettings(
        m_configDir->filePath("starry.conf"),
        configFileFormat());
    QCOMPARE(
        savedSettings.value(
            QStringLiteral("STARRY_SETTINGS/selectionPopupEnabled")).toBool(),
        false);

    m_config->setSelectionPopupEnabled(true);
    QVERIFY(selectionPopupSwitcher->isOn());
    m_config->setSelectionPopupEnabled(false);
    QVERIFY(!selectionPopupSwitcher->isOn());

    selectionPopupSwitcher->click();
    QVERIFY(selectionPopupSwitcher->isOn());
    QVERIFY(m_config->selectionPopupEnabled());
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
    QCOMPARE(menu->count(), 5);
    SButton *generalButton = qobject_cast<SButton *>(menu->itemWidget(menu->item(0)));
    SButton *pluginsButton = qobject_cast<SButton *>(menu->itemWidget(menu->item(1)));
    SButton *tasksButton = qobject_cast<SButton *>(menu->itemWidget(menu->item(2)));
    SSwitcher *debugModeSwitcher =
        m_settings->findChild<SSwitcher *>("debugModeSwitcher");
    QVERIFY(generalButton);
    QVERIFY(pluginsButton);
    QVERIFY(tasksButton);
    QVERIFY(debugModeSwitcher);
    QVERIFY(generalButton->isSelected());
    QVERIFY(!debugModeSwitcher->isOn());
    QCOMPARE(generalButton->role(), SButton::Role::Navigation);
    QCOMPARE(pluginsButton->role(), SButton::Role::Navigation);

    debugModeSwitcher->click();
    QVERIFY(debugModeSwitcher->isOn());
    QVERIFY(m_config->debugModeEnabled());
    QSettings savedSettings(
        m_configDir->filePath("starry.conf"),
        configFileFormat());
    QCOMPARE(
        savedSettings.value(
            QStringLiteral("STARRY_SETTINGS/debugModeEnabled")).toBool(),
        true);
    debugModeSwitcher->click();
    QVERIFY(!debugModeSwitcher->isOn());
    QVERIFY(!m_config->debugModeEnabled());

    tasksButton->click();
    QVERIFY(tasksButton->isSelected());
    QVERIFY(!pluginsButton->isSelected());
    QVERIFY(!generalButton->isSelected());
    QCOMPARE(menu->currentRow(), 2);
}

void SSettingsTests::detailPagesShowTitlesAtTop()
{
    struct ExpectedTitle
    {
        const char *objectName;
        QString text;
    };
    const QList<ExpectedTitle> expectedTitles{
        {"generalPageTitle", QStringLiteral("General Settings")},
        {"pluginsPageTitle", QStringLiteral("Plugins")},
        {"tasksPageTitle", QStringLiteral("Plugin Task Manager")},
        {"shortcutsPageTitle", QStringLiteral("Shortcuts")},
        {"aboutPageTitle", QStringLiteral("About")},
    };

    for (const ExpectedTitle &expected : expectedTitles)
    {
        QLabel *title = m_settings->findChild<QLabel *>(expected.objectName);
        QVERIFY(title);
        QCOMPARE(title->text(), expected.text);
        QVERIFY(title->parentWidget());
        QVERIFY(title->parentWidget()->layout());
        QCOMPARE(title->parentWidget()->layout()->itemAt(0)->widget(), title);
        QVERIFY(title->styleSheet().contains("font-size: 20px"));
    }

    m_settings->showContent(2);
    QCoreApplication::processEvents();
    QLabel *taskTitle =
        m_settings->findChild<QLabel *>("tasksPageTitle");
    QVBoxLayout *taskLayout =
        qobject_cast<QVBoxLayout *>(taskTitle->parentWidget()->layout());
    QVERIFY(taskLayout);
    QCOMPARE(
        taskTitle->alignment() & Qt::AlignVertical_Mask,
        Qt::AlignTop);
    QCOMPARE(taskLayout->stretch(2), 1);
    QCOMPARE(taskLayout->stretch(3), 1);
}

void SSettingsTests::supportsRuntimeLanguageSwitching()
{
    QComboBox *languageCombo =
        m_settings->findChild<QComboBox *>("languageComboBox");
    QListWidget *menu =
        m_settings->findChild<QListWidget *>("settingsMenuList");
    QVERIFY(languageCombo);
    QVERIFY(menu);
    QCOMPARE(languageCombo->count(), 6);
    SSwitcher *debugModeSwitcher =
        m_settings->findChild<SSwitcher *>("debugModeSwitcher");
    SSwitcher *selectionPopupSwitcher =
        m_settings->findChild<SSwitcher *>("selectionPopupSwitcher");
    QVERIFY(debugModeSwitcher);
    QVERIFY(selectionPopupSwitcher);

    struct ExpectedLanguage
    {
        QString code;
        QString generalText;
        QString enabledText;
        QString disabledText;
    };
    const QList<ExpectedLanguage> expectedLanguages{
        {QStringLiteral("zh_CN"), QStringLiteral("常规设置"),
         QStringLiteral("✓ 已开启"), QStringLiteral("已关闭")},
        {QStringLiteral("zh_TW"), QStringLiteral("一般設定"),
         QStringLiteral("✓ 已開啟"), QStringLiteral("已關閉")},
        {QStringLiteral("en"), QStringLiteral("General Settings"),
         QStringLiteral("✓ Enabled"), QStringLiteral("Disabled")},
        {QStringLiteral("de"), QStringLiteral("Allgemeine Einstellungen"),
         QStringLiteral("✓ Aktiviert"), QStringLiteral("Deaktiviert")},
        {QStringLiteral("fr"), QStringLiteral("Paramètres généraux"),
         QStringLiteral("✓ Activé"), QStringLiteral("Désactivé")},
        {QStringLiteral("ja"), QStringLiteral("一般設定"),
         QStringLiteral("✓ 有効"), QStringLiteral("無効")},
    };
    for (const ExpectedLanguage &language : expectedLanguages)
    {
        const int index = languageCombo->findData(language.code);
        QVERIFY(index >= 0);
        languageCombo->setCurrentIndex(index);
        SButton *generalButton =
            qobject_cast<SButton *>(menu->itemWidget(menu->item(0)));
        QVERIFY(generalButton);
        QTRY_COMPARE_WITH_TIMEOUT(
            generalButton->text(),
            language.generalText,
            1000);
        debugModeSwitcher->setStatus(false);
        QCOMPARE(debugModeSwitcher->text(), language.disabledText);
        debugModeSwitcher->setStatus(true);
        QCOMPARE(debugModeSwitcher->text(), language.enabledText);
        debugModeSwitcher->setStatus(false);
        selectionPopupSwitcher->setStatus(false);
        QCOMPARE(selectionPopupSwitcher->text(), language.disabledText);
        selectionPopupSwitcher->setStatus(true);
        QCOMPARE(selectionPopupSwitcher->text(), language.enabledText);
        QCOMPARE(m_config->languageCode(), language.code);
        QCOMPARE(
            QCoreApplication::translate("SSettings", "General Settings"),
            language.generalText);
    }

    SPluginEditor *editor = SPluginEditor::editor();
    editor->create();
    languageCombo->setCurrentIndex(
        languageCombo->findData(QStringLiteral("ja")));
    QTRY_COMPARE_WITH_TIMEOUT(
        editor->windowTitle(),
        QStringLiteral("新しいプラグインを作成"),
        1000);

    languageCombo->setCurrentIndex(
        languageCombo->findData(QStringLiteral("en")));
    QTRY_COMPARE_WITH_TIMEOUT(
        editor->windowTitle(),
        QStringLiteral("Create New Plugin"),
        1000);
    QCOMPARE(m_config->languageCode(), QStringLiteral("en"));

    QSettings savedSettings(
        m_configDir->filePath("starry.conf"),
        configFileFormat());
    QCOMPARE(
        savedSettings.value(QStringLiteral("STARRY_SETTINGS/language")).toString(),
        QStringLiteral("en"));
}

void SSettingsTests::aboutPageUsesCMakeVersion()
{
    QLabel *aboutContent =
        m_settings->findChild<QLabel *>("aboutContentLabel");
    QVERIFY(aboutContent);
    QCOMPARE(m_config->version(), QStringLiteral(STARRY_VERSION_STRING));
    QCOMPARE(m_config->major(), STARRY_VERSION_MAJOR);
    QCOMPARE(m_config->minor(), STARRY_VERSION_MINOR);
    QCOMPARE(m_config->patch(), STARRY_VERSION_PATCH);
    QVERIFY(aboutContent->text().startsWith(
        QStringLiteral("Version: %1\n").arg(QStringLiteral(STARRY_VERSION_STRING))));
}

void SSettingsTests::showsAccessibilityPermissionOnlyOnMac()
{
    QFrame *permissionCard =
        m_settings->findChild<QFrame *>("accessibilityPermissionCard");
#ifdef Q_OS_MACOS
    QVERIFY(permissionCard);
    QLabel *statusLabel =
        permissionCard->findChild<QLabel *>("accessibilityPermissionStatus");
    QPushButton *requestButton =
        permissionCard->findChild<QPushButton *>(
            "requestAccessibilityPermissionButton");
    QVERIFY(statusLabel);
    QVERIFY(requestButton);
    QVERIFY(statusLabel->text() == QStringLiteral("Granted")
            || statusLabel->text() == QStringLiteral("Not granted"));
    QCOMPARE(
        requestButton->isHidden(),
        statusLabel->text() == QStringLiteral("Granted"));

    SSwitcher *debugModeSwitcher =
        m_settings->findChild<SSwitcher *>("debugModeSwitcher");
    QVERIFY(debugModeSwitcher);
    QVERIFY(!debugModeSwitcher->isOn());
    debugModeSwitcher->click();
    QVERIFY(debugModeSwitcher->isOn());
    QVERIFY(!requestButton->isHidden());
    QVERIFY(requestButton->isEnabled());
    QCOMPARE(
        requestButton->text(),
        statusLabel->text() == QStringLiteral("Granted")
            ? QStringLiteral("Request again")
            : QStringLiteral("Request permission"));
    debugModeSwitcher->click();
    QVERIFY(!debugModeSwitcher->isOn());
#else
    QVERIFY(!permissionCard);
#endif
}

void SSettingsTests::generalSettingsFollowRuntimeThemeSwitch()
{
    QStyleHints *styleHints = QGuiApplication::styleHints();
    QVERIFY(styleHints);

    QFrame *languageCard =
        m_settings->findChild<QFrame *>("languageCard");
    QFrame *debugModeCard =
        m_settings->findChild<QFrame *>("debugModeCard");
    QFrame *selectionPopupCard =
        m_settings->findChild<QFrame *>("selectionPopupCard");
#ifdef Q_OS_MACOS
    QFrame *permissionCard =
        m_settings->findChild<QFrame *>("accessibilityPermissionCard");
#endif
    QComboBox *languageCombo =
        m_settings->findChild<QComboBox *>("languageComboBox");
    SSwitcher *debugModeSwitcher =
        m_settings->findChild<SSwitcher *>("debugModeSwitcher");
    QVERIFY(languageCard);
    QVERIFY(debugModeCard);
    QVERIFY(selectionPopupCard);
#ifdef Q_OS_MACOS
    QVERIFY(permissionCard);
#endif
    QVERIFY(languageCombo);
    QVERIFY(languageCombo->view());
    QVERIFY(debugModeSwitcher);

    styleHints->colorSchemeChanged(Qt::ColorScheme::Dark);
    QTRY_VERIFY_WITH_TIMEOUT(
        languageCard->styleSheet().contains("background: #1D2939"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(
        debugModeCard->styleSheet().contains("border: 1px solid #344054"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(
        selectionPopupCard->styleSheet().contains(
            "border: 1px solid #344054"), 1000);
#ifdef Q_OS_MACOS
    QTRY_VERIFY_WITH_TIMEOUT(
        permissionCard->styleSheet().contains("border: 1px solid #344054"), 1000);
#endif
    QTRY_VERIFY_WITH_TIMEOUT(
        languageCombo->styleSheet().contains("background: #344054"), 1000);
    QCOMPARE(
        languageCombo->property("arrowColor").value<QColor>(),
        QColor("#F2F4F7"));
    QVERIFY(languageCombo->styleSheet().contains(
        "QComboBox#languageComboBox::down-arrow { image: none; }"));
    QTRY_VERIFY_WITH_TIMEOUT(
        languageCombo->view()->styleSheet().contains("background: #1D2939"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(
        debugModeSwitcher->styleSheet().contains("background: #344054"), 1000);

    styleHints->colorSchemeChanged(Qt::ColorScheme::Light);
    QTRY_VERIFY_WITH_TIMEOUT(
        languageCard->styleSheet().contains("background: #FFFFFF"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(
        debugModeCard->styleSheet().contains("border: 1px solid #EAECF0"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(
        selectionPopupCard->styleSheet().contains(
            "border: 1px solid #EAECF0"), 1000);
#ifdef Q_OS_MACOS
    QTRY_VERIFY_WITH_TIMEOUT(
        permissionCard->styleSheet().contains("border: 1px solid #EAECF0"), 1000);
#endif
    QTRY_VERIFY_WITH_TIMEOUT(
        languageCombo->styleSheet().contains("border: 1px solid #D0D5DD"), 1000);
    QCOMPARE(
        languageCombo->property("arrowColor").value<QColor>(),
        QColor("#344054"));
    QTRY_VERIFY_WITH_TIMEOUT(
        languageCombo->view()->styleSheet().contains("background: #FFFFFF"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(
        debugModeSwitcher->styleSheet().contains("background: #F2F4F7"), 1000);

    styleHints->colorSchemeChanged(styleHints->colorScheme());
    QCoreApplication::processEvents();
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
    darkPalette.setColor(QPalette::Base, QColor("#182230"));
    darkPalette.setColor(QPalette::Text, QColor("#E4E7EC"));
    darkPalette.setColor(QPalette::PlaceholderText, QColor("#98A2B3"));
    darkPalette.setColor(QPalette::Highlight, QColor("#FB923C"));
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

    QFrame *editorCard = m_settings->findChild<QFrame *>("pluginEditorCard");
    QLineEdit *editorName = m_settings->findChild<QLineEdit *>("pluginNameEdit");
    QPlainTextEdit *editorCommand =
        m_settings->findChild<QPlainTextEdit *>("pluginScriptEdit");
    QVERIFY(editorCard);
    QVERIFY(editorName);
    QVERIFY(editorCommand);
    QVERIFY(editorCard->styleSheet().contains("background: #1D2939"));
    QVERIFY(editorCard->styleSheet().contains("color: palette(text)"));
    QCOMPARE(editorName->palette().color(QPalette::Base),
             darkPalette.color(QPalette::Base));
    QCOMPARE(editorName->palette().color(QPalette::Text),
             darkPalette.color(QPalette::Text));
    QCOMPARE(editorCommand->palette().color(QPalette::Base),
             darkPalette.color(QPalette::Base));
    QCOMPARE(editorCommand->palette().color(QPalette::Text),
             darkPalette.color(QPalette::Text));
    QCOMPARE(editorCommand->font().pointSizeF(), editorName->font().pointSizeF());
}

void SSettingsTests::editorRefreshesDuringRuntimeThemeSwitch()
{
    QStyleHints *styleHints = QGuiApplication::styleHints();
    QVERIFY(styleHints);
    struct PaletteRestorer
    {
        QPalette palette;
        ~PaletteRestorer()
        {
            QGuiApplication::setPalette(palette);
            QCoreApplication::processEvents();
        }
    } restorer{QGuiApplication::palette()};

    SPluginEditor *editor = SPluginEditor::editor();
    QFrame *card = editor->findChild<QFrame *>("pluginEditorCard");
    SButton *backButton = editor->findChild<SButton *>("cancelPluginButton");
    QLineEdit *nameEdit = editor->findChild<QLineEdit *>("pluginNameEdit");
    QPlainTextEdit *commandEdit =
        editor->findChild<QPlainTextEdit *>("pluginScriptEdit");
    QScrollArea *scrollArea =
        editor->findChild<QScrollArea *>("pluginEditorFormScrollArea");
    QVERIFY(card);
    QVERIFY(backButton);
    QVERIFY(nameEdit);
    QVERIFY(commandEdit);
    QVERIFY(scrollArea);

    // Emit the same signal delivered by Qt when the operating-system theme
    // changes. QStyleHints::setColorScheme() is not a reliable OS-theme
    // simulator on every platform plugin (notably macOS test runners).
    styleHints->colorSchemeChanged(Qt::ColorScheme::Dark);
    QTRY_VERIFY_WITH_TIMEOUT(
        card->styleSheet().contains("background: #1D2939"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(
        backButton->styleSheet().contains("border: 1px solid #475467"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(
        scrollArea->styleSheet().contains("#C2410C"), 1000);

    QPalette darkPalette = restorer.palette;
    darkPalette.setColor(QPalette::Window, QColor("#101828"));
    darkPalette.setColor(QPalette::Base, QColor("#182230"));
    darkPalette.setColor(QPalette::Text, QColor("#E4E7EC"));
    QGuiApplication::setPalette(darkPalette);
    QCoreApplication::processEvents();
    QCOMPARE(nameEdit->palette().color(QPalette::Base),
             darkPalette.color(QPalette::Base));
    QCOMPARE(commandEdit->viewport()->palette().color(QPalette::Text),
             darkPalette.color(QPalette::Text));

    styleHints->colorSchemeChanged(Qt::ColorScheme::Light);
    QTRY_VERIFY_WITH_TIMEOUT(
        card->styleSheet().contains("background: #FFFFFF"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(
        backButton->styleSheet().contains("border: 1px solid #D0D5DD"), 1000);
    QTRY_VERIFY_WITH_TIMEOUT(
        scrollArea->styleSheet().contains("#FED7AA"), 1000);

    QPalette lightPalette = restorer.palette;
    lightPalette.setColor(QPalette::Window, Qt::white);
    lightPalette.setColor(QPalette::Base, QColor("#FFFFFF"));
    lightPalette.setColor(QPalette::Text, QColor("#101828"));
    QGuiApplication::setPalette(lightPalette);
    QCoreApplication::processEvents();
    QCOMPARE(nameEdit->palette().color(QPalette::Base),
             lightPalette.color(QPalette::Base));
    QCOMPARE(commandEdit->viewport()->palette().color(QPalette::Text),
             lightPalette.color(QPalette::Text));
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
    lightPalette.setColor(QPalette::Base, QColor("#F8FAFC"));
    lightPalette.setColor(QPalette::Text, QColor("#172033"));
    lightPalette.setColor(QPalette::PlaceholderText, QColor("#64748B"));
    lightPalette.setColor(QPalette::Highlight, QColor("#F97316"));
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

void SSettingsTests::taskSelectionMatchesPluginSelection()
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

    QListWidget *pluginList = m_settings->findChild<QListWidget *>("pluginList");
    QListWidget *taskList =
        m_settings->findChild<QListWidget *>("pluginTaskList");
    QVERIFY(pluginList);
    QVERIFY(taskList);
    QVERIFY(!taskList->alternatingRowColors());
    QCOMPARE(taskList->spacing(), pluginList->spacing());

    const auto normalizedStyle = [] (QListWidget *list) {
        QString style = list->styleSheet();
        style.replace(QStringLiteral("QListWidget#") + list->objectName(),
                      QStringLiteral("QListWidget#sharedList"));
        return style;
    };

    QPalette lightPalette = restorer.palette;
    lightPalette.setColor(QPalette::Window, Qt::white);
    QGuiApplication::setPalette(lightPalette);
    QCoreApplication::processEvents();
    QCOMPARE(normalizedStyle(taskList), normalizedStyle(pluginList));
    QVERIFY(taskList->styleSheet().contains("background: #FFF7ED"));

    QPalette darkPalette = restorer.palette;
    darkPalette.setColor(QPalette::Window, QColor("#101828"));
    QGuiApplication::setPalette(darkPalette);
    QCoreApplication::processEvents();
    QCOMPARE(normalizedStyle(taskList), normalizedStyle(pluginList));
    QVERIFY(taskList->styleSheet().contains("background: #9A3412"));
}

void SSettingsTests::editorUsesInlineValidationInsideSettings()
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

    SPluginEditor *editor = SPluginEditor::editor();
    editor->create();
    QCoreApplication::processEvents();

    QStackedWidget *content = m_settings->findChild<QStackedWidget *>();
    QVERIFY(content);
    QCOMPARE(content->currentWidget(), editor);
    QCOMPARE(editor->parentWidget(), content);

    QLineEdit *nameEdit = editor->findChild<QLineEdit *>("pluginNameEdit");
    QPlainTextEdit *scriptEdit = editor->findChild<QPlainTextEdit *>("pluginScriptEdit");
    QPushButton *createButton = editor->findChild<QPushButton *>("createPluginButton");
    QPushButton *cancelButton = editor->findChild<QPushButton *>("cancelPluginButton");
    QPushButton *testButton = editor->findChild<QPushButton *>("testPluginButton");
    QPushButton *insertButton = editor->findChild<QPushButton *>("insertPlaintextButton");
    QPushButton *insertUrlButton = editor->findChild<QPushButton *>("insertUrlEncodedButton");
    QPushButton *iconPicker = editor->findChild<QPushButton *>("pluginIconPicker");
    QFrame *formCard = editor->findChild<QFrame *>("pluginEditorCard");
    QFrame *statusWidget = editor->findChild<QFrame *>("pluginEditorStatus");
    QScrollArea *formScrollArea =
        editor->findChild<QScrollArea *>("pluginEditorFormScrollArea");
    QLabel *nameError = editor->findChild<QLabel *>("pluginNameError");
    QVERIFY(nameEdit);
    QVERIFY(scriptEdit);
    QVERIFY(createButton);
    QVERIFY(cancelButton);
    QVERIFY(testButton);
    QVERIFY(insertButton);
    QVERIFY(insertUrlButton);
    QVERIFY(iconPicker);
    QVERIFY(formCard);
    QVERIFY(statusWidget);
    QVERIFY(formScrollArea);
    QVERIFY(nameError);
    QVERIFY(!createButton->isEnabled());
    QVERIFY(nameError->isHidden());
    QVERIFY(statusWidget->isHidden());
    QCOMPARE(iconPicker->minimumSize(), QSize(96, 96));
    QCOMPARE(iconPicker->maximumSize(), QSize(96, 96));
    QCOMPARE(iconPicker->size(), QSize(96, 96));
    QVERIFY(formCard->styleSheet().contains("border-radius: 12px"));
    QVERIFY(formCard->styleSheet().contains("background: #FFFFFF"));
    QVERIFY(formScrollArea->styleSheet().contains("QScrollBar::handle"));
    QVERIFY(formScrollArea->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff);
    QCOMPARE(formScrollArea->horizontalScrollBar()->maximum(), 0);
    QCOMPARE(nameEdit->palette().color(QPalette::Base),
             lightPalette.color(QPalette::Base));
    QCOMPARE(nameEdit->palette().color(QPalette::Text),
             lightPalette.color(QPalette::Text));
    QCOMPARE(scriptEdit->font().pointSizeF(), nameEdit->font().pointSizeF());
    QCOMPARE(scriptEdit->lineWrapMode(), QPlainTextEdit::NoWrap);
    QVERIFY(scriptEdit->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff);
    QVERIFY(insertButton->geometry().bottom()
            <= insertUrlButton->geometry().top());

    const QImage initialDefaultIcon =
        iconPicker->icon().pixmap(iconPicker->iconSize()).toImage();
    nameEdit->setText("Validated Plugin");
    QCoreApplication::processEvents();
    QVERIFY(iconPicker->icon().pixmap(iconPicker->iconSize()).toImage()
            != initialDefaultIcon);
    scriptEdit->setPlainText("/usr/bin/true");
    QVERIFY(createButton->isEnabled());
    insertButton->click();
    QVERIFY(scriptEdit->toPlainText().contains("$PLAINTEXT"));
    insertUrlButton->click();
    QVERIFY(scriptEdit->toPlainText().contains("$URLENCODED"));
    scriptEdit->setPlainText("starry copy2clipboard");
    testButton->click();
    QVERIFY(!statusWidget->isHidden());
    QCOMPARE(statusWidget->property("statusKind").toString(), QString("success"));

    nameEdit->setText("Invalid/Plugin");
    QVERIFY(!createButton->isEnabled());
    QVERIFY(!nameError->isHidden());
    QCOMPARE(nameEdit->property("validationError").toBool(), true);

    QTimer::singleShot(0, [] {
        for (QWidget *widget : QApplication::topLevelWidgets())
        {
            if (QMessageBox *box = qobject_cast<QMessageBox *>(widget))
            {
                if (QAbstractButton *discardButton = box->button(QMessageBox::Discard))
                {
                    discardButton->click();
                }
                return;
            }
        }
    });
    cancelButton->click();
    QVERIFY(content->currentWidget() != editor);

    SPluginInfo *info = makePlugin("EditorExisting");
    editor->edit(info);
    QPushButton *saveButton = editor->findChild<QPushButton *>("savePluginButton");
    QLineEdit *tipEdit = editor->findChild<QLineEdit *>("pluginTipEdit");
    QVERIFY(saveButton);
    QVERIFY(tipEdit);
    QCOMPARE(saveButton, createButton);
    QVERIFY(!saveButton->isEnabled());
    tipEdit->setText("Updated tip");
    QVERIFY(saveButton->isEnabled());

    QTimer::singleShot(0, [] {
        for (QWidget *widget : QApplication::topLevelWidgets())
        {
            if (QMessageBox *box = qobject_cast<QMessageBox *>(widget))
            {
                if (QAbstractButton *discardButton = box->button(QMessageBox::Discard))
                {
                    discardButton->click();
                }
                return;
            }
        }
    });
    cancelButton->click();
    info->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
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

void SSettingsTests::popupStatusTimeoutIgnoresHover()
{
    SPopup *popup = SPopup::instance();
    SPluginInfo *info = makePlugin("StatusTimeoutPlugin");
    info->script = "starry copy2clipboard";
    popup->addItem(info);
    popup->showPopup();

    info->popupItem->exec();
    QLabel *statusLabel =
        popup->findChild<QLabel *>("popupExecutionStatus");
    QVERIFY(statusLabel);
    QCOMPARE(statusLabel->text(), QString("Copied"));

    const QPointF localPosition(1.0, 1.0);
    const QPointF globalPosition(popup->mapToGlobal(QPoint(1, 1)));
    QEnterEvent enterEvent(
        localPosition,
        localPosition,
        globalPosition);
    QCoreApplication::sendEvent(popup, &enterEvent);
    QEvent leaveEvent(QEvent::Leave);
    QCoreApplication::sendEvent(popup, &leaveEvent);

    QTest::qWait(550);
    QVERIFY(popup->isVisible());
    QTRY_VERIFY_WITH_TIMEOUT(!popup->isVisible(), 500);

    popup->deleteItem(info->popupItem);
    info->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

void SSettingsTests::reportsNonZeroPluginExit()
{
#ifndef Q_OS_UNIX
    QSKIP("This process lifecycle test currently targets Unix executables");
#else
    const QString falseExecutable = QStandardPaths::findExecutable(QStringLiteral("false"));
    QVERIFY(!falseExecutable.isEmpty());

    SPluginInfo *info = makePlugin("NonZeroExitPlugin");
    info->script = falseExecutable;
    QPointer<SPopupItem> item(SPopupItem::create(info));
    QSignalSpy executionSpy(item, &SPopupItem::executionFinished);

    item->exec();
    QTRY_VERIFY_WITH_TIMEOUT(executionSpy.count() >= 2, 2000);
    QVERIFY(executionSpy.at(0).at(0).toBool());
    QVERIFY(!executionSpy.constLast().at(0).toBool());
    QVERIFY(executionSpy.constLast().at(1).toString().contains(QStringLiteral("exit code")));
    QTRY_VERIFY_WITH_TIMEOUT(SPluginTaskManager::instance()->activeTasks().isEmpty(), 2000);

    SPopupItem::remove(item);
    info->deleteLater();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
#endif
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
    QCOMPARE(statusLabel->text(), QString("Starting…"));
    QVERIFY(loadingIndicator->isVisible());
    QVERIFY(!info->popupItem->isVisible());
    QCOMPARE(popup->size(), popup->layout()->sizeHint().expandedTo(popup->minimumSize()));

    QTRY_COMPARE_WITH_TIMEOUT(executionSpy.count(), 1, 2000);
    QVERIFY(executionSpy.constFirst().constFirst().toBool());
    QCOMPARE(statusLabel->text(), QString("Started"));
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
    QCOMPARE(statusLabel->text(), QString("Starting…"));
    QTRY_COMPARE_WITH_TIMEOUT(failedSpy.count(), 1, 2000);
    QVERIFY(!failedSpy.constFirst().constFirst().toBool());
    QCOMPARE(statusLabel->text(), QString("Failed to start"));
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
