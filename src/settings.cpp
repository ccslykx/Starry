#include "settings.h"
#include "macro.h"

#include <QApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPixmap>
#include <QPainter>
#include <QLineEdit>
#include <QMessageBox>
#include <QFontMetrics>

/* Settings */
Settings* Settings::m_instance = nullptr;

Settings* Settings::instance(QWidget *parent)
{
    STARRY_DEBUGER
    if (!m_instance)
    {
        m_instance = new Settings(parent);
    }
    return m_instance;
}

void Settings::init()
{
    STARRY_DEBUGER
    // m_configFile = QApplication::applicationDirPath() + "/starry.conf";
    m_configFile = QDir::homePath() + "/.config/Starry/starry.conf";
    qDebug() << "CONFIG:" << m_configFile;
    m_menus = QVector<MenuItem*>();
    m_plugins = QVector<PluginItem*>();
    // 菜单页（左侧）
    if (!m_menuListWidget)
    {
        m_menuListWidget = new QListWidget(this);
    }
    m_menuListWidget->setItemAlignment(Qt::AlignCenter);
    m_menuListWidget->setFixedWidth(LABEL_BUTTON_WIDTH);
    QObject::connect(m_menuListWidget, &QListWidget::itemClicked, this, &Settings::showContent);

    // 内容页（右侧）
    if (!m_contentWidget)
    {
        m_contentWidget = new QStackedWidget(this);
    }

    // 内容页-插件
    if (!m_pluginWidget)
    {
        m_pluginWidget = new QListWidget(m_contentWidget);
    }
    if (!m_pluginListWidget)
    {
        m_pluginListWidget = new QListWidget(m_pluginWidget);
        m_pluginListWidget->setItemAlignment(Qt::AlignVCenter);
    }
    if (!m_pluginEditor)
    {
        m_pluginEditor = new PluginEditor;
    }
    QObject::connect(m_pluginEditor, &PluginEditor::created, this, &Settings::addPluginItem);
       
    // 创建新插件按钮
    Button *newPluginButton = new Button("创建新插件", m_pluginListWidget);
    newPluginButton->setAlignment(Qt::AlignCenter);
    newPluginButton->setStyleSheet(LABEL_BUTTON_STYLE);
    QObject::connect(newPluginButton, &Button::clicked, this, &Settings::createPlugin);
    
    QVBoxLayout *pluginsLayout = new QVBoxLayout(m_pluginWidget);
    pluginsLayout->addWidget(m_pluginListWidget);
    pluginsLayout->addWidget(newPluginButton);
    m_pluginWidget->setLayout(pluginsLayout);

    // 内容页-快捷键
    if (!m_shortcutWidget)
    {
        m_shortcutWidget = new QListWidget(m_contentWidget);
    }
    QLabel  *shortcutHelp = new QLabel("如果有什么需要的快捷捷功能，请联系作者", m_shortcutWidget);
    shortcutHelp->setAlignment(Qt::AlignCenter);
    QListWidgetItem *shortcutItem = new QListWidgetItem(m_shortcutWidget);
    shortcutItem->setSizeHint(QSize(m_shortcutWidget->size().width(), LABEL_BUTTON_HEIGHT));
    m_shortcutWidget->setItemWidget(shortcutItem, shortcutHelp);

    // 内容页-关于
    if (!m_aboutWidget)
    {
        m_aboutWidget = new QWidget(m_contentWidget);
    }
    QPixmap aboutPixmap(STARRY_ICON(256));
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
    MenuItem *plugins = MenuItem::create(tr("插件"), m_menuListWidget);
    MenuItem *shortcuts = MenuItem::create(tr("快捷键"), m_menuListWidget);
    MenuItem *about = MenuItem::create(tr("关于"), m_menuListWidget);

    addMenuItem(plugins);
    addMenuItem(shortcuts);
    addMenuItem(about);

    // 添加 内容页-插件 项目
    readPlugins();

    // 主界面
    QIcon windowIcon(STARRY_ICON(64));
    this->setLayout(mainLayout);
    this->setMinimumSize(800, 600);
    this->setWindowTitle(tr("Starry 设置"));
    this->setWindowIcon(windowIcon);
}

void Settings::addPluginItem(PluginItem* plugin)
{
    STARRY_DEBUGER
    QListWidgetItem *listWidgetItem = new QListWidgetItem;

    QSize size(m_pluginListWidget->size().width() - 14, LABEL_BUTTON_HEIGHT);

    listWidgetItem->setSizeHint(size);
    m_plugins.push_back(plugin);
    m_pluginListWidget->insertItem(0, listWidgetItem); // 新建的插件放在最上
    m_pluginListWidget->setItemWidget(listWidgetItem, plugin);
    QObject::connect(plugin->m_edit, &Button::clicked, this, [this, plugin](){
        this->editPlugin(plugin);
    });
    QObject::connect(plugin->m_delete, &Button::clicked, this, [this, plugin](){
        QMessageBox *box = new QMessageBox(QMessageBox::Icon::Question,  "提示", "确实要删除插件" + plugin->name() + "吗？", QMessageBox::Yes | QMessageBox::Cancel);
        QObject::connect(box, &QMessageBox::accepted, this, [this, plugin, box] {
            this->deletePluginItem(plugin);
            delete box;
        });
        box->show();
    });
}

