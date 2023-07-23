/**
 * @file AbstractMouseListener.h
 * @author Ccslykx (ccslykx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2023-07-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <QObject>

struct MouseStatus
{
    int     x; // Left is 0
    int     y; // Top is 0;
    int     isPressed; // 0 is not pressed and others are pressed
};

enum MouseMotion
{
    NotMoved,
    Left2Right,
    Right2Left,
    Top2Bottom,
    Bottom2Top,
    TopLeft2BottomRight,
    TopRight2BottomLeft,
    BottomLeft2TopRight,
    BottomRight2TopLeft,
    Undefined
} ;

class AbstractMouseListener : public QObject
{
    Q_OBJECT

public:
    virtual void startListen() = 0;
    virtual void stopListen() = 0;

    static MouseMotion getMouseMotivation(MouseStatus startStatus, MouseStatus endStatus)
    {
        if (startStatus.x == endStatus.x && startStatus.y == endStatus.y)
        {
            return NotMoved;
        }
        else if (startStatus.x < endStatus.x && startStatus.y == endStatus.y)
        {
            return Left2Right;
        }
        else if (startStatus.x > endStatus.x && startStatus.y == endStatus.y)
        {
            return Right2Left;
        }
        else if (startStatus.x == endStatus.x && startStatus.y < endStatus.y)
        {
            return Top2Bottom;
        }
        else if (startStatus.x == endStatus.x && startStatus.y > endStatus.y)
        {
            return Bottom2Top;
        }
        if (startStatus.x < endStatus.x && startStatus.y < endStatus.y)
        {
            return TopLeft2BottomRight;
        } 
        else if (startStatus.x < endStatus.x && startStatus.y > endStatus.y)
        {
            return BottomLeft2TopRight;
        }
        else if (startStatus.x > endStatus.x && startStatus.y < endStatus.y)
        {
            return TopRight2BottomLeft;
        }
        else if (startStatus.x > endStatus.x && startStatus.y > endStatus.y)
        {
            return BottomRight2TopLeft;
        }
        return Undefined;
    };

signals:
    void B1Pressed(MouseStatus);
    void B1Released(MouseStatus);
    void B1DoubleClicked(MouseStatus); 

};
