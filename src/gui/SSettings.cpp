#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPalette>
#include <QPointer>
#include <QPushButton>
#include <QStyle>
#include <QTimer>

#include "SSettings.h"
#include "SConfig.h"
#include "SPluginTaskManager.h"
#include "utils.h"

SSettings* SSettings::m_instance = nullptr;

SSettings* SSettings::instance(QWidget *parent)
{
    SDEBUG
    if (!m_instance)
    {
        m_instance = new SSettings(parent);
    }
    return m_instance;
}

void SSettings::showAndActivate()
{
    Qt::WindowStates state = windowState();
    state &= ~Qt::WindowMinimized;
    state |= Qt::WindowActive;
    setWindowState(state);
    show();
    raise();
    activateWindow();

    // A tray menu may still own focus while its QAction is being dispatched.
    // Retry after that menu has closed.
    QTimer::singleShot(0, this, [this] {
        raise();
        activateWindow();
    });
}

void SSettings::refreshTheme(bool force)
{
    const bool dark = QGuiApplication::palette().color(QPalette::Window).lightness() < 128;
    if (!force && m_styleInitialized && m_darkStyle == dark)
    {
        return;
    }
    m_darkStyle = dark;
    m_styleInitialized = true;

    if (!m_pluginListWidget)
    {
        return;
    }

    m_pluginListWidget->setStyleSheet(dark
        ? QStringLiteral(
            "QListWidget#pluginList { border: none; background: transparent; outline: none; }"
            "QListWidget#pluginList::item {"
            "  background: transparent; border: 1px solid transparent; border-radius: 7px;"
            "}"
            "QListWidget#pluginList::item:hover:!selected {"
            "  background: #344054; border-color: #475467;"
            "}"
            "QListWidget#pluginList::item:selected {"
            "  background: #9A3412; border: 1px solid #F97316;"
            "  border-left: 3px solid #FDBA74;"
            "}")
        : QStringLiteral(
            "QListWidget#pluginList { border: none; background: transparent; outline: none; }"
            "QListWidget#pluginList::item {"
            "  background: transparent; border: 1px solid transparent; border-radius: 7px;"
            "}"
            "QListWidget#pluginList::item:hover:!selected {"
            "  background: #F2F4F7; border-color: #D0D5DD;"
            "}"
            "QListWidget#pluginList::item:selected {"
            "  background: #FFF7ED; border: 1px solid #FED7AA;"
            "  border-left: 3px solid #F97316;"
            "}"));
}

