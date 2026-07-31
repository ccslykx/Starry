#include <QGuiApplication>
#include <QClipboard>
#include <QFontMetrics>
#include <QHBoxLayout>

#include "SConfig.h"
#include "SPluginCommand.h"
#include "SPluginTaskManager.h"
#include "SPopupItem.h"
#include "SSelection.h"
#include "utils.h"

SPopupItem* SPopupItem::create(SPluginInfo *info, QWidget *parent)
{
    SDEBUG
    return new SPopupItem(info, parent);
}

void SPopupItem::remove(SPopupItem *item)
{
    SDEBUG
    if (item)
    {
        item->deleteLater();
    }
}

SPluginInfo* SPopupItem::pluginInfo()
{
    return m_info;
}

bool SPopupItem::enabled() const
{
    return !(!m_info->iconEnabled && !m_info->nameEnabled);
}

void SPopupItem::exec()
{
    SDEBUG
    if (m_executionLocked)
    {
        return;
    }
    m_executionLocked = true;
    emit executionStarted(tr("Starting…"));

    const QString selectedText = SSelection::instance()->selection();
    SPluginCommand command;
    QString commandError;
    if (!SPluginCommand::parse(
            m_info->script,
            selectedText,
            &command,
            &commandError))
    {
        qWarning() << "Invalid plugin command:" << commandError;
        m_executionLocked = false;
        emit executionFinished(false, tr("Invalid command"));
        return;
    }

    if (SConfig::config()->debugModeEnabled())
    {
        qDebug() << "Plugin selected text:" << selectedText;
        qDebug() << "Expanded plugin command:"
                 << command.program << command.arguments;
    }

    if (command.isCopyToClipboardCommand())
    {
        QGuiApplication::clipboard()->setText(selectedText);
        m_executionLocked = false;
        emit executionFinished(true, tr("Copied"));
        return;
    }

    SPluginTask *task = SPluginTaskManager::instance()->startTask(
        m_info->name,
        command.program,
        command.arguments,
        m_info->script);
    if (!task)
    {
        m_executionLocked = false;
        emit executionFinished(false, tr("Failed to start"));
        return;
    }

    QObject::connect(task, &SPluginTask::started, this, [this] {
        m_executionLocked = false;
        emit executionFinished(true, tr("Started"));
    });
    QObject::connect(task, &SPluginTask::failed, this, [this] (SPluginTask *, const QString &reason) {
        qWarning() << "Plugin failed to start:" << reason;
        m_executionLocked = false;
        emit executionFinished(false, tr("Failed to start"));
    });
    QObject::connect(
        task,
        &SPluginTask::finished,
        this,
        [this, task] (
            SPluginTask *,
            int exitCode,
            QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::NormalExit && exitCode == 0)
        {
            return;
        }
        m_executionLocked = false;
        if (task->state() == SPluginTask::State::Terminated)
        {
            emit executionFinished(false, tr("Stopped"));
        }
        else if (exitStatus == QProcess::CrashExit)
        {
            emit executionFinished(false, tr("Process crashed"));
        }
        else
        {
            emit executionFinished(
                false,
                tr("Failed (exit code %1)").arg(exitCode));
        }
    });
}

/* private functions */

SPopupItem::SPopupItem(SPluginInfo *info, QWidget *parent)
    : m_info(info)
{
    this->setParent(parent);
    m_info->popupItem = this;

    initGui();
}

SPopupItem::~SPopupItem()
{
    SDEBUG
}

void SPopupItem::initGui()
{
    m_iconLabel = new QLabel(this);
    m_iconLabel->setPixmap(m_info->icon.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_iconLabel->setMinimumSize(24, 24);
    
    QFont nameFont;
    nameFont.setPointSize(12);
    m_nameLabel = new QLabel(this);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setMinimumSize(24, 24);
    m_nameLabel->setMaximumWidth(160);
    m_nameLabel->setFont(nameFont);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 3, 4, 3);
    layout->setSpacing(2);
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_nameLabel);

    this->setLayout(layout);
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setContentsMargins(0, 0, 0, 0);
    this->setWindowOpacity(0.6);
    this->setStyleSheet("background-color: rgba(220, 220, 220, 144);"
                        "border-radius: 8px;"
                        "color: #000000");
    this->setCursor(Qt::PointingHandCursor);
    this->setFocusPolicy(Qt::StrongFocus);
    
    m_iconLabel->setVisible(m_info->iconEnabled);
    m_nameLabel->setVisible(m_info->nameEnabled);
    this->setVisible(enabled());
    refreshText();
    this->adjustSize();

    QObject::connect(this, &SPopupItem::clicked, this, &SPopupItem::exec);
    QObject::connect(m_info, &SPluginInfo::iconChanged, this, [this] (SPluginInfo *info) {
        this->m_iconLabel->setPixmap(info->icon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    });
    QObject::connect(m_info, &SPluginInfo::nameChanged, this, &SPopupItem::refreshText);
    QObject::connect(m_info, &SPluginInfo::edited, this, &SPopupItem::refreshText);
    QObject::connect(m_info, &SPluginInfo::switchIconOn, this, [this] (SPluginInfo *info) {
        this->m_iconLabel->setVisible(info->iconEnabled);
        this->setVisible(this->enabled());
        this->adjustSize();
    });
    QObject::connect(m_info, &SPluginInfo::switchIconOff, this, [this] (SPluginInfo *info) {
        this->m_iconLabel->setVisible(info->iconEnabled);
        this->setVisible(this->enabled());
        this->adjustSize();
    });
    QObject::connect(m_info, &SPluginInfo::switchNameOn, this, [this] (SPluginInfo *info) {
        this->m_nameLabel->setVisible(info->nameEnabled);
        this->setVisible(this->enabled());
        this->adjustSize();
    });
    QObject::connect(m_info, &SPluginInfo::switchNameOff, this, [this] (SPluginInfo *info) {
        this->m_nameLabel->setVisible(info->nameEnabled);
        this->setVisible(this->enabled());
        this->adjustSize();
    });
    QObject::connect(m_info, &SPluginInfo::needDelete, this, [this] {
        this->setVisible(false);
    });
}

void SPopupItem::refreshText()
{
    if (!m_info)
    {
        return;
    }
    const QFontMetrics metrics(m_nameLabel->font());
    m_nameLabel->setText(metrics.elidedText(m_info->name, Qt::ElideRight, m_nameLabel->maximumWidth()));

    QString toolTip = m_info->name;
    if (!m_info->tip.trimmed().isEmpty())
    {
        toolTip += '\n' + m_info->tip.trimmed();
    }
    setToolTip(toolTip);
    m_iconLabel->setToolTip(toolTip);
    m_nameLabel->setToolTip(toolTip);
    setAccessibleName(m_info->name);
    setAccessibleDescription(m_info->tip);
}

void SPopupItem::mouseReleaseEvent(QMouseEvent *ev)
{
    SDEBUG
    if (ev != nullptr && ev->button() == Qt::LeftButton)
	{
		emit clicked();
	}
}

void SPopupItem::keyPressEvent(QKeyEvent *ev)
{
    if (ev && (ev->key() == Qt::Key_Return
        || ev->key() == Qt::Key_Enter
        || ev->key() == Qt::Key_Space))
    {
        emit clicked();
        ev->accept();
        return;
    }
    QWidget::keyPressEvent(ev);
}
