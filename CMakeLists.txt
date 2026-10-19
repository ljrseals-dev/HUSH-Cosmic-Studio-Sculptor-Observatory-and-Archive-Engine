cmake_minimum_required(VERSION 3.20)
project(HushUnifiedDAW VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HUSH_BUILD_TESTS "Build offline contract tests" ON)
option(HUSH_ENABLE_ALSA "Enable the Linux ALSA backend (future integration point)" OFF)

add_library(hush_core
    src/project.cpp
    src/operation_log.cpp
    src/mixer.cpp
)
target_include_directories(hush_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(hush_core PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(hush_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(hush_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if(HUSH_BUILD_TESTS)
  enable_testing()
  add_executable(hush_contract_tests tests/contract_tests.cpp)
  target_link_libraries(hush_contract_tests PRIVATE hush_core)
  add_test(NAME hush_contract_tests COMMAND hush_contract_tests)
endif()
