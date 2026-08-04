#include "SPluginInfo.h"

#include <QPainter>
#include <QRegularExpression>

#include "SConfig.h"
#include "utils.h"

SPluginInfo::SPluginInfo(
    const QString &_name,     
    const QString &_script,   
    const QString &_iconPath, 
    const int _index,         
    const QString &_tip,      
    const bool _iconEnabled,
    const bool _nameEnabled,
    const bool _usesDefaultIcon)
    : name(_name)
    , tip(_tip)
    , script(_script)
    , iconPath(_iconPath)
    , usesDefaultIcon(_usesDefaultIcon || _iconPath.isEmpty())
    , index(_index)
    , iconEnabled(_iconEnabled)
    , nameEnabled(_nameEnabled)
{
    SDEBUG
    icon = usesDefaultIcon ? createDefaultIcon(name) : QPixmap(iconPath);
}

SPluginInfo::SPluginInfo(
    const QString &_name,   
    const QString &_script, 
    const QPixmap &_icon,   
    const int _index,       
    const QString &_tip,    
    const bool _iconEnabled,
    const bool _nameEnabled,
    const bool _usesDefaultIcon)
    : name(_name)
    , tip(_tip)
    , script(_script)
    , icon((_usesDefaultIcon || _icon.isNull())
               ? createDefaultIcon(_name)
               : _icon)
    , usesDefaultIcon(_usesDefaultIcon || _icon.isNull())
    , index(_index)
    , iconEnabled(_iconEnabled)
    , nameEnabled(_nameEnabled)
{
    SDEBUG
}

QString SPluginInfo::defaultIconText(const QString &name)
{
    const QString trimmedName = name.trimmed();
    const QRegularExpression firstLetterExpression(
        QStringLiteral("(\\p{L})"));
    const QRegularExpressionMatch firstLetterMatch =
        firstLetterExpression.match(trimmedName);
    if (!firstLetterMatch.hasMatch())
    {
        return QStringLiteral("?");
    }

    const QString firstLetter = firstLetterMatch.captured(1);
    const QRegularExpression latinLetterExpression(
        QStringLiteral("^\\p{Latin}$"));
    if (latinLetterExpression.match(firstLetter).hasMatch())
    {
        const QRegularExpression latinWordExpression(
            QStringLiteral("\\p{Latin}+(?:['’]\\p{Latin}+)*"));
        QRegularExpressionMatchIterator words =
            latinWordExpression.globalMatch(trimmedName);
        QString initials;
        while (words.hasNext() && initials.size() < 2)
        {
            const QString word = words.next().captured();
            if (!word.isEmpty())
            {
                initials += word.left(1).toUpper();
            }
        }
        if (initials.size() >= 2)
        {
            return initials.left(2);
        }
    }

    return firstLetter.toUpper();
}

QPixmap SPluginInfo::createDefaultIcon(const QString &name, const QSize &size)
{
    const QSize iconSize = size.isValid() ? size : QSize(96, 96);
    QPixmap pixmap(iconSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#F97316")));
    const qreal radius = qMin(iconSize.width(), iconSize.height()) * 0.2;
    painter.drawRoundedRect(QRectF(QPointF(0, 0), QSizeF(iconSize)),
                            radius, radius);

    const QString text = defaultIconText(name);
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(qMax(
        12,
        qMin(iconSize.width(), iconSize.height())
            * (text.size() > 1 ? 38 : 50) / 96));
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, text);
    return pixmap;
}

void SPluginInfo::refreshDefaultIcon()
{
    if (usesDefaultIcon)
    {
        icon = createDefaultIcon(name);
    }
}

/* Private Functions */

SPluginInfo::~SPluginInfo()
{
    
}
