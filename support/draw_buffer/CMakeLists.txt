cmake_minimum_required(VERSION 3.20)

project(litehtml_draw_buffer LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LITEHTML_DRAW_BUFFER REQUIRED IMPORTED_TARGET GLOBAL cairo)

add_library(${PROJECT_NAME}
	draw_buffer.cpp
	draw_buffer.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC litehtml PkgConfig::LITEHTML_DRAW_BUFFER)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
