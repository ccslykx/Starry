#pragma once

#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStackedWidget>
#include <QVector>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QSettings>

class Settings;
class PluginItem;
class MenuItem;
class Button;
class Switcher;
class PluginEditor;

#define LABEL_BUTTON_STYLE "border-style: outset; border-width: 1px; border-radius:8px;"
#define LABEL_BUTTON_HEIGHT 48
#define LABEL_BUTTON_WIDTH LABEL_BUTTON_HEIGHT * 3
#define PLUGIN_BUTTON_HEIGHT 32
#define SPACING 4

class Settings : public QWidget
{
    Q_OBJECT

public:
    static Settings* instance(QWidget *parent = (QWidget*)nullptr);
    void init();
    void addPluginItem(PluginItem*); 
    void deletePluginItem(PluginItem*);
    void addMenuItem(MenuItem*);
    const QVector<PluginItem*> getPlugins(bool onlyEnabled = true); // 获取所有插件的信息
    void savePlugins(); // 保存插件到文件
    void readPlugins(); // 从配置文件读取插件设置

public slots:
    void showContent(); // 根据左侧菜单显示对应的右侧内容
    void createPlugin(); // 新建插件
    void editPlugin(PluginItem* plugin);

signals:
    void saveOnClose();

private:
    explicit Settings(QWidget *parent = (QWidget*)nullptr);
    ~Settings();

    void closeEvent(QCloseEvent *ev);

private:
    static Settings         *m_instance;
    
    QListWidget             *m_menuListWidget = nullptr; // 菜单页
    QStackedWidget          *m_contentWidget = nullptr; // 内容页
    QWidget                 *m_pluginWidget = nullptr; // 内容页-插件
    QListWidget             *m_pluginListWidget = nullptr; // 内容页-插件-已有插件列表
    QListWidget             *m_shortcutWidget = nullptr; // 内容页-快捷键
    QWidget                 *m_aboutWidget = nullptr; // 内容页-关于

    PluginEditor            *m_pluginEditor = nullptr;

    QVector<PluginItem*>    m_plugins;
    QVector<MenuItem*>      m_menus;
    QString                 m_configFile = "";
};

class PluginItem : public QWidget
{
    Q_OBJECT

public:
    static PluginItem* create(QPixmap icon, QString name, QString tip, QString script, bool enable = true, QWidget *parent = (QWidget*)nullptr);
    QPixmap*    icon(); 
    QString     name();
    QString     tip();
    QString     script();

private:
    PluginItem(QPixmap icon, QString name, QString tip, QString script, bool enable = true, QWidget *parent = (QWidget*)nullptr);

public:
    Button      *m_delete;
    QLabel      *m_iconLabel;
    QPixmap     *m_icon;
    QLabel      *m_name;
    QLabel      *m_tip;
    QString     m_script;
    Switcher    *m_switcher;
    Button      *m_edit;
    QLabel      *m_move;
};

class MenuItem : public QLabel
{
    Q_OBJECT
public:
    static MenuItem* create(QString, QWidget *parent = (QWidget*)nullptr);

private:
    MenuItem(QString&, QWidget *parent = (QWidget*)nullptr);

};

class Button : public QLabel
{
    Q_OBJECT
public:
    Button(const QString &text = "", QWidget *parent = (QWidget*)nullptr);

signals:
    void clicked();

private:
    void mouseReleaseEvent(QMouseEvent *ev);
};

class Switcher : public QLabel
{
    Q_OBJECT
public:
    Switcher(const QString &on, const QString &off, bool status, QWidget *parent = (QWidget*)nullptr);
    bool isOn();
    void setStatus(bool);

signals:
    void switchOn();
    void switchOff();

private:
    void switchStatus();
    void mouseReleaseEvent(QMouseEvent *ev);

private:
    bool    m_isOn;
    QString m_on;
    QString m_off;
};

/*
    PluginEditor
    创建的新插件通过 created(PluginItem*) 信号传递指针
    编辑插件，虽然也通过 edited(PluginItem*) 信号传递了参数，但目前尚未用到该信号
*/
class PluginEditor : public QWidget
{
    Q_OBJECT

public:
    PluginEditor();

    void createPlugin(QWidget *pluginParent);
    void editPlugin(PluginItem *plugin);

    QPixmap*    icon();
    QString     name();
    QString     tip();
    QString     script();

signals:
    void created(PluginItem*);
    void edited(PluginItem*);

private:
    QLabel      *m_iconLabel = nullptr;
    QLabel      *m_nameLabel = nullptr;
    QLabel      *m_tipLabel = nullptr;
    QLabel      *m_scriptLabel = nullptr;
    Button      *m_createButton = nullptr;
    Button      *m_saveButton = nullptr;

    QPixmap     *m_icon = nullptr;
    QLineEdit   *m_nameEdit = nullptr;
    QLineEdit   *m_tipEdit = nullptr;
    QLineEdit   *m_scriptEdit = nullptr;

    QVBoxLayout *m_layout = nullptr;
    QWidget     *m_buttonsWidget = nullptr;

    void *m_pointer = nullptr; // 用于编辑和创建插件功能
};