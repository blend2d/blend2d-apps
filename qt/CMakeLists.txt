cmake_minimum_required(VERSION 3.1)

project(bl_qt CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC TRUE)

set(SAMPLES_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "Samples directory")
set(BLEND2D_DIR "${SAMPLES_DIR}/../../blend2d" CACHE PATH "Location of 'blend2d'")

set(BLEND2D_BUILD_STATIC TRUE)
include("${BLEND2D_DIR}/CMakeLists.txt")

find_package(Qt5Widgets REQUIRED)

set(SAMPLES_LIBS ${BLEND2D_LIBS})
set(SAMPLES_CFLAGS ${BLEND2D_CFLAGS})
set(SAMPLES_INCLUDE_DIRS ${BLEND2D_INCLUDE_DIR})

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(GNU|Clang|AppleClang)$")
  list(APPEND SAMPLES_CFLAGS "-fvisibility=hidden")
endif()

set(SAMPLES_TARGETS
  "bl-qt-gradients"
  "bl-qt-ellipticarc"
  "bl-qt-stroke"
  "bl-qt-text"

  "bl-qt-bounces"
  "bl-qt-circles"
  "bl-qt-particles"
  "bl-qt-polys"
  "bl-qt-tiger"
)

foreach(sample ${SAMPLES_TARGETS})
  add_executable(${sample} src/${sample}.cpp src/qblcanvas.cpp)
  target_include_directories(${sample} BEFORE PRIVATE ${SAMPLES_INCLUDE_DIRS})
  target_compile_options(${sample} PRIVATE ${SAMPLES_CFLAGS})
  target_link_libraries(${sample} ${SAMPLES_LIBS} Qt5::Widgets)
  qt5_use_modules(${sample} Widgets)
endforeach()
