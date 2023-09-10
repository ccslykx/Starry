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
    QString getSelection_win();
    QString getSelection_linux();
    QString getSelection_mac();

private:
    static SSelection   *m_instance;
    static QClipboard   *m_clipboard;
    QString             m_selection = "";
};