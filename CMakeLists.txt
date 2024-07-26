cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

cmake_policy(PUSH)
cmake_policy(SET CMP0063 NEW) # Honor visibility properties.
cmake_policy(SET CMP0077 NEW) # option() honors normal variables.

# To avoid warnings when built with dependencies compiled by either homebrew or vcpkg.
set(CMAKE_OSX_DEPLOYMENT_TARGET 14.3)

# Blend2D-Apps - Configuration
# ============================

project(blend2d-apps C CXX)
include(FindPkgConfig)

set(BLEND2D_APPS_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "blend2d-apps directory")

# Dependencies - Blend2D (Requred)
# ================================

set(BLEND2D_DIR "${BLEND2D_APPS_DIR}/../blend2d" CACHE PATH "Location of 'blend2d'")
set(BLEND2D_STATIC TRUE)
include("${BLEND2D_DIR}/CMakeLists.txt")

# Dependencies - Probe
# ====================

pkg_check_modules(CAIRO cairo)
find_package(unofficial-skia CONFIG)
find_package(Qt6 COMPONENTS Core Gui Widgets)

# Dependencies - AntiGrain (Optional)
# ===================================

message("-- [blend2d-apps] adding support for AGG")
set(ANTIGRAIN_SRC
  3rdparty/agg2d/agg2d.h
  3rdparty/agg2d/agg2d.cpp
  3rdparty/agg/include/agg_alpha_mask_u8.h
  3rdparty/agg/include/agg_arc.h
  3rdparty/agg/include/agg_array.h
  3rdparty/agg/include/agg_arrowhead.h
  3rdparty/agg/include/agg_basics.h
  3rdparty/agg/include/agg_bezier_arc.h
  3rdparty/agg/include/agg_bitset_iterator.h
  3rdparty/agg/include/agg_blur.h
  3rdparty/agg/include/agg_bounding_rect.h
  3rdparty/agg/include/agg_bspline.h
  3rdparty/agg/include/agg_clip_liang_barsky.h
  3rdparty/agg/include/agg_color_gray.h
  3rdparty/agg/include/agg_color_rgba.h
  3rdparty/agg/include/agg_config.h
  3rdparty/agg/include/agg_conv_adaptor_vcgen.h
  3rdparty/agg/include/agg_conv_adaptor_vpgen.h
  3rdparty/agg/include/agg_conv_bspline.h
  3rdparty/agg/include/agg_conv_clip_polygon.h
  3rdparty/agg/include/agg_conv_clip_polyline.h
  3rdparty/agg/include/agg_conv_close_polygon.h
  3rdparty/agg/include/agg_conv_concat.h
  3rdparty/agg/include/agg_conv_contour.h
  3rdparty/agg/include/agg_conv_curve.h
  3rdparty/agg/include/agg_conv_dash.h
  3rdparty/agg/include/agg_conv_gpc.h
  3rdparty/agg/include/agg_conv_marker_adaptor.h
  3rdparty/agg/include/agg_conv_marker.h
  3rdparty/agg/include/agg_conv_segmentator.h
  3rdparty/agg/include/agg_conv_shorten_path.h
  3rdparty/agg/include/agg_conv_smooth_poly1.h
  3rdparty/agg/include/agg_conv_stroke.h
  3rdparty/agg/include/agg_conv_transform.h
  3rdparty/agg/include/agg_conv_unclose_polygon.h
  3rdparty/agg/include/agg_curves.h
  3rdparty/agg/include/agg_dda_line.h
  3rdparty/agg/include/agg_ellipse_bresenham.h
  3rdparty/agg/include/agg_ellipse.h
  3rdparty/agg/include/agg_font_cache_manager2.h
  3rdparty/agg/include/agg_font_cache_manager.h
  3rdparty/agg/include/agg_gamma_functions.h
  3rdparty/agg/include/agg_gamma_lut.h
  3rdparty/agg/include/agg_glyph_raster_bin.h
  3rdparty/agg/include/agg_gradient_lut.h
  3rdparty/agg/include/agg_gsv_text.h
  3rdparty/agg/include/agg_image_accessors.h
  3rdparty/agg/include/agg_image_filters.h
  3rdparty/agg/include/agg_line_aa_basics.h
  3rdparty/agg/include/agg_math.h
  3rdparty/agg/include/agg_math_stroke.h
  3rdparty/agg/include/agg_path_length.h
  3rdparty/agg/include/agg_path_storage.h
  3rdparty/agg/include/agg_path_storage_integer.h
  3rdparty/agg/include/agg_pattern_filters_rgba.h
  3rdparty/agg/include/agg_pixfmt_amask_adaptor.h
  3rdparty/agg/include/agg_pixfmt_base.h
  3rdparty/agg/include/agg_pixfmt_gray.h
  3rdparty/agg/include/agg_pixfmt_rgba.h
  3rdparty/agg/include/agg_pixfmt_rgb.h
  3rdparty/agg/include/agg_pixfmt_rgb_packed.h
  3rdparty/agg/include/agg_pixfmt_transposer.h
  3rdparty/agg/include/agg_rasterizer_cells_aa.h
  3rdparty/agg/include/agg_rasterizer_compound_aa.h
  3rdparty/agg/include/agg_rasterizer_outline_aa.h
  3rdparty/agg/include/agg_rasterizer_outline.h
  3rdparty/agg/include/agg_rasterizer_scanline_aa.h
  3rdparty/agg/include/agg_rasterizer_scanline_aa_nogamma.h
  3rdparty/agg/include/agg_rasterizer_sl_clip.h
  3rdparty/agg/include/agg_renderer_base.h
  3rdparty/agg/include/agg_renderer_markers.h
  3rdparty/agg/include/agg_renderer_mclip.h
  3rdparty/agg/include/agg_renderer_outline_aa.h
  3rdparty/agg/include/agg_renderer_outline_image.h
  3rdparty/agg/include/agg_renderer_primitives.h
  3rdparty/agg/include/agg_renderer_raster_text.h
  3rdparty/agg/include/agg_renderer_scanline.h
  3rdparty/agg/include/agg_rendering_buffer_dynarow.h
  3rdparty/agg/include/agg_rendering_buffer.h
  3rdparty/agg/include/agg_rounded_rect.h
  3rdparty/agg/include/agg_scanline_bin.h
  3rdparty/agg/include/agg_scanline_boolean_algebra.h
  3rdparty/agg/include/agg_scanline_p.h
  3rdparty/agg/include/agg_scanline_storage_aa.h
  3rdparty/agg/include/agg_scanline_storage_bin.h
  3rdparty/agg/include/agg_scanline_u.h
  3rdparty/agg/include/agg_shorten_path.h
  3rdparty/agg/include/agg_simul_eq.h
  3rdparty/agg/include/agg_span_allocator.h
  3rdparty/agg/include/agg_span_converter.h
  3rdparty/agg/include/agg_span_gouraud_gray.h
  3rdparty/agg/include/agg_span_gouraud.h
  3rdparty/agg/include/agg_span_gouraud_rgba.h
  3rdparty/agg/include/agg_span_gradient_alpha.h
  3rdparty/agg/include/agg_span_gradient_contour.h
  3rdparty/agg/include/agg_span_gradient.h
  3rdparty/agg/include/agg_span_gradient_image.h
  3rdparty/agg/include/agg_span_image_filter_gray.h
  3rdparty/agg/include/agg_span_image_filter.h
  3rdparty/agg/include/agg_span_image_filter_rgba.h
  3rdparty/agg/include/agg_span_image_filter_rgb.h
  3rdparty/agg/include/agg_span_interpolator_adaptor.h
  3rdparty/agg/include/agg_span_interpolator_linear.h
  3rdparty/agg/include/agg_span_interpolator_persp.h
  3rdparty/agg/include/agg_span_interpolator_trans.h
  3rdparty/agg/include/agg_span_pattern_gray.h
  3rdparty/agg/include/agg_span_pattern_rgba.h
  3rdparty/agg/include/agg_span_pattern_rgb.h
  3rdparty/agg/include/agg_span_solid.h
  3rdparty/agg/include/agg_span_subdiv_adaptor.h
  3rdparty/agg/include/agg_trans_affine.h
  3rdparty/agg/include/agg_trans_bilinear.h
  3rdparty/agg/include/agg_trans_double_path.h
  3rdparty/agg/include/agg_trans_perspective.h
  3rdparty/agg/include/agg_trans_single_path.h
  3rdparty/agg/include/agg_trans_viewport.h
  3rdparty/agg/include/agg_trans_warp_magnifier.h
  3rdparty/agg/include/agg_vcgen_bspline.h
  3rdparty/agg/include/agg_vcgen_contour.h
  3rdparty/agg/include/agg_vcgen_dash.h
  3rdparty/agg/include/agg_vcgen_markers_term.h
  3rdparty/agg/include/agg_vcgen_smooth_poly1.h
  3rdparty/agg/include/agg_vcgen_stroke.h
  3rdparty/agg/include/agg_vcgen_vertex_sequence.h
  3rdparty/agg/include/agg_vertex_sequence.h
  3rdparty/agg/include/agg_vpgen_clip_polygon.h
  3rdparty/agg/include/agg_vpgen_clip_polyline.h
  3rdparty/agg/include/agg_vpgen_segmentator.h
  3rdparty/agg/include/util/agg_color_conv.h
  3rdparty/agg/include/util/agg_color_conv_rgb16.h
  3rdparty/agg/include/util/agg_color_conv_rgb8.h
  3rdparty/agg/src/agg_arc.cpp
  3rdparty/agg/src/agg_arrowhead.cpp
  3rdparty/agg/src/agg_bezier_arc.cpp
  3rdparty/agg/src/agg_bspline.cpp
  3rdparty/agg/src/agg_color_rgba.cpp
  3rdparty/agg/src/agg_curves.cpp
  3rdparty/agg/src/agg_gsv_text.cpp
  3rdparty/agg/src/agg_image_filters.cpp
  3rdparty/agg/src/agg_line_aa_basics.cpp
  3rdparty/agg/src/agg_line_profile_aa.cpp
  3rdparty/agg/src/agg_rounded_rect.cpp
  3rdparty/agg/src/agg_sqrt_tables.cpp
  3rdparty/agg/src/agg_trans_affine.cpp
  3rdparty/agg/src/agg_trans_double_path.cpp
  3rdparty/agg/src/agg_trans_single_path.cpp
  3rdparty/agg/src/agg_trans_warp_magnifier.cpp
  3rdparty/agg/src/agg_vcgen_bspline.cpp
  3rdparty/agg/src/agg_vcgen_contour.cpp
  3rdparty/agg/src/agg_vcgen_dash.cpp
  3rdparty/agg/src/agg_vcgen_markers_term.cpp
  3rdparty/agg/src/agg_vcgen_smooth_poly1.cpp
  3rdparty/agg/src/agg_vcgen_stroke.cpp
  3rdparty/agg/src/agg_vpgen_clip_polygon.cpp
  3rdparty/agg/src/agg_vpgen_clip_polyline.cpp
  3rdparty/agg/src/agg_vpgen_segmentator.cpp
)

