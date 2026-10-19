# Offline source-level privacy guard. This is a narrow regression check, not proof
# that a binary can never communicate.
if(NOT DEFINED HUSH_SOURCE_ROOT)
  message(FATAL_ERROR "HUSH_SOURCE_ROOT must point at the source checkout")
endif()
file(GLOB_RECURSE HUSH_PRIVACY_SCAN_FILES
  "${HUSH_SOURCE_ROOT}/src/*.cpp"
  "${HUSH_SOURCE_ROOT}/src/*.cc"
  "${HUSH_SOURCE_ROOT}/include/*.hpp"
  "${HUSH_SOURCE_ROOT}/include/*.h"
)
set(HUSH_FORBIDDEN_NETWORK_APIS
  "QNetworkAccessManager"
  "QNetworkReply"
  "curl_easy_init"
  "curl_global_init"
  "WinHttpOpen"
  "InternetOpen"
  "NSURLSession"
  "sendBeacon"
  "Sentry::"
  "Crashpad"
  "Breakpad"
)
set(HUSH_PRIVACY_FAILURES "")
foreach(source_file IN LISTS HUSH_PRIVACY_SCAN_FILES)
  file(READ "${source_file}" source_text)
  foreach(forbidden_api IN LISTS HUSH_FORBIDDEN_NETWORK_APIS)
    string(FIND "${source_text}" "${forbidden_api}" match_position)
    if(NOT match_position EQUAL -1)
      file(RELATIVE_PATH relative_path "${HUSH_SOURCE_ROOT}" "${source_file}")
      string(APPEND HUSH_PRIVACY_FAILURES "\n  ${relative_path}: forbidden privacy API ${forbidden_api}")
    endif()
  endforeach()
endforeach()
if(NOT HUSH_PRIVACY_FAILURES STREQUAL "")
  message(FATAL_ERROR "HUSH privacy source audit failed:${HUSH_PRIVACY_FAILURES}")
endif()
message(STATUS "HUSH privacy source audit passed: no listed network, telemetry, or crash-reporting APIs found in src/ or include/.")
