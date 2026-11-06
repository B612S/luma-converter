cmake_minimum_required(VERSION 3.16)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" LUMA_CONVERTER_VERSION LIMIT_COUNT 1)
string(STRIP "${LUMA_CONVERTER_VERSION}" LUMA_CONVERTER_VERSION)
if(NOT LUMA_CONVERTER_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must be MAJOR.MINOR.PATCH (got '${LUMA_CONVERTER_VERSION}')")
endif()
project(luma-converter VERSION ${LUMA_CONVERTER_VERSION} LANGUAGES CXX)

include(GNUInstallDirs)
if(NOT CMAKE_INSTALL_DATADIR)
    set(CMAKE_INSTALL_DATADIR "share")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 6.6 REQUIRED COMPONENTS
    Core Gui Quick QuickControls2 Widgets DBus LinguistTools
)
find_package(Qt6 6.6 QUIET COMPONENTS Test Network)

qt_standard_project_setup(REQUIRES 6.6)

# The target name stays hyphen-free (CMake/Qt generate identifiers from it),
# only the installed binary is called luma-converter.
qt_add_executable(luma_converter
    main.cpp
    core/ConfigManager.cpp
    core/FormatPreset.cpp
    core/MediaProbe.cpp
    core/ConversionJob.cpp
    core/ConversionQueue.cpp
    core/Logger.cpp
    core/Notifier.cpp
)

set_target_properties(luma_converter PROPERTIES OUTPUT_NAME luma-converter)

qt_add_qml_module(luma_converter
    URI LumaConverter
    VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    QML_FILES
        qml/Main.qml
        qml/ConverterPage.qml
        qml/FormatBar.qml
        qml/FileDelegate.qml
        qml/SettingsDialog.qml
    SOURCES
        core/ConfigManager.hpp
        core/FormatPreset.hpp
        core/MediaProbe.hpp
        core/ConversionJob.hpp
        core/ConversionQueue.hpp
        core/Logger.hpp
        core/Notifier.hpp
)

qt_add_resources(luma_converter "assets"
    PREFIX "/"
    FILES
        assets/luma-converter.png
)

qt_add_translations(luma_converter
    TS_FILES
        locales/en.ts
        locales/ru.ts
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/en.qm
    ${CMAKE_CURRENT_BINARY_DIR}/ru.qm
    DESTINATION ${CMAKE_INSTALL_DATADIR}/luma-converter/translations
)

install(FILES luma-converter.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/luma-converter.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
install(FILES assets/luma-converter.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

target_include_directories(luma_converter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/core)

target_link_libraries(luma_converter PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    Qt6::DBus
)

target_compile_definitions(luma_converter PRIVATE
    LUMA_CONVERTER_VERSION="${PROJECT_VERSION}"
)

install(TARGETS luma_converter
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(IMPORTED_RUNTIME_ARTIFACTS luma_converter DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/qml/LumaConverter)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/LumaConverter/ DESTINATION ${CMAKE_INSTALL_LIBDIR}/qt6/qml/LumaConverter)

# ---- Tests (off by default, never ship in a package) ----
option(LUMA_CONVERTER_BUILD_TESTS "Build autotests (Qt Test)" OFF)
if (LUMA_CONVERTER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
