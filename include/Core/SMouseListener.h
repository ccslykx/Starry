#pragma once

#include <QPoint>
#include <QTimer>
#include <QClipboard>

#include "AbstractMouseListener.h"
#include "SSelection.h"

class SMouseListener : public AbstractMouseListener
{
    Q_OBJECT
public:
    static SMouseListener* instance();
    AbstractMouseListener* listener();
    void startListen() override;
    void stopListen() override;

signals:
    void canShow();

private:
    SMouseListener();
    ~SMouseListener();
    
    void init();
        /**
     * @brief 在鼠标划词松开后，等待`selectionChange`信号。如果未发出该信号，
     * 可能是鼠标发生了拖拽行为（而非选词）
     */
    void waitForSelectionChange();
    /**
     * @brief 如果`selection`发生了变化，可能是用键盘选择了其他内容，也可能是
     * 使用了中文输入法。如果是选择了其他内容，一定会发生`B1Released`事件（包括
     * 双击选择），因此只有发生`B1Released`后才可能调用`SPopup`的`showPopup()`。
     */
    void waitForB1Release();
    void onB1Pressed(MouseStatus);
    void onB1Released(MouseStatus);
    void onB1DoubleClicked(MouseStatus);

private:
    static SMouseListener   *m_instance;
    AbstractMouseListener   *m_listener = nullptr;
    SSelection              *m_selection = nullptr;

    QPoint  m_pressPos;
    QPoint  m_releasePos;
    bool    m_selectionChanged = false;
    bool    m_B1Released = false;

    QTimer  *m_waitSelectionChangeTimer = nullptr; // 在B1Released后等待QClipboard::selectionChanged信号
    QTimer  *m_waitB1ReleaseTimer = nullptr; // 在selectionChanged信号发出后等待B1Released信号
};