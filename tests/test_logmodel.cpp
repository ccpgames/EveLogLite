#include <QtTest/QtTest>
#include <QSettings>
#include <QTemporaryDir>
#include <QDir>
#include <QDateTime>

#include "logmodel.h"

// Test seam: LogModel receives messages over a socket (a private slot) and
// applies its retention policy after each batch through a protected helper.
// This subclass re-exposes just those two protected entry points so the tests
// can drive the exact same code paths the socket receive loop drives, without
// opening a real connection and without adding production API.
class TestableLogModel : public LogModel
{
public:
    using AbstractLogModel::addMessage;
    using LogModel::enforceRetentionPolicy;
};

namespace
{

// Builds a single head log message (not a multiline continuation) carrying a
// monotonic sequence number in its text, so tests can identify individual rows.
LogMessage* makeHead(int sequence, LogSeverity severity)
{
    auto message = new LogMessage;
    message->timestamp = QDateTime::currentDateTime();
    message->pid = 1;
    message->severity = severity;
    message->machineName = "test";
    message->executablePath = "test";
    message->module = "test";
    message->channel = "test";
    message->message = QString::number(sequence);
    return message;
}

int lswFileCount(const QString& directory)
{
    return QDir(directory).entryList(QStringList() << "*.lsw", QDir::Files).size();
}

}

class TestLogModel : public QObject
{
    Q_OBJECT

private slots:
    void freshModelHasOnlyThePhantomRow();

    // Behavior-equivalence guarantees for the RetentionMode refactor: Server and
    // Unbounded must behave exactly as before, migration must preserve existing
    // users, and Rolling must be defined but inert (no eviction) for now.
    void serverModeAutosavesAndClearsAtCap();
    void unboundedGrowsPastCap();
    void legacyServerModeMigratesToServer();
    void absentSettingsMigrateToUnbounded();
    void storedRollingModeRoutesThroughUnclamped();
    void rollingModeIsInertAndGrowsLikeUnbounded();
};

void TestLogModel::freshModelHasOnlyThePhantomRow()
{
    LogModel model;

    // rowCount() is the real buffer size plus one trailing sentinel row used by
    // the view; a fresh model has an empty buffer, so rowCount() == 0 + 1.
    QCOMPARE(model.rowCount(), 1);
}

void TestLogModel::serverModeAutosavesAndClearsAtCap()
{
    QTemporaryDir saveDir;
    QVERIFY(saveDir.isValid());

    const int cap = 8;
    TestableLogModel model;
    model.setAutoSaveDirectory(saveDir.path());
    model.setMaxMessages(cap);
    model.setRetentionMode(RetentionMode::Server);

    for (int i = 0; i < cap; ++i)
    {
        model.addMessage(makeHead(i, SEVERITY_INFO));
    }
    // Reaching the cap in Server mode must save the whole buffer to a file and
    // then wipe it (save-all-then-clear), leaving only the phantom row.
    model.enforceRetentionPolicy();

    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(lswFileCount(saveDir.path()), 1);
}

void TestLogModel::unboundedGrowsPastCap()
{
    QTemporaryDir saveDir;
    QVERIFY(saveDir.isValid());

    const int cap = 8;
    TestableLogModel model;
    model.setAutoSaveDirectory(saveDir.path());
    model.setMaxMessages(cap);
    // Unbounded is the default; set it explicitly to document intent.
    model.setRetentionMode(RetentionMode::Unbounded);

    const int total = 3 * cap;
    for (int i = 0; i < total; ++i)
    {
        model.addMessage(makeHead(i, SEVERITY_INFO));
    }
    model.enforceRetentionPolicy();

    // Nothing is evicted or saved: the buffer holds every message plus the
    // phantom row.
    QCOMPARE(model.rowCount(), total + 1);
    QCOMPARE(lswFileCount(saveDir.path()), 0);
}

void TestLogModel::legacyServerModeMigratesToServer()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings settings(dir.filePath("settings.ini"), QSettings::IniFormat);
    settings.setValue("serverMode", true);

    QCOMPARE(int(LogModel::retentionModeFromSettings(settings)), int(RetentionMode::Server));
}

void TestLogModel::absentSettingsMigrateToUnbounded()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings settings(dir.filePath("settings.ini"), QSettings::IniFormat);

    // No retentionMode and no serverMode key at all: a first-run user defaults
    // to Unbounded.
    QCOMPARE(int(LogModel::retentionModeFromSettings(settings)), int(RetentionMode::Unbounded));

    // An explicit legacy serverMode=false migrates to Unbounded too.
    settings.setValue("serverMode", false);
    QCOMPARE(int(LogModel::retentionModeFromSettings(settings)), int(RetentionMode::Unbounded));
}

void TestLogModel::storedRollingModeRoutesThroughUnclamped()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings settings(dir.filePath("settings.ini"), QSettings::IniFormat);
    // A stored Rolling value must survive load unchanged even though there is no
    // UI control to select it yet; it must not be coerced back to Server/Unbounded.
    settings.setValue("retentionMode", 2);

    QCOMPARE(int(LogModel::retentionModeFromSettings(settings)), int(RetentionMode::Rolling));

    // An explicit retentionMode is authoritative over any legacy serverMode key.
    settings.setValue("serverMode", true);
    settings.setValue("retentionMode", 0);
    QCOMPARE(int(LogModel::retentionModeFromSettings(settings)), int(RetentionMode::Unbounded));
}

void TestLogModel::rollingModeIsInertAndGrowsLikeUnbounded()
{
    QTemporaryDir saveDir;
    QVERIFY(saveDir.isValid());

    const int cap = 8;
    TestableLogModel model;
    model.setAutoSaveDirectory(saveDir.path());
    model.setMaxMessages(cap);
    // Force Rolling through the same routing the settings loader uses, proving
    // the value is honored and not clamped away.
    model.setRetentionMode(static_cast<RetentionMode>(2));
    QCOMPARE(int(model.retentionMode()), int(RetentionMode::Rolling));

    const int total = 3 * cap;
    for (int i = 0; i < total; ++i)
    {
        model.addMessage(makeHead(i, SEVERITY_INFO));
    }
    model.enforceRetentionPolicy();

    // Rolling eviction is not implemented yet, so the buffer grows unbounded and
    // nothing is written to disk.
    QCOMPARE(model.rowCount(), total + 1);
    QCOMPARE(lswFileCount(saveDir.path()), 0);
}

// QTEST_MAIN (a real QApplication), not QTEST_GUILESS_MAIN: LogModel's
// constructor builds QPixmap severity icons, and QPixmap construction requires
// a QGuiApplication with a platform plugin loaded, which a bare QCoreApplication
// does not provide. CMake sets QT_QPA_PLATFORM=minimal for this test so it
// still runs without a real display.
QTEST_MAIN(TestLogModel)
#include "test_logmodel.moc"
