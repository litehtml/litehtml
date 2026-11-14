cmake_minimum_required(VERSION 3.20)

project(litehtml_container_pango LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LITEHTML_PANGO REQUIRED IMPORTED_TARGET GLOBAL pango pangocairo)

add_library(${PROJECT_NAME}
	container_cairo_pango.cpp
	container_cairo_pango.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC litehtml_container_cairo PkgConfig::LITEHTML_PANGO)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
