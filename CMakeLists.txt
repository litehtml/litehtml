cmake_minimum_required(VERSION 2.8)

project(litehtml CXX)
include(CTest)
enable_testing()

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

set(HEADER_LITEHTML
    include/litehtml.h
    src/attributes.h
    src/background.h
    src/borders.h
    src/box.h
    src/context.h
    src/css_length.h
    src/css_margins.h
    src/css_offsets.h
    src/css_position.h
    src/css_selector.h
    src/document.h
    src/el_anchor.h
    src/el_base.h
    src/el_before_after.h
    src/el_body.h
    src/el_break.h
    src/el_cdata.h
    src/el_comment.h
    src/el_div.h
    src/el_font.h
    src/el_image.h
    src/el_link.h
    src/el_para.h
    src/el_script.h
    src/el_space.h
    src/el_style.h
    src/el_table.h
    src/el_td.h
    src/el_text.h
    src/el_title.h
    src/el_tr.h
    src/element.h
    src/html.h
    src/html_tag.h
    src/iterators.h
    src/media_query.h
    src/os_types.h
    src/style.h
    src/stylesheet.h
    src/table.h
    src/types.h
    src/utf8_strings.h
    src/web_color.h
)

set(TEST_LITEHTML
    containers/test/container_test.cpp
    test/contextTest.cpp
    test/cssTest.cpp
    test/documentTest.cpp
    test/layoutGlobalTest.cpp
    test/mediaQueryTest.cpp
    test/webColorTest.cpp
    test/program.cpp
)

add_library(${PROJECT_NAME} ${SOURCE_LITEHTML})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    C_STANDARD 99
    PUBLIC_HEADER "${HEADER_LITEHTML}"
)

# Export litehtml includes.
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

option(LITEHTML_UTF8 "Build litehtml with UTF-8 text conversion functions." OFF)
if (LITEHTML_UTF8)
  target_compile_definitions(${PROJECT_NAME} PUBLIC LITEHTML_UTF8)
endif()

# Gumbo
target_link_libraries(${PROJECT_NAME} PUBLIC gumbo)

# install and export
install(TARGETS ${PROJECT_NAME}
    EXPORT litehtmlTargets
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include/litehtml
)
install(FILES cmake/litehtmlConfig.cmake DESTINATION lib/cmake/litehtml)
install(EXPORT litehtmlTargets FILE litehtmlTargets.cmake DESTINATION lib/cmake/litehtml)

# Binary Master.css
if (WIN32)
    find_program(XXD_COMMAND xxd.exe ${CMAKE_CURRENT_SOURCE_DIR}/tool)
    file(TO_NATIVE_PATH ${XXD_COMMAND} XXD_COMMAND)
    file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include/master.css MASTER_FILE)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/master.css.inc
        COMMAND type ${MASTER_FILE} | "${XXD_COMMAND}" -i > ${CMAKE_CURRENT_SOURCE_DIR}/src/master.css.inc)
else()
    find_program(XXD_COMMAND xxd)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/master.css.inc
        COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/include/master.css | xxd -i > ${CMAKE_CURRENT_SOURCE_DIR}/src/master.css.inc)
endif()
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/master.css.inc PROPERTIES GENERATED TRUE)

# Tests
if (BUILD_TESTING)
    set(TEST_NAME ${PROJECT_NAME}_tests)
    add_executable(${TEST_NAME} ${TEST_LITEHTML} ${CMAKE_CURRENT_SOURCE_DIR}/src/master.css.inc)
    set_target_properties(${TEST_NAME} PROPERTIES
        CXX_STANDARD 11
        C_STANDARD 99
        PUBLIC_HEADER "${HEADER_LITEHTML}"
    )
    target_include_directories(${TEST_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/containers)
    target_link_libraries(${TEST_NAME} PRIVATE ${PROJECT_NAME})
    # tests
    add_test(NAME contextTest COMMAND ${TEST_NAME} 1)
    add_test(NAME cssTest COMMAND ${TEST_NAME} 2)
    add_test(NAME documentTest COMMAND ${TEST_NAME} 3)
    add_test(NAME layoutGlobalTest COMMAND ${TEST_NAME} 4)
    add_test(NAME mediaQueryTest COMMAND ${TEST_NAME} 5)
    add_test(NAME webColorTest COMMAND ${TEST_NAME} 6)
endif()
