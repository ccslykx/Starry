#include <QAbstractItemView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QComboBox>
#include <QFontMetrics>
#include <QFrame>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyle>
#include <QStyleHints>
#include <QTimer>

#include "SSettings.h"
#include "SConfig.h"
#include "SLanguageManager.h"
#include "SPluginTaskManager.h"
#include "SSwitcher.h"
#include "utils.h"

#ifdef Q_OS_MACOS
#include "Core/Platform/MacAccessibilityPermission.h"
#endif

namespace
{
class LanguageComboBox final : public QComboBox
{
public:
    using QComboBox::QComboBox;

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QComboBox::paintEvent(event);

        const char *colorProperty = isEnabled()
            ? "arrowColor"
            : "disabledArrowColor";
        const QColor color = property(colorProperty).value<QColor>();
        if (!color.isValid())
        {
            return;
        }

        const qreal centerX = width() - 16.0;
        const qreal centerY = height() / 2.0;
        QPainterPath arrow;
        arrow.moveTo(centerX - 4.0, centerY - 2.0);
        arrow.lineTo(centerX, centerY + 2.0);
        arrow.lineTo(centerX + 4.0, centerY - 2.0);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(arrow);
    }
};

QString selectableListStyleSheet(const QString &objectName, bool dark)
{
    const QString selector = QStringLiteral("QListWidget#%1").arg(objectName);
    if (dark)
    {
        return QStringLiteral(
            "%1 { border: none; background: transparent; outline: none; }"
            "%1::item {"
            "  background: transparent; border: 1px solid transparent; border-radius: 7px;"
            "}"
            "%1::item:hover:!selected {"
            "  background: #344054; border-color: #475467;"
            "}"
            "%1::item:selected {"
            "  background: #9A3412; border: 1px solid #F97316;"
            "  border-left: 3px solid #FDBA74;"
            "}")
            .arg(selector);
    }
    return QStringLiteral(
        "%1 { border: none; background: transparent; outline: none; }"
        "%1::item {"
        "  background: transparent; border: 1px solid transparent; border-radius: 7px;"
        "}"
        "%1::item:hover:!selected {"
        "  background: #F2F4F7; border-color: #D0D5DD;"
        "}"
        "%1::item:selected {"
        "  background: #FFF7ED; border: 1px solid #FED7AA;"
        "  border-left: 3px solid #F97316;"
        "}")
        .arg(selector);
}

QString generalSettingsCardStyleSheet(const QString &objectName, bool dark)
{
    const QString selector = QStringLiteral("QFrame#%1").arg(objectName);
    return dark
        ? QStringLiteral(
            "%1 {"
            "  background: #1D2939; border: 1px solid #344054;"
            "  border-radius: 10px;"
            "}")
              .arg(selector)
        : QStringLiteral(
            "%1 {"
            "  background: #FFFFFF; border: 1px solid #EAECF0;"
            "  border-radius: 10px;"
            "}")
              .arg(selector);
}

QString languageComboBoxStyleSheet(bool dark)
{
    if (dark)
    {
        return QStringLiteral(
            "QComboBox#languageComboBox {"
            "  min-height: 36px; padding: 0 36px 0 12px;"
            "  color: #F2F4F7; background: #344054;"
            "  border: 1px solid #475467; border-radius: 8px;"
            "  selection-color: #FFF7ED; selection-background-color: #9A3412;"
            "}"
            "QComboBox#languageComboBox:hover {"
            "  background: #475467; border-color: #667085;"
            "}"
            "QComboBox#languageComboBox:focus,"
            "QComboBox#languageComboBox:on { border: 2px solid #FB923C; }"
            "QComboBox#languageComboBox:disabled {"
            "  color: #667085; background: #1D2939; border-color: #344054;"
            "}"
            "QComboBox#languageComboBox::drop-down {"
            "  width: 32px; border: none; border-left: 1px solid #475467;"
            "}"
            "QComboBox#languageComboBox::down-arrow { image: none; }");
    }
    return QStringLiteral(
        "QComboBox#languageComboBox {"
        "  min-height: 36px; padding: 0 36px 0 12px;"
        "  color: #344054; background: #FFFFFF;"
        "  border: 1px solid #D0D5DD; border-radius: 8px;"
        "  selection-color: #9A3412; selection-background-color: #FFF7ED;"
        "}"
        "QComboBox#languageComboBox:hover {"
        "  background: #F9FAFB; border-color: #98A2B3;"
        "}"
        "QComboBox#languageComboBox:focus,"
        "QComboBox#languageComboBox:on { border: 2px solid #F97316; }"
        "QComboBox#languageComboBox:disabled {"
        "  color: #98A2B3; background: #F2F4F7; border-color: #EAECF0;"
        "}"
        "QComboBox#languageComboBox::drop-down {"
        "  width: 32px; border: none; border-left: 1px solid #D0D5DD;"
        "}"
        "QComboBox#languageComboBox::down-arrow { image: none; }");
}

QString languageComboPopupStyleSheet(bool dark)
{
    return dark
        ? QStringLiteral(
            "QAbstractItemView {"
            "  color: #F2F4F7; background: #1D2939;"
            "  border: 1px solid #475467; border-radius: 8px;"
            "  padding: 4px; outline: none; selection-color: #FFF7ED;"
            "  selection-background-color: #9A3412;"
            "}"
            "QAbstractItemView::item { min-height: 32px; padding: 0 8px; }"
            "QAbstractItemView::item:hover { background: #344054; }"
            "QAbstractItemView::item:selected { background: #9A3412; }")
        : QStringLiteral(
            "QAbstractItemView {"
            "  color: #344054; background: #FFFFFF;"
            "  border: 1px solid #D0D5DD; border-radius: 8px;"
            "  padding: 4px; outline: none; selection-color: #9A3412;"
            "  selection-background-color: #FFF7ED;"
            "}"
            "QAbstractItemView::item { min-height: 32px; padding: 0 8px; }"
            "QAbstractItemView::item:hover { background: #F2F4F7; }"
            "QAbstractItemView::item:selected { background: #FFF7ED; }");
}
}

