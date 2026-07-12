cmake_minimum_required(VERSION 3.10)

project(OpenFIREapp
        VERSION 3.0.3
        DESCRIPTION "OpenFIRE Configuration Desktop App"
        LANGUAGES CXX)
set(OFAPP_CODENAME "Tokinomiya")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT(CMAKE_BUILD_TYPE STREQUAL "Release"))
    add_compile_definitions(OFAPP_DEBUG)
endif()

# required to make use of definitions in `OpenFIREshared.h` for App usage
add_compile_definitions(OF_APP)

add_compile_definitions(OFAPP_VERSION="${PROJECT_VERSION}")

set(MINORVER_TIERLIST_NAMES
  "" ": Alpha" ": Beta" ": Gamma" ": Omega" ": Sigma"
)
math(EXPR MINORVER_TIERLIST_INDEX ${PROJECT_VERSION_MINOR})
list(GET MINORVER_TIERLIST_NAMES ${MINORVER_TIERLIST_INDEX} PROJECT_MINORVER_WORD)

set(PATCHVER_TIERLIST_NAMES
  "" " Full!" "': Special Champion Edition" " Turbo: Hyper Fighting" " EX Plus Alpha"
  " LOVE MAX!!!!!" " LOVEMAX SIXSTARS!!!!!!" " LOVEMAX SIXSTARS !!!!!! XTEND"
)
math(EXPR PATCHVER_TIERLIST_INDEX ${PROJECT_VERSION_PATCH})
list(GET PATCHVER_TIERLIST_NAMES ${PATCHVER_TIERLIST_INDEX} PROJECT_PATCH_NAME)

add_compile_definitions(OFAPP_CODENAME="${OFAPP_CODENAME}${PROJECT_MINORVER_WORD}${PROJECT_PATCH_NAME}")

if(OFAPP_GITHASH)
    add_compile_definitions(OFAPP_GITHASH="${OFAPP_GITHASH}")
endif()

if(NOT OFAPP_QT_VERSION)
    find_package(QT NAMES Qt6 Qt5 REQUIRED)
else()
    find_package(QT NAMES ${OFAPP_QT_VERSION} REQUIRED)
endif()

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Concurrent SerialPort)

set(TS_FILES
        translation/AppTranslations_en_US.ts
        translation/AppTranslations_it_IT.ts
)

set(PROJECT_SOURCES
        src/main.cpp
        src/appcommon.h
        src/appmainwindow.cpp
        src/appmainwindow.h
        src/appmainwindow.ui
        src/appserial.h
        src/appserial.cpp
        src/appcali.h
        src/appcali.cpp
        src/appcali.ui
        src/apppreviewer.h
        src/apppreviewer.cpp
        src/apppreviewer.ui
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
    qt_add_translations(
        TARGETS OpenFIREapp
        TS_FILES ${TS_FILES}
    )
else()
    add_executable(OpenFIREapp
        ${CMAKE_CURRENT_SOURCE_DIR}/img/ico/icon.rc
        ${PROJECT_SOURCES}
    )
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(OpenFIREapp PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::SerialPort)

# SVG Renderer has a different package name >= 6.1 for some reason
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
