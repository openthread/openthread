FIND_PROGRAM( LCOV_PATH lcov )
FIND_PROGRAM( GENHTML_PATH genhtml )

if (LCOV_PATH)
  # message ( "lcov: ${LCOV_PATH}" )

  add_custom_target(coverage_report
    COMMAND "${LCOV_PATH}" --rc lcov_branch_coverage=1  --no-checksum --base-directory "${CMAKE_CURRENT_SOURCE_DIR}" --directory src/CMakeFiles/${PROJECT_NAME}.dir --no-external --capture --output-file ${PROJECT_NAME}.info
    COMMAND "${GENHTML_PATH}" --rc genhtml_branch_coverage=1 --output-directory lcov ${PROJECT_NAME}.info
    COMMAND echo "Coverage report in: file://${CMAKE_BINARY_DIR}/lcov/index.html"
  )
endif()
