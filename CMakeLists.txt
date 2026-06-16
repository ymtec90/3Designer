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

# Use GLOB to find all C++ source files
file(GLOB SOURCES "src/*.cpp")

# Filter sources for the CLI executable (3designer) - remove bindings.cpp
set(CLI_SOURCES ${SOURCES})
list(REMOVE_ITEM CLI_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/bindings.cpp")

# Filter sources for the python module (designer_engine) - remove main.cpp and CLI.cpp
set(ENGINE_SOURCES ${SOURCES})
list(REMOVE_ITEM ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
list(REMOVE_ITEM ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/CLI.cpp")

# Define the CLI executable target
add_executable(3designer ${CLI_SOURCES})

target_include_directories(3designer PRIVATE
    src
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

pybind11_add_module(designer_engine ${ENGINE_SOURCES})

target_include_directories(designer_engine PRIVATE
    src
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


