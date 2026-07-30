#include <QHBoxLayout>
#include <QIcon>
#include <QGuiApplication>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>

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

void SPluginItem::setIndexToInfo(int index) /* Todo */
{
    SDEBUG
    if (!m_info || m_info->index == index)
    {
        return;
    }
    m_info->index = index;
    emit m_info->indexChanged(m_info);
}

void SPluginItem::refresh()
{
    SDEBUG
    if (m_info)
    {
        m_iconSwitcher->setPixmap(m_info->icon.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_nameSwitcher->setOnText(m_info->name);
        m_nameSwitcher->setOffText(m_info->name);
        m_tipLabel->setText(m_info->tip);
    }
}

void SPluginItem::refreshTheme()
{
    const bool dark = QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
    if (m_tipLabel)
    {
        m_tipLabel->setStyleSheet(QStringLiteral(
            "QLabel {"
            "  background-color: transparent; color: %1;"
            "  border: none;"
            "  padding: 0 10px;"
            "}").arg(dark ? QStringLiteral("#FFFFFF") : QStringLiteral("#000000")));
    }

    if (m_deleteButton)
    {
        QPixmap pixmap(18, 18);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(dark ? QColor(QStringLiteral("#FFFFFF"))
                      : QColor(QStringLiteral("#344054")));
        pen.setWidthF(1.7);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(4.0, 5.0), QPointF(14.0, 5.0));
        painter.drawLine(QPointF(7.0, 3.0), QPointF(11.0, 3.0));
        painter.drawRoundedRect(QRectF(5.0, 5.0, 8.0, 10.0), 1.5, 1.5);
        painter.drawLine(QPointF(8.0, 8.0), QPointF(8.0, 12.0));
        painter.drawLine(QPointF(10.0, 8.0), QPointF(10.0, 12.0));
        painter.end();
        m_deleteButton->setIcon(QIcon(pixmap));
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
    m_deleteButton->setObjectName("deletePluginButton");
    m_iconSwitcher = new SSwitcher("", "", m_info->iconEnabled, this);
    m_nameSwitcher = new SSwitcher(m_info->name, m_info->name, m_info->nameEnabled, this);
    m_tipLabel = new QLabel(m_info->tip, this);
    m_tipLabel->setObjectName("pluginTipLabel");
    m_tipLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_editButton = new SButton(tr("Edit"), this);
    m_editButton->setObjectName("editPluginButton");

    // Delete Button
    m_deleteButton->setRole(SButton::Role::DangerIcon);
    m_deleteButton->setIconSize(QSize(18, 18));
    m_deleteButton->setToolTip(tr("Delete plugin"));
    m_deleteButton->setAccessibleName(tr("Delete plugin"));
    m_deleteButton->setFixedSize(36, 36);
    // Icon
    m_iconSwitcher->setPixmap(m_info->icon.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_iconSwitcher->setToolTip(tr("Show or hide the plugin icon in the popup"));
    m_iconSwitcher->setAccessibleName(tr("Show plugin icon"));
    m_iconSwitcher->setFixedSize(40, 36);
    m_iconSwitcher->setStatus(m_info->iconEnabled);
    // Name
    m_nameSwitcher->setToolTip(tr("Show or hide the plugin name in the popup"));
    m_nameSwitcher->setAccessibleName(tr("Show plugin name"));
    m_nameSwitcher->setMinimumSize(96, 36);
    m_nameSwitcher->setMaximumSize(180, 36);
    m_nameSwitcher->setStatus(m_info->nameEnabled);
    // Tip
    m_tipLabel->setMinimumSize(160, 36);
    m_tipLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    refreshTheme();
    // Edit Button
    m_editButton->setRole(SButton::Role::Secondary);
    m_editButton->setMinimumWidth(64);
    m_editButton->setFixedHeight(36);
    m_editButton->setToolTip(tr("Edit plugin"));
    m_editButton->setAccessibleName(tr("Edit plugin"));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(8);
    layout->addWidget(m_iconSwitcher);
    layout->addWidget(m_nameSwitcher);
    layout->addWidget(m_tipLabel, 1);
    layout->addWidget(m_editButton);
    layout->addWidget(m_deleteButton);

    this->setLayout(layout);
    this->setFixedHeight(48);

    const auto requestSelection = [this] {
        emit selectionRequested();
    };
    QObject::connect(m_iconSwitcher, &QPushButton::pressed, this, requestSelection);
    QObject::connect(m_nameSwitcher, &QPushButton::pressed, this, requestSelection);
    QObject::connect(m_editButton, &QPushButton::pressed, this, requestSelection);
    QObject::connect(m_deleteButton, &QPushButton::pressed, this, requestSelection);

    QObject::connect(m_deleteButton, &SButton::clicked, [this] () {
        QMessageBox *box = new QMessageBox(QMessageBox::Icon::Question,  "提示", "确实要删除插件 " + this->m_info->name + " 吗？", QMessageBox::Yes | QMessageBox::Cancel, this);
        QObject::connect(box, &QMessageBox::accepted, this, [this] {
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

void SPluginItem::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event && (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange))
    {
        refreshTheme();
    }
}

void SPluginItem::mousePressEvent(QMouseEvent *event)
{
    emit selectionRequested();
    QWidget::mousePressEvent(event);
}
