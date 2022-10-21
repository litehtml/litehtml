#include "container_test.h"

container_test::container_test() {}
container_test::~container_test() {}
litehtml::uint_ptr container_test::create_font(const char* faceName, int size, int weight, litehtml::font_style italic, unsigned int decoration, litehtml::font_metrics* fm) {
  if (fm) {
    fm->ascent = 10;
    fm->descent = 5;
    fm->height = 10 + 5;
    fm->x_height = 3;
  }
  return (litehtml::uint_ptr)0;
}
void container_test::delete_font(litehtml::uint_ptr hFont) {}
int container_test::text_width(const char* text, litehtml::uint_ptr hFont) { return 0; }
void container_test::draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont, litehtml::web_color color, const litehtml::position& pos) {}
int container_test::pt_to_px(int pt) const { return (int)((double)pt * 96 / 72.0); }
int container_test::get_default_font_size() const { return 16; }
const char* container_test::get_default_font_name() const { return "Times New Roman"; }
void container_test::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) {}
void container_test::load_image(const char* src, const char* baseurl, bool redraw_on_ready) {}
void container_test::get_image_size(const char* src, const char* baseurl, litehtml::size& sz) {}
void container_test::draw_background(litehtml::uint_ptr hdc, const litehtml::background_paint& bg) {}
void container_test::make_url(const char* url, const char* basepath, litehtml::string& out) { out = url; }
void container_test::draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders, const litehtml::position& draw_pos, bool root) {}
void container_test::set_caption(const char* caption){};    //: set_caption
void container_test::set_base_url(const char* base_url){};  //: set_base_url
void container_test::link(const std::shared_ptr<litehtml::document>& ptr, const litehtml::element::ptr& el) {}
void container_test::on_anchor_click(const char* url, const litehtml::element::ptr& el) {}  //: on_anchor_click
void container_test::set_cursor(const char* cursor) {}                                      //: set_cursor
void container_test::transform_text(litehtml::string& text, litehtml::text_transform tt) {}
void container_test::import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) {}  //: import_css
void container_test::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius, bool valid_x, bool valid_y) {}
void container_test::del_clip() {}
void container_test::get_client_rect(litehtml::position& client) const {}  //: get_client_rect
std::shared_ptr<litehtml::element> container_test::create_element(const char* tag_name, const litehtml::string_map& attributes, const std::shared_ptr<litehtml::document>& doc) { return 0; }
void container_test::get_media_features(litehtml::media_features& media) const {
  litehtml::position client;
  get_client_rect(client);
  media.type = litehtml::media_type_screen;
  media.width = client.width;
  media.height = client.height;
  media.device_width = 100;
  media.device_height = 100;
  media.color = 8;
  media.monochrome = 0;
  media.color_index = 256;
  media.resolution = 96;
}
void container_test::get_language(litehtml::string& language, litehtml::string& culture) const {
  language = "en";
  culture = "";
}
//: resolve_color