void SSettings::initGui()
{
    SDEBUG
    // 菜单页（左侧）
    if (!m_menuListWidget)
    {
        m_menuListWidget = new QListWidget(this);
    }
    m_menuListWidget->setObjectName("settingsMenuList");
    m_menuListWidget->setItemAlignment(Qt::AlignCenter);
    m_menuListWidget->setFixedWidth(160);
    m_menuListWidget->setSpacing(4);
    m_menuListWidget->setStyleSheet(
        "QListWidget { border: none; background: transparent; outline: none; }"
        "QListWidget::item { border: none; background: transparent; }");

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
        m_pluginListWidget->setObjectName("pluginList");
        m_pluginListWidget->setItemAlignment(Qt::AlignVCenter);
        m_pluginListWidget->setDragDropMode(QAbstractItemView::InternalMove);
        m_pluginListWidget->setMinimumHeight(48);
        m_pluginListWidget->setSpacing(4);
        QObject::connect(m_pluginListWidget->model(), &QAbstractItemModel::rowsMoved, this, [this] {
            refreshPluginIndex();
        });
    }
    refreshTheme(true);

    SButton *newPluginButton = new SButton(tr("Create new plugin"), m_pluginListWidget);
    newPluginButton->setRole(SButton::Role::Primary);
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

    // 内容页-任务管理器
    if (!m_taskWidget)
    {
        m_taskWidget = new QWidget(m_contentWidget);
        QLabel *title = new QLabel(tr("Plugin Task Manager"), m_taskWidget);
        title->setStyleSheet("font-size: 20px; font-weight: 600;");
        QLabel *description = new QLabel(
            tr("Running plugins remain here until they exit. Force stopping a task may lose its unsaved data."),
            m_taskWidget);
        description->setWordWrap(true);

        m_taskListWidget = new QListWidget(m_taskWidget);
        m_taskListWidget->setObjectName("pluginTaskList");
        m_taskListWidget->setAlternatingRowColors(true);

        m_emptyTaskLabel = new QLabel(tr("No plugin tasks are running."), m_taskWidget);
        m_emptyTaskLabel->setAlignment(Qt::AlignCenter);
        m_emptyTaskLabel->setStyleSheet("color: #777777; padding: 24px;");

        QVBoxLayout *taskLayout = new QVBoxLayout(m_taskWidget);
        taskLayout->setContentsMargins(24, 24, 24, 24);
        taskLayout->setSpacing(12);
        taskLayout->addWidget(title);
        taskLayout->addWidget(description);
        taskLayout->addWidget(m_emptyTaskLabel);
        taskLayout->addWidget(m_taskListWidget);
        m_taskWidget->setLayout(taskLayout);

        SPluginTaskManager *taskManager = SPluginTaskManager::instance();
        QObject::connect(taskManager, &SPluginTaskManager::taskAdded, this, &SSettings::addTaskItem);
        QObject::connect(taskManager, &SPluginTaskManager::taskRemoved, this, &SSettings::removeTaskItem);
        for (SPluginTask *task : taskManager->activeTasks())
        {
            addTaskItem(task);
        }
        updateTaskEmptyState();
    }

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
    QPixmap aboutPixmap(SUtils::STARRY_ICON(256));
    QLabel *aboutIcon = new QLabel("Starry");
    aboutIcon->setPixmap(aboutPixmap.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
    m_contentWidget->addWidget(m_taskWidget);
    m_contentWidget->addWidget(m_shortcutWidget);
    m_contentWidget->addWidget(m_aboutWidget);
    m_contentWidget->addWidget(m_pluginEditor);
    QObject::connect(m_pluginEditor, &SPluginEditor::editingOpened, this, [this] {
        m_contentWidget->setCurrentWidget(m_pluginEditor);
    });
    QObject::connect(m_pluginEditor, &SPluginEditor::editingFinished, this, [this] {
        m_contentWidget->setCurrentWidget(m_pluginWidget);
    });

    // Layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(m_menuListWidget);
    mainLayout->addWidget(m_contentWidget);

    // 添加 菜单页 项目
    SButton *plugins = new SButton(tr("Plugins"), m_menuListWidget);
    SButton *tasks = new SButton(tr("Tasks"), m_menuListWidget);
    SButton *shortcuts = new SButton(tr("Shortcuts"), m_menuListWidget);
    SButton *about = new SButton(tr("About"), m_menuListWidget);
    plugins->setRole(SButton::Role::Navigation);
    tasks->setRole(SButton::Role::Navigation);
    shortcuts->setRole(SButton::Role::Navigation);
    about->setRole(SButton::Role::Navigation);

    QObject::connect(plugins, &SButton::clicked, this, [this](){ this->showContent(0); });
    QObject::connect(tasks, &SButton::clicked, this, [this](){ this->showContent(1); });
    QObject::connect(shortcuts, &SButton::clicked, this, [this](){ this->showContent(2); });
    QObject::connect(about, &SButton::clicked, this, [this](){ this->showContent(3); });

    addMenuItem(plugins); // ！！这里添加的顺序同上面showContent(index)内index
    addMenuItem(tasks);
    addMenuItem(shortcuts);
    addMenuItem(about);
    showContent(0);

    // 主界面
    QIcon windowIcon(SUtils::STARRY_ICON(64));
    this->setLayout(mainLayout);
    this->setMinimumSize(800, 600);
    this->setWindowTitle(tr("Starry 设置"));
    this->setWindowIcon(windowIcon);
}

