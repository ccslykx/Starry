#include <QGuiApplication>
#include <QClipboard>

#include "SPopupItem.h"
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

void SPopupItem::refresh()
{
    setText(m_info->name); /* TODO: icon support */
}

void SPopupItem::exec()
{
    SDEBUG
    QStringList args = m_info->script.split(" ");

    if (args.at(0) == "") // m_script为空时，args为只含有一个元素""的QList
    {
        qWarning() << "执行脚本为空";
        return;
    } else if (args.at(0) == "starry" && args.size() >= 2 && args.at(1) == "copy2clipboard") // TODO: 写成函数调用的形式
    {
        QString selection = QGuiApplication::clipboard()->text(QClipboard::Mode::Selection);
        QGuiApplication::clipboard()->setText(selection);
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
            arg = QGuiApplication::clipboard()->text(QClipboard::Mode::Selection);
        }
    }

    if (!m_process->waitForFinished())
    {
        m_process->close();
        m_process->kill();
    }

    try {
        m_process->start(cmd, args);
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

    this->setText(m_info->name);
    this->setAlignment(Qt::AlignCenter);
    this->setStyleSheet("border-style: outset;"
                        "border-width: 1px;"
                        "border-radius: 8px;"
                        "background-color: #CCCCCC;"
                        "color: #000000");
    /* TODO: read from SConfig */
    this->setMinimumSize(32, 32);
    this->adjustSize();
    this->setVisible(info->enabled);
    QObject::connect(this, &SPopupItem::clicked, this, &SPopupItem::exec);
    QObject::connect(m_info, &SPluginInfo::edited, this, &SPopupItem::refresh);
    QObject::connect(m_info, &SPluginInfo::switchOn, [this] (SPluginInfo *info) {
        this->setVisible(info->enabled);
    });
    QObject::connect(m_info, &SPluginInfo::switchOff, [this] (SPluginInfo *info) {
        this->setVisible(info->enabled);
    });
    QObject::connect(m_info, &SPluginInfo::needDelete, [this] (SPluginInfo *info) {
        this->setVisible(false);
    });
}

SPopupItem::~SPopupItem()
{
    SDEBUG
    stop();
    m_process = nullptr;
}

void SPopupItem::mouseReleaseEvent(QMouseEvent *ev)
{
    SDEBUG
    if (ev != nullptr && ev->button() == Qt::LeftButton)
	{
		emit clicked();
	}
}