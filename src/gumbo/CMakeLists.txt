cmake_minimum_required(VERSION 2.8)

project(gumbo C)

set(SOURCE_GUMBO
    attribute.c
    char_ref.c
    error.c
    parser.c
    string_buffer.c
    string_piece.c
    tag.c
    tokenizer.c
    utf8.c
    util.c
    vector.c
)

set(HEADER_GUMBO
        include/gumbo/attribute.h
        include/gumbo/char_ref.h
        include/gumbo/error.h
        include/gumbo.h
        include/gumbo/insertion_mode.h
        include/gumbo/parser.h
        include/gumbo/string_buffer.h
        include/gumbo/string_piece.h
        include/gumbo/tag_enum.h
        include/gumbo/tag_gperf.h
        include/gumbo/tag_sizes.h
        include/gumbo/tag_strings.h
        include/gumbo/token_type.h
        include/gumbo/tokenizer.h
        include/gumbo/tokenizer_states.h
        include/gumbo/utf8.h
        include/gumbo/util.h
        include/gumbo/vector.h
)

add_library(${PROJECT_NAME} ${SOURCE_GUMBO})

set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 99
    PUBLIC_HEADER "${HEADER_GUMBO}"
)

if(MSVC)
    target_include_directories(${PROJECT_NAME} PRIVATE visualc/include)
endif()

# Export gumbo includes.
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_include_directories(${PROJECT_NAME} PRIVATE include/gumbo)

# install and export
install(TARGETS ${PROJECT_NAME}
    EXPORT gumbo
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/gumbo
)
install(EXPORT gumbo FILE gumboConfig.cmake DESTINATION lib/cmake/gumbo)
