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
    using LogModel::recordMessage;
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

// Builds a head message whose text is several newline-separated lines. When the
// model has "break lines" on, recording this explodes into one head row plus one
// continuation row per extra line.
LogMessage* makeMultiline(int sequence, int lineCount, LogSeverity severity)
{
    auto message = makeHead(sequence, severity);
    QStringList lines;
    for (int i = 0; i < lineCount; ++i)
    {
        lines << QString("%1.%2").arg(sequence).arg(i);
    }
    message->message = lines.join('\n');
    return message;
}

int lswFileCount(const QString& directory)
{
    return QDir(directory).entryList(QStringList() << "*.lsw", QDir::Files).size();
}

// Real rows exclude the trailing sentinel row that rowCount() adds on top of the
// buffer size.
int realRowCount(const AbstractLogModel& model)
{
    return model.rowCount() - 1;
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

    // Rolling eviction (the core of this slice).
    void rollingBoundsBufferAtCap();
    void rollingDropsOldestKeepsNewest();
    void rollingStaysAtCapContinuously();
    void rollingSwitchingIntoModeTrimsImmediately();
    void rollingLoweringCapTrimsImmediately();
    void rollingStatisticsTrackBufferContents();
    void rollingMultilineStraddleIsSafe();
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

void TestLogModel::rollingBoundsBufferAtCap()
{
    QTemporaryDir saveDir;
    QVERIFY(saveDir.isValid());

    const int cap = 8;
    const int extra = 5;
    TestableLogModel model;
    model.setAutoSaveDirectory(saveDir.path());
    model.setMaxMessages(cap);
    model.setRetentionMode(RetentionMode::Rolling);

    for (int i = 0; i < cap + extra; ++i)
    {
        model.recordMessage(makeHead(i, SEVERITY_INFO));
    }
    model.enforceRetentionPolicy();

    // The buffer holds exactly the cap, and nothing was written to disk (Rolling
    // never autosaves).
    QCOMPARE(realRowCount(model), cap);
    QCOMPARE(lswFileCount(saveDir.path()), 0);
}

void TestLogModel::rollingDropsOldestKeepsNewest()
{
    const int cap = 8;
    const int extra = 5;
    TestableLogModel model;
    model.setMaxMessages(cap);
    model.setRetentionMode(RetentionMode::Rolling);

    // Each head carries its arrival sequence in the text; after the flood the
    // oldest `extra` are gone and the newest `cap` remain in order.
    for (int i = 0; i < cap + extra; ++i)
    {
        model.recordMessage(makeHead(i, SEVERITY_INFO));
        model.enforceRetentionPolicy();
    }

    QCOMPARE(realRowCount(model), cap);
    QCOMPARE(model.message(0)->message, QString::number(extra));
    QCOMPARE(model.message(cap - 1)->message, QString::number(cap + extra - 1));

    // Every surviving sequence is exactly the newest window, in order, and every
    // evicted sequence is absent.
    for (int row = 0; row < cap; ++row)
    {
        QCOMPARE(model.message(row)->message, QString::number(extra + row));
    }
}

void TestLogModel::rollingStaysAtCapContinuously()
{
    const int cap = 8;
    const int feeds = 60 * cap;
    TestableLogModel model;
    model.setMaxMessages(cap);
    model.setRetentionMode(RetentionMode::Rolling);

    // One-in-one-out: after every single message past the cap the real count must
    // be exactly the cap the whole way through, never cap+1 and never collapsing
    // to zero.
    for (int i = 0; i < feeds; ++i)
    {
        model.recordMessage(makeHead(i, SEVERITY_INFO));
        model.enforceRetentionPolicy();

        const int expected = (i + 1) < cap ? (i + 1) : cap;
        QCOMPARE(realRowCount(model), expected);
    }
}

void TestLogModel::rollingSwitchingIntoModeTrimsImmediately()
{
    const int cap = 8;
    const int total = 3 * cap;
    TestableLogModel model;
    model.setMaxMessages(cap);
    // Grow past the cap while Unbounded, so switching into Rolling has an
    // over-cap buffer to trim.
    model.setRetentionMode(RetentionMode::Unbounded);
    for (int i = 0; i < total; ++i)
    {
        model.recordMessage(makeHead(i, SEVERITY_INFO));
    }
    QCOMPARE(realRowCount(model), total);

    // Switching into Rolling trims down to the cap at once, without waiting for
    // the next message.
    model.setRetentionMode(RetentionMode::Rolling);
    QCOMPARE(realRowCount(model), cap);
    QCOMPARE(model.message(0)->message, QString::number(total - cap));
}

void TestLogModel::rollingLoweringCapTrimsImmediately()
{
    const int cap = 20;
    TestableLogModel model;
    model.setMaxMessages(cap);
    model.setRetentionMode(RetentionMode::Rolling);
    for (int i = 0; i < cap; ++i)
    {
        model.recordMessage(makeHead(i, SEVERITY_INFO));
        model.enforceRetentionPolicy();
    }
    QCOMPARE(realRowCount(model), cap);

    // Lowering the cap under Rolling trims the buffer down to the new cap
    // immediately.
    const int lowered = 5;
    model.setMaxMessages(lowered);
    QCOMPARE(realRowCount(model), lowered);
    QCOMPARE(model.message(0)->message, QString::number(cap - lowered));
}

void TestLogModel::rollingStatisticsTrackBufferContents()
{
    const int cap = 8;
    const int extra = 10;
    TestableLogModel model;
    model.setMaxMessages(cap);
    model.setRetentionMode(RetentionMode::Rolling);

    // A repeating mix of all four severities, so eviction has to decrement the
    // right per-severity counter and not just a single one.
    const LogSeverity cycle[] = { SEVERITY_ERR, SEVERITY_WARN, SEVERITY_NOTICE, SEVERITY_INFO };
    for (int i = 0; i < cap + extra; ++i)
    {
        model.recordMessage(makeHead(i, cycle[i % 4]));
        model.enforceRetentionPolicy();
    }
    QCOMPARE(realRowCount(model), cap);

    // Recompute the expected counts directly from the surviving head rows and
    // compare against the model's statistics; they must match exactly and none
    // may be negative.
    int expectedError = 0, expectedWarning = 0, expectedNotice = 0, expectedInfo = 0;
    for (int row = 0; row < cap; ++row)
    {
        switch (model.message(row)->severity)
        {
        case SEVERITY_ERR: ++expectedError; break;
        case SEVERITY_WARN: ++expectedWarning; break;
        case SEVERITY_NOTICE: ++expectedNotice; break;
        case SEVERITY_INFO: ++expectedInfo; break;
        default: break;
        }
    }

    const auto& stats = model.statistics();
    QCOMPARE(stats.error, expectedError);
    QCOMPARE(stats.warning, expectedWarning);
    QCOMPARE(stats.notice, expectedNotice);
    QCOMPARE(stats.info, expectedInfo);
    QVERIFY(stats.error >= 0);
    QVERIFY(stats.warning >= 0);
    QVERIFY(stats.notice >= 0);
    QVERIFY(stats.info >= 0);
}

void TestLogModel::rollingMultilineStraddleIsSafe()
{
    const int cap = 4;
    TestableLogModel model;
    model.setMaxMessages(cap);
    model.setBreakLines(true);
    model.setRetentionMode(RetentionMode::Rolling);

    // A multiline message explodes into a head plus three continuation rows.
    // Recording it first, then flooding single heads, forces eviction to cut
    // through the middle of that group (dropping the head, then continuations,
    // one per feed). Stopping after cap-1 heads leaves one orphaned continuation
    // still at the front of the buffer.
    model.recordMessage(makeMultiline(0, 4, SEVERITY_INFO));
    for (int i = 1; i < cap; ++i)
    {
        model.recordMessage(makeHead(i, SEVERITY_INFO));
        model.enforceRetentionPolicy();
    }

    // No crash, no double-free (verified under the Tester's debug/ASan build),
    // and the row cap still holds even with a split multiline group. The front
    // row is a continuation whose head was evicted.
    QCOMPARE(realRowCount(model), cap);
    QVERIFY(model.message(0)->isMultilineContinuation);

    // Collapsing back with a leading orphan continuation at the front must also
    // be safe: the collapse path tolerates a continuation whose head was evicted.
    model.setBreakLines(false);
    QVERIFY(model.rowCount() >= 1);
}

// QTEST_MAIN (a real QApplication), not QTEST_GUILESS_MAIN: LogModel's
// constructor builds QPixmap severity icons, and QPixmap construction requires
// a QGuiApplication with a platform plugin loaded, which a bare QCoreApplication
// does not provide. CMake sets QT_QPA_PLATFORM=minimal for this test so it
// still runs without a real display.
QTEST_MAIN(TestLogModel)
#include "test_logmodel.moc"
