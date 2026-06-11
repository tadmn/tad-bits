cmake_minimum_required(VERSION 3.10)

include(cmake/CPM.cmake)

project(tad-bits)

option(BUILD_TESTS "Build unit test executable" OFF)
option(INCLUDE_RESAMPLER "Include libsamplerate wrapper" OFF)

CPMAddPackage("gh:tadmn/choc#5685fb59db9e60d31a35df7bff0ce7967beaaf25")

add_library(tad-bits INTERFACE
  include/tb_AudioFeatures.h
  include/tb_Core.h
  include/tb_DspUtilities.h
  include/tb_FifoBuffer.h
  include/tb_Denormals.h
  include/tb_Interpolation.h
  include/tb_Math.h
  include/tb_SampleRateConverter.h
  include/tb_Space.h
  include/tb_Windowing.h
)

target_include_directories(tad-bits INTERFACE include)
target_link_libraries(tad-bits INTERFACE choc)

if (INCLUDE_RESAMPLER)
  CPMAddPackage(
    NAME libsamplerate
    GITHUB_REPOSITORY libsndfile/libsamplerate
    GIT_TAG 2ccde9568cca73c7b32c97fefca2e418c16ae5e3
    OPTIONS
    "BUILD_TESTING OFF"
    "LIBSAMPLERATE_EXAMPLES OFF"
    "LIBSAMPLERATE_INSTALL OFF"
  )

  target_link_libraries(tad-bits INTERFACE samplerate)
endif()

if (BUILD_TESTS)
  include(cmake/compile-options.cmake)
  CPMAddPackage("gh:catchorg/Catch2@3.15.0")
  add_executable(tad-bits-testrunner
    tests/test_SampleRateConverter.cpp
    tests/test_AudioFeatures.cpp
    tests/test_Denormals.cpp)

  target_link_libraries(tad-bits-testrunner PRIVATE tad-bits Catch2::Catch2WithMain)
  add_compiler_warnings(tad-bits-testrunner)
endif()