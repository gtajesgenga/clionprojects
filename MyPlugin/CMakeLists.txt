cmake_minimum_required(VERSION 3.7)
project(MyPlugin)

set(CMAKE_CXX_STANDARD 11)

# Setup of JsonCpp
include_directories(Resources/jsoncpp-0.5.0/include)
list(APPEND THIRD_PARTY_SOURCES
        Resources/jsoncpp-0.5.0/src/json_reader.cpp
        Resources/jsoncpp-0.5.0/src/json_value.cpp
        Resources/jsoncpp-0.5.0/src/json_writer.cpp
        )

add_definitions(
        -DORTHANC_ENABLE_MD5=0
        -DORTHANC_ENABLE_BASE64=0
        -DORTHANC_ENABLE_LOGGING=0
        -DHAS_ORTHANC_EXCEPTION=1
)

add_library(MyPlugin SHARED
        EntryPoints.cpp
        MyPluginPlugin.cpp
        OrthancContext.cpp
        Resources/Configuration.cpp
        Resources/Enumerations.cpp
        Resources/OrthancPluginCppWrapper.cpp
        Resources/Toolbox.cpp
        ${THIRD_PARTY_SOURCES})
