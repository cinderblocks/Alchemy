# -*- cmake -*-

# cmake_minimum_required should appear before any
# other commands to guarantee full compatibility
# with the version specified
cmake_minimum_required(VERSION 3.13.0 FATAL_ERROR)
cmake_policy(SET CMP0063 NEW)

set(ROOT_PROJECT_NAME "Alchemy" CACHE STRING
    "The root project/makefile/solution name. Defaults to Alchemy.")
project(${ROOT_PROJECT_NAME})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_STANDARD 11)
if(MSVC)
  set(CMAKE_CXX_STANDARD 20)
else()
  set(CMAKE_CXX_STANDARD 17)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

include(Variables)
include(BuildVersion)

add_subdirectory(cmake)
add_subdirectory(deps)

add_subdirectory(${LIBS_OPEN_PREFIX}llaudio)
add_subdirectory(${LIBS_OPEN_PREFIX}llappearance)
add_subdirectory(${LIBS_OPEN_PREFIX}llcharacter)
add_subdirectory(${LIBS_OPEN_PREFIX}llcommon)
add_subdirectory(${LIBS_OPEN_PREFIX}llcorehttp)
add_subdirectory(${LIBS_OPEN_PREFIX}llimage)
add_subdirectory(${LIBS_OPEN_PREFIX}llkdu)
add_subdirectory(${LIBS_OPEN_PREFIX}llimagej2coj)
add_subdirectory(${LIBS_OPEN_PREFIX}llinventory)
add_subdirectory(${LIBS_OPEN_PREFIX}llmath)
add_subdirectory(${LIBS_OPEN_PREFIX}llmessage)
add_subdirectory(${LIBS_OPEN_PREFIX}llprimitive)
add_subdirectory(${LIBS_OPEN_PREFIX}llrender)
add_subdirectory(${LIBS_OPEN_PREFIX}llfilesystem)
add_subdirectory(${LIBS_OPEN_PREFIX}llwindow)
add_subdirectory(${LIBS_OPEN_PREFIX}llxml)

if (WINDOWS AND EXISTS ${LIBS_CLOSED_DIR}copy_win_scripts)
  add_subdirectory(${LIBS_CLOSED_PREFIX}copy_win_scripts)
endif (WINDOWS AND EXISTS ${LIBS_CLOSED_DIR}copy_win_scripts)

add_custom_target(viewer)

add_subdirectory(${LIBS_OPEN_PREFIX}llplugin)
add_subdirectory(${LIBS_OPEN_PREFIX}llui)
add_subdirectory(${LIBS_OPEN_PREFIX}viewer_components)

# Legacy C++ tests. Build always, run if LL_TESTS is true.
add_subdirectory(${VIEWER_PREFIX}test)

if (ENABLE_MEDIA_PLUGINS)
# viewer media plugins
add_subdirectory(${LIBS_OPEN_PREFIX}media_plugins)
endif (ENABLE_MEDIA_PLUGINS)

# sets the 'startup project' for debugging from visual studio.
set_property(
    DIRECTORY ${VIEWER_PREFIX}
    PROPERTY VS_STARTUP_PROJECT alchemy-bin
    )

if (USE_BUGSPLAT)
    if (BUGSPLAT_DB)
        message(STATUS "Building with BugSplat; database '${BUGSPLAT_DB}'")
    else (BUGSPLAT_DB)
        message(WARNING "Building with BugSplat, but no database name set (BUGSPLAT_DB)")
    endif (BUGSPLAT_DB)
endif (USE_BUGSPLAT)

add_subdirectory(${VIEWER_PREFIX}newview)
add_dependencies(viewer alchemy-bin)

add_subdirectory(${VIEWER_PREFIX}doxygen EXCLUDE_FROM_ALL)

if (LL_TESTS)
  # Define after the custom targets are created so
  # individual apps can add themselves as dependencies
  add_subdirectory(${INTEGRATION_TESTS_PREFIX}integration_tests)
endif (LL_TESTS)

