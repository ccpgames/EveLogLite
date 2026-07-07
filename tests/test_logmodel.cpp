#include <QtTest/QtTest>

#include "logmodel.h"

// Tracer test for the Qt Test harness itself: proves LogLiteTests builds, links
// against the model translation units, and runs under ctest headlessly.
class TestLogModel : public QObject
{
    Q_OBJECT

private slots:
    void freshModelHasOnlyThePhantomRow();
};

void TestLogModel::freshModelHasOnlyThePhantomRow()
{
    LogModel model;

    // rowCount() is the real buffer size plus one trailing sentinel row used by
    // the view; a fresh model has an empty buffer, so rowCount() == 0 + 1.
    QCOMPARE(model.rowCount(), 1);
}

// QTEST_MAIN (a real QApplication), not QTEST_GUILESS_MAIN: LogModel's
// constructor builds QPixmap severity icons, and QPixmap construction requires
// a QGuiApplication with a platform plugin loaded, which a bare QCoreApplication
// does not provide. CMake sets QT_QPA_PLATFORM=offscreen for this test so it
// still runs without a real display.
QTEST_MAIN(TestLogModel)
#include "test_logmodel.moc"
