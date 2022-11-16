#include <gtest/gtest.h>

#include <assert.h>
#include "litehtml.h"
#include "test/container_test.h"
using namespace litehtml;

TEST(CSSTest, Parse) {
  container_test container;
  litehtml::document::ptr doc = std::make_shared<litehtml::document>(&container);
  css c;
  c.parse_stylesheet("/*Comment*/", nullptr, doc, nullptr);
  c.parse_stylesheet("html { display: none }", nullptr, doc, nullptr);
  // https://www.w3schools.com/cssref/pr_import_rule.asp
  c.parse_stylesheet("@import \"navigation.css\"; /* Using a string */", nullptr, doc, nullptr);
  c.parse_stylesheet("@import url(\"navigation.css\"); /* Using a url */", nullptr, doc, nullptr);
  c.parse_stylesheet("@import \"navigation.css\"", nullptr, doc, nullptr);
  c.parse_stylesheet("@import \"printstyle.css\" print;", nullptr, doc, nullptr);
  c.parse_stylesheet("@import \"mobstyle.css\" screen and (max-width: 768px);", nullptr, doc, nullptr);
  // https://www.w3schools.com/cssref/css3_pr_mediaquery.asp
  c.parse_stylesheet("@media only screen and (max-width: 600px) { body { background-color: lightblue; } }", nullptr, doc, nullptr);
}

TEST(CSSTest, Url) {
  string url;

  css::parse_css_url("", url);
  EXPECT_TRUE(url.empty());

  css::parse_css_url("value", url);
  EXPECT_TRUE(url.empty());

  css::parse_css_url("url()", url);
  EXPECT_TRUE(url.empty());

  css::parse_css_url("url(value)", url);
  EXPECT_TRUE(!strcmp(url.c_str(), "value"));

  css::parse_css_url("url('value')", url);
  EXPECT_TRUE(!strcmp(url.c_str(), "value"));

  css::parse_css_url("url(\"value\")", url);
  EXPECT_TRUE(!strcmp(url.c_str(), "value"));
}

TEST(CSSTest, LengthParse) {
  css_length length;

  length.fromString("calc(todo)");
  assert(length.is_predefined() == true);
  assert(length.predef() == 0);
  assert(length.val() == 0);
  assert(length.units() == css_units_none);

  length.fromString("top", "top;bottom", -1);
  assert(length.is_predefined() == true);
  assert(length.predef() == 0);
  assert(length.val() == 0);
  assert(length.units() == css_units_none);

  length.fromString("bottom", "top;bottom", -1);
  assert(length.is_predefined() == true);
  assert(length.predef() == 1);
  assert(length.val() == 0);
  assert(length.units() == css_units_none);

  length.fromString("bad", "top;bottom", -1);
  assert(length.is_predefined() == true);
  assert(length.predef() == -1);
  assert(length.val() == 0);
  assert(length.units() == css_units_none);

  length.fromString("123", "top;bottom", -1);
  assert(length.is_predefined() == false);
  assert(length.predef() == 0);
  assert(length.val() == 123);
  assert(length.units() == css_units_none);

  length.fromString("123px", "top;bottom", -1);
  assert(length.is_predefined() == false);
  assert(length.predef() == 0);
  assert(length.val() == 123);
  assert(length.units() == css_units_px);
}

