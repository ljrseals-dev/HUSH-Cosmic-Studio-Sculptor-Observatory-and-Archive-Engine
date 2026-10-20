cmake_minimum_required(VERSION 3.20)
project(HushUnifiedDAW VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HUSH_BUILD_TESTS "Build offline contract tests" ON)
option(HUSH_ENABLE_ALSA "Build the optional Linux ALSA capture/playback backend and host" OFF)
option(HUSH_BUILD_GUI "Build the optional Qt 6 mixer GUI" OFF)
option(HUSH_BUILD_STRESS_TOOLS "Build offline benchmark and soak tools" ON)

add_library(hush_core
    src/project.cpp
    src/operation_log.cpp
    src/mixer.cpp
    src/bus_graph.cpp
    src/bus_renderer.cpp
    src/clip_renderer.cpp
    src/preallocated_bus_processor.cpp
    src/realtime_bus_engine.cpp
    src/realtime_bus_engine_host.cpp
    src/realtime_track_mixer.cpp
    src/transport_engine.cpp
    src/automation_lane.cpp
    src/midi_learn.cpp
    src/studio_03a.cpp
    src/realtime_bus_audio_adapter.cpp
    src/realtime_bus_engine_host_audio_adapter.cpp
    src/mixer_engine_bridge.cpp
    src/recovery_journal.cpp
    src/autosave_controller.cpp
    src/wav_writer.cpp
    src/wav_reader.cpp
    src/recording_ring_buffer.cpp
    src/project_wav_export.cpp
)
target_include_directories(hush_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(hush_core PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(hush_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(hush_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if(HUSH_BUILD_GUI)
  find_package(Qt6 REQUIRED COMPONENTS Widgets)
  add_executable(hush_mixer_gui src/mixer_gui.cpp)
  target_link_libraries(hush_mixer_gui PRIVATE hush_core Qt6::Widgets)
  target_compile_features(hush_mixer_gui PRIVATE cxx_std_20)
  if(HUSH_ENABLE_ALSA)
    target_compile_definitions(hush_mixer_gui PRIVATE HUSH_GUI_HAS_ALSA=1)
  endif()
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

  add_executable(hush_alsa_realtime_host src/alsa_realtime_host.cpp)
  target_link_libraries(hush_alsa_realtime_host PRIVATE hush_core hush_alsa_backend)
  target_compile_features(hush_alsa_realtime_host PRIVATE cxx_std_20)
  if(MSVC)
    target_compile_options(hush_alsa_realtime_host PRIVATE /W4 /permissive-)
  else()
    target_compile_options(hush_alsa_realtime_host PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  endif()
  if(HUSH_BUILD_GUI)
    target_link_libraries(hush_mixer_gui PRIVATE hush_alsa_backend)
  endif()
endif()


if(HUSH_BUILD_STRESS_TOOLS)
  add_executable(hush_routing_benchmark tools/hush_routing_benchmark.cpp)
  target_link_libraries(hush_routing_benchmark PRIVATE hush_core)
  target_compile_features(hush_routing_benchmark PRIVATE cxx_std_20)

  add_executable(hush_voice_path_benchmark tools/hush_voice_path_benchmark.cpp)
  target_link_libraries(hush_voice_path_benchmark PRIVATE hush_core)
  target_compile_features(hush_voice_path_benchmark PRIVATE cxx_std_20)

  add_executable(hush_headless_soak tools/hush_headless_soak.cpp)
  target_link_libraries(hush_headless_soak PRIVATE hush_core)
  target_compile_features(hush_headless_soak PRIVATE cxx_std_20)
endif()

if(HUSH_BUILD_TESTS)
  enable_testing()

  if(HUSH_BUILD_GUI)
    add_executable(hush_qt_mixer_interaction_tests tests/qt_mixer_interaction_tests.cpp)
    target_link_libraries(hush_qt_mixer_interaction_tests PRIVATE hush_core Qt6::Widgets)
    target_compile_features(hush_qt_mixer_interaction_tests PRIVATE cxx_std_20)
    if(MSVC)
      else()
      endif()
    add_test(NAME hush_qt_mixer_interaction_tests COMMAND hush_qt_mixer_interaction_tests)
    set_tests_properties(hush_qt_mixer_interaction_tests PROPERTIES
      ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()

  if(HUSH_BUILD_STRESS_TOOLS)
    add_test(NAME hush_performance_soak_smoke
      COMMAND ${CMAKE_COMMAND}
        -DBENCHMARK=$<TARGET_FILE:hush_routing_benchmark>
        -DSOAK=$<TARGET_FILE:hush_headless_soak>
        -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/hush-performance-soak-smoke
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/performance_soak_smoke.cmake)
  endif()

  add_test(
    NAME hush_privacy_source_audit
    COMMAND ${CMAKE_COMMAND}
      -DHUSH_SOURCE_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/privacy_source_audit.cmake
  )
  add_executable(hush_pan_law_tests tests/pan_law_tests.cpp)
  target_link_libraries(hush_pan_law_tests PRIVATE hush_core)
  add_test(NAME hush_pan_law_tests COMMAND hush_pan_law_tests)

  add_executable(hush_contract_tests tests/contract_tests.cpp)
  target_link_libraries(hush_contract_tests PRIVATE hush_core)
  add_test(NAME hush_contract_tests COMMAND hush_contract_tests)

  add_executable(hush_bus_graph_tests tests/bus_graph_tests.cpp)
  target_link_libraries(hush_bus_graph_tests PRIVATE hush_core)
  add_test(NAME hush_bus_graph_tests COMMAND hush_bus_graph_tests)

  add_executable(hush_bus_renderer_tests tests/bus_renderer_tests.cpp)
  target_link_libraries(hush_bus_renderer_tests PRIVATE hush_core)
  add_test(NAME hush_bus_renderer_tests COMMAND hush_bus_renderer_tests)

  add_executable(hush_clip_renderer_tests tests/clip_renderer_tests.cpp)
  target_link_libraries(hush_clip_renderer_tests PRIVATE hush_core)
  add_test(NAME hush_clip_renderer_tests COMMAND hush_clip_renderer_tests)

  add_executable(hush_preallocated_bus_processor_tests tests/preallocated_bus_processor_tests.cpp)
  target_link_libraries(hush_preallocated_bus_processor_tests PRIVATE hush_core)
  add_test(NAME hush_preallocated_bus_processor_tests COMMAND hush_preallocated_bus_processor_tests)

  add_executable(hush_realtime_bus_engine_tests tests/realtime_bus_engine_tests.cpp)
  target_link_libraries(hush_realtime_bus_engine_tests PRIVATE hush_core)
  add_test(NAME hush_realtime_bus_engine_tests COMMAND hush_realtime_bus_engine_tests)

  find_package(Threads REQUIRED)
  add_executable(hush_realtime_bus_engine_host_tests tests/realtime_bus_engine_host_tests.cpp)
  target_link_libraries(hush_realtime_bus_engine_host_tests PRIVATE hush_core Threads::Threads)
  add_test(NAME hush_realtime_bus_engine_host_tests COMMAND hush_realtime_bus_engine_host_tests)

  add_executable(hush_realtime_bus_audio_adapter_tests tests/realtime_bus_audio_adapter_tests.cpp)
  target_link_libraries(hush_realtime_bus_audio_adapter_tests PRIVATE hush_core)
  add_test(NAME hush_realtime_bus_audio_adapter_tests COMMAND hush_realtime_bus_audio_adapter_tests)

  add_executable(hush_realtime_bus_engine_host_audio_adapter_tests tests/realtime_bus_engine_host_audio_adapter_tests.cpp)
  target_link_libraries(hush_realtime_bus_engine_host_audio_adapter_tests PRIVATE hush_core Threads::Threads)
  add_test(NAME hush_realtime_bus_engine_host_audio_adapter_tests COMMAND hush_realtime_bus_engine_host_audio_adapter_tests)

  add_executable(hush_mixer_engine_bridge_tests tests/mixer_engine_bridge_tests.cpp)
  target_link_libraries(hush_mixer_engine_bridge_tests PRIVATE hush_core)
  add_test(NAME hush_mixer_engine_bridge_tests COMMAND hush_mixer_engine_bridge_tests)

  add_executable(hush_realtime_track_mixer_tests tests/realtime_track_mixer_tests.cpp)
  target_link_libraries(hush_realtime_track_mixer_tests PRIVATE hush_core)
  add_test(NAME hush_realtime_track_mixer_tests COMMAND hush_realtime_track_mixer_tests)

  add_executable(hush_gui_engine_command_path_tests tests/gui_engine_command_path_tests.cpp)
  target_link_libraries(hush_gui_engine_command_path_tests PRIVATE hush_core)
  add_test(NAME hush_gui_engine_command_path_tests COMMAND hush_gui_engine_command_path_tests)

  add_executable(hush_transport_automation_tests tests/transport_automation_tests.cpp)
  target_link_libraries(hush_transport_automation_tests PRIVATE hush_core)
  add_test(NAME hush_transport_automation_tests COMMAND hush_transport_automation_tests)

  add_executable(hush_studio_03a_tests tests/studio_03a_tests.cpp)
  target_link_libraries(hush_studio_03a_tests PRIVATE hush_core)
  add_test(NAME hush_studio_03a_tests COMMAND hush_studio_03a_tests)

  add_executable(hush_midi_learn_tests tests/midi_learn_tests.cpp)
  target_link_libraries(hush_midi_learn_tests PRIVATE hush_core)
  add_test(NAME hush_midi_learn_tests COMMAND hush_midi_learn_tests)

  add_executable(hush_realtime_graph_publisher_tests tests/realtime_graph_publisher_tests.cpp)
  target_include_directories(hush_realtime_graph_publisher_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_features(hush_realtime_graph_publisher_tests PRIVATE cxx_std_20)
  target_link_libraries(hush_realtime_graph_publisher_tests PRIVATE Threads::Threads)
  add_test(NAME hush_realtime_graph_publisher_tests COMMAND hush_realtime_graph_publisher_tests)

  add_executable(hush_parameter_queue_tests tests/parameter_queue_tests.cpp)
  target_include_directories(hush_parameter_queue_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_features(hush_parameter_queue_tests PRIVATE cxx_std_20)
  add_test(NAME hush_parameter_queue_tests COMMAND hush_parameter_queue_tests)

  add_executable(hush_recovery_restart_tests tests/recovery_restart_tests.cpp)
  target_link_libraries(hush_recovery_restart_tests PRIVATE hush_core)
  add_test(NAME hush_recovery_restart_tests COMMAND hush_recovery_restart_tests)

  add_executable(hush_recovery_partial_write_tests tests/recovery_partial_write_tests.cpp)
  target_link_libraries(hush_recovery_partial_write_tests PRIVATE hush_core)
  add_test(NAME hush_recovery_partial_write_tests COMMAND hush_recovery_partial_write_tests)

  add_executable(hush_autosave_lifecycle_tests tests/autosave_lifecycle_tests.cpp)
  target_link_libraries(hush_autosave_lifecycle_tests PRIVATE hush_core Threads::Threads)
  target_compile_features(hush_autosave_lifecycle_tests PRIVATE cxx_std_20)
  add_test(NAME hush_autosave_lifecycle_tests COMMAND hush_autosave_lifecycle_tests)

  add_executable(hush_project_close_policy_tests tests/project_close_policy_tests.cpp)
  target_include_directories(hush_project_close_policy_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_compile_features(hush_project_close_policy_tests PRIVATE cxx_std_20)
  add_test(NAME hush_project_close_policy_tests COMMAND hush_project_close_policy_tests)

  add_executable(hush_wav_writer_tests tests/wav_writer_tests.cpp)
  target_link_libraries(hush_wav_writer_tests PRIVATE hush_core)
  add_test(NAME hush_wav_writer_tests COMMAND hush_wav_writer_tests)

  add_executable(hush_project_wav_export_tests tests/project_wav_export_tests.cpp)
  target_link_libraries(hush_project_wav_export_tests PRIVATE hush_core)
  add_test(NAME hush_project_wav_export_tests COMMAND hush_project_wav_export_tests)

  add_executable(hush_project_media_timeline_tests tests/project_media_timeline_tests.cpp)
  target_link_libraries(hush_project_media_timeline_tests PRIVATE hush_core)
  add_test(NAME hush_project_media_timeline_tests COMMAND hush_project_media_timeline_tests)

  add_executable(hush_wav_reader_tests tests/wav_reader_tests.cpp)
  target_link_libraries(hush_wav_reader_tests PRIVATE hush_core)
  add_test(NAME hush_wav_reader_tests COMMAND hush_wav_reader_tests)

  add_executable(hush_recording_ring_buffer_tests tests/recording_ring_buffer_tests.cpp)
  target_link_libraries(hush_recording_ring_buffer_tests PRIVATE hush_core)
  add_test(NAME hush_recording_ring_buffer_tests COMMAND hush_recording_ring_buffer_tests)

  if(HUSH_ENABLE_ALSA)
    add_test(NAME hush_alsa_host_help COMMAND hush_alsa_realtime_host --help)
    add_test(NAME hush_alsa_host_journal_requires_project
             COMMAND hush_alsa_realtime_host --journal unused-recovery.journal)
    set_tests_properties(hush_alsa_host_journal_requires_project PROPERTIES WILL_FAIL TRUE)
    add_test(NAME hush_alsa_host_recovery_failure
             COMMAND ${CMAKE_COMMAND}
               -DHOST=$<TARGET_FILE:hush_alsa_realtime_host>
               -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/hush-alsa-host-recovery-contract
               -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/test_alsa_host_recovery_failure.cmake)
  endif()
  # Keep assert-based contract checks active even when users build Release.
  if(MSVC)
    target_compile_options(hush_qt_mixer_interaction_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_pan_law_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_contract_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_bus_graph_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_recovery_restart_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_recovery_partial_write_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_autosave_lifecycle_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_project_close_policy_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_wav_writer_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_project_wav_export_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_project_media_timeline_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_wav_reader_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_recording_ring_buffer_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_bus_renderer_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_clip_renderer_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_preallocated_bus_processor_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_parameter_queue_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_realtime_bus_engine_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_realtime_bus_engine_host_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_realtime_bus_audio_adapter_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_realtime_bus_engine_host_audio_adapter_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_mixer_engine_bridge_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_realtime_track_mixer_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_gui_engine_command_path_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_transport_automation_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_studio_03a_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_midi_learn_tests PRIVATE /UNDEBUG)
    target_compile_options(hush_realtime_graph_publisher_tests PRIVATE /UNDEBUG)
  else()
    target_compile_options(hush_qt_mixer_interaction_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_pan_law_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_contract_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_bus_graph_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_recovery_restart_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_recovery_partial_write_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_autosave_lifecycle_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_project_close_policy_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_wav_writer_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_project_wav_export_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_project_media_timeline_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_wav_reader_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_recording_ring_buffer_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_bus_renderer_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_clip_renderer_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_preallocated_bus_processor_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_parameter_queue_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_realtime_bus_engine_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_realtime_bus_engine_host_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_realtime_bus_audio_adapter_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_realtime_bus_engine_host_audio_adapter_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_mixer_engine_bridge_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_realtime_track_mixer_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_gui_engine_command_path_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_transport_automation_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_studio_03a_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_midi_learn_tests PRIVATE -UNDEBUG)
    target_compile_options(hush_realtime_graph_publisher_tests PRIVATE -UNDEBUG)
  endif()
endif()
