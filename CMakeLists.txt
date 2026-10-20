cmake_minimum_required(VERSION 3.16)


project(surface_mesh_cutter LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Main library
add_library(surface_mesh_cutter
    src/StlIO.cpp
    src/MergeVertices.cpp
    src/MeshTopology.cpp
    src/PlaneCutInternal.cpp
    src/PlaneCut.cpp
)


target_include_directories(surface_mesh_cutter
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)


# Optional command-line application
option(BUILD_CUTTER_APP "Build the cutter command-line application" ON)


if(BUILD_CUTTER_APP)
    add_executable(cutter
        src/main.cpp
    )


    target_link_libraries(cutter
        PRIVATE
            surface_mesh_cutter
    )
endif()


# Tests
enable_testing()
add_subdirectory(tests)