SSettings* SSettings::m_instance = nullptr;

SSettings* SSettings::instance(QWidget *parent)
{
    SDEBUG
    if (!m_instance)
    {
        m_instance = new SSettings(parent);
    }
    return m_instance;
}

void SSettings::showAndActivate()
{
#ifdef Q_OS_MACOS
    refreshAccessibilityPermission();
#endif
    Qt::WindowStates state = windowState();
    state &= ~Qt::WindowMinimized;
    state |= Qt::WindowActive;
    setWindowState(state);
    show();
    raise();
    activateWindow();

    // A tray menu may still own focus while its QAction is being dispatched.
    // Retry after that menu has closed.
    QTimer::singleShot(0, this, [this] {
        raise();
        activateWindow();
    });
}

void SSettings::refreshTheme(bool force, Qt::ColorScheme scheme)
{
    const bool dark = scheme == Qt::ColorScheme::Dark
        || (scheme == Qt::ColorScheme::Unknown
            && QGuiApplication::palette().color(QPalette::Window).lightness() < 128);
    if (!force && m_styleInitialized && m_darkStyle == dark)
    {
        return;
    }
    m_darkStyle = dark;
    m_styleInitialized = true;

    if (m_generalTitleLabel)
    {
        m_generalTitleLabel->setStyleSheet(dark
            ? QStringLiteral(
                "font-size: 20px; font-weight: 600; color: #FFFFFF;")
            : QStringLiteral(
                "font-size: 20px; font-weight: 600; color: #101828;"));
    }
    if (m_generalDescriptionLabel)
    {
        m_generalDescriptionLabel->setStyleSheet(
            dark ? QStringLiteral("color: #98A2B3;")
                 : QStringLiteral("color: #667085;"));
    }
    if (m_languageCard)
    {
        m_languageCard->setStyleSheet(
            generalSettingsCardStyleSheet(m_languageCard->objectName(), dark));
    }
    if (m_debugModeCard)
    {
        m_debugModeCard->setStyleSheet(
            generalSettingsCardStyleSheet(m_debugModeCard->objectName(), dark));
    }
#ifdef Q_OS_MACOS
    if (m_accessibilityPermissionCard)
    {
        m_accessibilityPermissionCard->setStyleSheet(
            generalSettingsCardStyleSheet(
                m_accessibilityPermissionCard->objectName(), dark));
    }
#endif
    const QString titleStyle = dark
        ? QStringLiteral("font-weight: 600; color: #F2F4F7;")
        : QStringLiteral("font-weight: 600; color: #344054;");
    const QString descriptionStyle = dark
        ? QStringLiteral("color: #98A2B3;")
        : QStringLiteral("color: #667085;");
    if (m_languageTitleLabel)
    {
        m_languageTitleLabel->setStyleSheet(titleStyle);
    }
    if (m_languageDescriptionLabel)
    {
        m_languageDescriptionLabel->setStyleSheet(descriptionStyle);
    }
    if (m_debugTitleLabel)
    {
        m_debugTitleLabel->setStyleSheet(titleStyle);
    }
    if (m_debugDescriptionLabel)
    {
        m_debugDescriptionLabel->setStyleSheet(descriptionStyle);
    }
#ifdef Q_OS_MACOS
    if (m_accessibilityPermissionTitleLabel)
    {
        m_accessibilityPermissionTitleLabel->setStyleSheet(titleStyle);
    }
    if (m_accessibilityPermissionDescriptionLabel)
    {
        m_accessibilityPermissionDescriptionLabel->setStyleSheet(descriptionStyle);
    }
    refreshAccessibilityPermission();
#endif
    if (m_languageComboBox)
    {
        m_languageComboBox->setProperty(
            "arrowColor",
            dark ? QColor(QStringLiteral("#F2F4F7"))
                 : QColor(QStringLiteral("#344054")));
        m_languageComboBox->setProperty(
            "disabledArrowColor",
            dark ? QColor(QStringLiteral("#667085"))
                 : QColor(QStringLiteral("#98A2B3")));
        m_languageComboBox->setStyleSheet(languageComboBoxStyleSheet(dark));
        m_languageComboBox->update();
        if (QAbstractItemView *popupView = m_languageComboBox->view())
        {
            popupView->setStyleSheet(languageComboPopupStyleSheet(dark));
        }
    }
    if (m_pluginListWidget)
    {
        m_pluginListWidget->setStyleSheet(
            selectableListStyleSheet(m_pluginListWidget->objectName(), dark));
    }
    if (m_taskListWidget)
    {
        m_taskListWidget->setStyleSheet(
            selectableListStyleSheet(m_taskListWidget->objectName(), dark));
    }
}

void SSettings::syncLanguageSelection(const QString &code)
{
    if (!m_languageComboBox)
    {
        return;
    }
    const QString normalized =
        SLanguageManager::normalizedLanguageCode(code);
    const int index = m_languageComboBox->findData(normalized);
    if (index < 0)
    {
        return;
    }
    const QSignalBlocker blocker(m_languageComboBox);
    m_languageComboBox->setCurrentIndex(index);
}

