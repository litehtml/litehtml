#include <gtest/gtest.h>

#include "litehtml.h"
#include "test/container_test.h"

using namespace litehtml;

TEST(DocumentTest, AddFont) {
  container_test container;
  litehtml::document::ptr doc = std::make_shared<litehtml::document>(&container);
  font_metrics fm;
  doc->get_font(nullptr, 0, "normal", "normal", "", &fm);
  doc->get_font("inherit", 0, "normal", "normal", "", &fm);
  doc->get_font("Arial", 0, "bold", "normal", "underline", &fm);
  doc->get_font("Arial", 0, "bold", "normal", "line-through", &fm);
  doc->get_font("Arial", 0, "bold", "normal", "overline", &fm);
}

TEST(DocumentTest, Render) {
  container_test container;
  litehtml::document::ptr doc = document::createFromString("<html>Body</html>", &container);
  doc->render(100, render_fixed_only);
  doc->render(100, render_no_fixed);
  doc->render(100, render_all);
}

TEST(DocumentTest, Draw) {
  container_test container;
  litehtml::document::ptr doc = document::createFromString("<html>Body</html>", &container);
  position pos(0, 0, 100, 100);
  doc->draw((uint_ptr)0, 0, 0, &pos);
}

TEST(DocumentTest, CvtUnits) {
  container_test container;
  litehtml::document::ptr doc = std::make_shared<litehtml::document>(&container);
  bool is_percent;
  doc->to_pixels("", 10, &is_percent);
  css_length c;
  c.fromString("10%"), doc->cvt_units(c, 10, 100);
  c.fromString("10em"), doc->cvt_units(c, 10, 100);
  c.fromString("10pt"), doc->cvt_units(c, 10, 100);
  c.fromString("10in"), doc->cvt_units(c, 10, 100);
  c.fromString("10cm"), doc->cvt_units(c, 10, 100);
  c.fromString("10mm"), doc->cvt_units(c, 10, 100);
  c.fromString("10vm"), doc->cvt_units(c, 10, 100);
  c.fromString("10vh"), doc->cvt_units(c, 10, 100);
  c.fromString("10vmin"), doc->cvt_units(c, 10, 100);
  c.fromString("10vmax"), doc->cvt_units(c, 10, 100);
  c.fromString("10"), doc->cvt_units(c, 10, 100);
}

TEST(DocumentTest, MouseEvents) {
  container_test container;
  litehtml::document::ptr doc = std::make_shared<litehtml::document>(&container);
  position::vector redraw_boxes;
  doc->on_mouse_over(0, 0, 0, 0, redraw_boxes);
  doc->on_lbutton_down(0, 0, 0, 0, redraw_boxes);
  doc->on_lbutton_up(0, 0, 0, 0, redraw_boxes);
  doc->on_mouse_leave(redraw_boxes);
}

TEST(DocumentTest, CreateElement) {
  container_test container;
  litehtml::document::ptr doc = std::make_shared<litehtml::document>(&container);
  string_map map;
  doc->create_element("container", map);
  doc->create_element("br", map);
  doc->create_element("p", map);
  doc->create_element("img", map);
  doc->create_element("table", map);
  doc->create_element("td", map);
  doc->create_element("th", map);
  doc->create_element("link", map);
  doc->create_element("title", map);
  doc->create_element("a", map);
  doc->create_element("tr", map);
  doc->create_element("style", map);
  doc->create_element("base", map);
  doc->create_element("div", map);
  doc->create_element("script", map);
  doc->create_element("font", map);
  doc->create_element("tag", map);
}

TEST(DocumentTest, DeviceChange) {
  container_test container;
  litehtml::document::ptr doc = std::make_shared<litehtml::document>(&container);
  doc->media_changed();
  doc->lang_changed();
}

TEST(DocumentTest, Parse) {
  container_test container;
  document::createFromString("", &container);
}
