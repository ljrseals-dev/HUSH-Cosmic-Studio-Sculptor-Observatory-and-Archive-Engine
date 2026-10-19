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
    src/bus_graph.cpp
    src/bus_renderer.cpp
    src/recovery_journal.cpp
    src/autosave_controller.cpp
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

  add_executable(hush_bus_graph_tests tests/bus_graph_tests.cpp)
  target_link_libraries(hush_bus_graph_tests PRIVATE hush_core)
  add_test(NAME hush_bus_graph_tests COMMAND hush_bus_graph_tests)

  add_executable(hush_bus_renderer_tests tests/bus_renderer_tests.cpp)
  target_link_libraries(hush_bus_renderer_tests PRIVATE hush_core)
  add_test(NAME hush_bus_renderer_tests COMMAND hush_bus_renderer_tests)

  add_executable(hush_recovery_restart_tests tests/recovery_restart_tests.cpp)
  target_link_libraries(hush_recovery_restart_tests PRIVATE hush_core)
  add_test(NAME hush_recovery_restart_tests COMMAND hush_recovery_restart_tests)
  # Keep assert-based contract checks active even when users build Release.
  if(MSVC)
    target_compile_options(hush_contract_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_bus_graph_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_recovery_restart_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_bus_renderer_tests PRIVATE /UNDEBUG)
  else()
    target_compile_options(hush_contract_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_bus_graph_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_recovery_restart_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_bus_renderer_tests PRIVATE -UNDEBUG)
  endif()
endif()