TEST(CSSTest, ElementSelectorParse) {
  css_element_selector selector;
  // https://www.w3schools.com/cssref/css_selectors.asp
  selector.parse(".class");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(selector.m_attrs[0].type == select_class);
  EXPECT_TRUE(selector.m_attrs[0].name == _id("class"));

  selector.parse(".class1.class2");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.size() == 2);
  EXPECT_TRUE(selector.m_attrs[0].type == select_class);
  EXPECT_TRUE(selector.m_attrs[0].name == _id("class1"));
  EXPECT_TRUE(selector.m_attrs[1].type == select_class);
  EXPECT_TRUE(selector.m_attrs[1].name == _id("class2"));

  selector.parse("#id");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(selector.m_attrs[0].type == select_id);
  EXPECT_TRUE(selector.m_attrs[0].name == _id("id"));

  selector.parse("*");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.empty());

  selector.parse("element");
  EXPECT_TRUE(selector.m_tag == _id("element"));
  EXPECT_TRUE(selector.m_attrs.empty());

  selector.parse("[attribute]");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(selector.m_attrs[0].type == select_exists);
  EXPECT_TRUE(selector.m_attrs[0].name == _id("attribute"));

  selector.parse("[attribute=value]");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(selector.m_attrs[0].type == select_equal);
  EXPECT_TRUE(selector.m_attrs[0].name == _id("attribute"));
  EXPECT_TRUE(selector.m_attrs[0].val == "value");

  selector.parse("[attribute~=value]");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(selector.m_attrs[0].type == select_contain_str);
  EXPECT_TRUE(selector.m_attrs[0].name == _id("attribute"));
  EXPECT_TRUE(selector.m_attrs[0].val == "value");

  selector.parse("::after");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(selector.m_attrs[0].type == select_pseudo_element);
  EXPECT_TRUE(selector.m_attrs[0].name == _after_);

  selector.parse(":after");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(selector.m_attrs[0].type == select_pseudo_element);
  EXPECT_TRUE(selector.m_attrs[0].name == _after_);

  selector.parse(":active");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(selector.m_attrs[0].type == select_pseudo_class);
  EXPECT_TRUE(selector.m_attrs[0].name == _active_);

  selector.parse(":lang(language)");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(selector.m_attrs[0].type == select_pseudo_class);
  EXPECT_TRUE(selector.m_attrs[0].name == _lang_);
  EXPECT_TRUE(selector.m_attrs[0].val == "language");

  selector.parse(":not(div)");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(selector.m_attrs[0].type == select_pseudo_class);
  EXPECT_TRUE(selector.m_attrs[0].name == _not_);
  EXPECT_TRUE(selector.m_attrs[0].sel->m_tag == _div_);

  selector.parse(":nth-child(2n+3)");
  EXPECT_TRUE(selector.m_tag == star_id);
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(selector.m_attrs[0].type == select_pseudo_class);
  EXPECT_TRUE(selector.m_attrs[0].name == _nth_child_);
  EXPECT_TRUE(selector.m_attrs[0].a == 2);
  EXPECT_TRUE(selector.m_attrs[0].b == 3);

  // other
  selector.parse("tag:psudo#anchor");
  EXPECT_TRUE(selector.m_tag == _id("tag"));
  EXPECT_TRUE(selector.m_attrs.size() == 2);
}

TEST(CSSTest, StyleAdd) {
  style style;
  style.add("border: 5px solid red; background-image: value", "base", nullptr);
  style.add("border: 5px solid red!important; background-image: value", "base", nullptr);
}

