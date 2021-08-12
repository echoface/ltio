aux_source_directory(. BASE_UT_SOURCES)

ADD_EXECUTABLE(base_unittest
  ${BASE_UT_SOURCES}
  ../../base/sys/byteords_unittest.cc
  ../../base/sys/sys_info_unittest.cc
  )
target_link_libraries(base_unittest
  ltio
  profiler
  )
