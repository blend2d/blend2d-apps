cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0063 NEW) # Honor visibility properties.

project(bl_getting_started C CXX)

# Some basics, set accordingly to your needs.
set(CMAKE_CXX_STANDARD 11)

# Where the 'samples' and 'blend2d' are.
set(SAMPLES_DIR "${CMAKE_CURRENT_LIST_DIR}"    CACHE PATH "Samples directory")
set(BLEND2D_DIR "${SAMPLES_DIR}/../../blend2d" CACHE PATH "Location of 'blend2d'")

# Blend2D
# -------

set(BLEND2D_STATIC TRUE)
include("${BLEND2D_DIR}/CMakeLists.txt")

# C++ Samples
# -----------

set(SAMPLES_CXX_TARGETS
  "bl-getting-started-1"
  "bl-getting-started-2"
  "bl-getting-started-3"
  "bl-getting-started-4"
  "bl-getting-started-5"
  "bl-getting-started-6"
  "bl-getting-started-7"
  "bl-getting-started-8"
)

foreach(target ${SAMPLES_CXX_TARGETS})
  add_executable(${target} src/${target}.cpp)
  set_property(TARGET ${target} PROPERTY CXX_VISIBILITY_PRESET hidden)
  target_link_libraries(${target} Blend2D::Blend2D)
endforeach()

# C-API Samples
# -------------
#
# NOTE: It's possible these samples would link to C++ standard library as well.
# Since this is just examples we won't do anything here that could break some
# setups.
set(SAMPLES_C_TARGETS
  "bl-capi-sample"
)

foreach(target ${SAMPLES_C_TARGETS})
  add_executable(${target} src/${target}.c)
  set_property(TARGET ${target} PROPERTY CXX_VISIBILITY_PRESET hidden)
  target_link_libraries(${target} Blend2D::Blend2D)
endforeach()

# Resources
# ---------

# TODO: Not sure this works everywhere...
foreach(resource "texture.jpeg" "NotoSans-Regular.ttf")
  if(NOT CMAKE_BUILD_TYPE)
    add_custom_command(TARGET bl-getting-started-1 POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/../resources/${resource}"
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>/${resource}"
      COMMENT "Resources")
  else()
    add_custom_command(TARGET bl-getting-started-1 POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/../resources/${resource}"
        "${CMAKE_CURRENT_BINARY_DIR}/${resource}"
      COMMENT "Resources")
  endif()
endforeach()
