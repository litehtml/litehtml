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
    attribute.h
    char_ref.h
    error.h
    gumbo.h
    insertion_mode.h
    parser.h
    string_buffer.h
    string_piece.h
    tag_enum.h
    tag_gperf.h
    tag_sizes.h
    tag_strings.h
    token_type.h
    tokenizer.h
    tokenizer_states.h
    utf8.h
    util.h
    vector.h
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
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# install and export
install(TARGETS ${PROJECT_NAME}
    EXPORT gumbo
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/gumbo
)
install(EXPORT gumbo FILE gumboConfig.cmake DESTINATION lib/cmake/gumbo)
