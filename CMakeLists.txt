cmake_minimum_required (VERSION 3.10)

project ("GraphiT" LANGUAGES CXX)

option(GFT_BUILD_APPS "Build Graphi-T example apps" ON)
option(GFT_WITH_VULKAN "Build Graphi-T with Vulkan GPU backend" ON)
option(GFT_WITH_GLSLANG "Build Graphi-T with glslang for runtime-shader compilation" ON)



set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE PATH "Build Type" FORCE)
endif()

if(GFT_WITH_VULKAN)
    add_definitions(-DGFT_WITH_VULKAN=1)
    find_package(Vulkan)
    if(NOT Vulkan_FOUND)
        message("")
        message("-- Vulkan not found! Vulkan context is disabled")
        message("")
    else()
        message("-- Vulkan context enabled")
    endif()
endif()

set(LINK_LIBS
    ${Vulkan_LIBRARIES}
)
set(INC_DIRS
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/third"
    ${Vulkan_INCLUDE_DIRS}
)

# Runtime shader compilation support.
if(GFT_WITH_GLSLANG)
    add_definitions(-DGFT_WITH_GLSLANG=1)
    set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
    add_subdirectory(third/glslang)
endif()

# Used to inline kernel/shaders.
add_executable(bin2c
    "bin2c.cpp"
    "include/gft/assert.hpp"
    "include/gft/util.hpp"
    "src/gft/util.cpp"
)

# The GraphiT library.
include_directories(${INC_DIRS})
file(GLOB_RECURSE SRCS "${PROJECT_SOURCE_DIR}/src/*")
file(GLOB_RECURSE INCS "${PROJECT_SOURCE_DIR}/include/*")
add_library(GraphiT STATIC ${SRCS} ${INCS} )
target_link_libraries(GraphiT glslang SPIRV ${LINK_LIBS})
add_dependencies(GraphiT bin2c)

# GraphiT example apps.
if(GFT_BUILD_APPS)
    make_directory("${CMAKE_BINARY_DIR}/assets/")
    add_subdirectory("${PROJECT_SOURCE_DIR}/apps")
endif()
