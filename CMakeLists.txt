﻿cmake_minimum_required (VERSION 3.10)

project ("GraphiT" LANGUAGES CXX)

option(GFT_BUILD_APPS "Build Graphi-T example apps" ON)



set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE PATH "Build Type" FORCE)
endif()

set(LINK_LIBS
)
set(INC_DIRS
    "${PROJECT_SOURCE_DIR}/include"
)

if(APPLE)
find_library(Foundation NAMES Foundation)
find_library(AppKit NAMES AppKit)
find_library(Metal NAMES Metal)
find_library(MetalKit NAMES MetalKit)
list(APPEND LINK_LIBS ${Foundation} ${AppKit} ${Metal} ${MetalKit})
endif()

set(BUILD_STATIC_LIBS ON)
add_subdirectory(${PROJECT_SOURCE_DIR}/third/glm)
list(APPEND LINK_LIBS
    glm
)
unset(BUILD_STATIC_LIBS)

find_package(Vulkan)
add_subdirectory(third/VulkanMemoryAllocator)
if(NOT Vulkan_FOUND)
    message("")
    message("-- Vulkan not found! Vulkan context is disabled")
    message("")
else()
    message("-- Vulkan context enabled")
    list(APPEND INC_DIRS
        ${Vulkan_INCLUDE_DIRS}
        third/VulkanMemoryAllocator/include
    )
    list(APPEND LINK_LIBS
        ${Vulkan_LIBRARIES}
        VulkanMemoryAllocator
    )
endif()


# Runtime shader compilation support.
set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
add_subdirectory(third/glslang)
list(APPEND INC_DIRS
    "${PROJECT_SOURCE_DIR}/third"
)
list(APPEND LINK_LIBS
    glslang
    SPIRV
)

# Used to inline kernel/shaders.
add_executable(bin2c
    "bin2c.cpp"
    "include/gft/assert.hpp"
    "include/gft/util.hpp"
    "src/gft/util.cpp"
)

# The GraphiT library.
include_directories(${INC_DIRS})
file(GLOB_RECURSE SRCS "${PROJECT_SOURCE_DIR}/src/*.cpp")
if (APPLE)
    list(APPEND SRCS "${PROJECT_SOURCE_DIR}/src/gft/platform/macos.mm")
endif()
file(GLOB_RECURSE INCS "${PROJECT_SOURCE_DIR}/include/*.hpp")
add_library(GraphiT STATIC ${SRCS} ${INCS})
target_link_libraries(GraphiT ${LINK_LIBS})
add_dependencies(GraphiT bin2c)

# GraphiT example apps.
if(GFT_BUILD_APPS)
    make_directory("${CMAKE_BINARY_DIR}/assets/")
    add_subdirectory("${PROJECT_SOURCE_DIR}/apps")
endif()
