cmake_minimum_required(VERSION 3.31)

project(LogLite VERSION 1.4.3 LANGUAGES CXX)

include(cmake/CcpBuildConfigurations.cmake)
include(cmake/CcpTargetConfigurations.cmake)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Sql Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/src/LogLite_resource.rc")
qt_add_executable(LogLite WIN32 MACOSX_BUNDLE
        src/abstractlogmodel.cpp include/abstractlogmodel.h
        src/filter.ui
        src/filtercondition.cpp include/filtercondition.h
        src/filterconditions.cpp include/filterconditions.h
        src/filterdialog.cpp include/filterdialog.h
        src/filterhighlight.cpp include/filterhighlight.h src/filterhighlight.ui
        src/fixedheader.cpp include/fixedheader.h
        src/highlightdialog.cpp include/highlightdialog.h
        src/highlights.ui
        src/logfilter.cpp include/logfilter.h
        src/logmap.cpp include/logmap.h
        src/logmodel.cpp include/logmodel.h
        src/logmonitorfilemodel.cpp include/logmonitorfilemodel.h
        src/logstatistics.cpp include/logstatistics.h src/logstatistics.ui
        src/logview.cpp include/logview.h
        src/main.cpp
        src/mainwindow.cpp include/mainwindow.h src/mainwindow.ui
        src/overlaylayout.cpp include/overlaylayout.h
        src/settingsdialog.cpp include/settingsdialog.h src/settingsdialog.ui
        ${app_icon_resource_windows}
)

target_include_directories(LogLite PRIVATE include)

if(NOT GIT_FOUND)
    find_package(Git)
endif()
if(GIT_FOUND)
    get_target_property(TARGET_SOURCE_DIR LogLite SOURCE_DIR)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${TARGET_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    message(WARNING "Git was not found, version information will not contain git details")
endif()
if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "Unknown commit")
endif()

target_compile_definitions(LogLite PUBLIC
        APP_VERSION="${CMAKE_PROJECT_VERSION}"
        GIT_VERSION="${GIT_COMMIT_HASH}"
)

target_link_libraries(LogLite PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Network
        Qt::Sql
        Qt::Widgets
)

# Resources:
set_source_files_properties("src/resources/Counter.png"
        PROPERTIES QT_RESOURCE_ALIAS "counter"
)
set_source_files_properties("src/resources/Error.png"
        PROPERTIES QT_RESOURCE_ALIAS "error"
)
set_source_files_properties("src/resources/Fatal.png"
        PROPERTIES QT_RESOURCE_ALIAS "fatal"
)
set_source_files_properties("src/resources/Info.png"
        PROPERTIES QT_RESOURCE_ALIAS "info"
)
set_source_files_properties("src/resources/Mcall.png"
        PROPERTIES QT_RESOURCE_ALIAS "machocall"
)
set_source_files_properties("src/resources/Notice.png"
        PROPERTIES QT_RESOURCE_ALIAS "notice"
)
set_source_files_properties("src/resources/Overlap.png"
        PROPERTIES QT_RESOURCE_ALIAS "overlap"
)
set_source_files_properties("src/resources/Warning.png"
        PROPERTIES QT_RESOURCE_ALIAS "warning"
)
set_source_files_properties("src/resources/filter-off.png"
        PROPERTIES QT_RESOURCE_ALIAS "nofilter"
)
set_source_files_properties("src/resources/filter.png"
        PROPERTIES QT_RESOURCE_ALIAS "filter"
)
set_source_files_properties("src/resources/highlights-off.png"
        PROPERTIES QT_RESOURCE_ALIAS "nohighlights"
)
set_source_files_properties("src/resources/highlights.png"
        PROPERTIES QT_RESOURCE_ALIAS "highlights"
)
set_source_files_properties("src/resources/pulse.png"
        PROPERTIES QT_RESOURCE_ALIAS "pulse"
)
set_source_files_properties("src/resources/searchClose.png"
        PROPERTIES QT_RESOURCE_ALIAS "searchClose"
)
set_source_files_properties("src/resources/searchDown.png"
        PROPERTIES QT_RESOURCE_ALIAS "searchDown"
)
set_source_files_properties("src/resources/searchUp.png"
        PROPERTIES QT_RESOURCE_ALIAS "searchUp"
)
set(resources_resource_files
        "src/resources/Counter.png"
        "src/resources/Error.png"
        "src/resources/Fatal.png"
        "src/resources/Info.png"
        "src/resources/Mcall.png"
        "src/resources/Notice.png"
        "src/resources/Overlap.png"
        "src/resources/Warning.png"
        "src/resources/filter-off.png"
        "src/resources/filter.png"
        "src/resources/highlights-off.png"
        "src/resources/highlights.png"
        "src/resources/pulse.png"
        "src/resources/searchClose.png"
        "src/resources/searchDown.png"
        "src/resources/searchUp.png"
)

qt6_add_resources(LogLite "resources"
        PREFIX
        "/default"
        FILES
        ${resources_resource_files}
)

if(WIN32)
    target_link_libraries(LogLite PRIVATE
            Qt::WinExtras
    )
endif()

if(APPLE)
    set(ICONSET "${CMAKE_BINARY_DIR}/icon.icns")
    execute_process(
            COMMAND_ERROR_IS_FATAL ANY
            COMMAND iconutil -c icns -o ${ICONSET} ${CMAKE_BINARY_DIR}/icon.iconset
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/icon.iconset/
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon/16.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_16x16.png
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon/32.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_16x16@2x.png
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon/32.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_32x32.png
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon/64.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_32x32@2x.png
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon/64.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_64x64.png
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon/128.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_64x64@2x.png
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon/128.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_128x128.png
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon/256.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_128x128@2x.png
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon/256.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_256x256.png
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon/512.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_256x256@2x.png
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon/512.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_512x512.png
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/icon/1024.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_512x512@2x.png
    )
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set_source_files_properties(${ICONSET} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(LogLite PUBLIC ${ICONSET})
endif()