void SSettings::retranslateUi()
{
    if (!m_contentWidget)
    {
        return;
    }

    if (m_generalTitleLabel)
    {
        m_generalTitleLabel->setText(tr("General Settings"));
    }
    if (m_generalDescriptionLabel)
    {
        m_generalDescriptionLabel->setText(
            tr("Configure application-wide behavior."));
    }
    if (m_languageTitleLabel)
    {
        m_languageTitleLabel->setText(tr("Display language"));
    }
    if (m_languageDescriptionLabel)
    {
        m_languageDescriptionLabel->setText(
            tr("Choose the language used by Starry. Changes apply immediately."));
    }
    if (m_languageComboBox)
    {
        m_languageComboBox->setAccessibleName(tr("Display language"));
        syncLanguageSelection(
            SLanguageManager::instance()->currentLanguageCode());
    }
    if (m_debugTitleLabel)
    {
        m_debugTitleLabel->setText(tr("Enable debug mode"));
    }
    if (m_debugDescriptionLabel)
    {
        m_debugDescriptionLabel->setText(
            tr("When enabled, selected text and expanded plugin arguments are written "
               "to the debug log. The log may contain sensitive information."));
    }
    if (m_debugModeSwitcher)
    {
        m_debugModeSwitcher->setOnText(tr("Enabled"));
        m_debugModeSwitcher->setOffText(tr("Disabled"));
        m_debugModeSwitcher->setAccessibleName(tr("Enable debug mode"));
    }
#ifdef Q_OS_MACOS
    if (m_accessibilityPermissionTitleLabel)
    {
        m_accessibilityPermissionTitleLabel->setText(
            tr("Accessibility permission"));
    }
    if (m_accessibilityPermissionDescriptionLabel)
    {
        m_accessibilityPermissionDescriptionLabel->setText(
            tr("Allows Starry to monitor mouse actions and read selected text."));
    }
    if (m_requestAccessibilityPermissionButton)
    {
        m_requestAccessibilityPermissionButton->setAccessibleName(
            tr("Request accessibility permission"));
    }
    refreshAccessibilityPermission();
#endif
    if (m_newPluginButton)
    {
        m_newPluginButton->setText(tr("Create new plugin"));
        m_newPluginButton->setAccessibleName(tr("Create new plugin"));
    }
    if (m_taskTitleLabel)
    {
        m_taskTitleLabel->setText(tr("Plugin Task Manager"));
    }
    if (m_taskDescriptionLabel)
    {
        m_taskDescriptionLabel->setText(
            tr("Running plugins remain here until they exit. "
               "Force stopping a task may lose its unsaved data."));
    }
    if (m_emptyTaskLabel)
    {
        m_emptyTaskLabel->setText(tr("No plugin tasks are running."));
    }
    if (m_shortcutHelpLabel)
    {
        m_shortcutHelpLabel->setText(
            tr("Need another shortcut? Please contact the author."));
    }
    if (m_aboutContentLabel)
    {
        m_aboutContentLabel->setText(
            tr("Version: %1\nAuthor: Ccslykx\nContact: ccslykx@outlook.com")
                .arg(m_config->version()));
    }

    const QStringList menuTexts{
        tr("General Settings"),
        tr("Plugins"),
        tr("Tasks"),
        tr("Shortcuts"),
        tr("About"),
    };
    for (qsizetype index = 0;
         index < m_menuButtons.size() && index < menuTexts.size();
         ++index)
    {
        m_menuButtons.at(index)->setText(menuTexts.at(index));
        m_menuButtons.at(index)->setAccessibleName(menuTexts.at(index));
    }

    for (SPluginTask *task : m_taskItems.keys())
    {
        retranslateTaskRow(task);
    }
    setWindowTitle(tr("Starry Settings"));
}

