cmake_minimum_required(VERSION 3.9)

project(minesweeper)

file(GLOB PROJECT_SOURCE src/*)

set(PROJECT_DISTRIBS LICENSE README.md)

# Build configuration
if(MSVC)
  add_compile_options("/W4" "/wd4244" "/wd4324" "/wd4458" "/wd4100")
else()
  add_compile_options("-Wall" "-Wextra" "-Wdouble-promotion" "-Wno-unused-parameter")
  
  # Add debug build type with sanitizers
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options("-fsanitize=address" "-fsanitize=leak" "-g" "-O0")
    add_link_options("-fsanitize=address" "-fsanitize=leak")
  endif()
endif()

find_package (32BLIT CONFIG REQUIRED PATHS ../32blit-sdk $ENV{PATH_32BLIT_SDK})

blit_executable (${PROJECT_NAME} ${PROJECT_SOURCE})
blit_assets_yaml (${PROJECT_NAME} assets.yml)
blit_metadata (${PROJECT_NAME} metadata.yml)
add_custom_target (flash DEPENDS ${PROJECT_NAME}.flash)

# setup release packages
install (FILES ${PROJECT_DISTRIBS} DESTINATION .)
set (CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set (CPACK_GENERATOR "ZIP" "TGZ")
include (CPack)
