#include <QClipboard>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QMessageBox>
#include <QMimeData>
#include <QPalette>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QScrollArea>
#include <QShortcut>
#include <QStyle>
#include <QStyleHints>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>

#include "SButton.h"
#include "SConfig.h"
#include "SPluginCommand.h"
#include "SPluginEditor.h"
#include "SPluginTaskManager.h"
#include "SSelection.h"
#include "utils.h"

namespace
{
constexpr int TIP_MAX_LENGTH = 120;

void refreshPolish(QWidget *widget)
{
    if (!widget)
    {
        return;
    }
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

bool isSingleLocalFile(const QMimeData *mimeData)
{
    return mimeData
        && mimeData->hasUrls()
        && mimeData->urls().size() == 1
        && mimeData->urls().constFirst().isLocalFile();
}

QString scrollBarStyleSheet(bool dark)
{
    const QString handle = dark ? QStringLiteral("#C2410C")
                                : QStringLiteral("#FED7AA");
    const QString hover = dark ? QStringLiteral("#EA580C")
                               : QStringLiteral("#FDBA74");
    const QString pressed = dark ? QStringLiteral("#F97316")
                                 : QStringLiteral("#FB923C");
    return QStringLiteral(
        "QScrollBar:vertical {"
        "  width: 10px; margin: 2px 1px; background: transparent; border: none;"
        "}"
        "QScrollBar:horizontal {"
        "  height: 10px; margin: 1px 2px; background: transparent; border: none;"
        "}"
        "QScrollBar::handle {"
        "  background: %1; border: none; border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical { min-height: 28px; }"
        "QScrollBar::handle:horizontal { min-width: 28px; }"
        "QScrollBar::handle:hover { background: %2; }"
        "QScrollBar::handle:pressed { background: %3; }"
        "QScrollBar::add-line, QScrollBar::sub-line {"
        "  width: 0; height: 0; background: transparent; border: none;"
        "}"
        "QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }"
        "QScrollBar::corner { background: transparent; }")
        .arg(handle, hover, pressed);
}
}

SPluginEditor* SPluginEditor::m_instance = nullptr;

SPluginEditor* SPluginEditor::editor(QWidget *parent)
{
    SDEBUG
    if (!m_instance)
    {
        m_instance = new SPluginEditor(parent);
    }
    return m_instance;
}

void SPluginEditor::edit(SPluginInfo *info)
{
    SDEBUG
    if (!info)
    {
        return;
    }

    ++m_sessionId;
    m_updatingFields = true;
    m_editMode = true;
    m_editingInfo = info;
    m_icon = info->icon;
    m_iconChanged = false;
    m_nameTouched = false;
    m_scriptTouched = false;

    m_nameEdit->setText(info->name);
    m_tipEdit->setText(info->tip);
    m_scriptEdit->setPlainText(info->script);
    updateIconPreview();
    m_resetIconButton->setEnabled(true);
    m_submitButton->setText(tr("Save changes"));
    m_submitButton->setObjectName("savePluginButton");
    m_submitButton->setAccessibleName(tr("Save plugin"));
    m_titleLabel->setText(tr("Edit %1").arg(info->name));
    m_subtitleLabel->setText(tr("Update how this plugin appears and which command it runs."));
    setWindowTitle(tr("Edit %1").arg(info->name));

    m_updatingFields = false;
    setDirty(false);
    hideStatus();
    updateTipCounter();
    updateValidation();
    m_nameEdit->setFocus();

    emit editingOpened();
    if (!parentWidget())
    {
        show();
    }
}

void SPluginEditor::create()
{
    SDEBUG
    initialize();
    emit editingOpened();
    if (!parentWidget())
    {
        show();
    }
}

SPluginEditor::SPluginEditor(QWidget *parent)
    : QWidget(parent)
{
    SDEBUG
    initGui();
}

void SPluginEditor::initGui()
{
    SDEBUG
    m_titleLabel = new QLabel(this);
    m_subtitleLabel = new QLabel(this);
    m_dirtyLabel = new QLabel(tr("Unsaved changes"), this);
    m_iconLabel = new QLabel(tr("Plugin icon"), this);
    m_iconHintLabel = new QLabel(
        tr("Click the preview to choose an image, or drop an image anywhere on this page."),
        this);
    m_nameLabel = new QLabel(tr("Name"), this);
    m_nameErrorLabel = new QLabel(this);
    m_tipLabel = new QLabel(tr("Tip"), this);
    m_tipCounterLabel = new QLabel(this);
    m_scriptLabel = new QLabel(tr("Command"), this);
    m_scriptHelpLabel = new QLabel(
        tr("$PLAINTEXT inserts raw selected text. $URLENCODED safely encodes it for URLs. "
           "Commands run directly, without a shell."),
        this);
    m_scriptErrorLabel = new QLabel(this);
    m_statusLabel = new QLabel(this);

    m_submitButton = new SButton(tr("Create plugin"), this);
    m_testButton = new SButton(tr("Test run"), this);
    m_cancelButton = new SButton(tr("Back"), this);
    m_iconContainor = new SButton("", this);
    m_resetIconButton = new SButton(tr("Use default icon"), this);
    m_insertVariableButton = new SButton(tr("Insert $PLAINTEXT"), this);
    m_insertUrlEncodedButton = new SButton(tr("Insert $URLENCODED"), this);

    m_nameEdit = new QLineEdit(this);
    m_tipEdit = new QLineEdit(this);
    m_scriptEdit = new QPlainTextEdit(this);
    m_formCard = new QFrame(this);
    m_statusWidget = new QFrame(this);
    m_statusProgress = new QProgressBar(m_statusWidget);

    m_formCard->setObjectName("pluginEditorCard");
    m_formCard->setAttribute(Qt::WA_StyledBackground, true);
    m_statusWidget->setObjectName("pluginEditorStatus");
    m_statusWidget->setAttribute(Qt::WA_StyledBackground, true);
    m_dirtyLabel->setObjectName("pluginDirtyIndicator");
    m_nameErrorLabel->setObjectName("pluginNameError");
    m_scriptErrorLabel->setObjectName("pluginScriptError");
    m_tipCounterLabel->setObjectName("pluginTipCounter");
    m_statusLabel->setWordWrap(true);
    m_statusProgress->setRange(0, 0);
    m_statusProgress->setTextVisible(false);
    m_statusProgress->setFixedSize(24, 14);

    for (QLabel *label : {m_iconLabel, m_nameLabel, m_tipLabel, m_scriptLabel})
    {
        label->setProperty("fieldLabel", true);
    }
    for (QLabel *label : {m_iconHintLabel, m_tipCounterLabel, m_scriptHelpLabel})
    {
        label->setProperty("helperLabel", true);
    }
    for (QLabel *label : {m_nameErrorLabel, m_scriptErrorLabel})
    {
        label->setProperty("errorLabel", true);
        label->setWordWrap(true);
        label->setVisible(false);
    }

    m_submitButton->setRole(SButton::Role::Primary);
    m_submitButton->setObjectName("createPluginButton");
    m_testButton->setRole(SButton::Role::Secondary);
    m_testButton->setObjectName("testPluginButton");
    m_testButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_testButton->setIconSize(QSize(16, 16));
    m_cancelButton->setRole(SButton::Role::Secondary);
    m_cancelButton->setObjectName("cancelPluginButton");
    m_cancelButton->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    m_cancelButton->setIconSize(QSize(16, 16));
    m_resetIconButton->setRole(SButton::Role::Secondary);
    m_resetIconButton->setObjectName("resetPluginIconButton");
    m_insertVariableButton->setRole(SButton::Role::Secondary);
    m_insertVariableButton->setObjectName("insertPlaintextButton");
    m_insertUrlEncodedButton->setRole(SButton::Role::Secondary);
    m_insertUrlEncodedButton->setObjectName("insertUrlEncodedButton");

    m_iconContainor->setRole(SButton::Role::IconPicker);
    m_iconContainor->setObjectName("pluginIconPicker");
    m_iconContainor->setProperty("dragActive", false);
    m_iconContainor->setFixedSize(96, 96);
    m_iconContainor->setToolTip(tr("Select an image or drop one on this page."));
    m_iconContainor->setAccessibleName(tr("Plugin icon"));
    m_iconHintLabel->setWordWrap(true);

    m_nameEdit->setObjectName("pluginNameEdit");
    m_tipEdit->setObjectName("pluginTipEdit");
    m_scriptEdit->setObjectName("pluginScriptEdit");
    m_nameEdit->setClearButtonEnabled(true);
    m_tipEdit->setClearButtonEnabled(true);
    m_nameEdit->setMinimumHeight(40);
    m_tipEdit->setMinimumHeight(40);
    m_nameEdit->setMinimumWidth(0);
    m_tipEdit->setMinimumWidth(0);
    m_nameEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_tipEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_tipEdit->setMaxLength(TIP_MAX_LENGTH);
    m_nameEdit->setPlaceholderText(tr("Example: Search the web"));
    m_tipEdit->setPlaceholderText(tr("Short description shown in the popup"));
    m_scriptEdit->setPlaceholderText(
        tr("Example: open https://example.com?q=$URLENCODED"));
    QFont commandFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    const QFont inputFont = m_nameEdit->font();
    if (inputFont.pointSizeF() > 0.0)
    {
        commandFont.setPointSizeF(inputFont.pointSizeF());
    }
    else if (inputFont.pixelSize() > 0)
    {
        commandFont.setPixelSize(inputFont.pixelSize());
    }
    m_scriptEdit->setFont(commandFont);
    m_scriptEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_scriptEdit->setTabChangesFocus(true);
    m_scriptEdit->setMinimumHeight(112);
    m_scriptEdit->setMaximumHeight(160);
    m_scriptEdit->setMinimumWidth(0);
    m_scriptEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_scriptEdit->setAccessibleName(tr("Plugin command"));
    m_iconHintLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_scriptHelpLabel->setWordWrap(true);
    m_scriptHelpLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_formCard->setMinimumWidth(0);
    m_formCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QHBoxLayout *headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(12);
    headerLayout->addWidget(m_cancelButton, 0, Qt::AlignTop);

    QVBoxLayout *headingLayout = new QVBoxLayout;
    headingLayout->setContentsMargins(0, 0, 0, 0);
    headingLayout->setSpacing(4);
    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(10);
    titleLayout->addWidget(m_titleLabel);
    titleLayout->addWidget(m_dirtyLabel);
    titleLayout->addStretch();
    headingLayout->addLayout(titleLayout);
    headingLayout->addWidget(m_subtitleLabel);
    headerLayout->addLayout(headingLayout, 1);

    QVBoxLayout *formLayout = new QVBoxLayout(m_formCard);
    formLayout->setContentsMargins(22, 22, 22, 22);
    formLayout->setSpacing(8);

    formLayout->addWidget(m_iconLabel);
    QHBoxLayout *iconLayout = new QHBoxLayout;
    iconLayout->setContentsMargins(0, 0, 0, 8);
    iconLayout->setSpacing(16);
    iconLayout->addWidget(m_iconContainor);
    QVBoxLayout *iconActionsLayout = new QVBoxLayout;
    iconActionsLayout->setContentsMargins(0, 0, 0, 0);
    iconActionsLayout->setSpacing(8);
    iconActionsLayout->addWidget(m_iconHintLabel);
    iconActionsLayout->addWidget(m_resetIconButton, 0, Qt::AlignLeft);
    iconActionsLayout->addStretch();
    iconLayout->addLayout(iconActionsLayout, 1);
    formLayout->addLayout(iconLayout);

    formLayout->addWidget(m_nameLabel);
    formLayout->addWidget(m_nameEdit);
    formLayout->addWidget(m_nameErrorLabel);
    formLayout->addSpacing(4);

    formLayout->addWidget(m_tipLabel);
    formLayout->addWidget(m_tipEdit);
    QHBoxLayout *tipMetaLayout = new QHBoxLayout;
    tipMetaLayout->setContentsMargins(0, 0, 0, 4);
    tipMetaLayout->addStretch();
    tipMetaLayout->addWidget(m_tipCounterLabel);
    formLayout->addLayout(tipMetaLayout);

    formLayout->addWidget(m_scriptLabel);
    formLayout->addWidget(m_scriptEdit);
    QHBoxLayout *scriptMetaLayout = new QHBoxLayout;
    scriptMetaLayout->setContentsMargins(0, 0, 0, 0);
    scriptMetaLayout->setSpacing(12);
    scriptMetaLayout->addWidget(m_scriptHelpLabel, 1);
    scriptMetaLayout->addWidget(m_insertVariableButton);
    scriptMetaLayout->addWidget(m_insertUrlEncodedButton);
    formLayout->addLayout(scriptMetaLayout);
    formLayout->addWidget(m_scriptErrorLabel);

    QHBoxLayout *statusLayout = new QHBoxLayout(m_statusWidget);
    statusLayout->setContentsMargins(12, 9, 12, 9);
    statusLayout->setSpacing(8);
    statusLayout->addWidget(m_statusProgress);
    statusLayout->addWidget(m_statusLabel, 1);

    QHBoxLayout *actionLayout = new QHBoxLayout;
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(10);
    actionLayout->addStretch();
    actionLayout->addWidget(m_testButton);
    actionLayout->addWidget(m_submitButton);

    QWidget *contentWidget = new QWidget(this);
    contentWidget->setObjectName("pluginEditorContent");
    contentWidget->setMinimumWidth(0);
    contentWidget->setMaximumWidth(700);
    m_formScrollArea = new QScrollArea(contentWidget);
    m_formScrollArea->setObjectName("pluginEditorFormScrollArea");
    m_formScrollArea->setFrameShape(QFrame::NoFrame);
    m_formScrollArea->setWidgetResizable(true);
    m_formScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_formScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_formScrollArea->viewport()->setAutoFillBackground(false);
    m_formScrollArea->setWidget(m_formCard);

    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(16);
    contentLayout->addLayout(headerLayout);
    contentLayout->addWidget(m_formScrollArea, 1);
    contentLayout->addWidget(m_statusWidget);
    contentLayout->addLayout(actionLayout);

    QHBoxLayout *centerLayout = new QHBoxLayout;
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->addStretch();
    centerLayout->addWidget(contentWidget, 1);
    centerLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->addLayout(centerLayout, 1);
    setLayout(mainLayout);
    setAcceptDrops(true);

    QObject::connect(m_iconContainor, &SButton::clicked, this, [this] {
        const QString path = QFileDialog::getOpenFileName(
            this,
            tr("Select icon from file"),
            QDir::homePath(),
            tr("Images (*.bmp *.jpeg *.jpg *.png *.webp)"));
        if (!path.isEmpty())
        {
            loadIcon(path);
        }
    });
    QObject::connect(m_resetIconButton, &SButton::clicked, this, &SPluginEditor::resetIcon);
    QObject::connect(m_insertVariableButton, &SButton::clicked, this, [this] {
        QTextCursor cursor = m_scriptEdit->textCursor();
        cursor.insertText(QStringLiteral("$PLAINTEXT"));
        m_scriptEdit->setTextCursor(cursor);
        m_scriptEdit->setFocus();
    });
    QObject::connect(m_insertUrlEncodedButton, &SButton::clicked, this, [this] {
        QTextCursor cursor = m_scriptEdit->textCursor();
        cursor.insertText(QStringLiteral("$URLENCODED"));
        m_scriptEdit->setTextCursor(cursor);
        m_scriptEdit->setFocus();
    });
    QObject::connect(m_submitButton, &SButton::clicked, this, &SPluginEditor::submit);
    QObject::connect(m_testButton, &SButton::clicked, this, &SPluginEditor::testCommand);
    QObject::connect(m_cancelButton, &SButton::clicked, this, &SPluginEditor::requestBack);

    QObject::connect(m_nameEdit, &QLineEdit::textChanged, this, [this] {
        if (!m_updatingFields)
        {
            m_nameTouched = true;
            setDirty(true);
        }
        updateValidation();
    });
    QObject::connect(m_tipEdit, &QLineEdit::textChanged, this, [this] {
        updateTipCounter();
        if (!m_updatingFields)
        {
            setDirty(true);
        }
        updateValidation();
    });
    QObject::connect(m_scriptEdit, &QPlainTextEdit::textChanged, this, [this] {
        if (!m_updatingFields)
        {
            m_scriptTouched = true;
            setDirty(true);
            hideStatus();
        }
        updateValidation();
    });

    QShortcut *saveShortcut = new QShortcut(QKeySequence::Save, this);
    QObject::connect(saveShortcut, &QShortcut::activated, this, [this] {
        if (m_submitButton->isVisible() && m_submitButton->isEnabled())
        {
            submit();
        }
    });
    QShortcut *backShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    QObject::connect(backShortcut, &QShortcut::activated, this, &SPluginEditor::requestBack);

    if (QStyleHints *styleHints = QGuiApplication::styleHints())
    {
        QObject::connect(styleHints, &QStyleHints::colorSchemeChanged, this,
                         [this] (Qt::ColorScheme scheme) {
            m_pendingColorScheme = scheme;
            refreshTheme(true, scheme);
            QTimer::singleShot(0, this, [this, scheme] {
                if (m_pendingColorScheme == scheme)
                {
                    refreshTheme(true, scheme);
                }
            });
        });
    }

    refreshTheme(true);
    initialize();
}

void SPluginEditor::initialize()
{
    ++m_sessionId;
    m_updatingFields = true;
    m_editMode = false;
    m_editingInfo = nullptr;
    m_iconChanged = false;
    m_nameTouched = false;
    m_scriptTouched = false;

    m_icon = QPixmap(QStringLiteral(":/default_icon.png"));
    m_nameEdit->clear();
    m_tipEdit->clear();
    m_scriptEdit->clear();
    updateIconPreview();
    m_resetIconButton->setEnabled(false);
    m_submitButton->setText(tr("Create plugin"));
    m_submitButton->setObjectName("createPluginButton");
    m_submitButton->setAccessibleName(tr("Create plugin"));
    m_titleLabel->setText(tr("Create New Plugin"));
    m_subtitleLabel->setText(tr("Choose an icon, describe the plugin, and define the command to run."));
    setWindowTitle(tr("Create New Plugin"));

    m_updatingFields = false;
    setDirty(false);
    hideStatus();
    updateTipCounter();
    updateValidation();
    m_nameEdit->setFocus();
}

void SPluginEditor::submit()
{
    updateValidation();
    if (!m_submitButton->isEnabled())
    {
        return;
    }

    const QString name = m_nameEdit->text().trimmed();
    const QString script = m_scriptEdit->toPlainText().trimmed();
    const QString tip = m_tipEdit->text();

    if (m_editMode)
    {
        SPluginInfo *info = m_editingInfo.data();
        if (!info)
        {
            showStatus(tr("The plugin is no longer available."), StatusKind::Error);
            updateValidation();
            return;
        }

        SConfig *config = SConfig::config();
        if (info->name != name && !config->renamePlugin(info, name))
        {
            setFieldError(m_nameEdit, m_nameErrorLabel,
                          tr("The plugin could not be renamed."));
            return;
        }
        if (m_iconChanged)
        {
            info->icon = m_icon;
            emit info->iconChanged(info);
        }
        info->script = script;
        info->tip = tip;
        emit info->edited(info);
    }
    else
    {
        SPluginInfo *info = new SPluginInfo(name, script, m_icon, 0, tip, true);
        emit created(info);
    }

    setDirty(false);
    finishEditing();
}

void SPluginEditor::requestBack()
{
    if (!confirmDiscardChanges())
    {
        return;
    }
    finishEditing();
}

void SPluginEditor::finishEditing()
{
    initialize();
    emit editingFinished();
}

bool SPluginEditor::confirmDiscardChanges()
{
    if (!m_dirty)
    {
        return true;
    }

    return QMessageBox::question(
        this,
        tr("Discard changes?"),
        tr("Your unsaved plugin changes will be lost."),
        QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Cancel) == QMessageBox::Discard;
}

void SPluginEditor::updateValidation()
{
    const QString name = m_nameEdit->text();
    const QString script = m_scriptEdit->toPlainText().trimmed();
    SConfig *config = SConfig::config();
    QString nameError;
    QString scriptError;

    if (name.trimmed().isEmpty())
    {
        nameError = tr("Enter a plugin name.");
    }
    else if (!config->isPluginNameValid(name))
    {
        nameError = tr("The name contains unsupported characters or surrounding spaces.");
    }
    else if (!config->isPluginNameAvailable(name, m_editingInfo.data()))
    {
        nameError = tr("A plugin with this name already exists.");
    }

    if (script.isEmpty())
    {
        scriptError = tr("Enter a command to run.");
    }
    else
    {
        SPluginCommand command;
        QString commandError;
        if (!SPluginCommand::parse(script, QString(), &command, &commandError))
        {
            scriptError = commandError;
        }
    }

    setFieldError(m_nameEdit, m_nameErrorLabel,
                  m_nameTouched ? nameError : QString());
    setFieldError(m_scriptEdit, m_scriptErrorLabel,
                  m_scriptTouched ? scriptError : QString());

    const bool valid = nameError.isEmpty() && scriptError.isEmpty();
    const bool targetAvailable = !m_editMode || !m_editingInfo.isNull();
    const bool changedEnough = !m_editMode || m_dirty;
    m_submitButton->setEnabled(valid && targetAvailable && changedEnough);
    m_testButton->setEnabled(!script.isEmpty() && !m_testStarting);
}

void SPluginEditor::setFieldError(QWidget *field, QLabel *label, const QString &message)
{
    const bool error = !message.isEmpty();
    field->setProperty("validationError", error);
    label->setText(message);
    label->setVisible(error);
    refreshPolish(field);
}

void SPluginEditor::setDirty(bool dirty)
{
    m_dirty = dirty;
    m_dirtyLabel->setVisible(dirty);
}

void SPluginEditor::updateTipCounter()
{
    m_tipCounterLabel->setText(
        tr("%1 / %2").arg(m_tipEdit->text().size()).arg(TIP_MAX_LENGTH));
}

void SPluginEditor::showStatus(const QString &message, StatusKind kind)
{
    if (kind == StatusKind::Hidden)
    {
        hideStatus();
        return;
    }

    QString statusName;
    switch (kind)
    {
    case StatusKind::Loading:
        statusName = QStringLiteral("loading");
        break;
    case StatusKind::Success:
        statusName = QStringLiteral("success");
        break;
    case StatusKind::Error:
        statusName = QStringLiteral("error");
        break;
    case StatusKind::Hidden:
        break;
    }

    m_testStarting = kind == StatusKind::Loading;
    m_statusWidget->setProperty("statusKind", statusName);
    m_statusLabel->setText(message);
    m_statusProgress->setVisible(m_testStarting);
    m_statusWidget->setVisible(true);
    refreshPolish(m_statusWidget);
    updateValidation();
}

void SPluginEditor::hideStatus()
{
    m_testStarting = false;
    m_statusWidget->setVisible(false);
}

bool SPluginEditor::loadIcon(const QString &path)
{
    const QPixmap icon(path);
    if (icon.isNull())
    {
        showStatus(tr("The selected file is not a supported image."), StatusKind::Error);
        return false;
    }

    m_icon = icon;
    m_iconChanged = true;
    updateIconPreview();
    m_resetIconButton->setEnabled(true);
    setDirty(true);
    updateValidation();
    return true;
}

void SPluginEditor::resetIcon()
{
    m_icon = QPixmap(QStringLiteral(":/default_icon.png"));
    m_iconChanged = true;
    updateIconPreview();
    m_resetIconButton->setEnabled(false);
    setDirty(true);
    updateValidation();
}

void SPluginEditor::updateIconPreview()
{
    m_iconContainor->setPixmap(
        m_icon.scaled(76, 76, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void SPluginEditor::testCommand()
{
    const QString selectedText = SSelection::instance()->selection();
    SPluginCommand command;
    QString commandError;
    if (!SPluginCommand::parse(
            m_scriptEdit->toPlainText(),
            selectedText,
            &command,
            &commandError))
    {
        m_scriptTouched = true;
        updateValidation();
        showStatus(commandError, StatusKind::Error);
        return;
    }

    if (SConfig::config()->debugModeEnabled())
    {
        qDebug() << "Plugin test selected text:" << selectedText;
        qDebug() << "Expanded plugin test command:"
                 << command.program << command.arguments;
    }

    if (command.isCopyToClipboardCommand())
    {
        QGuiApplication::clipboard()->setText(selectedText);
        showStatus(tr("Test succeeded: copied the selected text."), StatusKind::Success);
        return;
    }

    const QString pluginName = m_nameEdit->text().trimmed().isEmpty()
        ? tr("Plugin test")
        : m_nameEdit->text().trimmed();
    showStatus(tr("Starting the test command…"), StatusKind::Loading);
    SPluginTask *task = SPluginTaskManager::instance()->startTask(
        pluginName,
        command.program,
        command.arguments,
        m_scriptEdit->toPlainText().trimmed());
    if (!task)
    {
        showStatus(tr("Test command failed to start."), StatusKind::Error);
        return;
    }

    const quint64 sessionId = m_sessionId;
    QObject::connect(task, &SPluginTask::started, this, [this, sessionId] {
        if (sessionId == m_sessionId)
        {
            showStatus(tr("Test command started. You can manage it on the Tasks page."),
                       StatusKind::Success);
        }
    });
    QObject::connect(task, &SPluginTask::failed, this, [this, sessionId] {
        if (sessionId == m_sessionId)
        {
            showStatus(tr("Test command failed to start."), StatusKind::Error);
        }
    });
    QObject::connect(
        task,
        &SPluginTask::finished,
        this,
        [this, sessionId, task] (
            SPluginTask *,
            int exitCode,
            QProcess::ExitStatus exitStatus) {
            if (sessionId != m_sessionId
                || (exitStatus == QProcess::NormalExit && exitCode == 0))
            {
                return;
            }
            if (task->state() == SPluginTask::State::Terminated)
            {
                showStatus(tr("Test command was stopped."), StatusKind::Error);
            }
            else if (exitStatus == QProcess::CrashExit)
            {
                showStatus(tr("Test command crashed."), StatusKind::Error);
            }
            else
            {
                showStatus(
                    tr("Test command exited with code %1.").arg(exitCode),
                    StatusKind::Error);
            }
        });
}

void SPluginEditor::refreshTheme(bool force, Qt::ColorScheme scheme)
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
    const QPalette applicationPalette = QGuiApplication::palette();

    m_titleLabel->setStyleSheet(dark
        ? QStringLiteral("font-size: 24px; font-weight: 650; color: #FFFFFF;")
        : QStringLiteral("font-size: 24px; font-weight: 650; color: #101828;"));
    m_subtitleLabel->setStyleSheet(dark
        ? QStringLiteral("color: #98A2B3;")
        : QStringLiteral("color: #667085;"));
    m_dirtyLabel->setStyleSheet(dark
        ? QStringLiteral(
            "color: #FFF7ED; background: #9A3412; border: 1px solid #F97316;"
            "border-radius: 8px; padding: 2px 8px; font-size: 12px;")
        : QStringLiteral(
            "color: #C2410C; background: #FFF7ED; border: 1px solid #FED7AA;"
            "border-radius: 8px; padding: 2px 8px; font-size: 12px;"));

    m_formCard->setStyleSheet(dark
        ? QStringLiteral(
            "QFrame#pluginEditorCard {"
            "  background: #1D2939; border: 1px solid #344054; border-radius: 12px;"
            "}"
            "QFrame#pluginEditorCard QLabel { background: transparent; border: none; }"
            "QFrame#pluginEditorCard QLabel[fieldLabel=\"true\"] {"
            "  color: #F2F4F7; font-weight: 600;"
            "}"
            "QFrame#pluginEditorCard QLabel[helperLabel=\"true\"] { color: #98A2B3; }"
            "QFrame#pluginEditorCard QLabel[errorLabel=\"true\"] { color: #FDA29B; }"
            "QFrame#pluginEditorCard QLineEdit,"
            "QFrame#pluginEditorCard QPlainTextEdit {"
            "  color: palette(text); background-color: palette(base);"
            "  border: 1px solid #475467; border-radius: 8px; padding: 0 12px;"
            "  selection-background-color: palette(highlight);"
            "}"
            "QFrame#pluginEditorCard QPlainTextEdit { padding: 10px 12px; }"
            "QFrame#pluginEditorCard QLineEdit:focus,"
            "QFrame#pluginEditorCard QPlainTextEdit:focus { border: 2px solid #FB923C; }"
            "QFrame#pluginEditorCard QLineEdit[validationError=\"true\"],"
            "QFrame#pluginEditorCard QPlainTextEdit[validationError=\"true\"] {"
            "  border: 2px solid #F97066; background-color: palette(base);"
            "}")
        : QStringLiteral(
            "QFrame#pluginEditorCard {"
            "  background: #FFFFFF; border: 1px solid #EAECF0; border-radius: 12px;"
            "}"
            "QFrame#pluginEditorCard QLabel { background: transparent; border: none; }"
            "QFrame#pluginEditorCard QLabel[fieldLabel=\"true\"] {"
            "  color: #344054; font-weight: 600;"
            "}"
            "QFrame#pluginEditorCard QLabel[helperLabel=\"true\"] { color: #667085; }"
            "QFrame#pluginEditorCard QLabel[errorLabel=\"true\"] { color: #B42318; }"
            "QFrame#pluginEditorCard QLineEdit,"
            "QFrame#pluginEditorCard QPlainTextEdit {"
            "  color: palette(text); background-color: palette(base);"
            "  border: 1px solid #D0D5DD; border-radius: 8px; padding: 0 12px;"
            "  selection-background-color: palette(highlight);"
            "}"
            "QFrame#pluginEditorCard QPlainTextEdit { padding: 10px 12px; }"
            "QFrame#pluginEditorCard QLineEdit:focus,"
            "QFrame#pluginEditorCard QPlainTextEdit:focus { border: 2px solid #F97316; }"
            "QFrame#pluginEditorCard QLineEdit[validationError=\"true\"],"
            "QFrame#pluginEditorCard QPlainTextEdit[validationError=\"true\"] {"
            "  border: 2px solid #D92D20; background-color: palette(base);"
            "}"));
    m_formCard->setStyleSheet(
        m_formCard->styleSheet() + scrollBarStyleSheet(dark));

    m_formScrollArea->setStyleSheet(
        QStringLiteral(
            "QScrollArea#pluginEditorFormScrollArea {"
            "  background: transparent; border: none;"
            "}")
        + scrollBarStyleSheet(dark));

    const QPalette inputPalette = applicationPalette;
    m_nameEdit->setPalette(inputPalette);
    m_tipEdit->setPalette(inputPalette);
    m_scriptEdit->setPalette(inputPalette);
    m_scriptEdit->viewport()->setPalette(inputPalette);
    m_scriptEdit->viewport()->setAutoFillBackground(true);
    m_testButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_cancelButton->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));

    m_statusWidget->setStyleSheet(dark
        ? QStringLiteral(
            "QFrame#pluginEditorStatus { border-radius: 8px; }"
            "QFrame#pluginEditorStatus[statusKind=\"loading\"] {"
            "  background: #7C2D12; border: 1px solid #F97316;"
            "}"
            "QFrame#pluginEditorStatus[statusKind=\"success\"] {"
            "  background: #18472E; border: 1px solid #32D583;"
            "}"
            "QFrame#pluginEditorStatus[statusKind=\"error\"] {"
            "  background: #551F24; border: 1px solid #F97066;"
            "}"
            "QFrame#pluginEditorStatus QLabel { color: #FFFFFF; border: none; }"
            "QFrame#pluginEditorStatus QProgressBar {"
            "  background: rgba(255, 255, 255, 45); border: none; border-radius: 7px;"
            "}"
            "QFrame#pluginEditorStatus QProgressBar::chunk {"
            "  background: #FDBA74; border-radius: 7px;"
            "}")
        : QStringLiteral(
            "QFrame#pluginEditorStatus { border-radius: 8px; }"
            "QFrame#pluginEditorStatus[statusKind=\"loading\"] {"
            "  background: #FFF7ED; border: 1px solid #FED7AA;"
            "}"
            "QFrame#pluginEditorStatus[statusKind=\"success\"] {"
            "  background: #ECFDF3; border: 1px solid #ABEFC6;"
            "}"
            "QFrame#pluginEditorStatus[statusKind=\"error\"] {"
            "  background: #FEF3F2; border: 1px solid #FECDCA;"
            "}"
            "QFrame#pluginEditorStatus[statusKind=\"loading\"] QLabel { color: #C2410C; }"
            "QFrame#pluginEditorStatus[statusKind=\"success\"] QLabel { color: #067647; }"
            "QFrame#pluginEditorStatus[statusKind=\"error\"] QLabel { color: #B42318; }"
            "QFrame#pluginEditorStatus QLabel { border: none; }"
            "QFrame#pluginEditorStatus QProgressBar {"
            "  background: rgba(249, 115, 22, 45); border: none; border-radius: 7px;"
            "}"
            "QFrame#pluginEditorStatus QProgressBar::chunk {"
            "  background: #F97316; border-radius: 7px;"
            "}"));

    if (QLayout *cardLayout = m_formCard->layout())
    {
        cardLayout->activate();
        m_formCard->setMinimumHeight(cardLayout->sizeHint().height());
    }
}

void SPluginEditor::setDragActive(bool active)
{
    if (m_dragActive == active)
    {
        return;
    }
    m_dragActive = active;
    m_iconContainor->setProperty("dragActive", active);
    refreshPolish(m_iconContainor);
}

void SPluginEditor::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event && (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::ThemeChange))
    {
        Qt::ColorScheme scheme = m_pendingColorScheme;
        if (event->type() == QEvent::ThemeChange)
        {
            scheme = QGuiApplication::styleHints()->colorScheme();
            m_pendingColorScheme = scheme;
        }
        // Palette updates may arrive after colorSchemeChanged. Always reapply the
        // input palettes even when the light/dark classification did not change.
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

void SPluginEditor::closeEvent(QCloseEvent *event)
{
    if (!event)
    {
        return;
    }
    if (!confirmDiscardChanges())
    {
        event->ignore();
        return;
    }
    finishEditing();
    event->accept();
}

void SPluginEditor::dragEnterEvent(QDragEnterEvent *event)
{
    if (event && isSingleLocalFile(event->mimeData()))
    {
        setDragActive(true);
        event->acceptProposedAction();
        return;
    }
    QWidget::dragEnterEvent(event);
}

void SPluginEditor::dragLeaveEvent(QDragLeaveEvent *event)
{
    setDragActive(false);
    QWidget::dragLeaveEvent(event);
}

void SPluginEditor::dropEvent(QDropEvent *event)
{
    setDragActive(false);
    if (event && isSingleLocalFile(event->mimeData())
        && loadIcon(event->mimeData()->urls().constFirst().toLocalFile()))
    {
        event->acceptProposedAction();
        return;
    }
    QWidget::dropEvent(event);
}
