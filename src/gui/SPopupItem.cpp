#include <QGuiApplication>
#include <QClipboard>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QProcess>

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
    emit executionStarted(tr("启动中…"));

    QStringList args = QProcess::splitCommand(m_info->script);

    if (args.isEmpty() || args.constFirst().isEmpty())
    {
        qWarning() << "执行脚本为空";
        m_executionLocked = false;
        emit executionFinished(false, tr("命令为空"));
        return;
    }
    if (args.constFirst() == "starry" && args.size() >= 2 && args.at(1) == "copy2clipboard")
    {
        QGuiApplication::clipboard()->setText(SSelection::instance()->selection());
        m_executionLocked = false;
        emit executionFinished(true, tr("已复制"));
        return;
    }

    // 获取cmd
    const QString cmd = args.takeFirst();

    // 是否需要将selection作为参数传递
    for (QString &arg : args)
    {
        if (arg == "$PLAINTEXT")
        {
            arg = SSelection::instance()->selection();
        }
    }

    qDebug() << cmd << args;
    SPluginTask *task = SPluginTaskManager::instance()->startTask(
        m_info->name,
        cmd,
        args,
        m_info->script);
    if (!task)
    {
        m_executionLocked = false;
        emit executionFinished(false, tr("启动失败"));
        return;
    }

    QObject::connect(task, &SPluginTask::started, this, [this] {
        m_executionLocked = false;
        emit executionFinished(true, tr("已启动"));
    });
    QObject::connect(task, &SPluginTask::failed, this, [this] (SPluginTask *, const QString &reason) {
        qWarning() << "插件调用失败，请检查指令是否存在或参数是否正确:" << reason;
        m_executionLocked = false;
        emit executionFinished(false, tr("启动失败"));
    });
    QObject::connect(task, &SPluginTask::finished, this, [this] {
        if (!m_executionLocked)
        {
            return;
        }
        m_executionLocked = false;
        emit executionFinished(false, tr("已中止"));
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
