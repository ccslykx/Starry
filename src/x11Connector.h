#pragma once

#include <QObject> // 要最先引入
#include <QElapsedTimer>
#include <X11/extensions/record.h>

typedef struct 
{
    int     x; // Left is 0
    int     y; // Top is 0;
    int     isPressed; // 0 is not pressed and others are pressed
} MouseStatus;

typedef enum 
{
    NotMoved,
    Left2Right,
    Right2Left,
    Top2Bottom,
    Bottom2Top,
    TopLeft2BottomRight,
    TopRight2BottomLeft,
    BottomLeft2TopRight,
    BottomRight2TopLeft
} MouseMotion;

class X11Connector : public QObject
{
    Q_OBJECT

public:
    static X11Connector* instance();
    void run();
    void stop();

public slots:
    static void handleRecordEvent(XRecordInterceptData *data);

signals:
    void B1Pressed(MouseStatus);
    void B1Released(MouseStatus);
    void B1DoubleClicked(MouseStatus);

private:
    X11Connector();
    ~X11Connector();
    void init();
    static void enableContext();

    static MouseMotion getMouseMotivation(MouseStatus startStatus, MouseStatus endStatus);

private:
    static X11Connector     *m_instance;

    static XRecordContext   m_context;
    static Display          *m_display;

    static MouseStatus      lastMouseStatus;
    static MouseStatus      currMouseStatus;

    static QElapsedTimer    *m_doubleClickTimer;
};