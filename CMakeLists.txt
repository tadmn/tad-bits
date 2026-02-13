cmake_minimum_required(VERSION 3.10)

include(cmake/CPM.cmake)

project(tad-bits)

option(BUILD_TESTS "Build unit test executable" OFF)
option(INCLUDE_RESAMPLER "Include libsamplerate wrapper" ON)

CPMAddPackage("gh:tadmn/choc#2f5b8627708ff4702b4fb6646fddf0c5f4a52007")

add_library(tad-bits INTERFACE
  include/tb_Core.h
  include/tb_DspUtilities.h
  include/tb_FifoBuffer.h
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
  CPMAddPackage("gh:catchorg/Catch2@3.5.2")
  add_executable(tad-bits-testrunner tests/test_SampleRateConverter.cpp)
  target_link_libraries(tad-bits-testrunner PRIVATE tad-bits Catch2::Catch2WithMain)
  add_compiler_warnings(tad-bits-testrunner)
endif()