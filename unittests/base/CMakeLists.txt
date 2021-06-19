aux_source_directory(. BASE_UT_SOURCES)

ADD_EXECUTABLE(base_unittest
  ${BASE_UT_SOURCES}
  )
target_link_libraries(base_unittest
  ltio
  profiler
  )
