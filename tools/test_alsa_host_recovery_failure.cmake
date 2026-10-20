if(NOT DEFINED HOST OR NOT DEFINED TEST_DIR)
  message(FATAL_ERROR "HOST and TEST_DIR must be provided")
endif()

file(MAKE_DIRECTORY "${TEST_DIR}")
set(project_file "${TEST_DIR}/project.json")
set(journal_file "${TEST_DIR}/broken-recovery.journal")
file(WRITE "${project_file}" "{\"schema_version\":1,\"id\":\"cli-recovery-test\",\"pan_law\":\"equal_power_minus_3_db\",\"tracks\":[]}")
file(WRITE "${journal_file}" "not-a-valid-recovery-record\n")

execute_process(
  COMMAND "${HOST}" --project "${project_file}" --journal "${journal_file}"
  RESULT_VARIABLE host_result
  OUTPUT_VARIABLE host_stdout
  ERROR_VARIABLE host_stderr
)

if(NOT host_result EQUAL 2)
  message(FATAL_ERROR "Expected recovery failure exit code 2, got '${host_result}'. stdout='${host_stdout}' stderr='${host_stderr}'")
endif()
if(NOT host_stderr MATCHES "HUSH_RECOVERY_FAILED")
  message(FATAL_ERROR "Host did not surface the recovery failure. stderr='${host_stderr}'")
endif()

file(REMOVE_RECURSE "${TEST_DIR}")
