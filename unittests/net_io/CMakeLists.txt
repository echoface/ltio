ADD_EXECUTABLE(net_unittest
  catch_main.cc
  maglev_unittest.cc
  router_unittest.cc
  client_unittest.cc
  net_base_unittest.cc
  co_io_service.cc
  )

TARGET_LINK_LIBRARIES(net_unittest
  ltio
)
