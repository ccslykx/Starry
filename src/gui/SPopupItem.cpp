#include <QGuiApplication>
#include <QClipboard>
#include <QHBoxLayout>

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
    if (!item)
    {
        delete item;
    }
}

SPluginInfo* SPopupItem::pluginInfo()
{
    return m_info;
}

bool SPopupItem::enabled()
{
    return !(!m_info->iconEnabled && !m_info->nameEnabled);
}

void SPopupItem::exec()
{
    SDEBUG
    SSelection *s = SSelection::instance();
    QStringList args = m_info->script.split(" ");

    if (args.at(0) == "") // m_script为空时，args为只含有一个元素""的QList
    {
        qWarning() << "执行脚本为空";
        return;
    } else if (args.at(0) == "starry" && args.size() >= 2 && args.at(1) == "copy2clipboard") // TODO: 写成函数调用的形式
    {
        QGuiApplication::clipboard()->setText(s->selection());
        return;
    }

    // 获取cmd
    QString cmd = args.at(0);
    args.pop_front(); 

    // 是否需要将selection作为参数传递
    for (QString &arg : args)
    {
        if (arg == "$PLAINTEXT")
        {
            arg = s->selection();
        }
    }

    if (!m_process->waitForFinished())
    {
        m_process->close();
        m_process->kill();
    }

    try {
        qDebug() << "m_process start";
        qDebug() << cmd << args;
        m_process->startDetached(cmd, args);
        qDebug() << "m_process started";
    }
    catch (...) { // TODO：异常捕获
        qWarning() << "插件调用出错，请检查指令是否有误"; 
    }
}

void SPopupItem::stop()
{
    SDEBUG
    if (!m_process->waitForFinished())
    {
        m_process->close();
        m_process->kill();
    }
}

/* private functions */

SPopupItem::SPopupItem(SPluginInfo *info, QWidget *parent)
    : m_info(info)
{
    this->setParent(parent);
    m_info->popupItem = this;
    if (!m_process)
    {
        m_process = new QProcess;
    }

    initGui();
}

SPopupItem::~SPopupItem()
{
    SDEBUG
    stop();
    m_process = nullptr;
}

void SPopupItem::initGui()
{

    m_iconLabel = new QLabel(this);
    m_iconLabel->setPixmap(m_info->icon.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_iconLabel->setMinimumSize(24, 24);
    
    QFont nameFont;
    nameFont.setPointSize(12);
    m_nameLabel = new QLabel(m_info->name, this);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setMinimumSize(24, 24);
    m_nameLabel->setFont(nameFont);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_nameLabel);

    this->setLayout(layout);
    this->setMinimumSize(24 + 8 * 2, 24 + 8 * 2); /* TODO: read from SConfig */
    this->setAttribute(Qt::WA_StyledBackground, true);
    this->setContentsMargins(0, 0, 0, 0);
    this->setStyleSheet("background-color: #CCCCCC;"
                        "border-radius: 8px;"
                        "color: #000000");
    
    m_iconLabel->setVisible(m_info->iconEnabled);
    m_nameLabel->setVisible(m_info->nameEnabled);
    this->adjustSize();

    QObject::connect(this, &SPopupItem::clicked, this, &SPopupItem::exec);
    QObject::connect(m_info, &SPluginInfo::iconChanged, [this] (SPluginInfo *info) {
        this->m_iconLabel->setPixmap(info->icon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    });
    QObject::connect(m_info, &SPluginInfo::nameChanged, [this] (SPluginInfo *info) {
        this->m_nameLabel->setText(info->name);
    });
    QObject::connect(m_info, &SPluginInfo::switchIconOn, [this] (SPluginInfo *info) {
        this->m_iconLabel->setVisible(info->iconEnabled);
        this->setVisible(this->enabled());
        this->adjustSize();
    });
    QObject::connect(m_info, &SPluginInfo::switchIconOff, [this] (SPluginInfo *info) {
        this->m_iconLabel->setVisible(info->iconEnabled);
        this->setVisible(this->enabled());
        this->adjustSize();
    });
    QObject::connect(m_info, &SPluginInfo::switchNameOn, [this] (SPluginInfo *info) {
        this->m_nameLabel->setVisible(info->nameEnabled);
        this->setVisible(this->enabled());
        this->adjustSize();
    });
    QObject::connect(m_info, &SPluginInfo::switchNameOff, [this] (SPluginInfo *info) {
        this->m_nameLabel->setVisible(info->nameEnabled);
        this->setVisible(this->enabled());
        this->adjustSize();
    });
    QObject::connect(m_info, &SPluginInfo::needDelete, [this] (SPluginInfo *info) {
        this->setVisible(false);
    });
}

void SPopupItem::mouseReleaseEvent(QMouseEvent *ev)
{
    SDEBUG
    if (ev != nullptr && ev->button() == Qt::LeftButton)
	{
		emit clicked();
	}
}