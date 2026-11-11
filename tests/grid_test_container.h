#ifndef LITEHTML_GRID_TEST_CONTAINER_H
#define LITEHTML_GRID_TEST_CONTAINER_H

#include "litehtml/document_container.h"
#include <cstring>

class grid_test_container : public litehtml::document_container
{
  public:
    litehtml::uint_ptr create_font(const litehtml::font_description& description, const litehtml::document*,
                                   litehtml::font_metrics*           metrics) override
    {
        metrics->font_size = metrics->height = metrics->ascent = metrics->x_height = metrics->ch_width =
            description.size;
        metrics->descent = 0;
        return 1;
    }
    void              delete_font(litehtml::uint_ptr) override {}
    litehtml::pixel_t text_width(const char* text, litehtml::uint_ptr) override
    {
        return static_cast<int>(std::strlen(text)) * 8;
    }
    void draw_text(litehtml::uint_ptr, const char*, litehtml::uint_ptr, litehtml::web_color,
                   const litehtml::position&) override
    {
    }
    litehtml::pixel_t pt_to_px(float value) const override
    {
        return litehtml::pixel_t(value * 4 / 3);
    }
    litehtml::pixel_t get_default_font_size() const override
    {
        return 16;
    }
    const char* get_default_font_name() const override
    {
        return "sans-serif";
    }
    void draw_list_marker(litehtml::uint_ptr, const litehtml::list_marker&) override {}
    void load_image(const char*, const char*, bool) override {}
    void get_image_size(const char*, const char*, litehtml::size&) override {}
    void draw_image(litehtml::uint_ptr, const litehtml::background_layer&, const std::string&,
                    const std::string&) override
    {
    }
    void draw_solid_fill(litehtml::uint_ptr, const litehtml::background_layer&, const litehtml::web_color&) override {}
    void draw_linear_gradient(litehtml::uint_ptr, const litehtml::background_layer&,
                              const litehtml::background_layer::linear_gradient&) override
    {
    }
    void draw_radial_gradient(litehtml::uint_ptr, const litehtml::background_layer&,
                              const litehtml::background_layer::radial_gradient&) override
    {
    }
    void draw_conic_gradient(litehtml::uint_ptr, const litehtml::background_layer&,
                             const litehtml::background_layer::conic_gradient&) override
    {
    }
    void draw_borders(litehtml::uint_ptr, const litehtml::borders&, const litehtml::position&, bool) override {}
    void set_caption(const char*) override {}
    void set_base_url(const char*) override {}
    void link(const std::shared_ptr<litehtml::document>&, const litehtml::element::ptr&) override {}
    void on_anchor_click(const char*, const litehtml::element::ptr&) override {}
    void on_mouse_event(const litehtml::element::ptr&, litehtml::mouse_event) override {}
    void set_cursor(const char*) override {}
    void transform_text(std::string&, litehtml::text_transform) override {}
    void import_css(std::string&, const std::string&, std::string&) override {}
    void set_clip(const litehtml::position&, const litehtml::border_radiuses&) override {}
    void del_clip() override {}
    void get_viewport(litehtml::position& viewport) const override
    {
        viewport = {0, 0, 800, 600};
    }
    litehtml::element::ptr create_element(const char*, const litehtml::string_map&,
                                          const std::shared_ptr<litehtml::document>&) override
    {
        return nullptr;
    }
    void get_media_features(litehtml::media_features& media) const override
    {
        media.type       = litehtml::media_type_screen;
        media.width      = 800;
        media.height     = 600;
        media.resolution = 96;
    }
    void get_language(std::string&, std::string&) const override {}
};

#endif // LITEHTML_GRID_TEST_CONTAINER_H
