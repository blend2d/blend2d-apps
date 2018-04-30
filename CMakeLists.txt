cmake_minimum_required(VERSION 3.1)

project(samples CXX)

# Some basics, set accordingly to your needs.
set(CMAKE_CXX_STANDARD 11)

# Where the 'samples' and 'b2d' are.
set(SAMPLES_DIR "${CMAKE_CURRENT_LIST_DIR}"
    CACHE PATH "Samples directory")

set(B2D_DIR "${SAMPLES_DIR}/../b2d"
    CACHE PATH "Location of 'b2d'")

# Enable Blend2D static build.
set(B2D_BUILD_STATIC TRUE)
include("${B2D_DIR}/CMakeLists.txt")

# Blend2D typically needs pthread on Linux and some Windows
# libraries on Windows platform. This will make sure that
# samples will have everything that is required.
set(SAMPLES_LIBS ${B2D_LIBS})
set(SAMPLES_CFLAGS ${B2D_CFLAGS})

set(SAMPLES_TARGETS
  "b2d-getting-started-1"
  "b2d-getting-started-2"
  "b2d-getting-started-3"
  "b2d-getting-started-4"
  "b2d-getting-started-5"
  "b2d-getting-started-6"
  "b2d-getting-started-7"
  "b2d-sample-tiger"
)

foreach(sample ${SAMPLES_TARGETS})
  # Add executable of the `app` and its dependencies.
  add_executable(${sample} src/${sample}.cpp)
  target_include_directories(${sample} BEFORE PRIVATE ${B2D_INCLUDE_DIR})
  target_compile_options(${sample} PRIVATE ${SAMPLES_CFLAGS})
  target_link_libraries(${sample} ${SAMPLES_LIBS})
endforeach()

foreach(resource "texture.jpeg" "NotoSans-Regular.ttf")
  file(COPY "${CMAKE_CURRENT_LIST_DIR}/resources/${resource}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
endforeach()
