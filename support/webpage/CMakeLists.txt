cmake_minimum_required(VERSION 3.20)

project(litehtml_webpage LANGUAGES CXX)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(LITEHTML_WEBPAGE REQUIRED IMPORTED_TARGET GLOBAL libcurl)

add_library(${PROJECT_NAME}
	web_page.cpp
	web_history.cpp
	http_request.cpp
	http_requests_pool.cpp
	web_page.h
	web_history.h
	http_request.h
	http_requests_pool.h
	html_host.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC litehtml_container_pango PkgConfig::LITEHTML_WEBPAGE Threads::Threads)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
