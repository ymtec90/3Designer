cmake_minimum_required(VERSION 3.12)
project(3Designer LANGUAGES CXX)

# Require C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCASCADE package without components to avoid 'Specified unsupported component' errors
find_package(OpenCASCADE REQUIRED)

# Add library search directories from OpenCASCADE variables
if(OpenCASCADE_LIBRARY_DIR)
    link_directories(${OpenCASCADE_LIBRARY_DIR})
endif()

# Define common engine sources
set(COMMON_SOURCES
    src/Feature.cpp
    src/FeatureTree.cpp
    src/WorkPlane.cpp
)

# Define the CLI executable target
add_executable(3designer
    src/main.cpp
    src/CLI.cpp
    ${COMMON_SOURCES}
)

target_include_directories(3designer PRIVATE
    include
    ${OpenCASCADE_INCLUDE_DIR}
)

target_link_libraries(3designer PRIVATE
    TKernel
    TKMath
    TKBRep
    TKGeomBase
    TKGeomAlgo
    TKTopAlgo
    TKPrim
    TKBO
    TKDESTL
    TKMesh
    TKXSBase
    TKFillet
)

set_target_properties(3designer PROPERTIES
    SKIP_BUILD_RPATH FALSE
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

# Define the pybind11 python module target
add_subdirectory(external/pybind11)

pybind11_add_module(designer_engine
    src/bindings.cpp
    ${COMMON_SOURCES}
)

target_include_directories(designer_engine PRIVATE
    include
    ${OpenCASCADE_INCLUDE_DIR}
)

target_link_libraries(designer_engine PRIVATE
    TKernel
    TKMath
    TKBRep
    TKGeomBase
    TKGeomAlgo
    TKTopAlgo
    TKPrim
    TKBO
    TKDESTL
    TKMesh
    TKXSBase
    TKFillet
)

set_target_properties(designer_engine PROPERTIES
    SKIP_BUILD_RPATH FALSE
    INSTALL_RPATH_USE_LINK_PATH TRUE
)


