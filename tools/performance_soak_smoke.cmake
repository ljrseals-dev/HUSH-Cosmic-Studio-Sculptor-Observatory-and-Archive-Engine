if(NOT DEFINED BENCHMARK OR NOT DEFINED SOAK OR NOT DEFINED TEST_DIR)
  message(FATAL_ERROR "BENCHMARK, SOAK, and TEST_DIR are required")
endif()

execute_process(
  COMMAND "${BENCHMARK}" 500 1 10000
  RESULT_VARIABLE bench_rc
  OUTPUT_VARIABLE bench_out
  ERROR_VARIABLE bench_err
  TIMEOUT 20
)
if(NOT bench_rc EQUAL 0)
  message(FATAL_ERROR "Benchmark smoke failed (${bench_rc}): ${bench_err}\n${bench_out}")
endif()
if(NOT bench_out MATCHES "\"status\"[ ]*:[ ]*\"pass\"")
  message(FATAL_ERROR "Benchmark did not report pass: ${bench_out}")
endif()
if(NOT bench_out MATCHES "\"tracks\"[ ]*:[ ]*500")
  message(FATAL_ERROR "Benchmark report does not show 500 tracks: ${bench_out}")
endif()

file(MAKE_DIRECTORY "${TEST_DIR}")
set(journal "${TEST_DIR}/soak-smoke.journal")
file(REMOVE "${journal}")
execute_process(
  COMMAND "${SOAK}" 1 "${journal}" 64
  RESULT_VARIABLE soak_rc
  OUTPUT_VARIABLE soak_out
  ERROR_VARIABLE soak_err
  TIMEOUT 20
)
if(NOT soak_rc EQUAL 0)
  message(FATAL_ERROR "Soak smoke failed (${soak_rc}): ${soak_err}\n${soak_out}")
endif()
if(NOT soak_out MATCHES "\"status\"[ ]*:[ ]*\"pass\"")
  message(FATAL_ERROR "Soak did not report pass: ${soak_out}")
endif()
if(NOT soak_out MATCHES "\"pass_threshold_configured\"[ ]*:[ ]*true")
  message(FATAL_ERROR "Soak report does not indicate an active threshold: ${soak_out}")
endif()
file(REMOVE "${journal}")
message(STATUS "500-track benchmark and bounded-memory soak smoke tests passed")
