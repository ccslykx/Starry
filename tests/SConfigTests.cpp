#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QImage>
#include <QPixmap>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

#include "SConfig.h"
#include "SPluginInfo.h"

namespace
{
QSettings::Format configFileFormat()
{
#ifdef Q_OS_WIN
    return QSettings::IniFormat;
#else
    return QSettings::NativeFormat;
#endif
}
}

class SConfigTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void generatesDefaultCharacterIcons();
    void repairsInvalidAndDuplicateIndexes();
    void validatesNamesAndRejectsDuplicates();
    void keepsLookupConsistentAfterRename();
    void normalizesIndexesAfterDelete();
    void persistsSelectionPopupState();
    void persistsDebugMode();
    void persistsLanguage();

private:
    static SPluginInfo *makePlugin(const QString &name, const int index = 0);

    std::unique_ptr<QTemporaryDir> m_configDir;
    SConfig *m_config = nullptr;
};

SPluginInfo *SConfigTests::makePlugin(const QString &name, const int index)
{
    QPixmap icon(2, 2);
    icon.fill(Qt::black);
    return new SPluginInfo(name, "echo test", icon, index, "test", true, true);
}

void SConfigTests::initTestCase()
{
    m_configDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_configDir->isValid());
    QVERIFY(QDir().mkpath(m_configDir->filePath("icons")));

    QSettings settings(m_configDir->filePath("starry.conf"), configFileFormat());
    const QString legacyDefaultIconPath =
        m_configDir->filePath("icons/legacy-default.png");
    const QString customIconPath =
        m_configDir->filePath("icons/custom.png");
    QVERIFY(QPixmap(QStringLiteral(":/default_icon.png")).save(
        legacyDefaultIconPath));
    QPixmap customIcon(16, 16);
    customIcon.fill(Qt::blue);
    QVERIFY(customIcon.save(customIconPath));

    settings.beginGroup("STARRY_PLUGINS");
    const QList<QPair<QString, int>> plugins{
        {"HighIndex", 999},
        {"NegativeIndex", -4},
        {"DuplicateIndex", 999},
    };
    for (const auto &[name, index] : plugins)
    {
        settings.beginGroup(name);
        settings.setValue("script", "echo test");
        settings.setValue("index", index);
        settings.setValue("iconEnabled", true);
        settings.setValue("nameEnabled", true);
        if (name == QStringLiteral("HighIndex"))
        {
            settings.setValue("iconPath", legacyDefaultIconPath);
        }
        else if (name == QStringLiteral("DuplicateIndex"))
        {
            settings.setValue("iconPath", customIconPath);
        }
        settings.endGroup();
    }
    settings.endGroup();
    settings.beginGroup("STARRY_SETTINGS");
    settings.setValue("theme", "dark");
    settings.setValue("language", "de");
    settings.endGroup();
    settings.sync();
    QCOMPARE(settings.status(), QSettings::NoError);

    m_config = SConfig::config(m_configDir->path());
    m_config->readFromFile(m_configDir->path());
}

void SConfigTests::generatesDefaultCharacterIcons()
{
    QCOMPARE(SPluginInfo::defaultIconText(QStringLiteral("翻译")),
             QStringLiteral("翻"));
    QCOMPARE(SPluginInfo::defaultIconText(QStringLiteral("search")),
             QStringLiteral("S"));
    QCOMPARE(SPluginInfo::defaultIconText(QStringLiteral("Google Search")),
             QStringLiteral("GS"));
    QCOMPARE(SPluginInfo::defaultIconText(QStringLiteral("quick-browse")),
             QStringLiteral("QB"));
    QCOMPARE(SPluginInfo::defaultIconText(QStringLiteral("123")),
             QStringLiteral("?"));

    const QPixmap icon = SPluginInfo::createDefaultIcon(
        QStringLiteral("Google Search"));
    QVERIFY(!icon.isNull());
    QCOMPARE(icon.size(), QSize(96, 96));
}

void SConfigTests::repairsInvalidAndDuplicateIndexes()
{
    const QVector<SPluginInfo *> plugins = m_config->getSPluginInfos();
    QCOMPARE(plugins.size(), 3);
    for (qsizetype index = 0; index < plugins.size(); ++index)
    {
        QCOMPARE(plugins.at(index)->index, static_cast<int>(index));
    }
    QCOMPARE(m_config->getSetting("theme").toString(), QString("dark"));
    QCOMPARE(m_config->languageCode(), QString("de"));
    QVERIFY(m_config->getSPluginInfo("HighIndex")->usesDefaultIcon);
    QVERIFY(m_config->getSPluginInfo("NegativeIndex")->usesDefaultIcon);
    QVERIFY(!m_config->getSPluginInfo("DuplicateIndex")->usesDefaultIcon);
    QVERIFY(!m_config->getSPluginInfo("HighIndex")->icon.isNull());
    QVERIFY(!m_config->getSPluginInfo("NegativeIndex")->icon.isNull());
    QVERIFY(!m_config->getSPluginInfo("DuplicateIndex")->icon.isNull());
    QCOMPARE(
        m_config->getSPluginInfo("DuplicateIndex")->icon.toImage().pixelColor(0, 0),
        QColor(Qt::blue));
}

