SET(BASE_SOURCES
  # chromium numerics
  numerics/checked_math_impl.h
  numerics/clamped_math_impl.h
  numerics/safe_conversions_impl.h
  numerics/safe_conversions_arm_impl.h
  numerics/safe_math_arm_impl.h
  numerics/safe_math_shared_impl.h
  numerics/safe_math_clang_gcc_impl.h

  # simple time utils
  time/time.cc
  time/time_delta.cc
  time/time_ticks.cc
  time/timestamp.cc
  time/time_utils.cc
  time/nspr/prtime.cc

  # posix-unix utils
  utils/sys_error.cc
  utils/rand_util.cc
  utils/rand_util_posix.cc
  utils/ns_convertor.cc
  utils/string/str_utils.cc

  #coroutine
  coroutine/co_loop.cc
  coroutine/co_runner.cc
  coroutine/co_mutex.cc
  coroutine/io_event.cc
  coroutine/wait_group.cc

  # task eventloop
  message_loop/event.cc
  message_loop/fd_event.cc
  message_loop/fdev_mgr.cc
  message_loop/event_pump.cc
  message_loop/linux_signal.cpp
  message_loop/file_util_linux.cc
  message_loop/io_multiplexer.cc
  message_loop/io_mux_epoll.cc
  #message_loop/timer_task_queue.cc
  message_loop/message_loop.cc
  message_loop/timeout_event.cc
  message_loop/timer_task_helper.cc
  message_loop/repeating_timer.cc

  #memory
  memory/spin_lock.cc
  memory/lazy_instance.cc

  # gzip compression utils
  utils/gzip/gzip_utils.cc

  # closure
  closure/location.cc
  )

if (LTIO_WITH_OPENSSL)
  list(APPEND BASE_SOURCES crypto/lt_ssl.cc)
endif()

# configure a header file to pass some of the CMake settings to the source code
configure_file(ltio_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/ltio_config.h)
message(STATUS "ltio_config.h gen in location ${CMAKE_CURRENT_BINARY_DIR}")

target_sources(ltio PRIVATE ${BASE_SOURCES})
target_sources(ltio_shared PRIVATE ${BASE_SOURCES})

add_executable(ut_ltbase
    sys/byteords_unittest.cc
    sys/sys_info_unittest.cc

    coroutine/coro_unittest.cc

    message_loop/messageloop_unittest.cc
    message_loop/repeat_timer_unittest.cc

    time/time_unittest.cc

    crypto/crypto_unittest.cc

    utils/utils_unittest.cc
    utils/base64/base64_unittest.cc
    )

target_link_libraries(ut_ltbase PRIVATE ltio profiler Catch2::Catch2WithMain)
