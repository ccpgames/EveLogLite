#[[
LogLiteTests - Qt Test target for the model layer (LogModel / AbstractLogModel).

Links the model translation units directly (not the LogLite executable). The
test main needs a real QApplication, not just a QCoreApplication: LogModel's
constructor builds QPixmap icons for each severity, and QPixmap construction
requires a QGuiApplication with a platform integration loaded.

Qt is built statically here, so the platform plugin has to be linked in and
registered explicitly; it is not discovered from a plugins directory at
runtime. The "cocoa" platform (macOS's native integration) is pulled in
automatically as a hard dependency of Qt6::Gui, but it needs a live
WindowServer connection, which a CI/SSH session will not have. "minimal" is
built by this project's Qt configuration and needs no display or window
server, so the test explicitly links and registers it and runs under
QT_QPA_PLATFORM=minimal below.
]]

find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(LogLiteTests
        test_logmodel.cpp
        ../src/abstractlogmodel.cpp ../include/abstractlogmodel.h
        ../src/logmodel.cpp ../include/logmodel.h
        ../src/logmonitorfilemodel.cpp ../include/logmonitorfilemodel.h
)

target_include_directories(LogLiteTests PRIVATE ../include)

# Widgets is required here even though these are headless model tests: logmodel.cpp
# calls into logmonitorfilemodel.cpp (LogMonitorFileModel::saveModel, used for
# autosave), which uses QMessageBox to report save errors. That symbol must resolve
# at link time whether or not a test happens to exercise the save path.
target_link_libraries(LogLiteTests PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Network
        Qt::Sql
        Qt::Widgets
        Qt::Test
        Qt6::QMinimalIntegrationPlugin
        Qt6::QMinimalIntegrationPlugin_init
)

add_test(NAME LogLiteTests COMMAND LogLiteTests)
set_tests_properties(LogLiteTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=minimal")
