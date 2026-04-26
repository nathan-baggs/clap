cmake_minimum_required(VERSION 3.22)

project(
  clap
  VERSION "0.0.1"
  DESCRIPTION "Command Line Argument Parser"
  LANGUAGES C CXX
)

include(FetchContent)

set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(FETCHCONTENT_QUIET FALSE)

add_subdirectory(include/clap)
option(CLAP_BUILD_EXAMPLES "Build examples" ON)
if(CLAP_BUILD_EXAMPLES)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/CMakeLists.txt")
    add_subdirectory(src)
  endif()
endif()

option(CLAP_BUILD_MODULES "Build C++ module interface" OFF)

option(CLAP_BUILD_TESTS "Build unit tests" ON)
if(CLAP_BUILD_TESTS)
  enable_testing()
  include(CTest)
  add_subdirectory(tests)
endif()

set(CPACK_GENERATOR "ZIP")
include(CPack)

