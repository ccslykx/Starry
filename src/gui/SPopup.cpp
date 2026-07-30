#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QProgressBar>

#include <algorithm>

#include "SPopup.h"
#include "SConfig.h"
#include "utils.h"

namespace
{
constexpr int STATUS_DISPLAY_TIMEOUT = 800;
}

SPopup* SPopup::m_instance = nullptr;

SPopup* SPopup::instance()
{
    SDEBUG
    if (!m_instance)
    {
        m_instance = new SPopup;
    }
    return m_instance;
}

void SPopup::update()
{
    std::stable_sort(m_items.begin(), m_items.end(), [] (SPopupItem *lhs, SPopupItem *rhs) {
        const SPluginInfo *lhsInfo = lhs ? lhs->pluginInfo() : nullptr;
        const SPluginInfo *rhsInfo = rhs ? rhs->pluginInfo() : nullptr;
        if (!lhsInfo || !rhsInfo)
        {
            return lhsInfo != nullptr;
        }
        return lhsInfo->index < rhsInfo->index;
    });

    for (SPopupItem *item : m_items)
    {
        m_layout->removeWidget(item);
    }
    for (SPopupItem *item : m_items)
    {
        m_layout->addWidget(item);
    }
    adjustSize();
    QWidget::update();
}

void SPopup::addItem(SPluginInfo *info)
{
    SDEBUG
    if (!info)
    {
        return;
    }
    SPopupItem *item = SPopupItem::create(info, this);
    addItem(item);

    QObject::connect(info, &SPluginInfo::needDelete, this, [this] (SPluginInfo *info) {
        deleteItem(info->popupItem);
    });
    QObject::connect(info, &SPluginInfo::indexChanged, this, [this] {
        update();
    });
}

void SPopup::addItem(SPopupItem *item)
{   
    SDEBUG
    if (!item)
    {
        return;
    }
    m_items.push_back(item);
    m_layout->addWidget(item);
    update();

    QObject::connect(item, &SPopupItem::executionStarted, this, [this] (const QString &message) {
        showExecutionStatus(message, true, true);
    });
    QObject::connect(item, &SPopupItem::executionFinished, this, [this] (bool success, const QString &message) {
        showExecutionStatus(message, false, success);
    });
}

void SPopup::deleteItem(SPopupItem *item)
{
    SDEBUG
    if (!item)
    {
        return;
    }
    m_layout->removeWidget(item);
    m_items.removeOne(item);
    SPopupItem::remove(item);
}

void SPopup::showPopup()
{
    restoreItems();
    const bool hasEnabledItem = std::any_of(m_items.cbegin(), m_items.cend(), [] (const SPopupItem *item) {
        return item && item->enabled();
    });
    if (!hasEnabledItem)
    {
        hide();
        return;
    }

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
    m_timer->start(ICON_TIMEOUT);
}

/* private functions */

SPopup::SPopup()
{
    SDEBUG
    m_config = SConfig::config();
    initGui();
}

SPopup::~SPopup()
{
    SDEBUG
    m_layout = nullptr;
    m_instance = nullptr;
}

void SPopup::initGui()
{
    if (!m_layout)
    {
        m_layout = new QHBoxLayout(this);
        m_layout->setAlignment(Qt::AlignTop);
        m_layout->setSpacing(4);
        m_layout->setContentsMargins(0, 0, 0, 0);
    }
    if (!m_timer)
    {
        m_timer = new QTimer(this);
        m_timer->setInterval(ICON_TIMEOUT);
        QObject::connect(m_timer, &QTimer::timeout, this, &QWidget::hide);
    }
    if (!m_statusWidget)
    {
        m_statusWidget = new QWidget(this);
        m_statusWidget->setObjectName("popupStatusWidget");
        m_statusWidget->setAttribute(Qt::WA_StyledBackground, true);
        m_statusWidget->setStyleSheet(
            "background-color: rgba(220, 220, 220, 224);"
            "border-radius: 8px;"
            "color: #000000;");

        m_loadingIndicator = new QProgressBar(m_statusWidget);
        m_loadingIndicator->setObjectName("popupLoadingIndicator");
        m_loadingIndicator->setRange(0, 0);
        m_loadingIndicator->setTextVisible(false);
        m_loadingIndicator->setFixedSize(24, 14);

        m_statusLabel = new QLabel(m_statusWidget);
        m_statusLabel->setObjectName("popupExecutionStatus");
        QFont statusFont = m_statusLabel->font();
        statusFont.setPointSize(12);
        statusFont.setBold(true);
        m_statusLabel->setFont(statusFont);

        QHBoxLayout *statusLayout = new QHBoxLayout(m_statusWidget);
        statusLayout->setContentsMargins(14, 10, 14, 10);
        statusLayout->setSpacing(8);
        statusLayout->addWidget(m_loadingIndicator);
        statusLayout->addWidget(m_statusLabel);
        m_statusWidget->setLayout(statusLayout);
        m_statusWidget->setVisible(false);
    }

    this->setMinimumSize(ICON_LENGTH, ICON_LENGTH);
    this->setFocusPolicy(Qt::StrongFocus);

    this->setAttribute(Qt::WA_TranslucentBackground, true);
    // https://stackoverflow.com/questions/966688/show-window-in-qt-without-stealing-focus
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::Popup | Qt::Window | Qt::WindowStaysOnTopHint); // Qt::Tool is important flag to make it work. I mean not stealing focus. 但对x11下Gnome44 无效
#ifdef __linux__
    QString dpEnv = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_TYPE");
    if (dpEnv.toUpper() == "X11")
    {
        this->setWindowFlag(Qt::X11BypassWindowManagerHint); // 在x11上不占用focus
    }
