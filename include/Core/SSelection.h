#pragma once

#include <QObject>
#include <QClipboard>

class SSelection : public QObject
{
    Q_OBJECT
public:
    static SSelection* instance();
    QString selection(); /* return m_selection */
    
signals:
    void started(); /* Started getting selection */
    void completed(); /* Got selection */

private:
    SSelection();
    void execute(); /* Try to get selection and set to m_selection */
    bool simulateCopy_win(); /* Simulate pushing Ctrl+C */
    bool simulateCopy_linux(); /* Simulate pushing Ctrl+C */
    bool simulateCopy_mac(); /* Simulate pushing Command+C */

private:
    static SSelection   *m_instance;
    static QClipboard   *m_clipboard;
    QString             m_selection;
};