set(DEPENDENCY_AGG_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/3rdparty" "${CMAKE_CURRENT_LIST_DIR}/3rdparty/agg/include")
set(DEPENDENCY_AGG_DEFINITIONS BLEND2D_APPS_ENABLE_AGG)
set(DEPENDENCY_AGG_LIBRARIES "")

# Dependencies - Cairo (Optional)
# ===============================

if(CAIRO_FOUND)
  message("-- [blend2d-apps] adding support for Cairo")
  set(DEPENDENCY_CAIRO_LIBRARIES ${CAIRO_LIBRARIES})
  set(DEPENDENCY_CAIRO_LIBRARY_DIRS ${CAIRO_LIBRARY_DIRS})
  set(DEPENDENCY_CAIRO_INCLUDE_DIRS ${CAIRO_INCLUDE_DIRS})
  set(DEPENDENCY_CAIRO_DEFINITIONS BLEND2D_APPS_ENABLE_CAIRO)
endif()

# Dependencies - SKIA (Optional)
# ==============================

if(TARGET unofficial::skia::skia)
  message("-- [blend2d-apps] adding support for SKIA")
  set(DEPENDENCY_SKIA_LIBRARIES unofficial::skia::skia)
  set(DEPENDENCY_SKIA_DEFINITIONS BLEND2D_APPS_ENABLE_SKIA)
endif()

# Dependencies - Qt (Optional)
# ============================

if(Qt6Gui_FOUND)
  message("-- [blend2d-apps] adding support for Qt6")
  list(APPEND DEPENDENCY_QT_LIBRARIES Qt6::Gui Qt6::Widgets)
  list(APPEND DEPENDENCY_QT_DEFINITIONS BLEND2D_APPS_ENABLE_QT)
endif()

# Blend2D-Apps - Blend2D Bench
# ============================

set(BLEND2D_BENCH_SRC
  bl_bench/app.cpp
  bl_bench/app.h
  bl_bench/images_data.h
  bl_bench/jsonbuilder.cpp
  bl_bench/jsonbuilder.h
  bl_bench/module.cpp
  bl_bench/module.h
  bl_bench/module_agg.cpp
  bl_bench/module_agg.h
  bl_bench/module_blend2d.cpp
  bl_bench/module_blend2d.h
  bl_bench/module_cairo.cpp
  bl_bench/module_cairo.h
  bl_bench/module_qt.cpp
  bl_bench/module_qt.h
  bl_bench/module_skia.cpp
  bl_bench/module_skia.h
  bl_bench/shapes_data.cpp
  bl_bench/shapes_data.h
)

add_executable(bl_bench ${BLEND2D_BENCH_SRC} ${ANTIGRAIN_SRC})
target_compile_features(bl_bench PUBLIC cxx_std_11)
set_property(TARGET bl_bench PROPERTY CXX_VISIBILITY_PRESET hidden)
target_include_directories(bl_bench PRIVATE ${DEPENDENCY_AGG_INCLUDE_DIRS} ${DEPENDENCY_CAIRO_INCLUDE_DIRS})
target_compile_definitions(bl_bench PRIVATE ${DEPENDENCY_AGG_DEFINITIONS} ${DEPENDENCY_CAIRO_DEFINITIONS} ${DEPENDENCY_QT_DEFINITIONS} ${DEPENDENCY_SKIA_DEFINITIONS})
target_link_libraries(bl_bench blend2d::blend2d ${DEPENDENCY_AGG_LIBRARUES} ${DEPENDENCY_CAIRO_LIBRARIES} ${DEPENDENCY_QT_LIBRARIES} ${DEPENDENCY_SKIA_LIBRARIES})
target_link_directories(bl_bench PRIVATE ${DEPENDENCY_CAIRO_LIBRARY_DIRS})

# Blend2D-Apps - Blend2D Qt Demos
# ===============================

set(BLEND2D_QT_DEMOS
  bl_gradients_demo
  bl_elliptic_arc_demo
  bl_stroke_demo
  bl_text_demo

  bl_bounces_demo
  bl_bubbles_demo
  bl_circles_demo
  bl_particles_demo
  bl_pattern_demo
  bl_paths_demo
  bl_rects_demo
  bl_sprites_demo
  bl_tiger_demo
)

if(Qt6Gui_FOUND)
  message("-- [blend2d-apps] adding Blend2D + Qt demos")
  foreach(target ${BLEND2D_QT_DEMOS})
    add_executable(${target} WIN32
      "bl_demos/${target}.cpp"
      "bl_demos/bl_qt_canvas.cpp"
      "bl_demos/bl_qt_canvas.h"
      "bl_demos/bl_qt_headers.h")
    target_compile_features(${target} PUBLIC cxx_std_11)
    set_property(TARGET ${target} PROPERTY AUTOMOC TRUE)
    set_property(TARGET ${target} PROPERTY CXX_VISIBILITY_PRESET hidden)
    target_link_libraries(${target} blend2d::blend2d Qt6::Gui Qt6::Widgets)
  endforeach()
endif()

# Blend2D-Apps - Blend2D LiteHtml Demo
# ====================================

set(BLEND2D_LITEHTML_DEMO_SRCS
  bl_litehtml/bl_litehtml_container.h
  bl_litehtml/bl_litehtml_container.cpp
  bl_litehtml/bl_litehtml_default_page.cpp
  bl_litehtml/bl_litehtml_demo.cpp
  bl_litehtml/bl_litehtml_view.h
  bl_litehtml/bl_litehtml_view.cpp
)

if(Qt6Gui_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/litehtml/CMakeLists.txt")
  set(LITEHTML_DIR "${CMAKE_CURRENT_LIST_DIR}/3rdparty/litehtml")
  add_definitions(-DLITEHTML_UTF8)
  option(LITEHTML_BUILD_TESTING OFF)
  add_subdirectory(3rdparty/litehtml)

  message("-- [blend2d-apps] adding Blend2D + Qt + LiteHtml demo")
  add_executable(bl_litehtml_demo WIN32 ${BLEND2D_LITEHTML_DEMO_SRCS})
  target_compile_features(bl_litehtml_demo PUBLIC cxx_std_11)
  set_property(TARGET bl_litehtml_demo PROPERTY AUTOMOC TRUE)
  set_property(TARGET bl_litehtml_demo PROPERTY CXX_VISIBILITY_PRESET hidden)
  target_link_libraries(bl_litehtml_demo blend2d::blend2d Qt6::Gui Qt6::Widgets litehtml)
endif()

cmake_policy(POP)