void SConfigTests::validatesNamesAndRejectsDuplicates()
{
    QVERIFY(!m_config->isPluginNameValid(""));
    QVERIFY(!m_config->isPluginNameValid(" plugin"));
    QVERIFY(!m_config->isPluginNameValid("plugin/name"));
    QVERIFY(m_config->isPluginNameValid("Plugin"));

    SPluginInfo *duplicate = makePlugin("HighIndex");
    QVERIFY(!m_config->addPlugin(duplicate, NewCreate));
    duplicate->deleteLater();

    SPluginInfo *alpha = new SPluginInfo(
        QStringLiteral("Alpha"),
        QStringLiteral("echo test"),
        QPixmap(),
        0,
        QStringLiteral("test"),
        true,
        true,
        true);
    QVERIFY(m_config->addPlugin(alpha, NewCreate));
    QCOMPARE(alpha->index, 3);
}

void SConfigTests::keepsLookupConsistentAfterRename()
{
    SPluginInfo *alpha = m_config->getSPluginInfo("Alpha");
    QVERIFY(alpha);
    const QImage oldDefaultIcon = alpha->icon.toImage();
    QVERIFY(m_config->renamePlugin(alpha, "Gamma"));
    QVERIFY(!m_config->getSPluginInfo("Alpha"));
    QCOMPARE(m_config->getSPluginInfo("Gamma"), alpha);
    QVERIFY(alpha->usesDefaultIcon);
    QVERIFY(alpha->icon.toImage() != oldDefaultIcon);

    SPluginInfo *duplicateName = makePlugin("HighIndex");
    QVERIFY(!m_config->renamePlugin(alpha, duplicateName->name));
    duplicateName->deleteLater();
    QCOMPARE(m_config->getSPluginInfo("Gamma"), alpha);
}

void SConfigTests::normalizesIndexesAfterDelete()
{
    SPluginInfo *beta = makePlugin("Beta");
    QVERIFY(m_config->addPlugin(beta, NewCreate));
    QCOMPARE(beta->index, 4);

    SPluginInfo *gamma = m_config->getSPluginInfo("Gamma");
    QVERIFY(gamma);
    emit gamma->needDelete(gamma);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    QVERIFY(!m_config->getSPluginInfo("Gamma"));
    const QVector<SPluginInfo *> plugins = m_config->getSPluginInfos();
    QCOMPARE(plugins.size(), 4);
    for (qsizetype index = 0; index < plugins.size(); ++index)
    {
        QCOMPARE(plugins.at(index)->index, static_cast<int>(index));
    }
    QCOMPARE(beta->index, 3);
}

void SConfigTests::persistsSelectionPopupState()
{
    QVERIFY(m_config->selectionPopupEnabled());
    QSignalSpy selectionPopupSpy(
        m_config, &SConfig::selectionPopupEnabledChanged);

    m_config->setSelectionPopupEnabled(false);
    QVERIFY(!m_config->selectionPopupEnabled());
    QCOMPARE(selectionPopupSpy.count(), 1);

    m_config->saveToFile(m_configDir->path());
    QSettings settings(m_configDir->filePath("starry.conf"), configFileFormat());
    QCOMPARE(
        settings.value(
            QStringLiteral("STARRY_SETTINGS/selectionPopupEnabled")).toBool(),
        false);

    m_config->setSelectionPopupEnabled(true);
    QVERIFY(m_config->selectionPopupEnabled());
    QCOMPARE(selectionPopupSpy.count(), 2);
}

void SConfigTests::persistsDebugMode()
{
    QVERIFY(!m_config->debugModeEnabled());
    QSignalSpy debugModeSpy(m_config, &SConfig::debugModeChanged);

    m_config->setDebugModeEnabled(true);
    QVERIFY(m_config->debugModeEnabled());
    QCOMPARE(debugModeSpy.count(), 1);

    m_config->saveToFile(m_configDir->path());
    QSettings settings(m_configDir->filePath("starry.conf"), configFileFormat());
    QCOMPARE(
        settings.value(QStringLiteral("STARRY_SETTINGS/debugModeEnabled")).toBool(),
        true);

    m_config->setDebugModeEnabled(false);
    QVERIFY(!m_config->debugModeEnabled());
    QCOMPARE(debugModeSpy.count(), 2);
}

void SConfigTests::persistsLanguage()
{
    QSignalSpy languageSpy(m_config, &SConfig::languageChanged);

    m_config->setLanguageCode(QStringLiteral("zh-HK"));
    QCOMPARE(m_config->languageCode(), QStringLiteral("zh_TW"));
    QCOMPARE(languageSpy.count(), 1);

    m_config->setLanguageCode(QStringLiteral("ja-JP"));
    QCOMPARE(m_config->languageCode(), QStringLiteral("ja"));
    QCOMPARE(languageSpy.count(), 2);

    m_config->saveToFile(m_configDir->path());
    QSettings settings(m_configDir->filePath("starry.conf"), configFileFormat());
    QCOMPARE(
        settings.value(QStringLiteral("STARRY_SETTINGS/language")).toString(),
        QStringLiteral("ja"));
}

QTEST_MAIN(SConfigTests)

#include "SConfigTests.moc"
