#include <QGuiApplication>

#include "tray.h"
#include "macro.h"

PopupTray*      PopupTray::m_instance = nullptr;
Popup*          PopupTray::m_popup = nullptr;
Settings*       PopupTray::m_settings = nullptr;
QClipboard*     PopupTray::m_clipboard = nullptr;
X11Connector*   PopupTray::m_x11Connector = nullptr;

PopupTray* PopupTray::instance(QApplication *app)
{
    STARRY_DEBUGER
    if (!m_instance) 
    {
        m_instance = new PopupTray(app);
    }
    return m_instance;
}

void PopupTray::refreshPopups()
{
    STARRY_DEBUGER
    const QVector<PluginItem*> plugins = m_settings->getPlugins(true);
    m_popup->update(plugins);
}

PopupTray::PopupTray(QApplication *app)
{
    STARRY_DEBUGER
    m_parent = app;
    m_pressPos = QCursor::pos();
    init();
}

PopupTray::~PopupTray()
{
    STARRY_DEBUGER
    if (m_menu)
    {
        delete m_menu;
    }
    m_menu = nullptr;
}

void PopupTray::init()
{
    STARRY_DEBUGER
    if (!m_menu)
    {
        m_menu = new QMenu;
    }
    if (!this->m_settings)
    {
        this->m_settings = Settings::instance();
        QObject::connect(m_settings, &Settings::saveOnClose, 
                         this, &PopupTray::refreshPopups);
    }
    if (!m_popup)
    {
        m_popup = Popup::instance();
        this->refreshPopups();
    }
    if (!m_clipboard)
    {
        m_clipboard = QGuiApplication::clipboard();
    }
    if (!m_waitSelectionChangeTimer)
    {
        m_waitSelectionChangeTimer = new QTimer(this);
        m_waitSelectionChangeTimer->setInterval(100);
        QObject::connect(m_waitSelectionChangeTimer, &QTimer::timeout, this, [this]() {
            if (this->m_selectionChanged) 
            {
                this->m_popup->showPopup();
                this->m_selectionChanged = false; // 在弹出后重置条件
            }
            this->m_waitSelectionChangeTimer->stop(); // 重置Timer
        });
    }
    if (!m_waitB1ReleaseTimer)
    {
        m_waitB1ReleaseTimer = new QTimer;
        m_waitB1ReleaseTimer->setInterval(100);
        QObject::connect(m_waitB1ReleaseTimer, &QTimer::timeout, this, [this]() {
            if (this->m_B1Released)
            {
                this->m_popup->showPopup();
                this->m_B1Released = false;
            }
            this->m_waitB1ReleaseTimer->stop();
        });
    }
    if (!m_x11Connector)
    {
        m_x11Connector = X11Connector::instance();
        this->connectX11();
        m_x11Connector->run();
    }
    QObject::connect(m_clipboard, &QClipboard::selectionChanged, this, [this](){
        qDebug() << "selectionChanged";
        this->m_selectionChanged = true;
        if (!this->m_B1Released) // if B1 is pressed
        {
            this->waitForB1Release();
        }
    });

    QAction *enable = new QAction(m_menu);
    enable->setText(tr("启用"));

    QAction *settings = new QAction(m_menu);
    settings->setText(tr("设置"));
    QObject::connect(settings, &QAction::triggered, this, [this](){
        this->m_settings->show();
    });

    QAction *exitTray = new QAction(m_menu);
    exitTray->setText(tr("退出"));
    // QObject::connect(exitTray, &QAction::triggered, QApplication::instance(), &QApplication::quit);
    QObject::connect(exitTray, &QAction::triggered, this, [this](){
        qDebug() << "exit clicked";
        this->m_x11Connector->stop();
        this->m_parent->quit();
    });
    
    // m_menu->addAction(enable); // TODO: 增加启用开关
    m_menu->addAction(settings);
    m_menu->addAction(exitTray);

    this->setContextMenu(m_menu);

    QIcon trayIcon(":/starry.png");
    this->setIcon(trayIcon);
}

void PopupTray::connectX11()
{
    QObject::connect(m_x11Connector, &X11Connector::B1Pressed, this, [this](MouseStatus status) {
        this->m_pressPos = QPoint(status.x, status.y);
        this->m_B1Released = false;
        this->m_selectionChanged = false; // 解决中文输入法导致selectionChanged的影响
    });
    QObject::connect(m_x11Connector, &X11Connector::B1Released, this, [this](MouseStatus status) {
        this->m_releasePos = QPoint(status.x, status.y);
        this->m_B1Released = true;

        double offset = sqrt(pow((double) this->m_releasePos.x() - (double) this->m_pressPos.x(), 2) + pow((double) this->m_releasePos.y() - (double) this->m_pressPos.y(), 2));
        if (offset >= 5.0)
        {
            // qDebug() << "鼠标位移量足够，认为是划词动作";
            this->waitForSelectionChange();
        } // else
            // qDebug() << "鼠标位移量太小，认为是微小振动";
    });
    QObject::connect(m_x11Connector, &X11Connector::B1DoubleClicked, this, [this](MouseStatus status) {
        this->waitForSelectionChange();
    });
}

void PopupTray::waitForSelectionChange()
{
    if (m_waitSelectionChangeTimer->isActive())
    {
        m_waitSelectionChangeTimer->stop();
    }
    m_waitSelectionChangeTimer->start();
}

void PopupTray::waitForB1Release()
{
    if (m_waitB1ReleaseTimer->isActive())
    {
        m_waitB1ReleaseTimer->stop();
    }
    m_waitB1ReleaseTimer->start();
}