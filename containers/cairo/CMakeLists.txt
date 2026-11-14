cmake_minimum_required(VERSION 3.20)

project(litehtml_container_cairo LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LITEHTML_CAIRO REQUIRED IMPORTED_TARGET GLOBAL cairo)

add_library(${PROJECT_NAME}
	container_cairo.cpp
	cairo_borders.cpp
	conic_gradient.cpp
	container_cairo.h
	cairo_borders.h
	cairo_images_cache.h
	conic_gradient.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC litehtml PkgConfig::LITEHTML_CAIRO)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
