cmake_minimum_required(VERSION 3.7)
project(dicomtoitk)
set (dicomtoitk_VERSION_MAJOR 1)
set (dicomtoitk_VERSION_MINOR 0)
set(dicomtoitk_VERSION ${dicomtoitk_VERSION_MAJOR}.${dicomtoitk_VERSION_MINOR})

set(CMAKE_CXX_STANDARD 14)

# Find ITK.
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})

add_library(dicomtoitk SHARED ${ITK_SOURCES} dicomToItk.cpp dicomToItk.h)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/dicomToItk/dicomtoitkConfigVersion.cmake"
        VERSION ${dicomtoitk_VERSION}
        COMPATIBILITY AnyNewerVersion)

install(TARGETS dicomtoitk EXPORT dicomToItkTargets DESTINATION lib)
install(FILES dicomToItk.h DESTINATION include/${PROJECT_NAME}-${dicomtoitk_VERSION_MAJOR}.${dicomtoitk_VERSION_MINOR})

export(EXPORT dicomToItkTargets
        FILE "${CMAKE_CURRENT_BINARY_DIR}/dicomToItk/dicomtoitkTargets.cmake"
        NAMESPACE DicomToItk::)


configure_file(cmake/dicomtoitkConfig.cmake
        "${CMAKE_CURRENT_BINARY_DIR}/dicomToItk/dicomtoitkConfig.cmake"
        COPYONLY)
set(ConfigPackageLocation lib/cmake/dicomToItk)
install(EXPORT dicomToItkTargets
        FILE
        dicomtoitkTargets.cmake
        NAMESPACE
        DicomToItk::
        DESTINATION
        ${ConfigPackageLocation})


install(FILES
        cmake/dicomtoitkConfig.cmake
        "${CMAKE_CURRENT_BINARY_DIR}/dicomToItk/dicomtoitkConfigVersion.cmake"
        DESTINATION
        ${ConfigPackageLocation})