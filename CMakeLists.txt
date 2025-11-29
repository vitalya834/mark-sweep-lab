cmake_minimum_required(VERSION 3.16)

project(mark_sweep_lab VERSION 1.0.0 LANGUAGES C)

option(MARK_SWEEP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

add_library(mark_sweep src/gc.c)
target_include_directories(mark_sweep PUBLIC include)
target_compile_features(mark_sweep PUBLIC c_std_11)

if(MSVC)
  target_compile_options(mark_sweep PRIVATE /W4)
  if(MARK_SWEEP_WARNINGS_AS_ERRORS)
    target_compile_options(mark_sweep PRIVATE /WX)
  endif()
else()
  target_compile_options(mark_sweep PRIVATE -Wall -Wextra -Wpedantic)
  if(MARK_SWEEP_WARNINGS_AS_ERRORS)
    target_compile_options(mark_sweep PRIVATE -Werror)
  endif()
endif()

add_executable(mark_sweep_demo examples/demo.c)
target_link_libraries(mark_sweep_demo PRIVATE mark_sweep)

include(CTest)
if(BUILD_TESTING)
  add_executable(mark_sweep_tests tests/test_gc.c)
  target_link_libraries(mark_sweep_tests PRIVATE mark_sweep)
  add_test(NAME mark_sweep_tests COMMAND mark_sweep_tests)
endif()
