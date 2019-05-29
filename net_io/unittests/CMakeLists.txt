ADD_EXECUTABLE(net_unittest
  catch_main.cc
  maglev_unittest.cc
	client_unittest.cc
	net_base_unittest.cc
)

TARGET_LINK_LIBRARIES(net_unittest
	ltnet
  ltbase
  ${LTBASE_LINKLIBS}
)