void SSettings::initGui()
{
    SDEBUG
    // 菜单页（左侧）
    if (!m_menuListWidget)
    {
        m_menuListWidget = new QListWidget(this);
    }
    m_menuListWidget->setObjectName("settingsMenuList");
    m_menuListWidget->setItemAlignment(Qt::AlignCenter);
    m_menuListWidget->setFixedWidth(160);
    m_menuListWidget->setSpacing(4);
    m_menuListWidget->setStyleSheet(
        "QListWidget { border: none; background: transparent; outline: none; }"
        "QListWidget::item { border: none; background: transparent; }");

    // 内容页（右侧）
    if (!m_contentWidget)
    {
        m_contentWidget = new QStackedWidget(this);
    }

    // 内容页-常规设置
    if (!m_generalWidget)
    {
        m_generalWidget = new QWidget(m_contentWidget);
        m_generalWidget->setObjectName("generalSettingsPage");

        m_generalTitleLabel = new QLabel(m_generalWidget);
        m_generalTitleLabel->setStyleSheet("font-size: 20px; font-weight: 600;");
        m_generalDescriptionLabel = new QLabel(m_generalWidget);
        m_generalDescriptionLabel->setWordWrap(true);

        m_languageCard = new QFrame(m_generalWidget);
        m_languageCard->setObjectName("languageCard");
        m_languageCard->setAttribute(Qt::WA_StyledBackground, true);

        m_languageTitleLabel = new QLabel(m_languageCard);
        m_languageTitleLabel->setObjectName("languageTitle");
        m_languageTitleLabel->setStyleSheet("font-weight: 600;");
        m_languageDescriptionLabel = new QLabel(m_languageCard);
        m_languageDescriptionLabel->setObjectName("languageDescription");
        m_languageDescriptionLabel->setWordWrap(true);

        m_languageComboBox = new LanguageComboBox(m_languageCard);
        m_languageComboBox->setObjectName("languageComboBox");
        m_languageComboBox->setMinimumWidth(160);
        for (const SLanguageManager::Language &language
             : SLanguageManager::supportedLanguages())
        {
            m_languageComboBox->addItem(language.nativeName, language.code);
        }

        QVBoxLayout *languageTextLayout = new QVBoxLayout;
        languageTextLayout->setContentsMargins(0, 0, 0, 0);
        languageTextLayout->setSpacing(4);
        languageTextLayout->addWidget(m_languageTitleLabel);
        languageTextLayout->addWidget(m_languageDescriptionLabel);

        QHBoxLayout *languageLayout = new QHBoxLayout(m_languageCard);
        languageLayout->setContentsMargins(18, 16, 18, 16);
        languageLayout->setSpacing(16);
        languageLayout->addLayout(languageTextLayout, 1);
        languageLayout->addWidget(m_languageComboBox, 0, Qt::AlignVCenter);

        m_debugModeCard = new QFrame(m_generalWidget);
        m_debugModeCard->setObjectName("debugModeCard");
        m_debugModeCard->setAttribute(Qt::WA_StyledBackground, true);

        m_debugTitleLabel = new QLabel(m_debugModeCard);
        m_debugTitleLabel->setObjectName("debugModeTitle");
        m_debugTitleLabel->setStyleSheet("font-weight: 600;");
        m_debugDescriptionLabel = new QLabel(m_debugModeCard);
        m_debugDescriptionLabel->setObjectName("debugModeDescription");
        m_debugDescriptionLabel->setWordWrap(true);

        m_debugModeSwitcher = new SSwitcher(
            QString(),
            QString(),
            m_config->debugModeEnabled(),
            m_debugModeCard);
        m_debugModeSwitcher->setObjectName("debugModeSwitcher");

        QVBoxLayout *debugTextLayout = new QVBoxLayout;
        debugTextLayout->setContentsMargins(0, 0, 0, 0);
        debugTextLayout->setSpacing(4);
        debugTextLayout->addWidget(m_debugTitleLabel);
        debugTextLayout->addWidget(m_debugDescriptionLabel);

        QHBoxLayout *debugLayout = new QHBoxLayout(m_debugModeCard);
        debugLayout->setContentsMargins(18, 16, 18, 16);
        debugLayout->setSpacing(16);
        debugLayout->addLayout(debugTextLayout, 1);
        debugLayout->addWidget(m_debugModeSwitcher, 0, Qt::AlignVCenter);

#ifdef Q_OS_MACOS
        m_accessibilityPermissionCard = new QFrame(m_generalWidget);
        m_accessibilityPermissionCard->setObjectName(
            "accessibilityPermissionCard");
        m_accessibilityPermissionCard->setAttribute(
            Qt::WA_StyledBackground, true);

        m_accessibilityPermissionTitleLabel =
            new QLabel(m_accessibilityPermissionCard);
        m_accessibilityPermissionTitleLabel->setObjectName(
            "accessibilityPermissionTitle");
        m_accessibilityPermissionTitleLabel->setStyleSheet(
            "font-weight: 600;");
        m_accessibilityPermissionDescriptionLabel =
            new QLabel(m_accessibilityPermissionCard);
        m_accessibilityPermissionDescriptionLabel->setObjectName(
            "accessibilityPermissionDescription");
        m_accessibilityPermissionDescriptionLabel->setWordWrap(true);

        m_accessibilityPermissionStatusLabel =
            new QLabel(m_accessibilityPermissionCard);
        m_accessibilityPermissionStatusLabel->setObjectName(
            "accessibilityPermissionStatus");
        m_accessibilityPermissionStatusLabel->setAlignment(Qt::AlignCenter);
        m_accessibilityPermissionStatusLabel->setMinimumHeight(28);
        m_accessibilityPermissionStatusLabel->setContentsMargins(10, 0, 10, 0);

        m_requestAccessibilityPermissionButton =
            new SButton(QString(), m_accessibilityPermissionCard);
        m_requestAccessibilityPermissionButton->setObjectName(
            "requestAccessibilityPermissionButton");
        m_requestAccessibilityPermissionButton->setRole(
            SButton::Role::Primary);

        QVBoxLayout *accessibilityPermissionTextLayout = new QVBoxLayout;
        accessibilityPermissionTextLayout->setContentsMargins(0, 0, 0, 0);
        accessibilityPermissionTextLayout->setSpacing(4);
        accessibilityPermissionTextLayout->addWidget(
            m_accessibilityPermissionTitleLabel);
        accessibilityPermissionTextLayout->addWidget(
            m_accessibilityPermissionDescriptionLabel);

        QHBoxLayout *accessibilityPermissionActionLayout = new QHBoxLayout;
        accessibilityPermissionActionLayout->setContentsMargins(0, 0, 0, 0);
        accessibilityPermissionActionLayout->setSpacing(10);
        accessibilityPermissionActionLayout->addWidget(
            m_accessibilityPermissionStatusLabel);
        accessibilityPermissionActionLayout->addWidget(
            m_requestAccessibilityPermissionButton);

        QHBoxLayout *accessibilityPermissionLayout =
            new QHBoxLayout(m_accessibilityPermissionCard);
        accessibilityPermissionLayout->setContentsMargins(18, 16, 18, 16);
        accessibilityPermissionLayout->setSpacing(16);
        accessibilityPermissionLayout->addLayout(
            accessibilityPermissionTextLayout, 1);
        accessibilityPermissionLayout->addLayout(
            accessibilityPermissionActionLayout);

        QObject::connect(
            m_requestAccessibilityPermissionButton,
            &SButton::clicked,
            this,
            [this] {
                MacAccessibilityPermission::request();
                refreshAccessibilityPermission();
                QTimer::singleShot(
                    1000, this, &SSettings::refreshAccessibilityPermission);
            });
#endif

        QVBoxLayout *generalLayout = new QVBoxLayout(m_generalWidget);
        generalLayout->setContentsMargins(24, 24, 24, 24);
        generalLayout->setSpacing(12);
        generalLayout->addWidget(m_generalTitleLabel);
        generalLayout->addWidget(m_generalDescriptionLabel);
        generalLayout->addWidget(m_languageCard);
        generalLayout->addWidget(m_debugModeCard);
#ifdef Q_OS_MACOS
        generalLayout->addWidget(m_accessibilityPermissionCard);
#endif
        generalLayout->addStretch();

        syncLanguageSelection(m_config->languageCode());
        QObject::connect(
            m_languageComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this] (int index) {
                const QString code = m_languageComboBox->itemData(index).toString();
                if (!SLanguageManager::instance()->setLanguage(code))
                {
                    syncLanguageSelection(m_config->languageCode());
                    return;
                }
                m_config->setLanguageCode(code);
                m_config->saveToFile(m_config->configPath());
            });
        QObject::connect(
            m_config,
            &SConfig::languageChanged,
            this,
            [this] (const QString &code) {
                if (SLanguageManager::instance()->setLanguage(code))
                {
                    syncLanguageSelection(code);
                }
            });
        QObject::connect(
            SLanguageManager::instance(),
            &SLanguageManager::languageChanged,
            this,
            [this] (const QString &code) {
                syncLanguageSelection(code);
                retranslateUi();
            });

        const auto updateDebugMode = [this] (bool enabled) {
            m_config->setDebugModeEnabled(enabled);
            m_config->saveToFile(m_config->configPath());
        };
        QObject::connect(m_debugModeSwitcher, &SSwitcher::switchOn, this,
                         [updateDebugMode] {
            updateDebugMode(true);
        });
        QObject::connect(m_debugModeSwitcher, &SSwitcher::switchOff, this,
                         [updateDebugMode] {
            updateDebugMode(false);
        });
        QObject::connect(m_config, &SConfig::debugModeChanged,
                         m_debugModeSwitcher, &SSwitcher::setStatus);
#ifdef Q_OS_MACOS
        QObject::connect(m_config, &SConfig::debugModeChanged,
                         this, [this] {
            refreshAccessibilityPermission();
        });
#endif
    }

    if (QStyleHints *styleHints = QGuiApplication::styleHints())
    {
        QObject::connect(
            styleHints,
            &QStyleHints::colorSchemeChanged,
            this,
            [this] (Qt::ColorScheme scheme) {
                m_pendingColorScheme = scheme;
                refreshTheme(true, scheme);
            });
    }

    // 内容页-插件页
    if (!m_pluginWidget)
    {
        m_pluginWidget = new QWidget(m_contentWidget);
    }
    if (!m_pluginListWidget)
    {
        m_pluginListWidget = new QListWidget(m_pluginWidget);
        m_pluginListWidget->setObjectName("pluginList");
        m_pluginListWidget->setItemAlignment(Qt::AlignVCenter);
        m_pluginListWidget->setDragDropMode(QAbstractItemView::InternalMove);
        m_pluginListWidget->setMinimumHeight(48);
        m_pluginListWidget->setSpacing(4);
        QObject::connect(m_pluginListWidget->model(), &QAbstractItemModel::rowsMoved, this, [this] {
            refreshPluginIndex();
        });
    }
    refreshTheme(true);

    m_newPluginButton = new SButton(QString(), m_pluginListWidget);
    m_newPluginButton->setRole(SButton::Role::Primary);
    QObject::connect(m_newPluginButton, &SButton::clicked,
                     this, &SSettings::onCreatePluginClicked);

    QVBoxLayout *pluginsLayout = new QVBoxLayout(m_pluginWidget);
    pluginsLayout->addWidget(m_pluginListWidget);
    pluginsLayout->addWidget(m_newPluginButton);
    m_pluginWidget->setLayout(pluginsLayout);

    // 内容页-插件编辑器
    if (!m_pluginEditor) 
    {
        m_pluginEditor = SPluginEditor::editor();
    }

    // 内容页-任务管理器
    if (!m_taskWidget)
    {
        m_taskWidget = new QWidget(m_contentWidget);
        m_taskTitleLabel = new QLabel(m_taskWidget);
        m_taskTitleLabel->setStyleSheet("font-size: 20px; font-weight: 600;");
        m_taskDescriptionLabel = new QLabel(m_taskWidget);
        m_taskDescriptionLabel->setWordWrap(true);

        m_taskListWidget = new QListWidget(m_taskWidget);
        m_taskListWidget->setObjectName("pluginTaskList");
        m_taskListWidget->setAlternatingRowColors(false);
        m_taskListWidget->setSpacing(4);
        refreshTheme(true);

        m_emptyTaskLabel = new QLabel(m_taskWidget);
        m_emptyTaskLabel->setAlignment(Qt::AlignCenter);
        m_emptyTaskLabel->setStyleSheet("color: #777777; padding: 24px;");

        QVBoxLayout *taskLayout = new QVBoxLayout(m_taskWidget);
        taskLayout->setContentsMargins(24, 24, 24, 24);
        taskLayout->setSpacing(12);
        taskLayout->addWidget(m_taskTitleLabel);
        taskLayout->addWidget(m_taskDescriptionLabel);
        taskLayout->addWidget(m_emptyTaskLabel);
        taskLayout->addWidget(m_taskListWidget);
        m_taskWidget->setLayout(taskLayout);

        SPluginTaskManager *taskManager = SPluginTaskManager::instance();
        QObject::connect(taskManager, &SPluginTaskManager::taskAdded, this, &SSettings::addTaskItem);
        QObject::connect(taskManager, &SPluginTaskManager::taskRemoved, this, &SSettings::removeTaskItem);
        for (SPluginTask *task : taskManager->activeTasks())
        {
            addTaskItem(task);
        }
        updateTaskEmptyState();
    }

    // 内容页-快捷键
    if (!m_shortcutWidget)
    {
        m_shortcutWidget = new QListWidget(m_contentWidget);
    }
    m_shortcutHelpLabel = new QLabel(m_shortcutWidget);
    m_shortcutHelpLabel->setAlignment(Qt::AlignCenter);
    QListWidgetItem *shortcutItem = new QListWidgetItem(m_shortcutWidget);
    shortcutItem->setSizeHint(QSize(m_shortcutWidget->size().width(), 48));
    m_shortcutWidget->setItemWidget(shortcutItem, m_shortcutHelpLabel);

    // 内容页-关于
    if (!m_aboutWidget)
    {
        m_aboutWidget = new QWidget(m_contentWidget);
    }
    QPixmap aboutPixmap(SUtils::STARRY_ICON(256));
    QLabel *aboutIcon = new QLabel("Starry");
    aboutIcon->setPixmap(aboutPixmap.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    aboutIcon->setFixedSize(256, 256);
    aboutIcon->setAlignment(Qt::AlignCenter);
    m_aboutContentLabel = new QLabel(m_aboutWidget);
    m_aboutContentLabel->setObjectName("aboutContentLabel");
    m_aboutContentLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *aboutLayout = new QVBoxLayout(m_aboutWidget);
    aboutLayout->setAlignment(Qt::AlignCenter);
    aboutLayout->addWidget(aboutIcon);
    aboutLayout->addWidget(m_aboutContentLabel);

    m_aboutWidget->setLayout(aboutLayout);

    // 内容页（右侧）
    m_contentWidget->addWidget(m_generalWidget);
    m_contentWidget->addWidget(m_pluginWidget);
    m_contentWidget->addWidget(m_taskWidget);
    m_contentWidget->addWidget(m_shortcutWidget);
    m_contentWidget->addWidget(m_aboutWidget);
    m_contentWidget->addWidget(m_pluginEditor);
    QObject::connect(m_pluginEditor, &SPluginEditor::editingOpened, this, [this] {
        m_contentWidget->setCurrentWidget(m_pluginEditor);
    });
    QObject::connect(m_pluginEditor, &SPluginEditor::editingFinished, this, [this] {
        m_contentWidget->setCurrentWidget(m_pluginWidget);
    });

    // Layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(m_menuListWidget);
    mainLayout->addWidget(m_contentWidget);

    // 添加 菜单页 项目
    SButton *general = new SButton(QString(), m_menuListWidget);
    SButton *plugins = new SButton(QString(), m_menuListWidget);
    SButton *tasks = new SButton(QString(), m_menuListWidget);
    SButton *shortcuts = new SButton(QString(), m_menuListWidget);
    SButton *about = new SButton(QString(), m_menuListWidget);
    general->setRole(SButton::Role::Navigation);
    plugins->setRole(SButton::Role::Navigation);
    tasks->setRole(SButton::Role::Navigation);
    shortcuts->setRole(SButton::Role::Navigation);
    about->setRole(SButton::Role::Navigation);

    QObject::connect(general, &SButton::clicked, this, [this](){ this->showContent(0); });
    QObject::connect(plugins, &SButton::clicked, this, [this](){ this->showContent(1); });
    QObject::connect(tasks, &SButton::clicked, this, [this](){ this->showContent(2); });
    QObject::connect(shortcuts, &SButton::clicked, this, [this](){ this->showContent(3); });
    QObject::connect(about, &SButton::clicked, this, [this](){ this->showContent(4); });

    addMenuItem(general); // ！！这里添加的顺序同上面showContent(index)内index
    addMenuItem(plugins); // ！！这里添加的顺序同上面showContent(index)内index
    addMenuItem(tasks);
    addMenuItem(shortcuts);
    addMenuItem(about);
    showContent(0);

    // 主界面
    QIcon windowIcon(SUtils::STARRY_ICON(64));
    this->setLayout(mainLayout);
    this->setMinimumSize(800, 600);
    this->setWindowIcon(windowIcon);
    retranslateUi();
    refreshTheme(
        true,
        QGuiApplication::styleHints()
            ? QGuiApplication::styleHints()->colorScheme()
            : Qt::ColorScheme::Unknown);
}

