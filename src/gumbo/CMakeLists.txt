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

add_library(${PROJECT_NAME} ${SOURCE_GUMBO})

set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 99
)

if(MSVC)
    target_include_directories(${PROJECT_NAME} PRIVATE visualc/include)
endif()

# Export gumbo includes.
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
