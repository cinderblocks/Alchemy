project(deps)

include(00-Common)
include(FetchContent)

set(CMAKE_FOLDER "Third Party")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

 FetchContent_Declare(
   meshoptimizer
   GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
   GIT_TAG        v0.14
   )

# FetchContent_Declare(
#   Catch2
#   GIT_REPOSITORY https://github.com/catchorg/Catch2.git
#   GIT_TAG        v2.11.0
#   )
# FetchContent_Declare(
#   fmt
#   GIT_REPOSITORY https://github.com/fmtlib/fmt.git
#   GIT_TAG        6.1.2
#   )
# FetchContent_Declare(
#   nlohmann_json
#   GIT_REPOSITORY https://github.com/nlohmann/json.git
#   GIT_TAG        v3.7.3
#   )
# FetchContent_Declare(
#   absl
#   GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
#   GIT_TAG        cde2e2410e58c884b3bf5f67c6511e6266036249
# )

# # This is a hack because absl has dumb cmake
# set(OLD_BUILD_TEST ${BUILD_TESTING})
# set(BUILD_TESTING OFF)
# FetchContent_MakeAvailable(absl)
# set(BUILD_TESTING ${OLD_BUILD_TEST})

# # Supress warnings inside abseil under MSVC
# if(WINDOWS)
#   target_compile_options(absl_strings PRIVATE /wd4018)
#   target_compile_options(absl_str_format_internal PRIVATE /wd4018)
#   target_compile_options(absl_flags_usage_internal PRIVATE /wd4018)
# endif()


# if (BUILD_TESTING)
#   FetchContent_MakeAvailable(Catch2)
# endif()

# #Download the rest of the libraries
# FetchContent_MakeAvailable(fmt)
FetchContent_MakeAvailable(meshoptimizer)

# # Typically you don't care so much for a third party library's tests to be
# # run from your own project's code.
# set(JSON_BuildTests OFF CACHE INTERNAL "")

# # If you only include this third party in PRIVATE source files, you do not
# # need to install it when your main project gets installed.
# set(JSON_Install OFF CACHE INTERNAL "")
# FetchContent_MakeAvailable(nlohmann_json)

unset(CMAKE_FOLDER)
unset(CMAKE_POSITION_INDEPENDENT_CODE)