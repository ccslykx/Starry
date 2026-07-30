#include <QCoreApplication>
#include <QAbstractButton>
#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPointer>
#include <QProgressBar>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyleHints>
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
    void editorRefreshesDuringRuntimeThemeSwitch();
    void pluginDeleteButtonFollowsEditButton();
    void pluginItemRespondsToThemeChanges();
    void pluginSelectionUsesOrangeBackground();
    void taskSelectionMatchesPluginSelection();
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

    nameEdit->setText("ValidatedPlugin");
    scriptEdit->setPlainText("/usr/bin/true");
    QVERIFY(createButton->isEnabled());
    insertButton->click();
    QVERIFY(scriptEdit->toPlainText().contains("$PLAINTEXT"));
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
