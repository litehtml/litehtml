cmake_minimum_required(VERSION 2.8)

project(litehtml CXX)

add_subdirectory(src/gumbo)

set(SOURCE_LITEHTML
    src/background.cpp
    src/box.cpp
    src/context.cpp
    src/css_length.cpp
    src/css_selector.cpp
    src/document.cpp
    src/el_anchor.cpp
    src/el_base.cpp
    src/el_before_after.cpp
    src/el_body.cpp
    src/el_break.cpp
    src/el_cdata.cpp
    src/el_comment.cpp
    src/el_div.cpp
    src/element.cpp
    src/el_font.cpp
    src/el_image.cpp
    src/el_link.cpp
    src/el_para.cpp
    src/el_script.cpp
    src/el_space.cpp
    src/el_style.cpp
    src/el_table.cpp
    src/el_td.cpp
    src/el_text.cpp
    src/el_title.cpp
    src/el_tr.cpp
    src/html.cpp
    src/html_tag.cpp
    src/iterators.cpp
    src/media_query.cpp
    src/style.cpp
    src/stylesheet.cpp
    src/table.cpp
    src/utf8_strings.cpp
    src/web_color.cpp
)

add_library(${PROJECT_NAME} ${SOURCE_LITEHTML})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    C_STANDARD 99
)

# Export litehtml includes.
target_include_directories(${PROJECT_NAME} PUBLIC src)
target_include_directories(${PROJECT_NAME} PUBLIC include)

# Gumbo
target_link_libraries(${PROJECT_NAME} PUBLIC gumbo)
