#include <QHBoxLayout>
#include <QIcon>
#include <QMessageBox>

#include "SPluginItem.h"
#include "SPluginInfo.h"
#include "SPluginEditor.h"
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

void SPluginItem::setIndexToInfo(size_t index) /* Todo */
{
    SDEBUG
    m_info->index = index;
    emit m_info->edited();
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

/* private functions */

SPluginItem::SPluginItem(SPluginInfo *pluginInfo, QWidget *parent)
    : m_info(pluginInfo) 
{
    SDEBUG
    this->setParent(parent);
    m_info->pluginItem = this;
    QObject::connect(m_info, &SPluginInfo::edited, this, &SPluginItem::refresh);
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

    QObject::connect(m_deleteButton, &SButton::clicked, [this] () {
        QMessageBox *box = new QMessageBox(QMessageBox::Icon::Question,  "提示", "确实要删除插件 " + this->m_info->name + " 吗？", QMessageBox::Yes | QMessageBox::Cancel, this);
        QObject::connect(box, &QMessageBox::accepted, this, [this, box] {
            emit m_info->needDelete(m_info);
        });
        box->show(); 
    });
    QObject::connect(m_editButton, &SButton::clicked, [this] () {
        SPluginEditor *editor = SPluginEditor::editor();
        editor->edit(this);
    });
    QObject::connect(m_switcher, &SSwitcher::switchOn, [this] () {
        this->m_info->enabled = true;
        emit m_info->switchOn(m_info);
        this->refresh();
    });
    QObject::connect(m_switcher, &SSwitcher::switchOff, [this] () {
        this->m_info->enabled = false;
        emit m_info->switchOff(m_info);
        this->refresh();
    });
}