void Settings::deletePluginItem(PluginItem* plugin)
{
    STARRY_DEBUGER
    m_pluginListWidget->takeItem(m_pluginListWidget->currentRow());
    m_plugins.removeOne(plugin);
    delete plugin;
    plugin = nullptr;
}

void Settings::addMenuItem(MenuItem* menu)
{
    STARRY_DEBUGER
    QListWidgetItem *listWidgetItem = new QListWidgetItem(m_menuListWidget);

    QSize size(m_menuListWidget->size().width() - 2, LABEL_BUTTON_HEIGHT);

    listWidgetItem->setSizeHint(size);
    m_menus.push_back(menu);
    m_menuListWidget->setItemWidget(listWidgetItem, menu);
}

const QVector<PluginItem*> Settings::getPlugins(bool onlyEnabled)
{
    STARRY_DEBUGER
    QVector<PluginItem*> plugins;
    if (onlyEnabled)
    {
        for (PluginItem* plugin : m_plugins)
        {
            if (plugin->m_switcher->isOn())
            {
                plugins.push_back(plugin);
            }
        }
        return plugins;
    } else {
        return QVector<PluginItem*>(m_plugins.constBegin(), m_plugins.constEnd());
    }
}

void Settings::savePlugins()
{
    STARRY_DEBUGER
    QSettings settings(m_configFile, QSettings::NativeFormat);
    settings.remove(QString("PLUGINS")); // 解决重启后之前已经删除的插件仍被加载
    settings.beginGroup(QString("PLUGINS"));
    for (PluginItem *plugin : m_plugins)
    {
        settings.beginGroup(plugin->name());
        // settings.setValue(QString("Icon"), TODO);
        settings.setValue(QString("Tip"), plugin->tip());
        settings.setValue(QString("Script"), plugin->script());
        settings.setValue(QString("Enabled"), plugin->m_switcher->isOn());
        settings.endGroup();
    }
    settings.endGroup();
}

void Settings::readPlugins()
{
    STARRY_DEBUGER
    QSettings settings(m_configFile, QSettings::NativeFormat);
    settings.beginGroup(QString("PLUGINS"));
    QStringList plugins = settings.childGroups();
    for (QString pluginName : plugins)
    {
        QPixmap pixmap;
        settings.beginGroup(pluginName);
        QString tip = settings.value(QString("Tip")).toString();
        QString script = settings.value(QString("Script")).toString();
        bool enable = settings.value(QString("Enabled")).toBool();
        settings.endGroup();
        PluginItem *pluginItem = PluginItem::create(pixmap, pluginName, tip, script, enable, m_pluginListWidget);
        addPluginItem(pluginItem);
    }
    settings.endGroup();
}

void Settings::showContent()
{
    STARRY_DEBUGER
    if (m_contentWidget->count() != m_menuListWidget->count())
    {
        qWarning() << "m_contentWidget->count() != m_menuListWidget->count()";
    }
    m_contentWidget->setCurrentIndex(m_menuListWidget->currentRow());
}

void Settings::createPlugin()
{
    STARRY_DEBUGER
    m_pluginEditor->createPlugin(this->m_pluginListWidget);
}

void Settings::editPlugin(PluginItem *plugin)
{
    STARRY_DEBUGER
    m_pluginEditor->editPlugin(plugin);
}

Settings::Settings(QWidget *parent)
{
    STARRY_DEBUGER
    this->setParent(parent);
    init();
}

Settings::~Settings()
{
    STARRY_DEBUGER
    if (m_instance)
    {
        delete m_instance;
    }
    m_instance = nullptr;
}

void Settings::closeEvent(QCloseEvent *ev)
{
    STARRY_DEBUGER
    emit saveOnClose();
    savePlugins();
    ev->accept();
}


/* PluginItem */

PluginItem* PluginItem::create(QPixmap icon, QString name, QString tip, 
    QString script, bool enable, QWidget *parent)
{
    STARRY_DEBUGER
    PluginItem *item = new PluginItem(icon, name, tip, script, enable, parent);
    return item;
}

