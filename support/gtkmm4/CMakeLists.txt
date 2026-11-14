cmake_minimum_required(VERSION 3.20)

project(litehtml_gtkmm4 LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LITEHTML_GTKMM4 REQUIRED IMPORTED_TARGET GLOBAL gtkmm-4.0)

add_library(${PROJECT_NAME}
	html_widget.cpp
	html_widget.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC litehtml_webpage litehtml_draw_buffer PkgConfig::LITEHTML_GTKMM4)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