void SSettings::addPluginItem(SPluginItem *item)
{
    SDEBUG
    if (!item)
    {
        return;
    }
    QListWidgetItem *listWidgetItem = new QListWidgetItem(m_pluginListWidget);
    QSize size(m_pluginListWidget->size().width() - 14, 48);
    listWidgetItem->setSizeHint(size);
    m_pluginListWidget->addItem(listWidgetItem);
    m_pluginListWidget->setItemWidget(listWidgetItem, item);
    QObject::connect(item, &SPluginItem::selectionRequested, m_pluginListWidget,
                     [this, listWidgetItem] {
        if (m_pluginListWidget->row(listWidgetItem) >= 0)
        {
            m_pluginListWidget->setCurrentItem(listWidgetItem);
        }
    });
}

void SSettings::addPluginItem(SPluginInfo *info)
{
    SDEBUG
    if (!info)
    {
        return;
    }
    SPluginItem *item = SPluginItem::create(info, this->m_pluginListWidget);
    addPluginItem(item);
    QObject::connect(info, &SPluginInfo::needDelete, [this, info] () {
        this->deletePluginItem(info->pluginItem);
    });
}

void SSettings::deletePluginItem(SPluginItem *item)
{
    SDEBUG
    if (!item)
    {
        return;
    }
    for (int row = 0; row < m_pluginListWidget->count(); ++row)
    {
        QListWidgetItem *listWidgetItem = m_pluginListWidget->item(row);
        if (m_pluginListWidget->itemWidget(listWidgetItem) != item)
        {
            continue;
        }
        m_pluginListWidget->removeItemWidget(listWidgetItem);
        delete m_pluginListWidget->takeItem(row);
        item->deleteLater();
        refreshPluginIndex();
        return;
    }
    qWarning() << "Plugin item to delete was not found";
}

