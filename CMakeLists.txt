cmake_minimum_required(VERSION 3.20)
project(galgame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

set(SDL3_DIR     "${CMAKE_SOURCE_DIR}/third_party/SDL3-3.4.18/cmake")
set(SDL3_ttf_DIR "${CMAKE_SOURCE_DIR}/third_party/SDL3_ttf-3.2.2/cmake")
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)

add_executable(galgame src/main.cpp src/script.cpp)
target_link_libraries(galgame PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)

# 构建测试
option(GALGAME_BUILD_TESTS "构建单元测试" OFF)
if (GALGAME_BUILD_TESTS)
    enable_testing()
    add_executable(script_tests tests/script_tests.cpp src/script.cpp)
    target_include_directories(script_tests PRIVATE src)
    set_target_properties(script_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
    add_test(NAME script_tests COMMAND script_tests)
endif()

# 两个 DLL 一起拷到 exe 旁边
add_custom_command(TARGET galgame POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_SOURCE_DIR}/third_party/SDL3-3.4.18/lib/x64/SDL3.dll"
          "${CMAKE_SOURCE_DIR}/third_party/SDL3_ttf-3.2.2/lib/x64/SDL3_ttf.dll"
          "$<TARGET_FILE_DIR:galgame>")

add_custom_command(TARGET galgame POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/assets"
          "$<TARGET_FILE_DIR:galgame>/assets")