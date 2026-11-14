cmake_minimum_required(VERSION 3.20)

project(litehtml_render2png LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LITEHTML_RENDER2PNG REQUIRED IMPORTED_TARGET GLOBAL gdk-3.0 gdk-pixbuf-2.0 fontconfig)

add_library(${PROJECT_NAME}
	render2png.cpp
	render2png.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC litehtml_container_pango PkgConfig::LITEHTML_RENDER2PNG)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