PluginItem::PluginItem(QPixmap icon, QString name, QString tip, 
    QString script, bool enable, QWidget *parent)
{
    STARRY_DEBUGER
    this->setParent(parent);

    this->m_delete = new Button("", this);
    this->m_icon = new QPixmap(icon);
    this->m_iconLabel = new QLabel(this);
    this->m_iconLabel->setWindowIcon(icon);
    this->m_name = new QLabel(name, this);
    this->m_tip = new QLabel(tip, this);
    this->m_script = script;
    this->m_switcher = new Switcher("已启用", "未启用", enable, this);
    this->m_edit = new Button(tr("编辑"), this);
    // this->m_move = new QLabel("移动", this); // TODO：完成移动排序功能后再添加到UI上

    QString delImagePath = ":/PluginItem_Delete.png"; // TODO：改用qrc
    QPixmap delPix(delImagePath);
    m_delete->setPixmap(delPix.scaled(24, 24));
    m_delete->setAlignment(Qt::AlignCenter);
    m_delete->setFixedSize(24, 24);

    m_iconLabel->setStyleSheet(LABEL_BUTTON_STYLE);
    m_iconLabel->setFixedSize(PLUGIN_BUTTON_HEIGHT, PLUGIN_BUTTON_HEIGHT);

    m_name->setStyleSheet(LABEL_BUTTON_STYLE);
    m_name->setMinimumSize(PLUGIN_BUTTON_HEIGHT * 2, PLUGIN_BUTTON_HEIGHT);
    m_name->setMaximumSize(PLUGIN_BUTTON_HEIGHT * 3, PLUGIN_BUTTON_HEIGHT);
    // QFontMetrics nameWidth(m_name->font());
    // m_name->setText(nameWidth.elidedText(name, Qt::ElideRight, m_name->width()));

    m_tip->setStyleSheet(LABEL_BUTTON_STYLE);
    m_tip->setMinimumSize(PLUGIN_BUTTON_HEIGHT * 5, PLUGIN_BUTTON_HEIGHT);
    // QFontMetrics tipWidth(m_tip->font());
    // m_tip->setText(tipWidth.elidedText(tip, Qt::ElideRight, m_tip->width()));

    m_switcher->setFixedSize(PLUGIN_BUTTON_HEIGHT * 2, PLUGIN_BUTTON_HEIGHT);

    m_edit->setStyleSheet(LABEL_BUTTON_STYLE);
    m_edit->setFixedSize(PLUGIN_BUTTON_HEIGHT, PLUGIN_BUTTON_HEIGHT);
    
    // m_move->setStyleSheet(LABEL_BUTTON_STYLE);
    // m_move->setFixedSize(PLUGIN_BUTTON_HEIGHT, PLUGIN_BUTTON_HEIGHT);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_delete);
    layout->addWidget(m_iconLabel);
    layout->addWidget(m_name);
    layout->addWidget(m_tip);
    layout->addWidget(m_switcher);
    layout->addWidget(m_edit);
    // layout->addWidget(m_move); 

    this->setLayout(layout);
    this->setFixedHeight(LABEL_BUTTON_HEIGHT);
}

QPixmap* PluginItem::icon()
{
    STARRY_DEBUGER
    return this->m_icon;
}

QString PluginItem::name()
{
    STARRY_DEBUGER
    return this->m_name->text();
}

QString PluginItem::tip()
{
    STARRY_DEBUGER
    return this->m_tip->text();
}

QString PluginItem::script()
{
    STARRY_DEBUGER
    return m_script;
}

/* MenuItem */

MenuItem* MenuItem::create(QString text, QWidget *parent)
{
    STARRY_DEBUGER
    MenuItem *item = new MenuItem(text, parent);
    return item;
}

MenuItem::MenuItem(QString &text, QWidget *parent)
{
    STARRY_DEBUGER
    this->setParent(parent);
    this->setText(text);
    this->setStyleSheet(LABEL_BUTTON_STYLE);
    this->setFixedHeight(LABEL_BUTTON_HEIGHT);
    this->setMaximumWidth(LABEL_BUTTON_HEIGHT * 3);
}

/* Button */
Button::Button(const QString& text, QWidget *parent)
{
    STARRY_DEBUGER
    this->setText(text);
    this->setParent(parent);
}

void Button::mouseReleaseEvent(QMouseEvent *ev)
{
    STARRY_DEBUGER
    if (ev != nullptr && ev->button() == Qt::LeftButton)
	{
		emit clicked();
	}
}

/* Switcher */
Switcher::Switcher(const QString &on, const QString &off, bool status, QWidget *parent)
    : m_on(on)
    , m_off(off)
    , m_isOn(status)
{
    STARRY_DEBUGER
    this->setParent(parent);
    this->setAlignment(Qt::AlignCenter);
    this->setStatus(status);
}

bool Switcher::isOn()
{
    STARRY_DEBUGER
    return m_isOn;
}

