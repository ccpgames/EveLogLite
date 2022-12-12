# Generated from LogLite.pro.

cmake_minimum_required(VERSION 3.16)
project(LogLite VERSION 1.4.3 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}")

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Network Sql)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS Widgets)

qt_add_executable(LogLite WIN32 MACOSX_BUNDLE
    abstractlogmodel.cpp abstractlogmodel.h
    filter.ui
    filtercondition.cpp filtercondition.h
    filterconditions.cpp filterconditions.h
    filterdialog.cpp filterdialog.h
    filterhighlight.cpp filterhighlight.h filterhighlight.ui
    fixedheader.cpp fixedheader.h
    highlightdialog.cpp highlightdialog.h
    highlights.ui
    logfilter.cpp logfilter.h
    logmap.cpp logmap.h
    logmodel.cpp logmodel.h
    logmonitorfilemodel.cpp logmonitorfilemodel.h
    logstatistics.cpp logstatistics.h logstatistics.ui
    logview.cpp logview.h
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    overlaylayout.cpp overlaylayout.h
    settingsdialog.cpp settingsdialog.h settingsdialog.ui
)
target_compile_definitions(LogLite PUBLIC
    APP_VERSION="${CMAKE_PROJECT_VERSION}"
    GIT_VERSION="FIXME"
)

target_link_libraries(LogLite PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Sql
)


# Resources:
set_source_files_properties("resources/Counter.png"
    PROPERTIES QT_RESOURCE_ALIAS "counter"
)
set_source_files_properties("resources/Error.png"
    PROPERTIES QT_RESOURCE_ALIAS "error"
)
set_source_files_properties("resources/Fatal.png"
    PROPERTIES QT_RESOURCE_ALIAS "fatal"
)
set_source_files_properties("resources/Info.png"
    PROPERTIES QT_RESOURCE_ALIAS "info"
)
set_source_files_properties("resources/Mcall.png"
    PROPERTIES QT_RESOURCE_ALIAS "machocall"
)
set_source_files_properties("resources/Notice.png"
    PROPERTIES QT_RESOURCE_ALIAS "notice"
)
set_source_files_properties("resources/Overlap.png"
    PROPERTIES QT_RESOURCE_ALIAS "overlap"
)
set_source_files_properties("resources/Warning.png"
    PROPERTIES QT_RESOURCE_ALIAS "warning"
)
set_source_files_properties("resources/filter-off.png"
    PROPERTIES QT_RESOURCE_ALIAS "nofilter"
)
set_source_files_properties("resources/filter.png"
    PROPERTIES QT_RESOURCE_ALIAS "filter"
)
set_source_files_properties("resources/highlights-off.png"
    PROPERTIES QT_RESOURCE_ALIAS "nohighlights"
)
set_source_files_properties("resources/highlights.png"
    PROPERTIES QT_RESOURCE_ALIAS "highlights"
)
set_source_files_properties("resources/pulse.png"
    PROPERTIES QT_RESOURCE_ALIAS "pulse"
)
set_source_files_properties("resources/searchClose.png"
    PROPERTIES QT_RESOURCE_ALIAS "searchClose"
)
set_source_files_properties("resources/searchDown.png"
    PROPERTIES QT_RESOURCE_ALIAS "searchDown"
)
set_source_files_properties("resources/searchUp.png"
    PROPERTIES QT_RESOURCE_ALIAS "searchUp"
)
set(resources_resource_files
    "resources/Counter.png"
    "resources/Error.png"
    "resources/Fatal.png"
    "resources/Info.png"
    "resources/Mcall.png"
    "resources/Notice.png"
    "resources/Overlap.png"
    "resources/Warning.png"
    "resources/filter-off.png"
    "resources/filter.png"
    "resources/highlights-off.png"
    "resources/highlights.png"
    "resources/pulse.png"
    "resources/searchClose.png"
    "resources/searchDown.png"
    "resources/searchUp.png"
)

qt6_add_resources(LogLite "resources"
    PREFIX
        "/default"
    FILES
        ${resources_resource_files}
)

if(WIN32)
    target_link_libraries(LogLite PUBLIC
        Qt::WinExtras
    )
endif()

if(APPLE)
    set(ICONSET "${CMAKE_BINARY_DIR}/icon.icns")
    execute_process(
        COMMAND_ERROR_IS_FATAL ANY
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/icon.iconset/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/16.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_16x16.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/32.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_16x16@2x.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/32.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_32x32.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/64.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_32x32@2x.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/64.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_64x64.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/128.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_64x64@2x.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/128.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_128x128.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/256.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_128x128@2x.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/256.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_256x256.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/512.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_256x256@2x.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/512.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_512x512.png
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/1024.png ${CMAKE_BINARY_DIR}/icon.iconset/icon_512x512@2x.png
        COMMAND iconutil -c icns -o ${ICONSET} ${CMAKE_BINARY_DIR}/icon.iconset
    )
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set_source_files_properties(${ICONSET} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(LogLite PUBLIC ${ICONSET})
endif()

if((QT_VERSION_MAJOR GREATER 4))
    target_link_libraries(LogLite PUBLIC
        Qt::Widgets
    )
endif()

install(TARGETS LogLite
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
