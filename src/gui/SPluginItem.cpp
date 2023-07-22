#include <QHBoxLayout>
#include <QIcon>

#include "SPluginItem.h"
#include "SPluginInfo.h"
#include "SButton.h"
#include "SSwitcher.h"
#include "utils.h"

SPluginItem* SPluginItem::create(SPluginInfo *pluginInfo, QWidget *parent)
{
    SDEBUG
    return new SPluginItem(pluginInfo, parent);
}
SPluginItem* SPluginItem::create(const QString &name, const QString &script,
    const QPixmap &icon, const QString &tip, bool enable, QWidget *parent)
{
    SDEBUG
    return new SPluginItem(name, script, icon, tip, enable, parent);
}

SPluginItem* SPluginItem::create(const QString &name, const QString &script,
    const QString &iconPath, const QString &tip, bool enable, QWidget *parent)
{
    SDEBUG
    return new SPluginItem(name, script, iconPath, tip, enable, parent);
}

void SPluginItem::remove(SPluginItem *item)
{
    SDEBUG
    if (item)
    {
        delete item;
    }
}

void SPluginItem::setIndexToInfo(size_t index)
{
    SDEBUG
    m_info->index = index;
}

void SPluginItem::refresh()
{
    SDEBUG
    if (m_info)
    {
        m_iconLabel->setPixmap(m_info->icon);
        m_nameLabel->setText(m_info->name);
        m_tipLabel->setText(m_info->tip);
    }
}

SPluginInfo* SPluginItem::pluginInfo()
{
    SDEBUG
    return this->m_info;
}

SButton* SPluginItem::deleteButton()
{
    SDEBUG
    return this->m_deleteButton;
}

SButton* SPluginItem::editButton()
{
    SDEBUG
    return this->m_editButton;
}

/* private functions */

SPluginItem::SPluginItem(SPluginInfo *pluginInfo, QWidget *parent)
    : m_info(pluginInfo) 
{
    SDEBUG
    this->setParent(parent);
    initGui();
}

SPluginItem::SPluginItem(const QString &name, const QString &script,
    const QPixmap &icon, const QString &tip, bool enable, QWidget *parent)
{
    SDEBUG
    SPluginInfo *info = new SPluginInfo(name, script, icon, 0, tip, enable);
    SPluginItem(std::move(info), parent); /* Need Test */
}

SPluginItem::SPluginItem(const QString &name, const QString &script,
    const QString &iconPath, const QString &tip, bool enable, QWidget *parent)
{
    SDEBUG
    SPluginInfo *info = new SPluginInfo(name, script, iconPath, 0, tip, enable);
    SPluginItem(std::move(info), parent); /* Need Test */
}

SPluginItem::~SPluginItem()
{
    SDEBUG
    /* TODO:
        delete m_info ?
     */
}

void SPluginItem::initGui()
{
    SDEBUG
    m_deleteButton = new SButton("", this);
    m_iconLabel = new QLabel(this); /* memory leaks alert !!! */
    m_iconLabel->setWindowIcon(m_info->icon);
    m_nameLabel = new QLabel(m_info->name, this);
    m_tipLabel = new QLabel(m_info->tip, this);
    m_switcher = new SSwitcher(tr("On"), tr("Off"), m_info->enabled, this);
    m_editButton = new SButton(tr("Edit"), this);

    // Delete Button
    QString delImgPath = ":/PluginItem_Delete.png";
    QPixmap delPixmap(delImgPath);
    m_deleteButton->setPixmap(delPixmap.scaled(24, 24));
    m_deleteButton->setAlignment(Qt::AlignCenter);
    m_deleteButton->setFixedSize(24, 24);
    m_deleteButton->setStyleSheet("border-width: 0px");
    // Icon
    m_iconLabel->setPixmap(m_info->icon);
    m_iconLabel->setFixedSize(32, 32);
    // Name
    m_nameLabel->setStyleSheet(S_BUTTON_STYLE);
    m_nameLabel->setMinimumSize(32 * 2, 32);
    m_nameLabel->setMaximumSize(32 * 3, 32);
    // Tip
    m_tipLabel->setStyleSheet(S_BUTTON_STYLE);
    m_tipLabel->setMinimumSize(32 * 5, 32);
    // On/Off Switcher
    m_switcher->setFixedSize(32 * 2, 32);
    // Edit Button
    m_editButton->setFixedSize(32, 32);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_deleteButton);
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_nameLabel);
    layout->addWidget(m_tipLabel);
    layout->addWidget(m_switcher);
    layout->addWidget(m_editButton);

    this->setLayout(layout);
    this->setFixedHeight(48);
}