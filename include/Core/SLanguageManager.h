#pragma once

#include <QObject>
#include <QString>
#include <QVector>

class QTranslator;

class SLanguageManager final : public QObject
{
    Q_OBJECT

public:
    struct Language
    {
        QString code;
        QString nativeName;
    };

    static SLanguageManager *instance();
    static QVector<Language> supportedLanguages();
    static QString systemLanguageCode();
    static QString normalizedLanguageCode(const QString &code);

    QString currentLanguageCode() const;
    bool setLanguage(const QString &code);

signals:
    void languageChanged(const QString &code);

private:
    explicit SLanguageManager(QObject *parent = nullptr);

    static SLanguageManager *m_instance;
    QTranslator *m_translator = nullptr;
    QString m_currentLanguageCode;
};