TEST(CSSTest, StyleAddProperty) {
  style style;
  style.add_property(_background_image_, "value", "base", false, nullptr);
  style.add_property(_border_spacing_, "1", nullptr, false, nullptr);
  style.add_property(_border_spacing_, "1 2", nullptr, false, nullptr);
  style.add_property(_border_, "5px solid red", nullptr, false, nullptr);
  style.add_property(_border_left_, "5px solid red", nullptr, false, nullptr);
  style.add_property(_border_right_, "5px solid red", nullptr, false, nullptr);
  style.add_property(_border_top_, "5px solid red", nullptr, false, nullptr);
  style.add_property(_border_bottom_, "5px solid red", nullptr, false, nullptr);
  style.add_property(_border_bottom_left_radius_, "1", nullptr, false, nullptr);
  style.add_property(_border_bottom_left_radius_, "1 2", nullptr, false, nullptr);
  style.add_property(_border_bottom_right_radius_, "1", nullptr, false, nullptr);
  style.add_property(_border_bottom_right_radius_, "1 2", nullptr, false, nullptr);
  style.add_property(_border_top_right_radius_, "1", nullptr, false, nullptr);
  style.add_property(_border_top_right_radius_, "1 2", nullptr, false, nullptr);
  style.add_property(_border_top_left_radius_, "1", nullptr, false, nullptr);
  style.add_property(_border_top_left_radius_, "1 2", nullptr, false, nullptr);
  style.add_property(_border_radius_, "1", nullptr, false, nullptr);
  style.add_property(_border_radius_, "1 2", nullptr, false, nullptr);
  style.add_property(_border_radius_x_, "1", nullptr, false, nullptr);
  style.add_property(_border_radius_x_, "1 2", nullptr, false, nullptr);
  style.add_property(_border_radius_x_, "1 2 3", nullptr, false, nullptr);
  style.add_property(_border_radius_x_, "1 2 3 4", nullptr, false, nullptr);
  style.add_property(_border_radius_y_, "1", nullptr, false, nullptr);
  style.add_property(_border_radius_y_, "1 2", nullptr, false, nullptr);
  style.add_property(_border_radius_y_, "1 2 3", nullptr, false, nullptr);
  style.add_property(_border_radius_y_, "1 2 3 4", nullptr, false, nullptr);
  style.add_property(_list_style_image_, "value", "base", false, nullptr);
  style.add_property(_background_, "url(value)", "base", false, nullptr);
  style.add_property(_background_, "repeat", nullptr, false, nullptr);
  style.add_property(_background_, "fixed", nullptr, false, nullptr);
  style.add_property(_background_, "border-box", nullptr, false, nullptr);
  style.add_property(_background_, "border-box border-box", nullptr, false, nullptr);
  style.add_property(_background_, "left", nullptr, false, nullptr);
  style.add_property(_background_, "1", nullptr, false, nullptr);
  style.add_property(_background_, "-1", nullptr, false, nullptr);
  style.add_property(_background_, "-1", nullptr, false, nullptr);
  style.add_property(_background_, "+1", nullptr, false, nullptr);
  style.add_property(_background_, "left 1", nullptr, false, nullptr);
  style.add_property(_background_, "red", nullptr, false, nullptr);
  style.add_property(_margin_, "1", nullptr, false, nullptr);
  style.add_property(_margin_, "1 2", nullptr, false, nullptr);
  style.add_property(_margin_, "1 2 3", nullptr, false, nullptr);
  style.add_property(_margin_, "1 2 3 4", nullptr, false, nullptr);
  style.add_property(_padding_, "1", nullptr, false, nullptr);
  style.add_property(_padding_, "1 2", nullptr, false, nullptr);
  style.add_property(_padding_, "1 2 3", nullptr, false, nullptr);
  style.add_property(_padding_, "1 2 3 4", nullptr, false, nullptr);
  style.add_property(_border_left_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_left_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_left_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_left_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_right_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_right_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_right_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_right_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_top_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_top_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_top_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_top_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_bottom_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_bottom_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_bottom_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_bottom_, "TBD", nullptr, false, nullptr);
  style.add_property(_border_width_, "1", nullptr, false, nullptr);
  style.add_property(_border_width_, "1 2", nullptr, false, nullptr);
  style.add_property(_border_width_, "1 2 3", nullptr, false, nullptr);
  style.add_property(_border_width_, "1 2 3 4", nullptr, false, nullptr);
  style.add_property(_border_style_, "1", nullptr, false, nullptr);
  style.add_property(_border_style_, "1 2", nullptr, false, nullptr);
  style.add_property(_border_style_, "1 2 3", nullptr, false, nullptr);
  style.add_property(_border_style_, "1 2 3 4", nullptr, false, nullptr);
  style.add_property(_border_color_, "1", nullptr, false, nullptr);
  style.add_property(_border_color_, "1 2", nullptr, false, nullptr);
  style.add_property(_border_color_, "1 2 3", nullptr, false, nullptr);
  style.add_property(_border_color_, "1 2 3 4", nullptr, false, nullptr);
  style.add_property(_font_, "TBD", nullptr, false, nullptr);
  style.add_property(_font_, "TBD", nullptr, false, nullptr);
  style.add_property(_font_, "TBD", nullptr, false, nullptr);
  style.add_property(_font_, "TBD", nullptr, false, nullptr);
  style.add_property(_id("unknown"), "value", nullptr, false, nullptr);
}
