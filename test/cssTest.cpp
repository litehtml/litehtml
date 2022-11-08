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
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "class"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "class"));

  selector.parse(".class1.class2");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 2);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "class1"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[1].val.c_str(), "class2"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "class"));

  selector.parse("#id");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "id"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "id"));

  selector.parse("*");
  EXPECT_TRUE(!strcmp(selector.m_tag.c_str(), "*"));
  EXPECT_TRUE(selector.m_attrs.empty());

  selector.parse("element");
  EXPECT_TRUE(!strcmp(selector.m_tag.c_str(), "element"));
  EXPECT_TRUE(selector.m_attrs.empty());

  selector.parse("[attribute]");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), ""));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "attribute"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_exists);

  selector.parse("[attribute=value]");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "value"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "attribute"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_equal);

  selector.parse("[attribute~=value]");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "value"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "attribute"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_contain_str);

  selector.parse("[attribute|=value]");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "value"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "attribute"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_start_str);

  selector.parse("[attribute^=value]");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "value"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "attribute"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_start_str);

  selector.parse("[attribute$=value]");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "value"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "attribute"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_end_str);

  selector.parse("[attribute*=value]");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "value"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "attribute"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_contain_str);

  selector.parse(":active");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "active"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse("::after");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "after"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo-el"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_element);

  selector.parse("::before");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "before"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo-el"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_element);

  selector.parse(":checked");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "checked"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":default");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "default"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":disabled");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "disabled"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":empty");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "empty"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":enabled");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "enabled"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":first-child");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "first-child"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse("::first-letter");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "first-letter"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo-el"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_element);

  selector.parse("::first-line");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "first-line"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo-el"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_element);

  selector.parse(":first-of-type");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "first-of-type"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":focus");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "focus"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":hover");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "hover"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":in-range");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "in-range"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":indeterminate");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "indeterminate"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":invalid");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "invalid"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":lang(language)");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "lang(language)"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":last-child");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "last-child"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":last-of-type");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "last-of-type"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":link");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "link"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":not(selector)");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "not(selector)"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":nth-child(n)");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "nth-child(n)"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":nth-last-child(n)");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "nth-last-child(n)"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":nth-last-of-type(n)");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "nth-last-of-type(n)"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":nth-of-type(n)");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "nth-of-type(n)"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":only-of-type");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "only-of-type"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":only-child");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "only-child"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":optional");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "optional"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":out-of-range");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "out-of-range"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse("::placeholder");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "placeholder"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo-el"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_element);

  selector.parse(":read-only");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "read-only"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":read-write");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "read-write"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":required");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "required"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":root");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "root"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse("::selection");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "selection"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo-el"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_element);

  selector.parse(":target");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "target"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":valid");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "valid"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  selector.parse(":visited");
  EXPECT_TRUE(selector.m_tag.empty());
  EXPECT_TRUE(selector.m_attrs.size() == 1);
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].val.c_str(), "visited"));
  EXPECT_TRUE(!strcmp(selector.m_attrs[0].attribute.c_str(), "pseudo"));
  EXPECT_TRUE(selector.m_attrs[0].condition == select_pseudo_class);

  // other
  selector.parse("tag:psudo#anchor");
  EXPECT_TRUE(!strcmp(selector.m_tag.c_str(), "tag"));
  EXPECT_TRUE(selector.m_attrs.size() == 2);
}

TEST(CSSTest, DISABLED_SelectorParse) {
  css_selector selector(nullptr, "");
  // https://www.w3schools.com/cssref/css_selectors.asp
  assert(!selector.parse(""));
  EXPECT_TRUE(selector.parse("element"));
  EXPECT_TRUE(selector.m_combinator == combinator_descendant);
  EXPECT_TRUE(!strcmp(selector.m_right.m_tag.c_str(), "element"));
  EXPECT_TRUE(selector.m_right.m_attrs.empty());
  EXPECT_TRUE(selector.m_left == nullptr);

  // assert(selector.parse("element,element"));
  // EXPECT_TRUE(selector.m_combinator == combinator_descendant);
  // EXPECT_TRUE(selector.m_right.m_tag.c_str(), "element");
  // EXPECT_TRUE(selector.m_right.m_attrs.empty());

  EXPECT_TRUE(selector.parse(".class1 .class2"));
  EXPECT_TRUE(selector.m_combinator == combinator_descendant);
  EXPECT_TRUE(selector.m_right.m_tag.empty());
  EXPECT_TRUE(selector.m_right.m_attrs.size() == 1);
  EXPECT_TRUE(selector.m_left->m_right.m_attrs.size() == 1);

  assert(selector.parse("element element"));
  EXPECT_TRUE(selector.m_combinator == combinator_descendant);
  EXPECT_TRUE(!strcmp(selector.m_right.m_tag.c_str(), "element"));
  EXPECT_TRUE(selector.m_right.m_attrs.empty());
  EXPECT_TRUE(!strcmp(selector.m_left->m_right.m_tag.c_str(), "element"));

  assert(selector.parse("element>element"));
  EXPECT_TRUE(selector.m_combinator == combinator_child);
  EXPECT_TRUE(!strcmp(selector.m_right.m_tag.c_str(), "element"));
  EXPECT_TRUE(selector.m_right.m_attrs.empty());
  EXPECT_TRUE(!strcmp(selector.m_left->m_right.m_tag.c_str(), "element"));

  assert(selector.parse("element+element"));
  EXPECT_TRUE(selector.m_combinator == combinator_adjacent_sibling);
  EXPECT_TRUE(!strcmp(selector.m_right.m_tag.c_str(), "element"));
  EXPECT_TRUE(selector.m_right.m_attrs.empty());
  EXPECT_TRUE(!strcmp(selector.m_left->m_right.m_tag.c_str(), "element"));

  assert(selector.parse("element1~element2"));
  EXPECT_TRUE(selector.m_combinator == combinator_general_sibling);
  EXPECT_TRUE(!strcmp(selector.m_right.m_tag.c_str(), "element2"));
  EXPECT_TRUE(selector.m_right.m_attrs.empty());
  EXPECT_TRUE(!strcmp(selector.m_left->m_right.m_tag.c_str(), "element1"));
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
