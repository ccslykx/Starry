#include <QClipboard>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QGuiApplication>
#include <QMimeData>
#include <QProcess>
#include <QStyle>
#include <QUrl>

#include "SConfig.h"
#include "SPluginTaskManager.h"
#include "SSelection.h"
#include "SSettings.h"
#include "SPluginEditor.h"
#include "SButton.h"
#include "utils.h"

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
    m_editingInfo = info;
    m_iconPath.clear();
    m_icon = info->icon;

    m_iconContainor->setPixmap(info->icon.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_nameEdit->setText(info->name);
    m_tipEdit->setText(info->tip);
    m_scriptEdit->setText(info->script);
    m_cButton->setVisible(false);
    m_eButton->setVisible(true);

    const QString title = tr("Edit") + ' ' + info->name;
    m_titleLabel->setText(title);
    this->setWindowTitle(title);
    updateValidation();
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
    m_titleLabel->setText(tr("Create New Plugin"));
    this->setWindowTitle(tr("Create New Plugin"));
    updateValidation();
    emit editingOpened();
    if (!parentWidget())
    {
        show();
    }
}

/* private functions */

SPluginEditor::SPluginEditor(QWidget *parent)
    : QWidget(parent)
{
    SDEBUG
    initGui();
}

void SPluginEditor::initGui()
{
    SDEBUG
    if (!m_titleLabel)  m_titleLabel = new QLabel(this);
    if (!m_iconLabel)   m_iconLabel = new QLabel(tr("Icon"), this);
    if (!m_nameLabel)   m_nameLabel = new QLabel(tr("Name"), this);
    if (!m_tipLabel)    m_tipLabel = new QLabel(tr("Tip"), this);
    if (!m_scriptLabel) m_scriptLabel = new QLabel(tr("Script"), this);
    if (!m_validationLabel) m_validationLabel = new QLabel(this);
    if (!m_eButton)     m_eButton = new SButton(tr("Save"), this);
    if (!m_cButton)     m_cButton = new SButton(tr("Create"), this);
    if (!m_testButton)  m_testButton = new SButton(tr("Test run"), this);
    if (!m_cancelButton) m_cancelButton = new SButton(tr("Back"), this);

    if (!m_iconContainor)   m_iconContainor = new SButton("", this);
    if (!m_nameEdit)        m_nameEdit = new QLineEdit(this);
    if (!m_tipEdit)         m_tipEdit = new QLineEdit(this);
    if (!m_scriptEdit)      m_scriptEdit = new QLineEdit(this);

    m_eButton->setRole(SButton::Role::Primary);
    m_cButton->setRole(SButton::Role::Primary);
    m_testButton->setRole(SButton::Role::Secondary);
    m_testButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_testButton->setIconSize(QSize(16, 16));
    m_cancelButton->setRole(SButton::Role::Secondary);
    m_cancelButton->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    m_cancelButton->setIconSize(QSize(16, 16));
    m_iconContainor->setRole(SButton::Role::IconPicker);
    m_iconContainor->setFixedSize(64, 64);
    m_iconContainor->setToolTip(tr("Click to select an image, or drop an image anywhere on this page."));
    m_iconContainor->setAccessibleName(tr("Plugin icon"));
    m_titleLabel->setStyleSheet("font-size: 20px; font-weight: 600;");
    m_validationLabel->setWordWrap(true);

    m_nameEdit->setObjectName("pluginNameEdit");
    m_tipEdit->setObjectName("pluginTipEdit");
    m_scriptEdit->setObjectName("pluginScriptEdit");
    m_eButton->setObjectName("savePluginButton");
    m_cButton->setObjectName("createPluginButton");
    m_testButton->setObjectName("testPluginButton");
    m_cancelButton->setObjectName("cancelPluginButton");
    m_nameEdit->setClearButtonEnabled(true);
    m_tipEdit->setClearButtonEnabled(true);
    m_scriptEdit->setClearButtonEnabled(true);
    m_scriptEdit->setPlaceholderText(tr("Example: open https://example.com?q=$PLAINTEXT"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);
    layout->addWidget(m_titleLabel);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setHorizontalSpacing(16);
    formLayout->setVerticalSpacing(12);
    formLayout->addRow(m_iconLabel, m_iconContainor);
    formLayout->addRow(m_nameLabel, m_nameEdit);
    formLayout->addRow(m_tipLabel, m_tipEdit);
    formLayout->addRow(m_scriptLabel, m_scriptEdit);
    layout->addLayout(formLayout);
    layout->addWidget(m_validationLabel);
    layout->addStretch();

    QHBoxLayout *actionLayout = new QHBoxLayout;
    actionLayout->addWidget(m_cancelButton);
    actionLayout->addStretch();
    actionLayout->addWidget(m_testButton);
    actionLayout->addWidget(m_eButton);
    actionLayout->addWidget(m_cButton);
    layout->addLayout(actionLayout);

    this->setLayout(layout);
    this->setAcceptDrops(true);

    QObject::connect(m_iconContainor, &SButton::clicked, [this] () {
        const QString tmp = QFileDialog::getOpenFileName(
            this,
            tr("Select icon from file"),
            QDir::homePath(),
            tr("Images (*.bmp *.jpeg *.jpg *.png *.webp)"));
        if (tmp.isEmpty()) // Canceled
        {
            return;
        }
        loadIcon(tmp);
    });
    QObject::connect(m_eButton, &SButton::clicked, this, [this]() {
        SPluginInfo *info = this->m_editingInfo.data();
        updateValidation();
        if (!info || !m_eButton->isEnabled())
        {
            return;
        }
        const QString name = this->m_nameEdit->text().trimmed();
        const QString script = this->m_scriptEdit->text().trimmed();
        SConfig *config = SConfig::config();
        if (info->name != name && !config->renamePlugin(info, name))
        {
            showValidation(tr("The plugin could not be renamed."), true);
            return;
        }
        if (!m_iconPath.isEmpty())
        {
            info->icon = this->m_icon;
            emit info->iconChanged(info);
        }

        info->script = script;
        info->tip = this->m_tipEdit->text();
        
        emit info->edited(info);
        emit editingFinished();
        initialize();
    });
    QObject::connect(m_cButton, &SButton::clicked, this, [this]() {
        updateValidation();
        if (!m_cButton->isEnabled())
        {
            return;
        }
        const QString name = this->m_nameEdit->text().trimmed();
        const QString script = this->m_scriptEdit->text().trimmed();
        SPluginInfo *info = new SPluginInfo(name, script, m_icon, 0, this->m_tipEdit->text(), true);
        emit created(info);
        emit editingFinished();
        initialize();
    });
    QObject::connect(m_testButton, &SButton::clicked, this, &SPluginEditor::testCommand);
    QObject::connect(m_cancelButton, &SButton::clicked, this, [this] {
        emit editingFinished();
        initialize();
    });
    QObject::connect(m_nameEdit, &QLineEdit::textChanged, this, &SPluginEditor::updateValidation);
    QObject::connect(m_scriptEdit, &QLineEdit::textChanged, this, &SPluginEditor::updateValidation);

    initialize();
}

void SPluginEditor::initialize()
{
    QPixmap defaultIcon(":/default_icon.png");
    m_icon = defaultIcon;
    m_iconContainor->setPixmap(defaultIcon.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_nameEdit->setText("");
    m_tipEdit->setText("");
    m_scriptEdit->setText("");
    m_iconPath = QString();
    m_editingInfo = nullptr;

    m_eButton->setVisible(false);
    m_cButton->setVisible(true);
    m_titleLabel->setText(tr("Create New Plugin"));

    updateValidation();
    m_nameEdit->setFocus();
}

void SPluginEditor::updateValidation()
{
    const QString name = m_nameEdit->text();
    const QString script = m_scriptEdit->text().trimmed();
    SConfig *config = SConfig::config();
    QString message;

    if (name.trimmed().isEmpty())
    {
        message = tr("Enter a plugin name.");
    }
    else if (!config->isPluginNameValid(name))
    {
        message = tr("The name contains unsupported characters or surrounding spaces.");
    }
    else if (!config->isPluginNameAvailable(name, m_editingInfo.data()))
    {
        message = tr("A plugin with this name already exists.");
    }
    else if (script.isEmpty())
    {
        message = tr("Enter a command to run.");
    }

    const bool valid = message.isEmpty();
    m_eButton->setEnabled(valid && !m_editingInfo.isNull());
    m_cButton->setEnabled(valid && m_editingInfo.isNull());
    m_testButton->setEnabled(!script.isEmpty());
    showValidation(valid ? tr("Ready to save.") : message, !valid);
}

void SPluginEditor::showValidation(const QString &message, bool error)
{
    m_validationLabel->setText(message);
    m_validationLabel->setStyleSheet(error ? "color: #B42318;" : "color: #16794B;");
}

bool SPluginEditor::loadIcon(const QString &path)
{
    const QPixmap icon(path);
    if (icon.isNull())
    {
        showValidation(tr("The dropped file is not a supported image."), true);
        return false;
    }
    m_iconPath = path;
    m_icon = icon;
    m_iconContainor->setPixmap(icon.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    updateValidation();
    return true;
}

void SPluginEditor::testCommand()
{
    QStringList args = QProcess::splitCommand(m_scriptEdit->text().trimmed());
    if (args.isEmpty() || args.constFirst().isEmpty())
    {
        showValidation(tr("Enter a command before testing."), true);
        return;
    }

    if (args.constFirst() == "starry" && args.size() >= 2 && args.at(1) == "copy2clipboard")
    {
        QGuiApplication::clipboard()->setText(SSelection::instance()->selection());
        showValidation(tr("Test succeeded: copied the selected text."), false);
        return;
    }

    const QString command = args.takeFirst();
    for (QString &argument : args)
    {
        if (argument == "$PLAINTEXT")
        {
            argument = SSelection::instance()->selection();
        }
    }
    const QString pluginName = m_nameEdit->text().trimmed().isEmpty()
        ? tr("Plugin test")
        : m_nameEdit->text().trimmed();
    SPluginTask *task = SPluginTaskManager::instance()->startTask(
        pluginName,
        command,
        args,
        m_scriptEdit->text().trimmed());
    if (!task)
    {
        showValidation(tr("Test command failed to start."), true);
        return;
    }
    showValidation(tr("Test command is starting… You can manage it on the Tasks page."), false);
    QObject::connect(task, &SPluginTask::started, this, [this] {
        showValidation(tr("Test command started. You can manage it on the Tasks page."), false);
    });
    QObject::connect(task, &SPluginTask::failed, this, [this] {
        showValidation(tr("Test command failed to start."), true);
    });
}

void SPluginEditor::closeEvent(QCloseEvent *ev)
{
    emit editingFinished();
    initialize();
    ev->accept();
}

void SPluginEditor::dragEnterEvent(QDragEnterEvent *event)
{
    if (event && event->mimeData()->hasUrls() && event->mimeData()->urls().size() == 1
        && event->mimeData()->urls().constFirst().isLocalFile())
    {
        event->acceptProposedAction();
        return;
    }
    QWidget::dragEnterEvent(event);
}

void SPluginEditor::dropEvent(QDropEvent *event)
{
    if (event && event->mimeData()->hasUrls() && event->mimeData()->urls().size() == 1)
    {
        if (loadIcon(event->mimeData()->urls().constFirst().toLocalFile()))
        {
            event->acceptProposedAction();
            return;
        }
    }
    QWidget::dropEvent(event);
}
