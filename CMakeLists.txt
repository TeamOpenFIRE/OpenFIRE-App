cmake_minimum_required(VERSION 3.5)

project(OpenFIREapp VERSION 0.9 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT OFAPP_QT_VERSIONS)
    set(OFAPP_QT_VERSIONS Qt6)
endif()

if(OFAPP_VERSION)
    add_compile_definitions(OFAPP_VERSION="${OFAPP_VERSION}")
endif()

if(OFAPP_GITHASH)
    add_compile_definitions(OFAPP_GITHASH="${OFAPP_GITHASH}")
endif()

find_package(QT NAMES ${OFAPP_QT_VERSIONS} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES
        translation/AppTranslations_en_US.ts
)

set(PROJECT_SOURCES
        src/main.cpp
        src/constants.h
        src/appmainwindow.cpp
        src/appmainwindow.h
        src/appmainwindow.ui
        src/appserial.h
        src/appserial.cpp
        src/appcali.h
        src/appcali.cpp
        src/appcali.ui
        src/appdebug.h
        src/appdebug.cpp
        src/appdebug.ui
        src/appabout.h
        src/appabout.cpp
        src/appabout.ui
        img/vectors.qrc
        boards/boardPics/boards.qrc
        boards/OpenFIREshared.h
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(OpenFIREapp
        MANUAL_FINALIZATION
        ${CMAKE_CURRENT_SOURCE_DIR}/img/ico/icon.rc
        ${PROJECT_SOURCES}
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

else()
    add_executable(OpenFIREapp
        ${CMAKE_CURRENT_SOURCE_DIR}/img/ico/icon.rc
        ${PROJECT_SOURCES}
    )
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

endif()

target_link_libraries(OpenFIREapp PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Serial Port
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS SerialPort)
target_link_libraries(OpenFIREapp PRIVATE Qt${QT_VERSION_MAJOR}::SerialPort)

# SVG Renderer
if(${QT_VERSION} VERSION_LESS 6.1.0)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Svg)
    target_link_libraries(OpenFIREapp PRIVATE Qt${QT_VERSION_MAJOR}::Svg)
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS SvgWidgets)
    target_link_libraries(OpenFIREapp PRIVATE Qt${QT_VERSION_MAJOR}::SvgWidgets)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER org.TeamOpenFIRE.OpenFIREapp)
endif()
set_target_properties(OpenFIREapp PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS OpenFIREapp
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(OpenFIREapp)
endif()
