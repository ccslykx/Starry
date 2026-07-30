#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QPixmap>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include <memory>

#include "SConfig.h"
#include "SPluginInfo.h"

class SConfigTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void repairsInvalidAndDuplicateIndexes();
    void validatesNamesAndRejectsDuplicates();
    void keepsLookupConsistentAfterRename();
    void normalizesIndexesAfterDelete();

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

    QSettings settings(m_configDir->filePath("starry.conf"), QSettings::NativeFormat);
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
        settings.endGroup();
    }
    settings.endGroup();
    settings.beginGroup("STARRY_SETTINGS");
    settings.setValue("theme", "dark");
    settings.endGroup();
    settings.sync();
    QCOMPARE(settings.status(), QSettings::NoError);

    m_config = SConfig::config(m_configDir->path());
    m_config->readFromFile(m_configDir->path());
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

    SPluginInfo *alpha = makePlugin("Alpha");
    QVERIFY(m_config->addPlugin(alpha, NewCreate));
    QCOMPARE(alpha->index, 3);
}

void SConfigTests::keepsLookupConsistentAfterRename()
{
    SPluginInfo *alpha = m_config->getSPluginInfo("Alpha");
    QVERIFY(alpha);
    QVERIFY(m_config->renamePlugin(alpha, "Gamma"));
    QVERIFY(!m_config->getSPluginInfo("Alpha"));
    QCOMPARE(m_config->getSPluginInfo("Gamma"), alpha);

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

QTEST_MAIN(SConfigTests)

#include "SConfigTests.moc"