void SSettings::addMenuItem(QWidget *item)
{
    SDEBUG
    QListWidgetItem *listWidgetItem = new QListWidgetItem(m_menuListWidget);
    QSize size(m_menuListWidget->size().width() - 12, 44);
    listWidgetItem->setSizeHint(size);
    m_menuListWidget->addItem(listWidgetItem);
    m_menuListWidget->setItemWidget(listWidgetItem, item);
    if (SButton *button = qobject_cast<SButton *>(item))
    {
        m_menuButtons.push_back(button);
    }
}

void SSettings::showContent(int index)
{
    SDEBUG
    if (index == -1) {
        index = m_menuListWidget->currentRow();
    }
    if (index < 0 || index >= m_contentWidget->count())
    {
        return;
    }
    m_contentWidget->setCurrentIndex(index);
    if (index < m_menuListWidget->count())
    {
        m_menuListWidget->setCurrentRow(index);
    }
    for (qsizetype buttonIndex = 0; buttonIndex < m_menuButtons.size(); ++buttonIndex)
    {
        m_menuButtons.at(buttonIndex)->setSelected(buttonIndex == static_cast<qsizetype>(index));
    }
}

#ifdef Q_OS_MACOS
void SSettings::refreshAccessibilityPermission()
{
    if (!m_accessibilityPermissionStatusLabel
        || !m_requestAccessibilityPermissionButton)
    {
        return;
    }

    const bool granted = MacAccessibilityPermission::isGranted();
    const bool repeatedRequestEnabled =
        granted && m_config->debugModeEnabled();
    m_accessibilityPermissionStatusLabel->setText(
        granted ? tr("Granted") : tr("Not granted"));
    m_accessibilityPermissionStatusLabel->setAccessibleName(
        tr("Accessibility permission status"));
    m_accessibilityPermissionStatusLabel->setAccessibleDescription(
        m_accessibilityPermissionStatusLabel->text());
    m_accessibilityPermissionStatusLabel->setStyleSheet(
        granted
            ? (m_darkStyle
                ? QStringLiteral(
                    "color: #6CE9A6; background: #054F31;"
                    "border: 1px solid #067647; border-radius: 7px;"
                    "font-weight: 600;")
                : QStringLiteral(
                    "color: #067647; background: #ECFDF3;"
                    "border: 1px solid #ABEFC6; border-radius: 7px;"
                    "font-weight: 600;"))
            : (m_darkStyle
                ? QStringLiteral(
                    "color: #FDBA74; background: #7C2D12;"
                    "border: 1px solid #C2410C; border-radius: 7px;"
                    "font-weight: 600;")
                : QStringLiteral(
                    "color: #C2410C; background: #FFF7ED;"
                    "border: 1px solid #FED7AA; border-radius: 7px;"
                    "font-weight: 600;")));
    m_requestAccessibilityPermissionButton->setText(
        repeatedRequestEnabled ? tr("Request again")
                               : tr("Request permission"));
    m_requestAccessibilityPermissionButton->setEnabled(
        !granted || repeatedRequestEnabled);
    m_requestAccessibilityPermissionButton->setVisible(
        !granted || repeatedRequestEnabled);
}
#endif

