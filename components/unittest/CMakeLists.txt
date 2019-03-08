ADD_EXECUTABLE(component_unittest
  catch_main.cc
  data_accumulator_unittest.cc
)
TARGET_LINK_LIBRARIES(component_unittest
  ${LTBASE_LINKLIBS}
)
