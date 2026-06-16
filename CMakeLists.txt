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

# Automate source file collection from the src/ directory
file(GLOB SOURCES "src/*.cpp")

# Define the executable target
add_executable(3designer ${SOURCES})

# Target-specific include directories (local include/ and OCCT include/)
target_include_directories(3designer PRIVATE
    include
    ${OpenCASCADE_INCLUDE_DIR}
)

# Link explicitly listing the required OCCT toolkit names (highly compatible)
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
)

# Configure target RPATH properties for dynamic library loading at runtime
set_target_properties(3designer PROPERTIES
    SKIP_BUILD_RPATH FALSE
    INSTALL_RPATH_USE_LINK_PATH TRUE
)


