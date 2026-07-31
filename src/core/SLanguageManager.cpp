#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>

#include "SLanguageManager.h"

SLanguageManager *SLanguageManager::m_instance = nullptr;

SLanguageManager *SLanguageManager::instance()
{
    if (!m_instance)
    {
        m_instance = new SLanguageManager(QCoreApplication::instance());
        QObject::connect(m_instance, &QObject::destroyed, [] {
            m_instance = nullptr;
        });
    }
    return m_instance;
}

QVector<SLanguageManager::Language> SLanguageManager::supportedLanguages()
{
    return {
        {QStringLiteral("zh_CN"), QStringLiteral("简体中文")},
        {QStringLiteral("zh_TW"), QStringLiteral("繁體中文")},
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("de"), QStringLiteral("Deutsch")},
        {QStringLiteral("fr"), QStringLiteral("Français")},
        {QStringLiteral("ja"), QStringLiteral("日本語")},
    };
}

QString SLanguageManager::systemLanguageCode()
{
    const QLocale locale = QLocale::system();
    switch (locale.language())
    {
    case QLocale::Chinese:
        switch (locale.territory())
        {
        case QLocale::Taiwan:
        case QLocale::HongKong:
        case QLocale::Macao:
            return QStringLiteral("zh_TW");
        default:
            return QStringLiteral("zh_CN");
        }
    case QLocale::German:
        return QStringLiteral("de");
    case QLocale::French:
        return QStringLiteral("fr");
    case QLocale::Japanese:
        return QStringLiteral("ja");
    default:
        return QStringLiteral("en");
    }
}

QString SLanguageManager::normalizedLanguageCode(const QString &code)
{
    QString normalized = code.trimmed();
    normalized.replace(QLatin1Char('-'), QLatin1Char('_'));
    if (normalized.compare(QStringLiteral("zh_TW"), Qt::CaseInsensitive) == 0
        || normalized.compare(QStringLiteral("zh_HK"), Qt::CaseInsensitive) == 0
        || normalized.compare(QStringLiteral("zh_MO"), Qt::CaseInsensitive) == 0)
    {
        return QStringLiteral("zh_TW");
    }
    if (normalized.startsWith(QStringLiteral("zh"), Qt::CaseInsensitive))
    {
        return QStringLiteral("zh_CN");
    }
    if (normalized.startsWith(QStringLiteral("de"), Qt::CaseInsensitive))
    {
        return QStringLiteral("de");
    }
    if (normalized.startsWith(QStringLiteral("fr"), Qt::CaseInsensitive))
    {
        return QStringLiteral("fr");
    }
    if (normalized.startsWith(QStringLiteral("ja"), Qt::CaseInsensitive))
    {
        return QStringLiteral("ja");
    }
    if (normalized.startsWith(QStringLiteral("en"), Qt::CaseInsensitive))
    {
        return QStringLiteral("en");
    }
    return QString();
}

QString SLanguageManager::currentLanguageCode() const
{
    return m_currentLanguageCode.isEmpty()
        ? QStringLiteral("en")
        : m_currentLanguageCode;
}

bool SLanguageManager::setLanguage(const QString &code)
{
    QString normalized = normalizedLanguageCode(code);
    if (normalized.isEmpty())
    {
        normalized = systemLanguageCode();
    }
    if (normalized == m_currentLanguageCode)
    {
        return true;
    }

    QTranslator *nextTranslator = nullptr;
    if (normalized != QStringLiteral("en"))
    {
        nextTranslator = new QTranslator(this);
        if (!nextTranslator->load(
                QStringLiteral(":/i18n/starry_%1.qm").arg(normalized)))
        {
            delete nextTranslator;
            return false;
        }
    }

    if (m_translator)
    {
        QCoreApplication::removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }
    if (nextTranslator)
    {
        QCoreApplication::installTranslator(nextTranslator);
        m_translator = nextTranslator;
    }

    m_currentLanguageCode = normalized;
    emit languageChanged(normalized);
    return true;
}

SLanguageManager::SLanguageManager(QObject *parent)
    : QObject(parent)
{
}
