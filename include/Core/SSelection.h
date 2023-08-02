#pragma once

#include <QObject>
#include <QClipboard>

class SSelection : public QObject
{
    Q_OBJECT
public:
    static SSelection* instance();
    void refresh(); /* return m_selection */
    QString selection();
    
signals:
    void started(); /* Started getting selection */
    void completed(); /* Got selection */
    void selectionChanged();

private:
    SSelection();
    QString simulateCopy_win(); /* Simulate pushing Ctrl+C */
    QString simulateCopy_linux(); /* Simulate pushing Ctrl+C */
    QString simulateCopy_mac(); /* Simulate pushing Command+C */

private:
    static SSelection   *m_instance;
    static QClipboard   *m_clipboard;
    QString             m_selection = "";
};