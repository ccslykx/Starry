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
        m_iconSwitcher->setPixmap(m_info->icon.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_nameSwitcher->setText(m_info->name);
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
    m_iconSwitcher = new SSwitcher("", "", m_info->iconEnabled, this);
    m_nameSwitcher = new SSwitcher(m_info->name, m_info->name, m_info->nameEnabled, this);
    m_tipLabel = new QLabel(m_info->tip, this);
    m_editButton = new SButton(tr("Edit"), this);

    // Delete Button
    QString delImgPath = ":/PluginItem_Delete.png";
    QPixmap delPixmap(delImgPath);
    m_deleteButton->setPixmap(delPixmap.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_deleteButton->setAlignment(Qt::AlignCenter);
    m_deleteButton->setFixedSize(28, 28);
    m_deleteButton->setStyleSheet(
        "background-color: #E9524A;"
        "border-style: outset;"
        "border-width: 2px;"
        "border-radius: 14;"
        "border-color: #E9524A");
    // Icon
    m_iconSwitcher->setPixmap(m_info->icon.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_iconSwitcher->setFixedSize(32, 32);
    m_iconSwitcher->setOnStyleSheet(
        "background-color: #59C837;"
        "border-style: outset;"
        "border-width: 2px;"
        "border-radius: 8px;"
        "border-color: #59C837");
    m_iconSwitcher->setOffStyleSheet(
        "background-color: gray;"
        "border-style: outset;"
        "border-width: 2px;"
        "border-radius: 8px;"
        "border-color: gray");
    m_iconSwitcher->setStatus(m_info->iconEnabled);
    // Name
    m_nameSwitcher->setStyleSheet(S_BUTTON_STYLE);
    m_nameSwitcher->setMinimumSize(32 * 2, 32);
    m_nameSwitcher->setMaximumSize(32 * 3, 32);
    m_nameSwitcher->setOnStyleSheet(
        "background-color: #59C837;"
        "border-style: outset;"
        "border-width: 2px;"
        "border-radius: 8px;"
        "border-color: #59C837");
    m_nameSwitcher->setOffStyleSheet(
        "background-color: gray;"
        "border-style: outset;"
        "border-width: 2px;"
        "border-radius: 8px;"
        "border-color: gray");
    m_nameSwitcher->setStatus(m_info->nameEnabled);
    // Tip
    m_tipLabel->setStyleSheet(S_BUTTON_STYLE);
    m_tipLabel->setMinimumSize(32 * 5, 32);
    // Edit Button
    m_editButton->setFixedSize(32, 32);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_deleteButton);
    layout->addWidget(m_iconSwitcher);
    layout->addWidget(m_nameSwitcher);
    layout->addWidget(m_tipLabel);
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
        editor->edit(m_info);
    });
    QObject::connect(m_iconSwitcher, &SSwitcher::switchOn, [this] () {
        this->m_info->iconEnabled = true;
        emit m_info->switchIconOn(m_info);
        this->refresh();
    });
    QObject::connect(m_iconSwitcher, &SSwitcher::switchOff, [this] () {
        this->m_info->iconEnabled = false;
        emit m_info->switchIconOff(m_info);
        this->refresh();
    });
    QObject::connect(m_nameSwitcher, &SSwitcher::switchOn, [this] () {
        this->m_info->nameEnabled = true;
        emit m_info->switchNameOn(m_info);
        this->refresh();
    });
    QObject::connect(m_nameSwitcher, &SSwitcher::switchOff, [this] () {
        this->m_info->nameEnabled = false;
        emit m_info->switchNameOff(m_info);
        this->refresh();
    });
}