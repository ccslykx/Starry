#include "popup.h"
#include "macro.h"

#include <QGuiApplication>
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QTimer>

/* Popup */

Popup* Popup::m_instance = nullptr;

Popup* Popup::instance()
{
    STARRY_DEBUGER
    if (!m_instance) {
        m_instance = new Popup;
    }
    return m_instance;
}

void Popup::init()
{
    STARRY_DEBUGER
    m_items = QVector<PopupItem*>();
    if (!m_layout)
    {
        m_layout = new QHBoxLayout(this);
        m_layout->setAlignment(Qt::AlignTop);
    }
    if (!m_timer)
    {
        m_timer = new QTimer(this);
        m_timer->setInterval(ICON_TIMEOUT);
        QObject::connect(m_timer, &QTimer::timeout, this, &QWidget::hide);
    }

    this->setMinimumSize(ICON_LENGTH, ICON_LENGTH);

    this->setAttribute(Qt::WA_TranslucentBackground);
    // https://stackoverflow.com/questions/966688/show-window-in-qt-without-stealing-focus
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint); // Qt::Tool is important flag to make it work. I mean not stealing focus. 但对x11下Gnome44 无效
    this->setWindowFlags(Qt::Popup | Qt::Window); // 不在任务栏显示
    this->setWindowFlags(Qt::X11BypassWindowManagerHint); // 在x11上不占用focus
    this->setStyleSheet("border-style: outset; border-width: 1px; border-radius:8px;");
}

void Popup::update(const QVector<PluginItem*> plugins)
{
    STARRY_DEBUGER
    for (PopupItem* item : m_items)
    {
        deleteItem(item);
    }
    m_items.clear();

    size_t n = plugins.size();
    if (n == 0)
    {
        return;
    }
    // 向m_layout里添加插件
    for (PluginItem* plugin : plugins)
    {
        PopupItem *item = PopupItem::createItem(plugin->m_script, plugin->m_name->text(), *plugin->m_icon);
        addItem(item);
    }
}

void Popup::addItem(PopupItem* item)
{
    STARRY_DEBUGER
    m_items.push_back(item);
    m_layout->addWidget(item);

    // 点击PopupItem后，Popup自动隐藏
    QObject::connect(item, &PopupItem::clicked, this, [this]() { this->hide(); });
}

void Popup::deleteItem(PopupItem* item)
{
    STARRY_DEBUGER
    m_layout->removeWidget(item); 
    m_items.removeOne(item);
    PopupItem::deleteItem(item);
}

void Popup::showPopup()//QPoint& point, bool up)
{
    STARRY_DEBUGER
    if (this->isVisible()) // 如果当前已经弹出，则先隐藏，再弹出
    { 
        this->setVisible(false);
        if (m_timer->isActive())
        {
            m_timer->stop();
        }
    }

    this->adjustGeometry(QCursor::pos());
    this->setVisible(true);

    // 计时自动隐藏
    m_timer->start();
}

Popup::Popup()
{
    STARRY_DEBUGER
    init();
}

Popup::~Popup()
{
    STARRY_DEBUGER
    if (m_layout)
    {
        delete m_layout;
    }
    m_layout = nullptr;
    if (m_instance) 
    {
        delete m_instance;
    }
    m_instance = nullptr;
}

void Popup::adjustGeometry(QPoint loc)
{
    this->adjustSize();

    QRect screenGeometry = this->screen()->virtualGeometry();
    qDebug() << screenGeometry.x() << screenGeometry.y();
    qDebug() << screenGeometry.width() << screenGeometry.height();

    if (loc.x() + this->width() > screenGeometry.width())
    {
        loc.setX(screenGeometry.width() - this->width());
    }
    if (loc.y() + this->height() > screenGeometry.height())
    {
        loc.setY(screenGeometry.height() - this->height());
    }

    this->setGeometry(loc.x(), loc.y(), this->width(), this->height());
}


/* PopupItem */

PopupItem* PopupItem::createItem(const QString &script, const QString &name, const QPixmap &icon)
{
    STARRY_DEBUGER
    PopupItem *item = new PopupItem(script, name, icon);
    return item;
}

void PopupItem::deleteItem(PopupItem *item)
{
    STARRY_DEBUGER
    if (item)
    {
        delete item;
    }
    item = nullptr;
}

void PopupItem::exec()
{
    STARRY_DEBUGER
    QStringList args = m_script.split(" ");

    if (args.at(0) == "") // m_script为空时，args为只含有一个元素""的QList
    {
        qWarning() << "执行脚本为空";
        return;
    } else if (args.at(0) == "starry" && args.size() >= 2 && args.at(1) == "copy2clipboard") // TODO: 写成函数调用的形式
    {
        QString selection = QGuiApplication::clipboard()->text(QClipboard::Mode::Selection);
        QGuiApplication::clipboard()->setText(selection);
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
            arg = QGuiApplication::clipboard()->text(QClipboard::Mode::Selection);
        }
    }

    if (!m_process->waitForFinished())
    {
        m_process->close();
        m_process->kill();
    }

    try {
        m_process->start(cmd, args);
    }
    catch (...) { // TODO：异常捕获
        qWarning() << "插件调用出错，请检查指令是否有误"; 
    }
}

void PopupItem::stop()
{
    STARRY_DEBUGER
    if (!m_process->waitForFinished())
    {
        m_process->close();
        m_process->kill();
    }
}

PopupItem::PopupItem(const QString &script, const QString &name, const QPixmap &icon)
    : m_script(script), m_name(name), m_icon(icon)
{
    STARRY_DEBUGER
    if (!m_process)
    {
        m_process = new QProcess;
    }

    this->setText(m_name);
    this->setAlignment(Qt::AlignCenter);
    this->setStyleSheet("border-style: outset;"
                        "border-width: 1px;"
                        "border-radius: 8px;"
                        "background-color: gray");
    
    this->setMinimumSize(ICON_LENGTH, ICON_LENGTH);
    this->adjustSize();
    QObject::connect(this, &PopupItem::clicked, this, &PopupItem::exec);
};

PopupItem::~PopupItem()
{
    STARRY_DEBUGER
    stop();
    m_process = nullptr;
}

void PopupItem::mouseReleaseEvent(QMouseEvent *ev)
{
    STARRY_DEBUGER
    if (ev != nullptr && ev->button() == Qt::LeftButton)
	{
		emit clicked();
	}
}
