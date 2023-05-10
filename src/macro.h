#include <QDebug>
#include <qglobal.h>

#define STARRY_VERSION_MAJOR QString("0")
#define STARRY_VERSION_MINOR QString("1")
#define STARRY_VERSION_PATCH QString("0")
#define STARRY_VERSION STARRY_VERSION_MAJOR + "." + STARRY_VERSION_MINOR + "." + STARRY_VERSION_PATCH

// #define DEBUG

#ifdef DEBUG
    #define STARRY_DEBUGER qDebug() << "[FILE:" << __FILE__ << ", LINE:" << __LINE__ << ", FUNC:" << Q_FUNC_INFO << "]";
#else
    #define STARRY_DEBUGER
#endif