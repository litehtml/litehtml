cmake_minimum_required(VERSION 3.11)

project(gumbo C)

# Soname
# MAJOR is incremented when symbols are removed or changed in an incompatible way
# MINOR is incremented when new symbols are added
set(PROJECT_MAJOR 0)
set(PROJECT_MINOR 14.1)

set(SOURCE_GUMBO
    src/attribute.c
    src/char_ref.c
    src/char_ref_gperf.c
    src/error.c
    src/parser.c
    src/string_buffer.c
    src/string_piece.c
    src/tag.c
    src/tokenizer.c
    src/utf8.c
    src/util.c
    src/vector.c
)

set(HEADER_GUMBO
    src/gumbo.h
    src/tag_enum.h
)

set(PROJECT_LIB_VERSION ${PROJECT_MAJOR}.${PROJECT_MINOR}.0)
set(PROJECT_SO_VERSION ${PROJECT_MAJOR})

if (MSVC)
	add_compile_options(/wd4244 /wd4267)
endif()

add_library(${PROJECT_NAME} ${SOURCE_GUMBO})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_LIB_VERSION} SOVERSION ${PROJECT_SO_VERSION})

set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 99
    PUBLIC_HEADER "${HEADER_GUMBO}"
)

if (MSVC OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    target_include_directories(${PROJECT_NAME} PRIVATE visualc/include)
endif()

# Export gumbo includes.
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/src
    $<INSTALL_INTERFACE:include>)

# install and export
install(TARGETS ${PROJECT_NAME}
    EXPORT gumbo
    RUNTIME DESTINATION bin COMPONENT libraries
    ARCHIVE DESTINATION lib${LIB_SUFFIX} COMPONENT libraries
    LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT libraries
    PUBLIC_HEADER DESTINATION include/gumbo
)
install(EXPORT gumbo FILE gumboConfig.cmake DESTINATION lib${LIB_SUFFIX}/cmake/gumbo)
