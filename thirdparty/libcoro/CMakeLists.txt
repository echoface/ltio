cmake_minimum_required(VERSION 3.5)
ADD_DEFINITIONS(-fPIC)

include (CheckFunctionExists)
include (CheckIncludeFiles)
include (CheckSymbolExists)

project (libcoro VERSION 0.2.1 LANGUAGES C CXX)


add_library(coro coro.c coro.h)
set_target_properties(coro
  PROPERTIES COMPILE_DEFINITIONS "CORO_ASM=1"
  )

add_executable(coro_test test.c)
target_link_libraries (coro_test coro)

add_executable(coro_bench bench.cc)
target_link_libraries (coro_bench coro)

check_include_files (setjmp.h HAVE_SETJMP_H)
check_include_files (ucontext.h HAVE_UCONTEXT_H)
check_symbol_exists (sigaltstack "signal.h" HAVE_SIGALTSTACK)

configure_file(config.h.in ${PROJECT_SOURCE_DIR}/config.h)
#include_directories("${PROJECT_BINARY_DIR}")
