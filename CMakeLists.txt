cmake_minimum_required(VERSION 3.10)

project(tad-bits)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)