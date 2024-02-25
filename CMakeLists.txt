cmake_minimum_required(VERSION 3.5)

project(G4A-GUImain VERSION 0.9 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES G4A-GUImain_en_US.ts)

set(PROJECT_SOURCES
        main.cpp
        guiwindow.cpp
        guiwindow.h
        guiwindow.ui
        vectors.qrc
        about.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(G4A-GUImain
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET G4A-GUImain APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(G4A-GUImain SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(G4A-GUImain
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(G4A-GUImain PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Serial Port
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS SerialPort)
target_link_libraries(G4A-GUImain PRIVATE Qt${QT_VERSION_MAJOR}::SerialPort)

# SVG Renderer
if(${QT_VERSION} VERSION_LESS 6.1.0)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Svg)
    target_link_libraries(G4A-GUImain PRIVATE Qt${QT_VERSION_MAJOR}::Svg)
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS SvgWidgets)
    target_link_libraries(G4A-GUImain PRIVATE Qt${QT_VERSION_MAJOR}::SvgWidgets)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.G4A-GUImain)
endif()
set_target_properties(G4A-GUImain PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS G4A-GUImain
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(G4A-GUImain)
endif()
