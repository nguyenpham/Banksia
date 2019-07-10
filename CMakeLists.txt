cmake_minimum_required(VERSION 3.7)

project(banksia
  DESCRIPTION "Chess tournament manager for chess engines"
  LANGUAGES CXX)

# Require C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type to "Release"
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
  message(STATUS "Setting build type: ${CMAKE_BUILD_TYPE}")
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release")
endif()

# Enable compiler warnings for gcc
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wall -Wno-sign-compare)
endif()

# Link with threads library
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

# MSVC settings
if(MSVC)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
               PROPERTY VS_STARTUP_PROJECT banksia)
endif()

# Put executable in the build directory root
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# Subdirectories
add_subdirectory(src)
