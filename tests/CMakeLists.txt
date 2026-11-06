# Tests (Qt Test). Run: cmake -B build -S . -DLUMA_CONVERTER_BUILD_TESTS=ON &&
#                     ctest --test-dir build --output-on-failure
find_package(Qt6 QUIET COMPONENTS Test)

if (NOT Qt6Test_FOUND)
    message(STATUS "Qt6::Test not found — autotests disabled")
    return()
endif()

# The tested part of the application: no QML, no GUI.
add_library(luma_converter_testcore STATIC
    ${CMAKE_SOURCE_DIR}/core/ConfigManager.cpp
    ${CMAKE_SOURCE_DIR}/core/ConfigManager.hpp
    ${CMAKE_SOURCE_DIR}/core/FormatPreset.cpp
    ${CMAKE_SOURCE_DIR}/core/FormatPreset.hpp
    ${CMAKE_SOURCE_DIR}/core/MediaProbe.cpp
    ${CMAKE_SOURCE_DIR}/core/MediaProbe.hpp
    ${CMAKE_SOURCE_DIR}/core/ConversionJob.cpp
    ${CMAKE_SOURCE_DIR}/core/ConversionJob.hpp
    ${CMAKE_SOURCE_DIR}/core/ConversionQueue.cpp
    ${CMAKE_SOURCE_DIR}/core/ConversionQueue.hpp
    ${CMAKE_SOURCE_DIR}/core/Logger.cpp
    ${CMAKE_SOURCE_DIR}/core/Logger.hpp
    ${CMAKE_SOURCE_DIR}/core/Notifier.cpp
    ${CMAKE_SOURCE_DIR}/core/Notifier.hpp
)

target_include_directories(luma_converter_testcore PUBLIC ${CMAKE_SOURCE_DIR}/core)
target_link_libraries(luma_converter_testcore PUBLIC Qt6::Core Qt6::DBus)

foreach(test tst_formats tst_config tst_conversion)
    add_executable(${test} ${test}.cpp)
    target_link_libraries(${test} PRIVATE luma_converter_testcore Qt6::Test)
    add_test(NAME ${test} COMMAND ${test})
    set_tests_properties(${test} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        LABELS "unit"
    )
endforeach()

set_tests_properties(tst_conversion PROPERTIES LABELS "integration;ffmpeg")