#elif __APPLE__ && TARGET_OS_MAC/* Need Test */
	this->setAttribute(Qt::WA_MacAlwaysShowToolWindow);
#endif
}

void SPopup::adjustGeometry(QPoint loc)
{
    this->adjustSize();

    QScreen *targetScreen = QGuiApplication::screenAt(loc);
    if (!targetScreen)
    {
        targetScreen = QGuiApplication::primaryScreen();
    }
    if (!targetScreen)
    {
        this->move(loc);
        return;
    }

    const QRect available = targetScreen->availableGeometry();
    const QPoint cursorPosition = loc;
    loc += QPoint(12, 16);
    if (loc.x() + width() > available.right() + 1)
    {
        loc.setX(cursorPosition.x() - width() - 12);
    }
    if (loc.y() + height() > available.bottom() + 1)
    {
        loc.setY(cursorPosition.y() - height() - 12);
    }
    const int maxX = qMax(available.left(), available.right() - this->width() + 1);
    const int maxY = qMax(available.top(), available.bottom() - this->height() + 1);
    loc.setX(qBound(available.left(), loc.x(), maxX));
    loc.setY(qBound(available.top(), loc.y(), maxY));

    this->setGeometry(QRect(loc, this->size()));
}

void SPopup::showExecutionStatus(const QString &message, bool loading, bool success)
{
    for (SPopupItem *item : m_items)
    {
        if (item)
        {
            item->setVisible(false);
        }
    }
    if (m_layout->indexOf(m_statusWidget) < 0)
    {
        m_layout->addWidget(m_statusWidget);
    }

    m_statusLoading = loading;
    m_loadingIndicator->setVisible(loading);
    m_statusLabel->setText(message);
    m_statusLabel->setStyleSheet(
        loading ? "color: #2F5F8F;"
                : (success ? "color: #16794B;" : "color: #B42318;"));
    m_statusWidget->setAccessibleName(message);
    m_statusWidget->setVisible(true);
    adjustToVisibleContent();

    if (loading)
    {
        m_timer->stop();
    }
    else
    {
        m_timer->start(STATUS_DISPLAY_TIMEOUT);
    }
}

void SPopup::adjustToVisibleContent()
{
    m_statusLabel->adjustSize();
    m_statusWidget->adjustSize();
    m_layout->invalidate();
    m_layout->activate();
    resize(m_layout->sizeHint().expandedTo(minimumSize()));

    QScreen *targetScreen = QGuiApplication::screenAt(frameGeometry().center());
    if (!targetScreen)
    {
        targetScreen = QGuiApplication::primaryScreen();
    }
    if (!targetScreen)
    {
        return;
    }

    const QRect available = targetScreen->availableGeometry();
    const int maxX = qMax(available.left(), available.right() - width() + 1);
    const int maxY = qMax(available.top(), available.bottom() - height() + 1);
    move(
        qBound(available.left(), x(), maxX),
        qBound(available.top(), y(), maxY));
}

void SPopup::restoreItems()
{
    m_statusLoading = false;
    if (m_statusWidget)
    {
        m_layout->removeWidget(m_statusWidget);
        m_statusWidget->setVisible(false);
    }
    for (SPopupItem *item : m_items)
    {
        if (item)
        {
            item->setVisible(item->enabled());
        }
    }
}

void SPopup::enterEvent(QEnterEvent *event)
{
    m_timer->stop();
    QWidget::enterEvent(event);
}

void SPopup::leaveEvent(QEvent *event)
{
    if (isVisible() && !m_statusLoading)
    {
        m_timer->start(ICON_TIMEOUT);
    }
    QWidget::leaveEvent(event);
}

void SPopup::keyPressEvent(QKeyEvent *event)
{
    if (event && event->key() == Qt::Key_Escape)
    {
        hide();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}
