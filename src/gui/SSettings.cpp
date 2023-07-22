#include <QDebug>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QMessageBox>

#include "SSettings.h"
#include "SConfig.h"

#define SDEBUG qDebug() << "[FILE:" << __FILE__ << ", LINE:" << __LINE__ << ", FUNC:" << Q_FUNC_INFO << "]";

SSettings* SSettings::m_instance = nullptr;

extern SConfig *config;

SSettings* SSettings::instance(QWidget *parent)
{
    SDEBUG
    if (!m_instance)
    {
        m_instance = new SSettings(parent);
    }
    return m_instance;
}

void SSettings::initGui()
{
    SDEBUG
    // 菜单页（左侧）
    if (!m_menuListWidget)
    {
        m_menuListWidget = new QListWidget(this);
    }
    m_menuListWidget->setItemAlignment(Qt::AlignCenter);
    m_menuListWidget->setFixedWidth(48 * 3);

    // 内容页（右侧）
    if (!m_contentWidget)
    {
        m_contentWidget = new QStackedWidget(this);
    }

    // 内容页-插件页
    if (!m_pluginWidget)
    {
        m_pluginWidget = new QWidget(m_contentWidget);
    }
    if (!m_pluginListWidget)
    {
        m_pluginListWidget = new QListWidget(m_pluginWidget);
        m_pluginListWidget->setItemAlignment(Qt::AlignVCenter);
        m_pluginListWidget->setDragDropMode(QAbstractItemView::InternalMove);
        m_pluginListWidget->setMinimumHeight(48);
    }

    SButton *newPluginButton = new SButton(tr("Create new plugin"), m_pluginListWidget);
    newPluginButton->setAlignment(Qt::AlignCenter);
    QObject::connect(newPluginButton, &SButton::clicked, this, &SSettings::onCreatePluginClicked);

    QVBoxLayout *pluginsLayout = new QVBoxLayout(m_pluginWidget);
    pluginsLayout->addWidget(m_pluginListWidget);
    pluginsLayout->addWidget(newPluginButton);
    m_pluginWidget->setLayout(pluginsLayout);

    // 内容页-插件编辑器
    if (!m_pluginEditor) 
    {
        m_pluginEditor = SPluginEditor::editor();
    }
    QObject::connect(m_pluginEditor, &SPluginEditor::edited, this, &SSettings::onPluginEdited);
    QObject::connect(m_pluginEditor, &SPluginEditor::created, this, &SSettings::onPluginCreated);

    // 内容页-快捷键
    if (!m_shortcutWidget)
    {
        m_shortcutWidget = new QListWidget(m_contentWidget);
    }
    QLabel *shortcutHelp = new QLabel("如果有什么需要的快捷捷功能，请联系作者", m_shortcutWidget);
    shortcutHelp->setAlignment(Qt::AlignCenter);
    QListWidgetItem *shortcutItem = new QListWidgetItem(m_shortcutWidget);
    shortcutItem->setSizeHint(QSize(m_shortcutWidget->size().width(), 48));
    m_shortcutWidget->setItemWidget(shortcutItem, shortcutHelp);

    // 内容页-关于
    if (!m_aboutWidget)
    {
        m_aboutWidget = new QWidget(m_contentWidget);
    }
    QPixmap aboutPixmap;// TODO: (STARRY_ICON(256));
    QLabel *aboutIcon = new QLabel("Starry");
    aboutIcon->setPixmap(aboutPixmap.scaled(256, 256, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    aboutIcon->setFixedSize(256, 256);
    aboutIcon->setAlignment(Qt::AlignCenter);
    QLabel *aboutContent = new QLabel("版本：" + STARRY_VERSION + "\n作者：Ccslykx\n联系方式：ccslykx@outlook.com", m_aboutWidget);
    aboutContent->setAlignment(Qt::AlignCenter);

    QVBoxLayout *aboutLayout = new QVBoxLayout(m_aboutWidget);
    aboutLayout->setAlignment(Qt::AlignCenter);
    aboutLayout->addWidget(aboutIcon);
    aboutLayout->addWidget(aboutContent);

    m_aboutWidget->setLayout(aboutLayout);

    // 内容页（右侧）
    m_contentWidget->addWidget(m_pluginWidget);
    m_contentWidget->addWidget(m_shortcutWidget);
    m_contentWidget->addWidget(m_aboutWidget);

    // Layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(m_menuListWidget);
    mainLayout->addWidget(m_contentWidget);

    // 添加 菜单页 项目
    SButton *plugins = new SButton(tr("Plugins"), m_menuListWidget);
    SButton *shortcuts = new SButton(tr("Shortcuts"), m_menuListWidget);
    SButton *about = new SButton(tr("About"), m_menuListWidget);

    QObject::connect(plugins, &SButton::clicked, this, [this](){ this->showContent(0); });
    QObject::connect(shortcuts, &SButton::clicked, this, [this](){ this->showContent(1); });
    QObject::connect(about, &SButton::clicked, this, [this](){ this->showContent(2); });

    addMenuItem(plugins); // ！！这里添加的顺序同上面showContent(index)内index
    addMenuItem(shortcuts);
    addMenuItem(about);

    // 添加 内容页-插件 项目
    readPlugins();

    // 主界面
    QIcon windowIcon; // TODO: (STARRY_ICON(64));
    this->setLayout(mainLayout);
    this->setMinimumSize(800, 600);
    this->setWindowTitle(tr("Starry 设置"));
    this->setWindowIcon(windowIcon);
}

void SSettings::addPluginItem(SPluginItem *item)
{
    SDEBUG
    QListWidgetItem *listWidgetItem = new QListWidgetItem(m_pluginListWidget);
    QSize size(m_pluginListWidget->size().width() - 14, 48);
    listWidgetItem->setSizeHint(size);
    m_pluginListWidget->addItem(listWidgetItem);
    m_pluginListWidget->setItemWidget(listWidgetItem, item);
    config->addPlugin(item);
    QObject::connect(item->deleteButton(), &SButton::clicked, this, [this, item]() {
        this->onDeletePluginClicked(item);
    }); /* TODO */
    QObject::connect(item->editButton(), &SButton::clicked, this, [this, item]() {
        this->onEditPluginClicked(item);
    });
}

void SSettings::addPluginItem(SPluginInfo *info)
{
    SDEBUG
    SPluginItem *item = SPluginItem::create(info, this->m_pluginListWidget);
    addPluginItem(item);
}

void SSettings::deletePluginItem(SPluginItem *item)
{
    SDEBUG
    int cur = m_pluginListWidget->currentRow();
    m_pluginListWidget->takeItem(cur);
    config->deletePlugin(item->pluginInfo()); /* TODO: confirm */
    SPluginItem::remove(item);
}

void SSettings::addMenuItem(QLabel *item)
{
    SDEBUG
    QListWidgetItem *listWidgetItem = new QListWidgetItem(m_menuListWidget);
    QSize size(m_menuListWidget->size().width() - 2, 48);
    listWidgetItem->setSizeHint(size);
    m_menuListWidget->addItem(listWidgetItem);
    m_menuListWidget->setItemWidget(listWidgetItem, item);
}

void SSettings::readPlugins()
{
    SDEBUG
    for (SPluginInfo *info : config->getSPluginInfos())
    {
        addPluginItem(info);
    }
}

void SSettings::showContent(int index)
{
    SDEBUG
    if (index == -1) {
        index = m_menuListWidget->currentRow();
    }
    if (0 < index || m_contentWidget->count() <= index)
    {
        qWarning() <<"invalid index";
    }
    m_contentWidget->setCurrentIndex(index);
}

void SSettings::onCreatePluginClicked()
{
    SDEBUG
    m_pluginEditor->create();
    /* TODO: 当编辑窗口未关闭时，设置窗口不可点击 */
}

void SSettings::onDeletePluginClicked(SPluginItem *item)
{
    SDEBUG
    QMessageBox *box = new QMessageBox(QMessageBox::Icon::Question,  "提示", "确实要删除插件 " + item->pluginInfo()->name + " 吗？", QMessageBox::Yes | QMessageBox::Cancel, this);
    QObject::connect(box, &QMessageBox::accepted, this, [this, item, box] {
        this->deletePluginItem(item);
        delete box;
    });
    box->show(); 
}

void SSettings::onEditPluginClicked(SPluginItem *item)
{
    SDEBUG
    m_pluginEditor->edit(item);
    /* TODO: 当编辑窗口未关闭时，设置窗口不可点击 */
}

void SSettings::onPluginEdited(SPluginItem *item)
{
    SDEBUG
    item->refresh();
}

void SSettings::onPluginCreated(SPluginInfo *info)
{
    SDEBUG
    addPluginItem(info);
}

/* private functions */

SSettings::SSettings(QWidget *parent)
{
    SDEBUG
    this->setParent(parent);
    initGui();
}

SSettings::~SSettings()
{
    SDEBUG
    if (m_instance)
    {
        delete m_instance;
    }
    m_instance = nullptr;
}

void SSettings::closeEvent(QCloseEvent *ev)
{
    SDEBUG
    refreashPluginIndex();
    emit saveOnClose();
    config->saveToFile();
    ev->accept();
}

void SSettings::refreashPluginIndex()
{
    SDEBUG
    /* TODO: or Delete */
}
