#include <assert.h>
#include "litehtml.h"
#include "test/container_test.h"
using namespace litehtml;

static void CssParseTest() {
  container_test container;
  litehtml::document::ptr doc = std::make_shared<litehtml::document>(&container, nullptr);
  media_query_list::ptr media = media_query_list::ptr();
  css c;
  c.parse_stylesheet(_t("/*Comment*/"), nullptr, doc, nullptr);
  c.parse_stylesheet(_t("html { display: none }"), nullptr, doc, nullptr);
  // https://www.w3schools.com/cssref/pr_import_rule.asp
  c.parse_stylesheet(_t("@import \"navigation.css\"; /* Using a string */"), nullptr, doc, nullptr);
  c.parse_stylesheet(_t("@import url(\"navigation.css\"); /* Using a url */"), nullptr, doc, nullptr);
  c.parse_stylesheet(_t("@import \"navigation.css\""), nullptr, doc, nullptr);
  c.parse_stylesheet(_t("@import \"printstyle.css\" print;"), nullptr, doc, nullptr);
  c.parse_stylesheet(_t("@import \"mobstyle.css\" screen and (max-width: 768px);"), nullptr, doc, nullptr);
  // https://www.w3schools.com/cssref/css3_pr_mediaquery.asp
  c.parse_stylesheet(_t("@media only screen and (max-width: 600px) { body { background-color: lightblue; } }"), nullptr, doc, nullptr);
}

static void CssParseUrlTest() {
  tstring url;
  css::parse_css_url(_t(""), url), assert(url.empty());
  css::parse_css_url(_t("value"), url), assert(url.empty());
  css::parse_css_url(_t("url()"), url), assert(url.empty());
  css::parse_css_url(_t("url(value)"), url), assert(!t_strcmp(url.c_str(), _t("value")));
  css::parse_css_url(_t("url('value')"), url), assert(!t_strcmp(url.c_str(), _t("value")));
  css::parse_css_url(_t("url(\"value\")"), url), assert(!t_strcmp(url.c_str(), _t("value")));
}

static void CssLengthParseTest() {
  css_length length;
  length.fromString(_t("calc(todo)")), assert(length.is_predefined() == true), assert(length.predef() == 0), assert(length.val() == 0), assert(length.units() == css_units_none);
  length.fromString(_t("top"), _t("top;bottom"), -1), assert(length.is_predefined() == true), assert(length.predef() == 0), assert(length.val() == 0), assert(length.units() == css_units_none);
  length.fromString(_t("bottom"), _t("top;bottom"), -1), assert(length.is_predefined() == true), assert(length.predef() == 1), assert(length.val() == 0), assert(length.units() == css_units_none);
  length.fromString(_t("bad"), _t("top;bottom"), -1), assert(length.is_predefined() == true), assert(length.predef() == -1), assert(length.val() == 0), assert(length.units() == css_units_none);
  length.fromString(_t("123"), _t("top;bottom"), -1), assert(length.is_predefined() == false), assert(length.predef() == 0), assert(length.val() == 123), assert(length.units() == css_units_none);
  length.fromString(_t("123px"), _t("top;bottom"), -1), assert(length.is_predefined() == false), assert(length.predef() == 0), assert(length.val() == 123), assert(length.units() == css_units_px);
}