void SSettings::onCreatePluginClicked()
{
    SDEBUG
    m_pluginEditor->create();
    /* TODO: 当编辑窗口未关闭时，设置窗口不可点击 */
}

/* private functions */

SSettings::SSettings(QWidget *parent)
{
    SDEBUG
    this->setParent(parent);
    m_config = SConfig::config();
    SLanguageManager::instance()->setLanguage(m_config->languageCode());
    initGui();
}

SSettings::~SSettings()
{
    SDEBUG
    m_instance = nullptr;
}

void SSettings::closeEvent(QCloseEvent *ev)
{
    SDEBUG
    refreshPluginIndex();
    emit windowClose();
    m_config->saveToFile(m_config->configPath());
    ev->accept();
}

void SSettings::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (!event)
    {
        return;
    }
    if (event->type() == QEvent::LanguageChange)
    {
        retranslateUi();
    }
#ifdef Q_OS_MACOS
    if (event->type() == QEvent::ActivationChange && isActiveWindow())
    {
        refreshAccessibilityPermission();
    }
#endif
    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::ThemeChange)
    {
        Qt::ColorScheme scheme = m_pendingColorScheme;
        if (event->type() == QEvent::ThemeChange)
        {
            scheme = QGuiApplication::styleHints()->colorScheme();
            m_pendingColorScheme = scheme;
        }
        refreshTheme(true, scheme);

        if (event->type() != QEvent::ThemeChange
            && m_pendingColorScheme != Qt::ColorScheme::Unknown)
        {
            const bool paletteIsDark =
                QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
            const bool pendingIsDark =
                m_pendingColorScheme == Qt::ColorScheme::Dark;
            if (paletteIsDark == pendingIsDark)
            {
                m_pendingColorScheme = Qt::ColorScheme::Unknown;
            }
        }
    }
}

