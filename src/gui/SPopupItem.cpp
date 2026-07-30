#include <QGuiApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QProcess>

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

bool SPopupItem::enabled()
{
    return !(!m_info->iconEnabled && !m_info->nameEnabled);
}

void SPopupItem::exec()
{
    SDEBUG
    SSelection *s = SSelection::instance();
    QStringList args = QProcess::splitCommand(m_info->script);

    if (args.isEmpty() || args.constFirst().isEmpty())
    {
        qWarning() << "执行脚本为空";
        return;
    }
    if (args.constFirst() == "starry" && args.size() >= 2 && args.at(1) == "copy2clipboard")
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

    qDebug() << cmd << args;
    if (!QProcess::startDetached(cmd, args))
    {
        qWarning() << "插件调用失败，请检查指令是否存在或参数是否正确:" << cmd << args;
    }
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
    m_nameLabel = new QLabel(m_info->name, this);
    m_nameLabel->setAlignment(Qt::AlignCenter);
    m_nameLabel->setMinimumSize(24, 24);
    m_nameLabel->setFont(nameFont);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
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
    
    m_iconLabel->setVisible(m_info->iconEnabled);
    m_nameLabel->setVisible(m_info->nameEnabled);
    this->adjustSize();

    QObject::connect(this, &SPopupItem::clicked, this, &SPopupItem::exec);
    QObject::connect(m_info, &SPluginInfo::iconChanged, this, [this] (SPluginInfo *info) {
        this->m_iconLabel->setPixmap(info->icon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    });
    QObject::connect(m_info, &SPluginInfo::nameChanged, this, [this] (SPluginInfo *info) {
        this->m_nameLabel->setText(info->name);
    });
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

void SPopupItem::mouseReleaseEvent(QMouseEvent *ev)
{
    SDEBUG
    if (ev != nullptr && ev->button() == Qt::LeftButton)
	{
		emit clicked();
	}
}
