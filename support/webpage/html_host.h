#ifndef LITEBROWSER_HTML_HOST_H
#define LITEBROWSER_HTML_HOST_H

#include <cairo.h>
#include <litehtml.h>
#include <functional>

namespace litebrowser
{
    class html_host_interface
    {
      public:
        virtual ~html_host_interface() = default;

        virtual void             open_url(const std::string& url)                      = 0;
        virtual void             update_cursor()                                       = 0;
        virtual void             scroll_to(int x, int y)                               = 0;
        virtual void             get_viewport(litehtml::position& viewport) const      = 0;
        virtual void             redraw_boxes(const litehtml::position::vector& boxes) = 0;
        virtual int              get_render_width()                                    = 0;
        virtual double           get_dpi()                                             = 0;
        virtual int              get_screen_width()                                    = 0;
        virtual int              get_screen_height()                                   = 0;
        virtual cairo_surface_t* load_image(const std::string& path)                   = 0;

        // Shows the option list of a select. When the user chooses an option the host must call
        // on_selected with the updated data; web_page applies it to the document.
        virtual void show_select_popup(
            const litehtml::element::ptr& el, const char* element_id, const litehtml::select_control_data& data,
            const std::function<void(const litehtml::select_control_data&)>& on_selected) = 0;

        // Navigates to the action of a submitted form. The action is already resolved by web_page.
        virtual void submit_form(const litehtml::form_data& data) = 0;
    };

    class browser_notify_interface
    {
      public:
        virtual ~browser_notify_interface() = default;

        virtual void redraw()                                        = 0;
        virtual void render()                                        = 0;
        virtual void update_state()                                  = 0;
        virtual void on_set_caption(const std::string& caption_text) = 0;
        virtual void on_page_loaded(uint64_t web_page_id)            = 0;
    };

} // namespace litebrowser

#endif // LITEBROWSER_HTML_HOST_H