void SSettings::refreshPluginIndex() /* Need to be optmized */
{
    SDEBUG
    for (int i = 0; i < m_pluginListWidget->count(); ++i)
    {
        QListWidgetItem *item = m_pluginListWidget->item(i);
        SPluginItem *pluginItem = (SPluginItem *) m_pluginListWidget->itemWidget(item);
        pluginItem->setIndexToInfo(i);
    }
}

void SSettings::addTaskItem(SPluginTask *task)
{
    if (!task || m_taskItems.contains(task))
    {
        return;
    }

    QListWidgetItem *listItem = new QListWidgetItem;
    listItem->setSizeHint(QSize(m_taskListWidget->width(), 58));
    m_taskListWidget->addItem(listItem);

    QWidget *row = new QWidget(m_taskListWidget);
    row->setObjectName("pluginTaskRow");
    row->setAttribute(Qt::WA_StyledBackground, true);
    row->setStyleSheet(
        "QWidget#pluginTaskRow { background: transparent; border: none; }");
    QLabel *nameLabel = new QLabel(task->pluginName(), row);
    nameLabel->setMinimumWidth(120);
    nameLabel->setStyleSheet("font-weight: 600;");

    QLabel *commandLabel = new QLabel(row);
    commandLabel->setText(QFontMetrics(commandLabel->font()).elidedText(
        task->commandLine(),
        Qt::ElideMiddle,
        280));
    commandLabel->setToolTip(task->commandLine());
    commandLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QLabel *statusLabel = new QLabel(row);
    statusLabel->setObjectName("pluginTaskStatusLabel");
    statusLabel->setMinimumWidth(120);

    SButton *stopButton = new SButton(QString(), row);
    stopButton->setRole(SButton::Role::Danger);
    stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    stopButton->setIconSize(QSize(16, 16));
    stopButton->setObjectName("forceStopTaskButton");
    stopButton->setMinimumWidth(92);

    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(8, 4, 8, 4);
    rowLayout->setSpacing(12);
    rowLayout->addWidget(nameLabel);
    rowLayout->addWidget(commandLabel, 1);
    rowLayout->addWidget(statusLabel);
    rowLayout->addWidget(stopButton);
    row->setLayout(rowLayout);
    m_taskListWidget->setItemWidget(listItem, row);
    m_taskItems.insert(task, listItem);

    retranslateTaskRow(task);
    QObject::connect(task, &SPluginTask::stateChanged, row, [this, task] {
        retranslateTaskRow(task);
    });
    QObject::connect(stopButton, &SButton::clicked, row, [this, task] {
        const QPointer<SPluginTask> guardedTask(task);
        const QMessageBox::StandardButton choice = QMessageBox::question(
            this,
            tr("Confirm force stop"),
            tr("Force stop plugin \"%1\"? Unsaved data in the plugin may be lost.")
                .arg(task->pluginName()),
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (choice == QMessageBox::Yes && guardedTask)
        {
            guardedTask->forceStop();
        }
    });
    updateTaskEmptyState();
}

void SSettings::retranslateTaskRow(SPluginTask *task)
{
    QListWidgetItem *listItem = m_taskItems.value(task, nullptr);
    if (!task || !listItem || !m_taskListWidget)
    {
        return;
    }
    QWidget *row = m_taskListWidget->itemWidget(listItem);
    if (!row)
    {
        return;
    }
    QLabel *statusLabel = row->findChild<QLabel *>("pluginTaskStatusLabel");
    SButton *stopButton = row->findChild<SButton *>("forceStopTaskButton");
    if (!statusLabel || !stopButton)
    {
        return;
    }

    QString status;
    switch (task->state())
    {
    case SPluginTask::State::Starting:
        status = tr("Starting…");
        break;
    case SPluginTask::State::Running:
        status = tr("Running · PID %1").arg(task->processId());
        break;
    case SPluginTask::State::Stopping:
        status = tr("Stopping…");
        break;
    case SPluginTask::State::Finished:
        status = tr("Finished");
        break;
    case SPluginTask::State::Failed:
        status = tr("Failed");
        break;
    case SPluginTask::State::Terminated:
        status = tr("Terminated");
        break;
    }
    statusLabel->setText(status);
    stopButton->setText(tr("Force stop"));
    stopButton->setToolTip(tr("Immediately terminate this plugin process"));
    stopButton->setAccessibleName(
        tr("Force stop plugin") + ' ' + task->pluginName());
    stopButton->setEnabled(
        task->state() == SPluginTask::State::Starting
        || task->state() == SPluginTask::State::Running);
}

void SSettings::removeTaskItem(SPluginTask *task)
{
    QListWidgetItem *listItem = m_taskItems.take(task);
    if (!listItem)
    {
        return;
    }
    const int rowIndex = m_taskListWidget->row(listItem);
    QWidget *rowWidget = m_taskListWidget->itemWidget(listItem);
    m_taskListWidget->removeItemWidget(listItem);
    delete m_taskListWidget->takeItem(rowIndex);
    if (rowWidget)
    {
        rowWidget->deleteLater();
    }
    updateTaskEmptyState();
}

void SSettings::updateTaskEmptyState()
{
    if (!m_taskListWidget || !m_emptyTaskLabel)
    {
        return;
    }
    const bool empty = m_taskItems.isEmpty();
    m_emptyTaskLabel->setVisible(empty);
    m_taskListWidget->setVisible(!empty);
}
