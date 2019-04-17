cmake_minimum_required(VERSION 3.1)

project(bl_getting_started C CXX)

# Some basics, set accordingly to your needs.
set(CMAKE_CXX_STANDARD 11)

# Where the 'samples' and 'blend2d' are.
set(SAMPLES_DIR "${CMAKE_CURRENT_LIST_DIR}"    CACHE PATH "Samples directory")
set(BLEND2D_DIR "${SAMPLES_DIR}/../../blend2d" CACHE PATH "Location of 'blend2d'")

# Blend2D
# -------

set(BLEND2D_BUILD_STATIC TRUE)
include("${BLEND2D_DIR}/CMakeLists.txt")
set(SAMPLES_LIBS ${BLEND2D_LIBS})
set(SAMPLES_CFLAGS ${BLEND2D_CFLAGS})

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(GNU|Clang|AppleClang)$")
  list(APPEND SAMPLES_CFLAGS "-fvisibility=hidden")
endif()

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

foreach(sample ${SAMPLES_CXX_TARGETS})
  add_executable(${sample} src/${sample}.cpp)
  target_include_directories(${sample} BEFORE PRIVATE ${BLEND2D_INCLUDE_DIR})
  target_compile_options(${sample} PRIVATE ${SAMPLES_CFLAGS})
  target_link_libraries(${sample} ${SAMPLES_LIBS})
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

foreach(sample ${SAMPLES_C_TARGETS})
  add_executable(${sample} src/${sample}.c)
  target_include_directories(${sample} BEFORE PRIVATE ${BLEND2D_INCLUDE_DIR})
  target_compile_options(${sample} PRIVATE ${SAMPLES_CFLAGS})
  target_link_libraries(${sample} ${SAMPLES_LIBS})
endforeach()

# Resources
# ---------

# TODO: Not sure this works everywhere...
foreach(resource "texture.jpeg" "NotoSans-Regular.ttf")
  if(NOT "${CMAKE_BUILD_TYPE}")
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