static void CssElementSelectorParseTest() {
  css_element_selector selector;
  // https://www.w3schools.com/cssref/css_selectors.asp
  selector.parse(_t(".class")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("class"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("class")));
  selector.parse(_t(".class1.class2")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 2), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("class1"))), assert(!t_strcmp(selector.m_attrs[1].val.c_str(), _t("class2"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("class")));
  selector.parse(_t("#id")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("id"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("id")));
  selector.parse(_t("*")), assert(!t_strcmp(selector.m_tag.c_str(), _t("*"))), assert(selector.m_attrs.empty());
  selector.parse(_t("element")), assert(!t_strcmp(selector.m_tag.c_str(), _t("element"))), assert(selector.m_attrs.empty());
  selector.parse(_t("[attribute]")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t(""))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("attribute"))), assert(selector.m_attrs[0].condition == select_exists);
  selector.parse(_t("[attribute=value]")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("value"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("attribute"))), assert(selector.m_attrs[0].condition == select_equal);
  selector.parse(_t("[attribute~=value]")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("value"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("attribute"))), assert(selector.m_attrs[0].condition == select_contain_str);
  selector.parse(_t("[attribute|=value]")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("value"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("attribute"))), assert(selector.m_attrs[0].condition == select_start_str);
  selector.parse(_t("[attribute^=value]")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("value"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("attribute"))), assert(selector.m_attrs[0].condition == select_start_str);
  selector.parse(_t("[attribute$=value]")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("value"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("attribute"))), assert(selector.m_attrs[0].condition == select_end_str);
  selector.parse(_t("[attribute*=value]")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("value"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("attribute"))), assert(selector.m_attrs[0].condition == select_contain_str);
  selector.parse(_t(":active")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("active"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t("::after")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("after"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo-el"))), assert(selector.m_attrs[0].condition == select_pseudo_element);
  selector.parse(_t("::before")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("before"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo-el"))), assert(selector.m_attrs[0].condition == select_pseudo_element);
  selector.parse(_t(":checked")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("checked"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":default")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("default"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":disabled")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("disabled"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":empty")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("empty"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":enabled")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("enabled"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":first-child")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("first-child"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t("::first-letter")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("first-letter"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo-el"))), assert(selector.m_attrs[0].condition == select_pseudo_element);
  selector.parse(_t("::first-line")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("first-line"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo-el"))), assert(selector.m_attrs[0].condition == select_pseudo_element);
  selector.parse(_t(":first-of-type")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("first-of-type"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":focus")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("focus"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":hover")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("hover"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":in-range")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("in-range"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":indeterminate")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("indeterminate"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":invalid")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("invalid"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":lang(language)")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("lang(language)"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":last-child")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("last-child"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":last-of-type")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("last-of-type"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":link")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("link"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":not(selector)")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("not(selector)"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":nth-child(n)")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("nth-child(n)"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":nth-last-child(n)")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("nth-last-child(n)"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":nth-last-of-type(n)")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("nth-last-of-type(n)"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":nth-of-type(n)")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("nth-of-type(n)"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":only-of-type")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("only-of-type"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":only-child")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("only-child"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":optional")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("optional"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":out-of-range")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("out-of-range"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t("::placeholder")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("placeholder"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo-el"))), assert(selector.m_attrs[0].condition == select_pseudo_element);
  selector.parse(_t(":read-only")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("read-only"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":read-write")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("read-write"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":required")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("required"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":root")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("root"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t("::selection")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("selection"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo-el"))), assert(selector.m_attrs[0].condition == select_pseudo_element);
  selector.parse(_t(":target")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("target"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":valid")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("valid"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  selector.parse(_t(":visited")), assert(selector.m_tag.empty()), assert(selector.m_attrs.size() == 1), assert(!t_strcmp(selector.m_attrs[0].val.c_str(), _t("visited"))), assert(!t_strcmp(selector.m_attrs[0].attribute.c_str(), _t("pseudo"))), assert(selector.m_attrs[0].condition == select_pseudo_class);
  // other
  selector.parse(_t("tag:psudo#anchor")), assert(!t_strcmp(selector.m_tag.c_str(), _t("tag"))), assert(selector.m_attrs.size() == 2);
}

static void CssSelectorParseTest() {
  css_selector selector(nullptr);
  // https://www.w3schools.com/cssref/css_selectors.asp
  assert(!selector.parse(_t(""))), assert(selector.parse(_t("element"))), assert(selector.m_combinator == combinator_descendant), assert(!t_strcmp(selector.m_right.m_tag.c_str(), _t("element"))), assert(selector.m_right.m_attrs.empty()), assert(selector.m_left == nullptr);
  // assert(selector.parse(_t("element,element"))), assert(selector.m_combinator == combinator_descendant), assert(selector.m_right.m_tag.c_str(), _t("element")), assert(selector.m_right.m_attrs.empty());
  assert(selector.parse(_t(".class1 .class2"))), assert(selector.m_combinator == combinator_descendant), assert(selector.m_right.m_tag.empty()), assert(selector.m_right.m_attrs.size() == 1), assert(selector.m_left->m_right.m_attrs.size() == 1);
  assert(selector.parse(_t("element element"))), assert(selector.m_combinator == combinator_descendant), assert(!t_strcmp(selector.m_right.m_tag.c_str(), _t("element"))), assert(selector.m_right.m_attrs.empty()), assert(!t_strcmp(selector.m_left->m_right.m_tag.c_str(), _t("element")));
  assert(selector.parse(_t("element>element"))), assert(selector.m_combinator == combinator_child), assert(!t_strcmp(selector.m_right.m_tag.c_str(), _t("element"))), assert(selector.m_right.m_attrs.empty()), assert(!t_strcmp(selector.m_left->m_right.m_tag.c_str(), _t("element")));
  assert(selector.parse(_t("element+element"))), assert(selector.m_combinator == combinator_adjacent_sibling), assert(!t_strcmp(selector.m_right.m_tag.c_str(), _t("element"))), assert(selector.m_right.m_attrs.empty()), assert(!t_strcmp(selector.m_left->m_right.m_tag.c_str(), _t("element")));
  assert(selector.parse(_t("element1~element2"))), assert(selector.m_combinator == combinator_general_sibling), assert(!t_strcmp(selector.m_right.m_tag.c_str(), _t("element2"))), assert(selector.m_right.m_attrs.empty()), assert(!t_strcmp(selector.m_left->m_right.m_tag.c_str(), _t("element1")));
}

static void StyleAddTest() {
  style style;
  style.add(_t("border: 5px solid red; background-image: value"), _t("base"));
  style.add(_t("border: 5px solid red!important; background-image: value"), _t("base"));
}

static void StyleAddPropertyTest() {
  style style;
  style.add_property(_t("background-image"), _t("value"), _t("base"), false);
  style.add_property(_t("border-spacing"), _t("1"), nullptr, false);
  style.add_property(_t("border-spacing"), _t("1 2"), nullptr, false);
  style.add_property(_t("border"), _t("5px solid red"), nullptr, false);
  style.add_property(_t("border-left"), _t("5px solid red"), nullptr, false);
  style.add_property(_t("border-right"), _t("5px solid red"), nullptr, false);
  style.add_property(_t("border-top"), _t("5px solid red"), nullptr, false);
  style.add_property(_t("border-bottom"), _t("5px solid red"), nullptr, false);
  style.add_property(_t("border-bottom-left-radius"), _t("1"), nullptr, false);
  style.add_property(_t("border-bottom-left-radius"), _t("1 2"), nullptr, false);
  style.add_property(_t("border-bottom-right-radius"), _t("1"), nullptr, false);
  style.add_property(_t("border-bottom-right-radius"), _t("1 2"), nullptr, false);
  style.add_property(_t("border-top-right-radius"), _t("1"), nullptr, false);
  style.add_property(_t("border-top-right-radius"), _t("1 2"), nullptr, false);
  style.add_property(_t("border-top-left-radius"), _t("1"), nullptr, false);
  style.add_property(_t("border-top-left-radius"), _t("1 2"), nullptr, false);
  style.add_property(_t("border-radius"), _t("1"), nullptr, false);
  style.add_property(_t("border-radius"), _t("1 2"), nullptr, false);
  style.add_property(_t("border-radius-x"), _t("1"), nullptr, false);
  style.add_property(_t("border-radius-x"), _t("1 2"), nullptr, false);
  style.add_property(_t("border-radius-x"), _t("1 2 3"), nullptr, false);
  style.add_property(_t("border-radius-x"), _t("1 2 3 4"), nullptr, false);
  style.add_property(_t("border-radius-y"), _t("1"), nullptr, false);
  style.add_property(_t("border-radius-y"), _t("1 2"), nullptr, false);
  style.add_property(_t("border-radius-y"), _t("1 2 3"), nullptr, false);
  style.add_property(_t("border-radius-y"), _t("1 2 3 4"), nullptr, false);
  style.add_property(_t("list-style-image"), _t("value"), _t("base"), false);
  style.add_property(_t("background"), _t("url(value)"), _t("base"), false);
  style.add_property(_t("background"), _t("repeat"), nullptr, false);
  style.add_property(_t("background"), _t("fixed"), nullptr, false);
  style.add_property(_t("background"), _t("border-box"), nullptr, false);
  style.add_property(_t("background"), _t("border-box border-box"), nullptr, false);
  style.add_property(_t("background"), _t("left"), nullptr, false);
  style.add_property(_t("background"), _t("1"), nullptr, false);
  style.add_property(_t("background"), _t("-1"), nullptr, false);
  style.add_property(_t("background"), _t("-1"), nullptr, false);
  style.add_property(_t("background"), _t("+1"), nullptr, false);
  style.add_property(_t("background"), _t("left 1"), nullptr, false);
  style.add_property(_t("background"), _t("red"), nullptr, false);
  style.add_property(_t("margin"), _t("1"), nullptr, false);
  style.add_property(_t("margin"), _t("1 2"), nullptr, false);
  style.add_property(_t("margin"), _t("1 2 3"), nullptr, false);
  style.add_property(_t("margin"), _t("1 2 3 4"), nullptr, false);
  style.add_property(_t("padding"), _t("1"), nullptr, false);
  style.add_property(_t("padding"), _t("1 2"), nullptr, false);
  style.add_property(_t("padding"), _t("1 2 3"), nullptr, false);
  style.add_property(_t("padding"), _t("1 2 3 4"), nullptr, false);
  style.add_property(_t("border-left"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-left"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-left"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-left"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-right"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-right"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-right"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-right"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-top"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-top"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-top"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-top"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-bottom"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-bottom"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-bottom"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-bottom"), _t("TBD"), nullptr, false);
  style.add_property(_t("border-width"), _t("1"), nullptr, false);
  style.add_property(_t("border-width"), _t("1 2"), nullptr, false);
  style.add_property(_t("border-width"), _t("1 2 3"), nullptr, false);
  style.add_property(_t("border-width"), _t("1 2 3 4"), nullptr, false);
  style.add_property(_t("border-style"), _t("1"), nullptr, false);
  style.add_property(_t("border-style"), _t("1 2"), nullptr, false);
  style.add_property(_t("border-style"), _t("1 2 3"), nullptr, false);
  style.add_property(_t("border-style"), _t("1 2 3 4"), nullptr, false);
  style.add_property(_t("border-color"), _t("1"), nullptr, false);
  style.add_property(_t("border-color"), _t("1 2"), nullptr, false);
  style.add_property(_t("border-color"), _t("1 2 3"), nullptr, false);
  style.add_property(_t("border-color"), _t("1 2 3 4"), nullptr, false);
  style.add_property(_t("font"), _t("TBD"), nullptr, false);
  style.add_property(_t("font"), _t("TBD"), nullptr, false);
  style.add_property(_t("font"), _t("TBD"), nullptr, false);
  style.add_property(_t("font"), _t("TBD"), nullptr, false);
  style.add_property(_t("unknown"), _t("value"), nullptr, false);
}

void cssTest() {
  CssParseTest();
  CssParseUrlTest();
  CssLengthParseTest();
  CssElementSelectorParseTest();
  CssSelectorParseTest();
  StyleAddTest();
  StyleAddPropertyTest();
}