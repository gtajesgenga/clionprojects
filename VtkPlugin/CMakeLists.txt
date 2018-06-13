cmake_minimum_required(VERSION 3.7)
project(VtkPlugin)

set(CMAKE_CXX_STANDARD 11)

set(USE_SYSTEM_BOOST OFF BOOL "Use the system version of Boost")
set(USE_SYSTEM_JSONCPP OFF BOOL "Use the system version of JsonCpp")
set(STATIC_BUILD ON BOOL "Static build of the third-party libraries (necessary for Windows)")
set(ALLOW_DOWNLOADS ON BOOL "Allow CMake to download packages")

include(${CMAKE_SOURCE_DIR}/Resources/DownloadPackage.cmake)
include(${CMAKE_SOURCE_DIR}/Resources/BoostConfiguration.cmake)
include(${CMAKE_SOURCE_DIR}/Resources/JsonCppConfiguration.cmake)

include_directories(${CMAKE_SOURCE_DIR}/Resources)

# Find ITK.
#find_package(ITK REQUIRED)
#include(${ITK_USE_FILE})
find_package(dicomtoitk)

set(CORE_SOURCES ${BOOST_SOURCES} ${JSONCPP_SOURCES} ${ITK_SOURCES})

add_library(VtkPlugin SHARED ${CORE_SOURCES} VtkPlugin.cpp)