void Switcher::setStatus(bool status)
{
    STARRY_DEBUGER
    m_isOn = status;
    if (m_isOn)
    {
        this->setText(m_on);
        this->setStyleSheet(LABEL_BUTTON_STYLE + QString("background-color: #59C837"));
    } else {
        this->setText(m_off);
        this->setStyleSheet(LABEL_BUTTON_STYLE + QString("background-color: gray"));
    }
}

void Switcher::switchStatus()
{
    STARRY_DEBUGER
    setStatus(!m_isOn);
}

void Switcher::mouseReleaseEvent(QMouseEvent *ev)
{
    STARRY_DEBUGER
    if (ev != nullptr && ev->button() == Qt::LeftButton)
	{
        switchStatus();
		emit m_isOn ? switchOn() : switchOff();
	}
}

/* PluginEditor */

PluginEditor::PluginEditor()
{
    STARRY_DEBUGER
    this->setWindowTitle(tr("创建新插件"));
    
    if (!m_iconLabel)   { m_iconLabel = new QLabel(tr("插件图标"), this); };
    if (!m_nameLabel)   { m_nameLabel = new QLabel(tr("插件名称"), this); };
    if (!m_tipLabel)    { m_tipLabel = new QLabel(tr("插件描述"), this); };
    if (!m_scriptLabel) { m_scriptLabel = new QLabel(tr("执行脚本"), this); }
    
    if (!m_createButton)      { m_createButton = new Button(tr("创建"), this); }
    if (!m_saveButton)      { m_saveButton = new Button(tr("保存"), this); }
    m_createButton->setAlignment(Qt::AlignCenter);
    m_createButton->setStyleSheet(LABEL_BUTTON_STYLE);
    m_saveButton->setAlignment(Qt::AlignCenter);
    m_saveButton->setStyleSheet(LABEL_BUTTON_STYLE);

    QObject::connect(m_createButton, &Button::clicked, this, [this]() {
        PluginItem *newItem = PluginItem::create(*this->icon(), this->name(), this->tip(), this->script(), (QWidget*) this->m_pointer);
        emit this->created(newItem);
        this->close();
    });
    QObject::connect(m_saveButton, &Button::clicked, this, [this]() {
        PluginItem *plugin = (PluginItem*) this->m_pointer;
        plugin->m_icon = this->icon();
        plugin->m_name->setText(this->name());
        plugin->m_tip->setText(this->tip());
        plugin->m_script = this->script();
        emit this->edited(plugin);
        this->close();
    });

    m_icon = new QPixmap; /* Todo: 改成默认图标 */
    if (!m_nameEdit)    { m_nameEdit = new QLineEdit(this); }
    if (!m_tipEdit)     { m_tipEdit = new QLineEdit(this); }
    if (!m_scriptEdit)  { m_scriptEdit = new QLineEdit(this); }

    if (!m_layout)      { m_layout = new QVBoxLayout(this); }
    m_layout->addWidget(m_iconLabel);
    m_layout->addWidget(m_nameLabel);
    m_layout->addWidget(m_nameEdit);
    m_layout->addWidget(m_tipLabel);
    m_layout->addWidget(m_tipEdit);
    m_layout->addWidget(m_scriptLabel);
    m_layout->addWidget(m_scriptEdit);
    m_layout->addWidget(m_createButton);
    m_layout->addWidget(m_saveButton);

    this->setLayout(m_layout);
}

void PluginEditor::createPlugin(QWidget *pluginParent)
{
    STARRY_DEBUGER
    m_pointer = pluginParent;
    m_icon = new QPixmap; /* Todo: 改成默认图标 */
    m_nameEdit->clear();
    m_tipEdit->clear();
    m_scriptEdit->clear();
    setWindowTitle(tr("创建新插件"));
    m_saveButton->setVisible(false);
    m_createButton->setVisible(true);

    m_nameEdit->setFocus();
    show();
}

void PluginEditor::editPlugin(PluginItem *plugin)
{
    STARRY_DEBUGER
    if (!plugin)
    {
        qWarning() << "Editing (nullptr) PluginItem";
        return;
    }
    m_pointer = plugin;
    m_icon = plugin->icon();
    m_nameEdit->setText(plugin->name());
    m_tipEdit->setText(plugin->tip());
    m_scriptEdit->setText(plugin->script());
    setWindowTitle(tr("编辑插件：") + plugin->name());
    m_createButton->setVisible(false);
    m_saveButton->setVisible(true);
    show();
}

QPixmap* PluginEditor::icon()
{
    STARRY_DEBUGER
    return m_icon;
}

QString PluginEditor::name()
{
    STARRY_DEBUGER
    return this->m_nameEdit->text();
}

QString PluginEditor::tip()
{
    STARRY_DEBUGER
    return this->m_tipEdit->text();
}

QString PluginEditor::script()
{
    STARRY_DEBUGER
    return this->m_scriptEdit->text();
}
