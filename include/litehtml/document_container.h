#ifndef LITEHTML_DOCUMENT_CONTAINER_H
#define LITEHTML_DOCUMENT_CONTAINER_H

#include "types.h"
#include "web_color.h"
#include "background.h"
#include "borders.h"
#include "box_shadow.h"
#include "clipping.h"
#include "controls.h"
#include "element.h"
#include "font_description.h"
#include <memory>
#include <functional>

namespace litehtml
{
    struct list_marker
    {
        std::string     image;
        const char*     baseurl;
        list_style_type marker_type;
        web_color       color;
        position        pos;
        int             index;
        uint_ptr        font;
    };

    enum mouse_event
    {
        mouse_event_enter,
        mouse_event_leave,
    };

    // call back interface to draw text, images and other elements
    class document_container
    {
      public:
        virtual litehtml::uint_ptr create_font(const font_description& descr, const document* doc,
                                               litehtml::font_metrics* fm)                                       = 0;
        virtual void               delete_font(litehtml::uint_ptr hFont)                                         = 0;
        virtual pixel_t            text_width(const char* text, litehtml::uint_ptr hFont)                        = 0;
        virtual void               draw_text(litehtml::uint_ptr hdc, const char* text, litehtml::uint_ptr hFont,
                                             litehtml::web_color color, const litehtml::position& pos,
                                             const litehtml::clipping* clip)                                     = 0;
        virtual pixel_t            pt_to_px(float pt) const                                                      = 0;
        virtual pixel_t            get_default_font_size() const                                                 = 0;
        virtual const char*        get_default_font_name() const                                                 = 0;
        virtual void               draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker,
                                                    const litehtml::clipping* clip)                              = 0;
        virtual void               load_image(const char* src, const char* baseurl, bool redraw_on_ready)        = 0;
        virtual void               get_image_size(const char* src, const char* baseurl, litehtml::size& sz)      = 0;
        virtual void draw_image(litehtml::uint_ptr hdc, const background_layer& layer, const std::string& url,
                                const std::string& base_url, const litehtml::clipping* clip)                     = 0;
        virtual void draw_solid_fill(litehtml::uint_ptr hdc, const background_layer& layer, const web_color& color,
                                     const litehtml::clipping* clip)                                             = 0;
        virtual void draw_linear_gradient(litehtml::uint_ptr hdc, const background_layer& layer,
                                          const background_layer::linear_gradient& gradient,
                                          const litehtml::clipping*                clip)                         = 0;
        virtual void draw_radial_gradient(litehtml::uint_ptr hdc, const background_layer& layer,
                                          const background_layer::radial_gradient& gradient,
                                          const litehtml::clipping*                clip)                         = 0;
        virtual void draw_conic_gradient(litehtml::uint_ptr hdc, const background_layer& layer,
                                         const background_layer::conic_gradient& gradient,
                                         const litehtml::clipping*               clip)                           = 0;
        virtual void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders& borders,
                                  const litehtml::position& draw_pos, bool root, const litehtml::clipping* clip) = 0;

        // Draws one box shadow. `box` is the border box for outset shadows and the padding box for
        // inset shadows; `radius` is the border radius of that box. `shadow` holds pixel offsets.
        // An outset shadow paints outside the rounded `box` only and must be clipped out of it;
        // an inset shadow paints inside the rounded `box`. Both must honour `clip`.
        virtual void draw_box_shadow(litehtml::uint_ptr /*hdc*/, const litehtml::position& /*box*/,
                                     const litehtml::border_radiuses& /*radius*/,
                                     const litehtml::shadow_paint& /*shadow*/, const litehtml::clipping* /*clip*/)
        {
        }

        // Draw the dropdown marker of a one-line select (a down-pointing chevron) inside pos.
        virtual void draw_combobox_marker(litehtml::uint_ptr hdc, const litehtml::position& pos,
                                          const litehtml::web_color& color, const litehtml::clipping* clip) = 0;
        // Draw the check mark of a checked checkbox inside pos.
        virtual void draw_checkbox_marker(litehtml::uint_ptr hdc, const litehtml::position& pos,
                                          const litehtml::web_color& color, const litehtml::clipping* clip) = 0;

        virtual void set_caption(const char* caption)                                                       = 0;
        virtual void set_base_url(const char* base_url)                                                     = 0;
        virtual void link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) = 0;
        virtual void on_anchor_click(const char* url, const litehtml::element::ptr& el)                     = 0;
        virtual bool on_element_click(const litehtml::element::ptr& /*el*/)
        {
            return false;
        };
        virtual void on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event)    = 0;
        virtual void set_cursor(const char* cursor)                                                   = 0;
        virtual void transform_text(std::string& text, litehtml::text_transform tt)                   = 0;
        virtual void import_css(std::string& text, const std::string& url, std::string& baseurl)      = 0;
        virtual void get_viewport(litehtml::position& viewport) const                                 = 0;
        virtual litehtml::element::ptr create_element(const char* tag_name, const litehtml::string_map& attributes,
                                                      const std::shared_ptr<litehtml::document>& doc) = 0;

        virtual void        get_media_features(litehtml::media_features& media) const       = 0;
        virtual void        get_language(std::string& language, std::string& culture) const = 0;
        virtual std::string resolve_color(const std::string& /*color*/) const
        {
            return {};
        }
        virtual void split_text(const char* text, const std::function<void(const char*)>& on_word,
                                const std::function<void(const char*)>& on_space);

        // Form controls integration. All of these functions have default implementations, so
        // containers that do not support controls keep working.

        // Called when the focused element changes. el is nullptr when the focus is removed.
        // element_id is nullptr when the element has no id attribute. control_data contains the
        // current state of the control: bool for checkbox/radio, text_control_data for text-like
        // controls, select_control_data for select and monostate for other elements.
        virtual void on_focus_change(const litehtml::element::ptr& /*el*/, const char* /*element_id*/,
                                     litehtml::control_type /*type*/, const litehtml::control_data& /*data*/)
        {
        }

        // Lets the container provide the authoritative text state of a text-like control. The
        // control fills data with its cached state before the call; the container may replace it.
        virtual void get_text_data(const litehtml::element::ptr& /*el*/, litehtml::text_control_data& /*data*/) {}

        // Called when the value of a control changes (not when only the caret moves). The payload is
        // bool for checkbox/radio, text_control_data for text-like controls and select_control_data
        // for select.
        virtual void on_change(const litehtml::element::ptr& /*el*/, const char* /*element_id*/,
                               const litehtml::control_data& /*data*/)
        {
        }

        // Called when a form is submitted.
        virtual void on_submit(const litehtml::form_data& /*data*/) {}

        // Requests the container to show the option list of a one-line select (dropdown). Listbox selects
        // (multiple or size > 1) are rendered inline and select their rows directly, so they never call this.
        // Returns true when the popup was shown; the container applies the chosen item with
        // document::update_focused.
        virtual bool show_select_popup(const litehtml::element::ptr& /*el*/, const char* /*element_id*/,
                                       const litehtml::select_control_data& /*data*/)
        {
            return false;
        }

      protected:
        virtual ~document_container() = default;
    };
} // namespace litehtml

#endif // LITEHTML_DOCUMENT_CONTAINER_H