void SSettings::addPluginItem(SPluginItem *item)
{
    SDEBUG
    if (!item)
    {
        return;
    }
    QListWidgetItem *listWidgetItem = new QListWidgetItem(m_pluginListWidget);
    QSize size(m_pluginListWidget->size().width() - 14, 48);
    listWidgetItem->setSizeHint(size);
    m_pluginListWidget->addItem(listWidgetItem);
    m_pluginListWidget->setItemWidget(listWidgetItem, item);
    QObject::connect(item, &SPluginItem::selectionRequested, m_pluginListWidget,
                     [this, listWidgetItem] {
        if (m_pluginListWidget->row(listWidgetItem) >= 0)
        {
            m_pluginListWidget->setCurrentItem(listWidgetItem);
        }
    });
}

void SSettings::addPluginItem(SPluginInfo *info)
{
    SDEBUG
    if (!info)
    {
        return;
    }
    SPluginItem *item = SPluginItem::create(info, this->m_pluginListWidget);
    addPluginItem(item);
    QObject::connect(info, &SPluginInfo::needDelete, [this, info] () {
        this->deletePluginItem(info->pluginItem);
    });
}

void SSettings::deletePluginItem(SPluginItem *item)
{
    SDEBUG
    if (!item)
    {
        return;
    }
    for (int row = 0; row < m_pluginListWidget->count(); ++row)
    {
        QListWidgetItem *listWidgetItem = m_pluginListWidget->item(row);
        if (m_pluginListWidget->itemWidget(listWidgetItem) != item)
        {
            continue;
        }
        m_pluginListWidget->removeItemWidget(listWidgetItem);
        delete m_pluginListWidget->takeItem(row);
        item->deleteLater();
        refreshPluginIndex();
        return;
    }
    qWarning() << "Plugin item to delete was not found";
}

void SSettings::addMenuItem(QWidget *item)
{
    SDEBUG
    QListWidgetItem *listWidgetItem = new QListWidgetItem(m_menuListWidget);
    QSize size(m_menuListWidget->size().width() - 12, 44);
    listWidgetItem->setSizeHint(size);
    m_menuListWidget->addItem(listWidgetItem);
    m_menuListWidget->setItemWidget(listWidgetItem, item);
    if (SButton *button = qobject_cast<SButton *>(item))
    {
        m_menuButtons.push_back(button);
    }
}

void SSettings::showContent(int index)
{
    SDEBUG
    if (index == -1) {
        index = m_menuListWidget->currentRow();
    }
    if (index < 0 || index >= m_contentWidget->count())
    {
        return;
    }
    m_contentWidget->setCurrentIndex(index);
    if (index < m_menuListWidget->count())
    {
        m_menuListWidget->setCurrentRow(index);
    }
    for (qsizetype buttonIndex = 0; buttonIndex < m_menuButtons.size(); ++buttonIndex)
    {
        m_menuButtons.at(buttonIndex)->setSelected(buttonIndex == static_cast<qsizetype>(index));
    }
}

void SSettings::onCreatePluginClicked()
{
    SDEBUG
    m_pluginEditor->create();
    /* TODO: 当编辑窗口未关闭时，设置窗口不可点击 */
}

/* private functions */

SSettings::SSettings(QWidget *parent)
{
    SDEBUG
    this->setParent(parent);
    m_config = SConfig::config();
    initGui();
}

SSettings::~SSettings()
{
    SDEBUG
    m_instance = nullptr;
}

void SSettings::closeEvent(QCloseEvent *ev)
{
    SDEBUG
    refreshPluginIndex();
    emit windowClose();
    m_config->saveToFile(m_config->configPath());
    ev->accept();
}

void SSettings::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event && (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange))
    {
        refreshTheme();
    }
}

void SSettings::refreshPluginIndex() /* Need to be optmized */
{
    SDEBUG
    for (int i = 0; i < m_pluginListWidget->count(); ++i)
    {
        QListWidgetItem *item = m_pluginListWidget->item(i);
        SPluginItem *pluginItem = (SPluginItem *) m_pluginListWidget->itemWidget(item);
        pluginItem->setIndexToInfo(i);
    }
}

