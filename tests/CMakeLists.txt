FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

mark_as_advanced(BUILD_GMOCK BUILD_GTEST gtest_hide_internal_symbols)

add_executable(unit_tests
  clap_tests.cpp
)

target_compile_options(unit_tests PRIVATE
  -Wall
  -Wextra
  -pedantic
  -Werror
  -Wconversion-null
  -Wmissing-declarations
  -Woverlength-strings
  -Wpointer-arith
  -Wunused-local-typedefs
  -Wunused-result
  -Wvarargs
  -Wvla
  -Wwrite-strings
  -Wno-missing-declarations
)

target_link_libraries(unit_tests gmock_main clap::clap)
gtest_discover_tests(unit_tests DISCOVERY_MODE PRE_TEST)

add_executable(simple
  simple.cpp
)

target_compile_options(simple PRIVATE
  -Wall
  -Wextra
  -pedantic
  -Werror
  -Wconversion-null
  -Wmissing-declarations
  -Woverlength-strings
  -Wpointer-arith
  -Wunused-local-typedefs
  -Wunused-result
  -Wvarargs
  -Wvla
  -Wwrite-strings
  -Wno-missing-declarations
)

target_link_libraries(simple clap::clap)


