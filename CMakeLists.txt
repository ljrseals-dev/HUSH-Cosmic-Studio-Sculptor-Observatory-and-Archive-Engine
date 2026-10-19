cmake_minimum_required(VERSION 3.20)
project(HushUnifiedDAW VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HUSH_BUILD_TESTS "Build offline contract tests" ON)
option(HUSH_ENABLE_ALSA "Build the optional Linux ALSA capture/playback backend" OFF)

add_library(hush_core
    src/project.cpp
    src/operation_log.cpp
    src/mixer.cpp
    src/bus_graph.cpp
    src/bus_renderer.cpp
    src/preallocated_bus_processor.cpp
    src/realtime_bus_engine.cpp
    src/realtime_bus_audio_adapter.cpp
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

if(HUSH_ENABLE_ALSA)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "HUSH_ENABLE_ALSA is supported only on Linux")
  endif()
  find_package(ALSA REQUIRED)
  add_library(hush_alsa_backend src/alsa_audio_backend.cpp)
  target_include_directories(hush_alsa_backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${ALSA_INCLUDE_DIRS})
  target_compile_features(hush_alsa_backend PUBLIC cxx_std_20)
  target_link_libraries(hush_alsa_backend PUBLIC ${ALSA_LIBRARIES})
  if(MSVC)
    target_compile_options(hush_alsa_backend PRIVATE /W4 /permissive-)
  else()
    target_compile_options(hush_alsa_backend PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  endif()
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

  add_executable(hush_preallocated_bus_processor_tests tests/preallocated_bus_processor_tests.cpp)
  target_link_libraries(hush_preallocated_bus_processor_tests PRIVATE hush_core)
  add_test(NAME hush_preallocated_bus_processor_tests COMMAND hush_preallocated_bus_processor_tests)

  add_executable(hush_realtime_bus_engine_tests tests/realtime_bus_engine_tests.cpp)
  target_link_libraries(hush_realtime_bus_engine_tests PRIVATE hush_core)
  add_test(NAME hush_realtime_bus_engine_tests COMMAND hush_realtime_bus_engine_tests)

  add_executable(hush_realtime_bus_audio_adapter_tests tests/realtime_bus_audio_adapter_tests.cpp)
  target_link_libraries(hush_realtime_bus_audio_adapter_tests PRIVATE hush_core)
  add_test(NAME hush_realtime_bus_audio_adapter_tests COMMAND hush_realtime_bus_audio_adapter_tests)

  add_executable(hush_parameter_queue_tests tests/parameter_queue_tests.cpp)
  target_include_directories(hush_parameter_queue_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_features(hush_parameter_queue_tests PRIVATE cxx_std_20)
  add_test(NAME hush_parameter_queue_tests COMMAND hush_parameter_queue_tests)

  add_executable(hush_recovery_restart_tests tests/recovery_restart_tests.cpp)
  target_link_libraries(hush_recovery_restart_tests PRIVATE hush_core)
  add_test(NAME hush_recovery_restart_tests COMMAND hush_recovery_restart_tests)
  # Keep assert-based contract checks active even when users build Release.
  if(MSVC)
    target_compile_options(hush_contract_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_bus_graph_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_recovery_restart_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_bus_renderer_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_preallocated_bus_processor_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_parameter_queue_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_realtime_bus_engine_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_realtime_bus_audio_adapter_tests PRIVATE /UNDEBUG)
  else()
    target_compile_options(hush_contract_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_bus_graph_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_recovery_restart_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_bus_renderer_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_preallocated_bus_processor_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_parameter_queue_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_realtime_bus_engine_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_realtime_bus_audio_adapter_tests PRIVATE -UNDEBUG)
  endif()
endif()
