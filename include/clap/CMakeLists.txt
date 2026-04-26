add_library(clap INTERFACE)
add_library(clap::clap ALIAS clap)

if(CLAP_BUILD_MODULES)
        if(CMAKE_VERSION VERSION_LESS 3.28)
                message(FATAL_ERROR "CLAP_BUILD_MODULES requires CMake 3.28 or newer for CXX_MODULES file sets")
        endif()

        target_sources(clap INTERFACE
                FILE_SET modules TYPE CXX_MODULES
                BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                FILES ${CMAKE_CURRENT_SOURCE_DIR}/clap.cppm
        )
endif()

target_include_directories(clap INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_options(clap INTERFACE
  -freflection
  -fcontracts
)

target_compile_features(clap INTERFACE cxx_std_26)
target_link_libraries(clap INTERFACE stdc++exp)

include(GNUInstallDirs)

if(CLAP_BUILD_MODULES)
  install(TARGETS clap
          EXPORT clapTargets
          FILE_SET modules DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/clap)
else()
  install(TARGETS clap EXPORT clapTargets)
endif()

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.cppm")

install(EXPORT clapTargets
        NAMESPACE clap::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clap)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/clapConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/clapConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clap
)
write_basic_package_version_file("clapConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/clapConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/clapConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/clap)
