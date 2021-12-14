cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0063 NEW) # Honor visibility properties.

project(bl_qt CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC TRUE)

set(SAMPLES_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "Samples directory")
set(BLEND2D_DIR "${SAMPLES_DIR}/../../blend2d" CACHE PATH "Location of 'blend2d'")

set(BLEND2D_STATIC TRUE)
include("${BLEND2D_DIR}/CMakeLists.txt")

find_package(Qt5Widgets REQUIRED)

set(SAMPLES_TARGETS
  "bl-qt-gradients"
  "bl-qt-ellipticarc"
  "bl-qt-stroke"
  "bl-qt-text"

  "bl-qt-bounces"
  "bl-qt-circles"
  "bl-qt-particles"
  "bl-qt-polys"
  "bl-qt-rects"
  "bl-qt-tiger"
)

foreach(target ${SAMPLES_TARGETS})
  add_executable(${target} WIN32 src/${target}.cpp src/qblcanvas.cpp)
  set_property(TARGET ${target} PROPERTY CXX_VISIBILITY_PRESET hidden)
  target_link_libraries(${target} Qt5::Widgets blend2d::blend2d)
endforeach()