void SSettings::addTaskItem(SPluginTask *task)
{
    if (!task || m_taskItems.contains(task))
    {
        return;
    }

    QListWidgetItem *listItem = new QListWidgetItem;
    listItem->setSizeHint(QSize(m_taskListWidget->width(), 58));
    m_taskListWidget->addItem(listItem);

    QWidget *row = new QWidget(m_taskListWidget);
    QLabel *nameLabel = new QLabel(task->pluginName(), row);
    nameLabel->setMinimumWidth(120);
    nameLabel->setStyleSheet("font-weight: 600;");

    QLabel *commandLabel = new QLabel(row);
    commandLabel->setText(QFontMetrics(commandLabel->font()).elidedText(
        task->commandLine(),
        Qt::ElideMiddle,
        280));
    commandLabel->setToolTip(task->commandLine());
    commandLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QLabel *statusLabel = new QLabel(row);
    statusLabel->setMinimumWidth(120);

    SButton *stopButton = new SButton(tr("Force stop"), row);
    stopButton->setRole(SButton::Role::Danger);
    stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    stopButton->setIconSize(QSize(16, 16));
    stopButton->setObjectName("forceStopTaskButton");
    stopButton->setMinimumWidth(92);
    stopButton->setToolTip(tr("Immediately terminate this plugin process"));
    stopButton->setAccessibleName(tr("Force stop plugin") + ' ' + task->pluginName());

    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(8, 4, 8, 4);
    rowLayout->setSpacing(12);
    rowLayout->addWidget(nameLabel);
    rowLayout->addWidget(commandLabel, 1);
    rowLayout->addWidget(statusLabel);
    rowLayout->addWidget(stopButton);
    row->setLayout(rowLayout);
    m_taskListWidget->setItemWidget(listItem, row);
    m_taskItems.insert(task, listItem);

    auto updateStatus = [task, statusLabel, stopButton] {
        QString status;
        switch (task->state())
        {
        case SPluginTask::State::Starting:
            status = tr("Starting…");
            break;
        case SPluginTask::State::Running:
            status = tr("Running · PID %1").arg(task->processId());
            break;
        case SPluginTask::State::Stopping:
            status = tr("Stopping…");
            break;
        case SPluginTask::State::Finished:
            status = tr("Finished");
            break;
        case SPluginTask::State::Failed:
            status = tr("Failed");
            break;
        case SPluginTask::State::Terminated:
            status = tr("Terminated");
            break;
        }
        statusLabel->setText(status);
        stopButton->setEnabled(
            task->state() == SPluginTask::State::Starting
            || task->state() == SPluginTask::State::Running);
    };
    updateStatus();
    QObject::connect(task, &SPluginTask::stateChanged, row, [updateStatus] {
        updateStatus();
    });
    QObject::connect(stopButton, &SButton::clicked, row, [this, task] {
        const QPointer<SPluginTask> guardedTask(task);
        const QMessageBox::StandardButton choice = QMessageBox::question(
            this,
            tr("Confirm force stop"),
            tr("Force stop plugin \"%1\"? Unsaved data in the plugin may be lost.")
                .arg(task->pluginName()),
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Cancel);
        if (choice == QMessageBox::Yes && guardedTask)
        {
            guardedTask->forceStop();
        }
    });
    updateTaskEmptyState();
}

void SSettings::removeTaskItem(SPluginTask *task)
{
    QListWidgetItem *listItem = m_taskItems.take(task);
    if (!listItem)
    {
        return;
    }
    const int rowIndex = m_taskListWidget->row(listItem);
    QWidget *rowWidget = m_taskListWidget->itemWidget(listItem);
    m_taskListWidget->removeItemWidget(listItem);
    delete m_taskListWidget->takeItem(rowIndex);
    if (rowWidget)
    {
        rowWidget->deleteLater();
    }
    updateTaskEmptyState();
}

void SSettings::updateTaskEmptyState()
{
    if (!m_taskListWidget || !m_emptyTaskLabel)
    {
        return;
    }
    const bool empty = m_taskItems.isEmpty();
    m_emptyTaskLabel->setVisible(empty);
    m_taskListWidget->setVisible(!empty);
}
