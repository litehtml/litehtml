#include <gtest/gtest.h>

#include <assert.h>
#include "litehtml.h"
using namespace litehtml;

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
