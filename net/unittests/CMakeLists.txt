ADD_EXECUTABLE(net_unittest
  catch_main.cc
	client_unittest.cc
)

TARGET_LINK_LIBRARIES(net_unittest
	ltnet
  ltbase
  ${LTBASE_LINKLIBS}
)
