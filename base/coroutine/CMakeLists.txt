
if (LTIO_USE_ACO_CORO_IMPL)
  configure_file(./impl/aco.h.in ./coroutine.h)
  configure_file(./impl/aco.cc.in ./coroutine.cc)
else()
  configure_file(./impl/coro.h.in ./coroutine.h)
  configure_file(./impl/coro.cc.in ./coroutine.cc)
endif()
