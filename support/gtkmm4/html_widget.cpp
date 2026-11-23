#include "html_widget.h"

#include <litehtml/render_item.h>
#include <litehtml/url.h>
#include <gdk/gdkkeysyms.h>
#include <cmath>
#include <memory>
#include <optional>
#include <sstream>

using litehtml::operator""_px;

namespace
{
    litehtml::web_color blend(const litehtml::web_color& fg, const litehtml::web_color& bg, double factor)
    {
        const auto mix = [factor](int f, int b) {
            return static_cast<litehtml::byte>(
                std::lround(static_cast<double>(f) * factor + static_cast<double>(b) * (1.0 - factor)));
        };
        return {mix(fg.red, bg.red), mix(fg.green, bg.green), mix(fg.blue, bg.blue)};
    }

    std::string css_color(const litehtml::web_color& color)
    {
        return "#" + color.to_string();
    }

    std::string css_font_family(const std::string& family)
    {
        if(family.empty() || family.find_first_of(",\"'") != std::string::npos)
        {
            return family;
        }
        if(family.find(' ') != std::string::npos)
        {
            return "\"" + family + "\"";
        }
        return family;
    }

    bool map_key_event(guint keyval, Gdk::ModifierType state, litebrowser::key_event& event)
    {
        event.ctrl  = (state & Gdk::ModifierType::CONTROL_MASK) != Gdk::ModifierType::NO_MODIFIER_MASK;
        event.shift = (state & Gdk::ModifierType::SHIFT_MASK) != Gdk::ModifierType::NO_MODIFIER_MASK;

        switch(keyval)
        {
        case GDK_KEY_BackSpace:
            event.kind = litebrowser::key_event::kind::backspace;
            return true;
        case GDK_KEY_Delete:
        case GDK_KEY_KP_Delete:
            event.kind = litebrowser::key_event::kind::delete_;
            return true;
        case GDK_KEY_Left:
        case GDK_KEY_KP_Left:
            event.kind = litebrowser::key_event::kind::left;
            return true;
        case GDK_KEY_Right:
        case GDK_KEY_KP_Right:
            event.kind = litebrowser::key_event::kind::right;
            return true;
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
            event.kind = litebrowser::key_event::kind::up;
            return true;
        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
            event.kind = litebrowser::key_event::kind::down;
            return true;
        case GDK_KEY_Home:
        case GDK_KEY_KP_Home:
            event.kind = litebrowser::key_event::kind::home;
            return true;
        case GDK_KEY_End:
        case GDK_KEY_KP_End:
            event.kind = litebrowser::key_event::kind::end;
            return true;
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
            event.kind = litebrowser::key_event::kind::enter;
            return true;
        case GDK_KEY_space:
            event.kind = litebrowser::key_event::kind::space;
            return true;
        case GDK_KEY_Escape:
            event.kind = litebrowser::key_event::kind::escape;
            return true;
        default:
            break;
        }

        if(event.ctrl)
        {
            switch(keyval)
            {
            case GDK_KEY_a:
            case GDK_KEY_A:
                event.kind = litebrowser::key_event::kind::select_all;
                return true;
            case GDK_KEY_c:
            case GDK_KEY_C:
                event.kind = litebrowser::key_event::kind::copy;
                return true;
            case GDK_KEY_x:
            case GDK_KEY_X:
                event.kind = litebrowser::key_event::kind::cut;
                return true;
            case GDK_KEY_v:
            case GDK_KEY_V:
                event.kind = litebrowser::key_event::kind::paste;
                return true;
            default:
                return false;
            }
        }

        if((state & Gdk::ModifierType::ALT_MASK) == Gdk::ModifierType::NO_MODIFIER_MASK)
        {
            const gunichar ch = gdk_keyval_to_unicode(keyval);
            if(ch >= 0x20 && ch != 0x7F)
            {
                char buffer[7] = {};
                int  length    = g_unichar_to_utf8(ch, buffer);
                if(length > 0)
                {
                    event.kind = litebrowser::key_event::kind::character;
                    event.text.assign(buffer, static_cast<size_t>(length));
                    return true;
                }
            }
        }
        return false;
    }
} // namespace

html_widget::html_widget()
{
    add_css_class("litehtml");

    m_notifier = std::make_shared<html_widget_notifier>();
    m_notifier->connect_redraw(sigc::mem_fun(*this, &html_widget::on_redraw));
    m_notifier->connect_render(sigc::mem_fun(*this, &html_widget::render));
    m_notifier->connect_update_state([this]() { m_sig_update_state.emit(get_state()); });
    m_notifier->connect_on_page_loaded(sigc::mem_fun(*this, &html_widget::on_page_loaded));
    m_notifier->connect_on_set_caption(sigc::mem_fun(*this, &html_widget::set_caption));

    set_focusable(true);

    m_vadjustment = Gtk::Adjustment::create(0.0, 0.0, 0.0, 10);
    m_vadjustment->signal_value_changed().connect(sigc::mem_fun(*this, &html_widget::on_vadjustment_changed));
    m_hadjustment = Gtk::Adjustment::create(0.0, 0.0, 0.0, 10);
    m_hadjustment->signal_value_changed().connect(sigc::mem_fun(*this, &html_widget::on_hadjustment_changed));

    m_hadjustment->signal_changed().connect(sigc::mem_fun(*this, &html_widget::on_adjustments_changed));
    m_vadjustment->signal_changed().connect(sigc::mem_fun(*this, &html_widget::on_adjustments_changed));

    m_vscrollbar = Gtk::make_managed<Gtk::Scrollbar>(m_vadjustment, Gtk::Orientation::VERTICAL);
    m_vscrollbar->insert_at_end(*this);
    m_vscrollbar->set_visible(true);
    m_vscrollbar->add_css_class("right");
    m_vscrollbar->add_css_class("overlay-indicator");

    m_hscrollbar = Gtk::make_managed<Gtk::Scrollbar>(m_hadjustment, Gtk::Orientation::HORIZONTAL);
    m_hscrollbar->insert_at_end(*this);
    m_hscrollbar->set_visible(true);
    m_hscrollbar->add_css_class("bottom");
    m_hscrollbar->add_css_class("overlay-indicator");

    m_scroll_controller = Gtk::EventControllerScroll::create();
    m_scroll_controller->set_flags(Gtk::EventControllerScroll::Flags::BOTH_AXES);
    m_scroll_controller->signal_scroll().connect(sigc::mem_fun(*this, &html_widget::on_scroll), false);
    add_controller(m_scroll_controller);

    auto click_gesture = Gtk::GestureClick::create();
    click_gesture->set_button(GDK_BUTTON_PRIMARY);
    click_gesture->signal_pressed().connect(sigc::mem_fun(*this, &html_widget::on_button_press_event));
    click_gesture->signal_released().connect(sigc::mem_fun(*this, &html_widget::on_button_release_event));
    add_controller(click_gesture);

    auto move_gesture = Gtk::EventControllerMotion::create();
    move_gesture->signal_motion().connect(sigc::mem_fun(*this, &html_widget::on_mouse_move));
    move_gesture->signal_leave().connect([this]() { restart_scrollbar_timer(); });
    add_controller(move_gesture);

    auto key_controller = Gtk::EventControllerKey::create();
    key_controller->signal_key_pressed().connect(sigc::mem_fun(*this, &html_widget::on_key_pressed), false);
    add_controller(key_controller);
}

html_widget::~html_widget()
{
    m_caret_timer.disconnect();
    m_select_popup.reset();
    remove_select_popup_style();
    m_notifier->disconnect();

    if(!gobj())
    {
        return;
    }

    while(Widget* child = get_first_child())
    {
        child->unparent();
    }
}

double html_widget::get_dpi()
{
    // DPI is always 96 (default). Scaling will make things larger.
    return 96.0;
}

int html_widget::get_screen_width()
{
    auto display = Gdk::Display::get_default();
    if(display)
    {
        auto monitors = display->get_monitors();
        if(monitors->get_n_items() > 0)
        {
            auto           monitor = monitors->get_typed_object<Gdk::Monitor>(0);
            Gdk::Rectangle rect;
            monitor->get_geometry(rect);
            return rect.get_width();
        }
    }
    return 800;
}

int html_widget::get_screen_height()
{
    auto display = Gdk::Display::get_default();
    if(display)
    {
        auto monitors = display->get_monitors();
        if(monitors->get_n_items() > 0)
        {
            auto           monitor = monitors->get_typed_object<Gdk::Monitor>(0);
            Gdk::Rectangle rect;
            monitor->get_geometry(rect);
            return rect.get_height();
        }
    }
    return 800;
}

void html_widget::snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot>& snapshot)
{
    if(get_allocated_width() <= 0 || get_allocated_height() <= 0)
    {
        return;
    }

    {
        auto allocation = get_allocation();
        auto cr         = snapshot->append_cairo(Gdk::Rectangle(0, 0, allocation.get_width(), allocation.get_height()));
        if(m_draw_buffer.get_cairo_surface())
        {
            cr->scale(1.0 / m_draw_buffer.get_scale_factor(), 1.0 / m_draw_buffer.get_scale_factor());
            cairo_set_source_surface(cr->cobj(), m_draw_buffer.get_cairo_surface(), 0, 0);
            cr->paint();
        }
    }

    snapshot_child(*m_vscrollbar, snapshot);
    snapshot_child(*m_hscrollbar, snapshot);
}

void html_widget::get_viewport(litehtml::position& viewport) const
{
    viewport.x      = m_draw_buffer.get_left();
    viewport.y      = m_draw_buffer.get_top();
    viewport.width  = m_draw_buffer.get_width();
    viewport.height = m_draw_buffer.get_height();
}

void html_widget::set_caption(const std::string& caption)
{
    auto root = get_root();
    if(root)
    {
        auto window = dynamic_cast<Gtk::Window*>(root);
        if(window)
        {
            window->set_title(caption.c_str());
        };
    }
}

cairo_surface_t* html_widget::load_image(const std::string& path)
{
    Glib::RefPtr<Gdk::Pixbuf> ptr;

    try
    {
        ptr = Gdk::Pixbuf::create_from_file(path);
    }
    catch(...)
    {
    }

    if(!ptr)
    {
        return nullptr;
    }

    cairo_surface_t* ret = nullptr;

    if(ptr->get_has_alpha())
    {
        ret = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ptr->get_width(), ptr->get_height());
    } else
    {
        ret = cairo_image_surface_create(CAIRO_FORMAT_RGB24, ptr->get_width(), ptr->get_height());
    }

    Cairo::RefPtr<Cairo::Surface> surface = std::make_shared<Cairo::Surface>(ret, false);
    Cairo::RefPtr<Cairo::Context> ctx     = Cairo::Context::create(surface);
    Gdk::Cairo::set_source_pixbuf(ctx, ptr, 0.0, 0.0);
    ctx->paint();

    return ret;
}

void html_widget::open_page(const std::string& url, const std::string& fragment)
{
    {
        std::lock_guard<std::mutex> lock(m_page_mutex);
        if(m_current_page)
        {
            m_current_page->stop_loading();
        }
        m_next_page = std::make_shared<litebrowser::web_page>(this, m_notifier, 10);
        m_next_page->open(url, fragment);
    }
    m_sig_set_address.emit(url);
    m_sig_update_state.emit(get_state());
}

void html_widget::open_page_post(const std::string& url, const litehtml::form_data& data)
{
    {
        std::lock_guard<std::mutex> lock(m_page_mutex);
        if(m_current_page)
        {
            m_current_page->stop_loading();
        }
        m_next_page = std::make_shared<litebrowser::web_page>(this, m_notifier, 10);
        m_next_page->open_post(url, data, "");
    }
    m_sig_set_address.emit(url);
    m_sig_update_state.emit(get_state());
    m_history.url_opened(url);
}

void html_widget::submit_form(const litehtml::form_data& data)
{
    const std::string method = data.method.empty() ? "get" : data.method;
    if(method == "post")
    {
        open_page_post(data.action, data);
        return;
    }

    std::string query;
    for(const auto& field : data.fields)
    {
        if(!query.empty())
        {
            query += '&';
        }
        query += litehtml::url::encode(field.name);
        query += '=';
        query += litehtml::url::encode(field.value);
    }

    std::string url = data.action;
    if(!query.empty())
    {
        url += url.find('?') == std::string::npos ? "?" : "&";
        url += query;
    }
    open_url(url);
}

void html_widget::show_select_popup(const litehtml::element::ptr&        el, const char* /*element_id*/,
                                    const litehtml::select_control_data& data,
                                    const std::function<void(const litehtml::select_control_data&)>& on_selected)
{
    const uint64_t generation = ++m_select_popup_generation;
    if(m_select_popup)
    {
        m_select_popup->popdown();
        m_select_popup.reset();
    }
    remove_select_popup_style();
    m_select_popup_callback = on_selected;

    // Border box of the select in widget coordinates.
    litehtml::position         box;
    litehtml::web_color        background = litehtml::web_color::white;
    litehtml::web_color        text       = litehtml::web_color::black;
    litehtml::web_color        border     = litehtml::web_color(118, 118, 118);
    litehtml::font_description font;
    font.family = "sans-serif";
    font.size   = 16_px;
    font.weight = 400;
    font.style  = litehtml::font_style_normal;

    if(el)
    {
        box = el->get_placement();
        if(const auto ri = el->get_render_item())
        {
            const litehtml::margins paddings  = ri->get_paddings();
            const litehtml::margins borders   = ri->get_borders();
            box.x                            -= paddings.left + borders.left;
            box.y                            -= paddings.top + borders.top;
            box.width                        += paddings.width() + borders.width();
            box.height                       += paddings.height() + borders.height();
        }
        box.x -= m_draw_buffer.get_left();
        box.y -= m_draw_buffer.get_top();

        const auto& css = el->css();
        if(css.get_bg().m_color.alpha != 0)
        {
            background = css.get_bg().m_color;
        }
        text                = css.get_color();
        const auto& borders = css.get_borders();
        if(borders.top.style != litehtml::border_style_none && litehtml::pixel_t(borders.top.width.val()) > 0_px)
        {
            border = borders.top.color;
        }
        font = css.get_font_description();
        if(font.family.empty())
        {
            font.family = "sans-serif";
        }
        if(font.weight <= 0)
        {
            font.weight = 400;
        }
        if(font.size <= 0_px)
        {
            font.size = 16_px;
        }
    }

    // The popup must be opaque; flatten a translucent control background over white.
    if(background.alpha != 255)
    {
        background = blend(background, litehtml::web_color::white, static_cast<double>(background.alpha) / 255.0);
    }
    const litehtml::web_color hover_color    = blend(text, background, 0.06);
    const litehtml::web_color selected_color = blend(text, background, 0.10);
    const litehtml::web_color group_color    = blend(text, background, 0.65);
    const litehtml::web_color disabled_color = blend(text, background, 0.45);

    // The popup matches the select box: 1px border on each side, no extra padding or shadow.
    const int popup_width   = std::max(1, static_cast<int>(box.width));
    const int content_width = std::max(1, popup_width - 2);
    // Popup height is limited to the widget; the list scrolls when it does not fit.
    const int widget_height = get_allocated_height();

    const std::string popup_class = "litehtml-select-popup-" + std::to_string(generation);

    std::ostringstream css_text;
    css_text << "popover." << popup_class << " {\n"
             << "    background: transparent;\n"
             << "}\n"
             << "popover." << popup_class << " > contents {\n"
             << "    padding: 0;\n"
             << "    margin: 0;\n"
             << "    min-width: 0;\n"
             << "    min-height: 0;\n"
             << "    border: 1px solid " << css_color(border) << ";\n"
             << "    border-radius: 0;\n"
             << "    box-shadow: none;\n"
             << "    background-color: " << css_color(background) << ";\n"
             << "    color: " << css_color(text) << ";\n"
             << "    font-family: " << css_font_family(font.family) << ";\n"
             << "    font-size: " << font.size.value() << "px;\n"
             << "    font-weight: " << font.weight << ";\n"
             << "    font-style: " << (font.style == litehtml::font_style_italic ? "italic" : "normal") << ";\n"
             << "}\n"
             << "popover." << popup_class << " list {\n"
             << "    padding: 0;\n"
             << "    background: transparent;\n"
             << "    color: " << css_color(text) << ";\n"
             << "}\n"
             << "popover." << popup_class << " list > row {\n"
             << "    background: transparent;\n"
             << "    color: " << css_color(text) << ";\n"
             << "}\n"
             << "popover." << popup_class << " list > row > label {\n"
             << "    color: " << css_color(text) << ";\n"
             << "}\n"
             << "popover." << popup_class << " list > row:hover {\n"
             << "    background-color: " << css_color(hover_color) << ";\n"
             << "}\n"
             << "popover." << popup_class << " list > row.selected {\n"
             << "    background-color: " << css_color(selected_color) << ";\n"
             << "}\n"
             << "popover." << popup_class << " list > row.group-label > label {\n"
             << "    color: " << css_color(group_color) << ";\n"
             << "}\n"
             << "popover." << popup_class << " list > row:disabled > label {\n"
             << "    color: " << css_color(disabled_color) << ";\n"
             << "}\n";

    m_select_popup_display = get_display();
    if(m_select_popup_display)
    {
        auto provider = Gtk::CssProvider::create();
        try
        {
            provider->load_from_data(css_text.str());
            Gtk::StyleProvider::add_provider_for_display(m_select_popup_display, provider,
                                                         GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            m_select_popup_provider = provider;
        }
        catch(const Glib::Error&)
        {
            m_select_popup_display.reset();
        }
    }

    auto popover = std::make_unique<Gtk::Popover>();
    popover->add_css_class(popup_class);
    popover->set_has_arrow(false);
    popover->set_position(Gtk::PositionType::BOTTOM);

    auto list = Gtk::make_managed<Gtk::ListBox>();
    list->set_selection_mode(Gtk::SelectionMode::NONE);

    auto data_ptr = std::make_shared<litehtml::select_control_data>(data);
    auto row_map  = std::make_shared<std::vector<int>>();

    for(const auto& group : data.groups)
    {
        if(!group.label.empty())
        {
            auto label = Gtk::make_managed<Gtk::Label>(group.label);
            label->set_xalign(0.0f);
            label->set_ellipsize(Pango::EllipsizeMode::END);
            label->set_margin_start(8);
            label->set_margin_end(8);
            label->set_margin_top(4);
            label->set_margin_bottom(2);
            auto row = Gtk::make_managed<Gtk::ListBoxRow>();
            row->add_css_class("group-label");
            row->set_activatable(false);
            row->set_selectable(false);
            row->set_child(*label);
            list->append(*row);
            row_map->push_back(-1);
        }

        for(const auto& option : group.options)
        {
            auto label = Gtk::make_managed<Gtk::Label>(option.label);
            label->set_xalign(0.0f);
            label->set_hexpand(true);
            label->set_ellipsize(Pango::EllipsizeMode::END);
            label->set_margin_start(group.label.empty() ? 8 : 20);
            label->set_margin_end(8);
            label->set_margin_top(3);
            label->set_margin_bottom(3);
            auto       row     = Gtk::make_managed<Gtk::ListBoxRow>();
            const bool enabled = !option.disabled && !group.disabled;
            row->set_sensitive(enabled);
            row->set_activatable(enabled);
            row->set_selectable(false);
            row->set_child(*label);
            if(option.selected)
            {
                row->add_css_class("selected");
            }
            list->append(*row);
            row_map->push_back(option.index);
        }
    }

    list->signal_row_activated().connect([this, data_ptr, row_map](Gtk::ListBoxRow* row) {
        if(!row)
        {
            return;
        }
        const int row_index = row->get_index();
        if(row_index < 0 || static_cast<size_t>(row_index) >= row_map->size())
        {
            return;
        }
        const int option_index = (*row_map)[static_cast<size_t>(row_index)];
        if(option_index < 0)
        {
            return;
        }

        auto updated = *data_ptr;
        for(auto& group : updated.groups)
        {
            for(auto& option : group.options)
            {
                if(option.index == option_index)
                {
                    option.selected = updated.multiple ? !option.selected : true;
                } else if(!updated.multiple)
                {
                    option.selected = false;
                }
            }
        }

        if(m_select_popup_callback)
        {
            m_select_popup_callback(updated);
        }
        if(!updated.multiple && m_select_popup)
        {
            m_select_popup->popdown();
        }
    });

    // Show every item; the list scrolls only when it is taller than the widget.
    auto scrolled = Gtk::make_managed<Gtk::ScrolledWindow>();
    scrolled->set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
    scrolled->set_size_request(content_width, -1);
    scrolled->set_max_content_width(content_width);
    scrolled->set_propagate_natural_width(false);
    scrolled->set_propagate_natural_height(true);
    if(widget_height > 2)
    {
        scrolled->set_max_content_height(widget_height - 2);
    }
    scrolled->set_child(*list);
    popover->set_child(*scrolled);
    popover->set_parent(*this);

    if(el)
    {
        popover->set_pointing_to(Gdk::Rectangle(static_cast<int>(box.x), static_cast<int>(box.y), popup_width,
                                                std::max(1, static_cast<int>(box.height))));
    }

    popover->signal_closed().connect([this, generation]() {
        if(generation != m_select_popup_generation)
        {
            return;
        }
        m_select_popup_callback = {};
        remove_select_popup_style();
        Glib::signal_idle().connect_once([this, generation]() {
            if(generation == m_select_popup_generation)
            {
                m_select_popup.reset();
            }
        });
    });

    auto* popup_ptr = popover.get();
    m_select_popup  = std::move(popover);
    popup_ptr->popup();
}

void html_widget::remove_select_popup_style()
{
    if(m_select_popup_provider && m_select_popup_display)
    {
        Gtk::StyleProvider::remove_provider_for_display(m_select_popup_display, m_select_popup_provider);
    }
    m_select_popup_provider.reset();
    m_select_popup_display.reset();
}

void html_widget::update_caret_timer()
{
    auto       page       = current_page();
    const bool text_focus = page && page->has_text_focus();
    if(text_focus)
    {
        m_caret_visible = true;
        page->set_caret_visible(true);
        m_caret_timer.disconnect();
        m_caret_timer = Glib::signal_timeout().connect(sigc::mem_fun(*this, &html_widget::on_caret_tick), 500);
    } else
    {
        if(m_caret_timer.connected())
        {
            m_caret_timer.disconnect();
        }
        m_caret_visible = true;
    }
}

bool html_widget::on_caret_tick()
{
    auto page = current_page();
    if(!page || !page->has_text_focus())
    {
        return false;
    }
    m_caret_visible = !m_caret_visible;
    page->set_caret_visible(m_caret_visible);
    return true;
}

void html_widget::scroll_to(int x, int y)
{
    m_hadjustment->set_value(x);
    m_vadjustment->set_value(y);
}

void html_widget::on_redraw()
{
    m_draw_buffer.redraw(get_draw_function(current_page()));
    queue_draw();
}

inline static void update_rect(const litehtml::position& pos, Gdk::Rectangle& rect, bool& is_first)
{
    litehtml::position rounded_box = pos;
    rounded_box.round();
    if(is_first)
    {
        rect     = Gdk::Rectangle(rounded_box.x, rounded_box.y, rounded_box.width, rounded_box.height);
        is_first = false;
    } else
    {
        rect.join(Gdk::Rectangle(rounded_box.x, rounded_box.y, rounded_box.width, rounded_box.height));
    }
}

void html_widget::on_button_press_event(int /* n_press */, double x, double y)
{
    if(!has_focus())
    {
        grab_focus();
    }
    auto page = current_page();
    if(page)
    {
        Gdk::Rectangle rect(0, 0, 0, 0);
        bool           is_first = true;
        page->on_lbutton_down(static_cast<int>(x + m_draw_buffer.get_left()),
                              static_cast<int>(y + m_draw_buffer.get_top()), static_cast<int>(x), static_cast<int>(y),
                              [&](const litehtml::position& box) { update_rect(box, rect, is_first); });
        if(!rect.has_zero_area())
        {
            m_draw_buffer.redraw_area(get_draw_function(current_page()), rect.get_x(), rect.get_y(), rect.get_width(),
                                      rect.get_height());
            queue_draw();
        }
        update_caret_timer();
    }
}

void html_widget::on_button_release_event(int /* n_press */, double x, double y)
{
    auto page = current_page();
    if(page)
    {
        Gdk::Rectangle rect(0, 0, 0, 0);
        bool           is_first = true;
        page->on_lbutton_up(static_cast<int>(x + m_draw_buffer.get_left()),
                            static_cast<int>(y + m_draw_buffer.get_top()), static_cast<int>(x), static_cast<int>(y),
                            [&](const litehtml::position& box) { update_rect(box, rect, is_first); });
        if(!rect.has_zero_area())
        {
            m_draw_buffer.redraw_area(get_draw_function(current_page()), rect.get_x(), rect.get_y(), rect.get_width(),
                                      rect.get_height());
            queue_draw();
        }
        update_caret_timer();
    }
}

void html_widget::on_mouse_move(double x, double y)
{
    m_mouse_x          = x;
    m_mouse_y          = y;
    bool restart_timer = true;
    if(m_hscrollbar->is_visible())
    {
        if(y >= get_allocated_height() - 16)
        {
            m_hscrollbar->add_css_class("hovering");
            restart_timer = false;
        } else
        {
            m_hscrollbar->remove_css_class("hovering");
        }
    }
    if(m_vscrollbar->is_visible())
    {
        if(x >= get_allocated_width() - 16)
        {
            m_vscrollbar->add_css_class("hovering");
            restart_timer = false;
        } else
        {
            m_vscrollbar->remove_css_class("hovering");
        }
    }
    m_hscrollbar->set_opacity(1);
    m_vscrollbar->set_opacity(1);
    if(restart_timer)
    {
        restart_scrollbar_timer();
    } else
    {
        m_scrollbar_timer.disconnect();
    }

    std::shared_ptr<litebrowser::web_page> page = current_page();
    if(page)
    {
        Gdk::Rectangle rect(0, 0, 0, 0);
        bool           is_first = true;
        page->on_mouse_over(static_cast<int>(x + m_draw_buffer.get_left()),
                            static_cast<int>(y + m_draw_buffer.get_top()), static_cast<int>(x), static_cast<int>(y),
                            [&](const litehtml::position& box) { update_rect(box, rect, is_first); });
        if(!rect.has_zero_area())
        {
            m_draw_buffer.redraw_area(get_draw_function(current_page()), rect.get_x(), rect.get_y(), rect.get_width(),
                                      rect.get_height());
            queue_draw();
        }
    }
}

bool html_widget::on_key_pressed(guint keyval, guint /* keycode */, Gdk::ModifierType state)
{
    auto page = current_page();
    if(page)
    {
        litebrowser::key_event event;
        if(map_key_event(keyval, state, event))
        {
            if(event.kind == litebrowser::key_event::kind::paste)
            {
                auto clipboard = get_clipboard();
                clipboard->read_text_async(sigc::track_object(
                    [this, page, clipboard](const Glib::RefPtr<Gio::AsyncResult>& result) {
                        try
                        {
                            const Glib::ustring    text = clipboard->read_text_finish(result);
                            litebrowser::key_event paste_event;
                            paste_event.kind = litebrowser::key_event::kind::paste;
                            paste_event.text = text;
                            if(page->on_key_event(paste_event).handled)
                            {
                                m_caret_visible = true;
                                update_caret_timer();
                                queue_draw();
                            }
                        }
                        catch(...)
                        {
                        }
                    },
                    *this));
                return true;
            }

            const auto result = page->on_key_event(event);
            if(result.handled)
            {
                if(result.set_clipboard && !result.clipboard_text.empty())
                {
                    get_clipboard()->set_text(result.clipboard_text);
                }
                m_caret_visible = true;
                update_caret_timer();
                queue_draw();
                return true;
            }
        }
    }

    if(keyval == GDK_KEY_Tab || keyval == GDK_KEY_ISO_Left_Tab)
    {
        const bool backward = keyval == GDK_KEY_ISO_Left_Tab ||
                              (state & Gdk::ModifierType::SHIFT_MASK) != Gdk::ModifierType::NO_MODIFIER_MASK;
        return focus_navigate(!backward);
    }

    switch(keyval)
    {
    case GDK_KEY_KP_Page_Down:
    case GDK_KEY_Page_Down:
        m_vadjustment->set_value(m_vadjustment->get_value() + m_vadjustment->get_page_size());
        break;
    case GDK_KEY_KP_Page_Up:
    case GDK_KEY_Page_Up:
        m_vadjustment->set_value(m_vadjustment->get_value() - m_vadjustment->get_page_size());
        break;
    case GDK_KEY_KP_Down:
    case GDK_KEY_Down:
        m_vadjustment->set_value(m_vadjustment->get_value() + m_vadjustment->get_step_increment());
        break;
    case GDK_KEY_KP_Up:
    case GDK_KEY_Up:
        m_vadjustment->set_value(m_vadjustment->get_value() - m_vadjustment->get_step_increment());
        break;
    case GDK_KEY_KP_Home:
    case GDK_KEY_Home:
        m_vadjustment->set_value(0);
        break;
    case GDK_KEY_KP_End:
    case GDK_KEY_End:
        m_vadjustment->set_value(m_vadjustment->get_upper());
        break;
    case GDK_KEY_KP_Left:
    case GDK_KEY_Left:
        m_hadjustment->set_value(m_hadjustment->get_value() - m_hadjustment->get_step_increment());
        break;
    case GDK_KEY_KP_Right:
    case GDK_KEY_Right:
        m_hadjustment->set_value(m_hadjustment->get_value() + m_hadjustment->get_step_increment());
        break;

    default:
        break;
    }

    return false;
}

bool html_widget::focus_navigate(bool forward)
{
    std::shared_ptr<litebrowser::web_page> page = current_page();
    if(!page)
    {
        return false;
    }

    litehtml::position::vector boxes;

    const std::function<void(const litehtml::position&)> redraw = [&boxes](const litehtml::position& box) {
        boxes.push_back(box);
    };

    const std::optional<litehtml::position> pos = forward ? page->focus_next(redraw) : page->focus_prev(redraw);
    if(!pos)
    {
        return false;
    }

    update_view_port(page);
    if(!boxes.empty())
    {
        redraw_boxes(boxes);
    }
    scroll_into_view(*pos);
    update_caret_timer();
    return true;
}

void html_widget::scroll_into_view(const litehtml::position& box)
{
    const int left        = m_draw_buffer.get_left();
    const int top         = m_draw_buffer.get_top();
    const int view_width  = m_draw_buffer.get_width();
    const int view_height = m_draw_buffer.get_height();
    if(view_width <= 0 || view_height <= 0)
    {
        return;
    }

    const int box_left   = static_cast<int>(box.left());
    const int box_right  = static_cast<int>(box.right());
    const int box_top    = static_cast<int>(box.top());
    const int box_bottom = static_cast<int>(box.bottom());

    int new_left = left;
    int new_top  = top;

    if(box_left < left)
    {
        new_left = box_left;
    } else if(box_right > left + view_width)
    {
        new_left = box_right - view_width;
    }
    if(box_top < top)
    {
        new_top = box_top;
    } else if(box_bottom > top + view_height)
    {
        new_top = box_bottom - view_height;
    }

    if(new_left != left || new_top != top)
    {
        scroll_to(new_left, new_top);
    }
}

void html_widget::update_cursor()
{
    std::shared_ptr<litebrowser::web_page> page = current_page();
    if(page)
    {
        if(page->get_cursor() == "auto")
        {
            set_cursor("default");
        } else
        {
            set_cursor(page->get_cursor());
        }
    } else
    {
        set_cursor("default");
    }
}

void html_widget::size_allocate_vfunc(int width, int height, int /* baseline */)
{
    allocate_scrollbars(width, height);
    std::shared_ptr<litebrowser::web_page> page = current_page();
    if(page)
    {
        if(m_rendered_width != width || m_rendered_height != height)
        {
            m_rendered_width  = width;
            m_rendered_height = height;
            m_draw_buffer.on_size_allocate(get_draw_function(page), width, height);
            page->media_changed();
            page->render(m_rendered_width);
            update_view_port(page);
            m_draw_buffer.redraw(get_draw_function(page));
            queue_draw();
        }
    } else
    {
        m_draw_buffer.on_size_allocate(get_draw_function(page), width, height);
    }
}

void html_widget::allocate_scrollbars(int width, int height)
{
    m_hadjustment->set_page_size(width);
    m_vadjustment->set_page_size(height);
    if(m_vadjustment->get_value() > m_vadjustment->get_upper() - m_vadjustment->get_page_size())
    {
        m_vadjustment->set_value(m_vadjustment->get_upper() - m_vadjustment->get_page_size());
    }
    if(m_hadjustment->get_value() > m_hadjustment->get_upper() - m_hadjustment->get_page_size())
    {
        m_hadjustment->set_value(m_hadjustment->get_upper() - m_hadjustment->get_page_size());
    }

    int minimum = 0, natural = 0, m_baseline = 0, n_baseline = 0;

    m_vscrollbar->measure(Gtk::Orientation::HORIZONTAL, -1, minimum, natural, m_baseline, n_baseline);
    Gtk::Allocation vscrollbar_allocation(width - natural, 0, natural, height);
    m_vscrollbar->size_allocate(vscrollbar_allocation, -1);

    minimum = 0, natural = 0, m_baseline = 0, n_baseline = 0;
    m_hscrollbar->measure(Gtk::Orientation::VERTICAL, -1, minimum, natural, m_baseline, n_baseline);
    Gtk::Allocation hscrollbar_allocation(0, height - natural, width, natural);
    m_hscrollbar->size_allocate(hscrollbar_allocation, -1);
}

void html_widget::on_vadjustment_changed()
{
    auto page = current_page();
    m_draw_buffer.on_scroll(get_draw_function(page), static_cast<int>(m_hadjustment->get_value()),
                            static_cast<int>(m_vadjustment->get_value()),
                            page ? page->get_fixed_boxes() : litehtml::position::vector{});

    queue_draw();
}

void html_widget::on_hadjustment_changed()
{
    auto page = current_page();
    m_draw_buffer.on_scroll(get_draw_function(page), static_cast<int>(m_hadjustment->get_value()),
                            static_cast<int>(m_vadjustment->get_value()),
                            page ? page->get_fixed_boxes() : litehtml::position::vector{});

    queue_draw();
}

void html_widget::on_adjustments_changed()
{
    m_hscrollbar->set_visible(m_hadjustment->get_upper() > 0);
    m_vscrollbar->set_visible(m_vadjustment->get_upper() > 0);
}

bool html_widget::on_scroll(double dx, double dy)
{
    m_accumulated_scroll_dx += dx;
    m_accumulated_scroll_dy += dy;
    if(m_tick_id == 0)
    {
        m_tick_id = add_tick_callback(sigc::mem_fun(*this, &html_widget::on_frame_tick));
    }
    return true;
}

bool html_widget::on_frame_tick(const Glib::RefPtr<Gdk::FrameClock>& /* frame_clock */)
{
    auto   unit     = m_scroll_controller->get_unit();
    double final_dx = m_accumulated_scroll_dx;
    double final_dy = m_accumulated_scroll_dy;

    if(unit == Gdk::ScrollUnit::WHEEL)
    {
        final_dx *= 60.0;
        final_dy *= 60.0;
    }

    auto page = current_page();
    if(page)
    {
        bool dx_used = false;
        bool dy_used = false;
        auto values  = page->on_scroll(static_cast<float>(final_dx), static_cast<float>(final_dy),
                                       static_cast<int>(m_mouse_x + m_draw_buffer.get_left()),
                                       static_cast<int>(m_mouse_y + m_draw_buffer.get_top()),
                                       static_cast<int>(m_mouse_x), static_cast<int>(m_mouse_y));
        if(!values.empty())
        {
            for(const auto& val : values)
            {
                if(val.dx != 0_px)
                {
                    dx_used = true;
                }
                if(val.dy != 0_px)
                {
                    dy_used = true;
                }
                m_draw_buffer.redraw_area(get_draw_function(current_page()), val.scroll_box.left(),
                                          val.scroll_box.top(), val.scroll_box.width, val.scroll_box.height);
            }
            queue_draw();
        }
        if(!dx_used)
        {
            m_hadjustment->set_value(m_hadjustment->get_value() + final_dx);
        }
        if(!dy_used)
        {
            m_vadjustment->set_value(m_vadjustment->get_value() + final_dy);
        }
    }
    m_accumulated_scroll_dx = 0;
    m_accumulated_scroll_dy = 0;
    m_tick_id               = 0;
    return false;
}

bool html_widget::on_scrollbar_timeout()
{
    m_hscrollbar->remove_css_class("hovering");
    m_vscrollbar->remove_css_class("hovering");
    m_hscrollbar->set_opacity(0);
    m_vscrollbar->set_opacity(0);
    return false;
}

void html_widget::on_realize()
{
    Gtk::Widget::on_realize();

    if(auto native = get_native())
    {
        if(auto surface = native->get_surface())
        {
            surface->property_scale().signal_changed().connect([this]() {
                if(auto native = get_native())
                {
                    if(auto surface = native->get_surface())
                    {
                        m_draw_buffer.set_scale_factor(get_draw_function(current_page()), surface->get_scale());
                        queue_draw();
                    }
                }
            });
        }
    }
}

void html_widget::update_view_port(const std::shared_ptr<litebrowser::web_page>& page)
{
    if(page)
    {
        auto allocation = get_allocation();
        if(allocation.get_width() < page->width())
        {
            m_hadjustment->set_upper(page->width());
        } else
        {
            m_hadjustment->set_upper(0);
        }
        if(allocation.get_height() < page->height())
        {
            m_vadjustment->set_upper(page->height());
        } else
        {
            m_vadjustment->set_upper(0);
        }
    } else
    {
        m_hadjustment->set_upper(0);
        m_vadjustment->set_upper(0);
    }
}

void html_widget::restart_scrollbar_timer()
{
    if(m_scrollbar_timer)
    {
        m_scrollbar_timer.disconnect();
    }

    m_scrollbar_timer =
        Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &html_widget::on_scrollbar_timeout), 2);
}

void html_widget::redraw_boxes(const litehtml::position::vector& boxes)
{
    if(boxes.empty())
    {
        return;
    }

    Gdk::Rectangle rect(0, 0, 0, 0);
    bool           is_first = true;
    for(const auto& pos : boxes)
    {
        if(is_first)
        {
            rect     = Gdk::Rectangle(pos.x, pos.y, pos.width, pos.height);
            is_first = false;
        } else
        {
            rect.join(Gdk::Rectangle(pos.x, pos.y, pos.width, pos.height));
        }
    }

    if(!rect.has_zero_area())
    {
        m_draw_buffer.redraw_area(get_draw_function(current_page()), rect.get_x(), rect.get_y(), rect.get_width(),
                                  rect.get_height());
        queue_draw();
    }
}

int html_widget::get_render_width()
{
    return m_draw_buffer.get_width();
}

void html_widget::on_page_loaded(uint64_t web_page_id)
{
    std::string url;
    {
        std::lock_guard<std::mutex> lock(m_page_mutex);
        if(m_next_page->id() != web_page_id)
        {
            return;
        }
        m_current_page = m_next_page;
        m_next_page    = nullptr;
        url            = m_current_page->url();
        update_view_port(m_current_page);
    }
    scroll_to(0, 0);
    on_redraw();
    m_sig_set_address.emit(url);
    m_sig_update_state.emit(get_state());
}

void html_widget::show_fragment(const std::string& fragment)
{
    std::shared_ptr<litebrowser::web_page> page = current_page();
    if(page)
    {
        page->show_fragment(fragment);
    }
}

void html_widget::open_url(const std::string& url)
{
    std::string fragment;
    std::string s_url = url;

    m_sig_set_address.emit(url);

    std::string::size_type fragment_pos = s_url.find_first_of(L'#');
    if(fragment_pos != std::wstring::npos)
    {
        fragment = s_url.substr(fragment_pos + 1);
        s_url.erase(fragment_pos);
    }

    bool open_hash_only = false;
    bool reload         = false;

    auto current_url = m_history.current();
    fragment_pos     = current_url.find_first_of(L'#');
    if(fragment_pos != std::wstring::npos)
    {
        current_url.erase(fragment_pos);
    }

    if(!current_url.empty())
    {
        if(m_history.current() != url)
        {
            if(current_url == s_url)
            {
                open_hash_only = true;
            }
        } else
        {
            reload = true;
        }
    }
    if(!open_hash_only)
    {
        open_page(url, fragment);
    } else
    {
        show_fragment(fragment);
    }
    if(!reload)
    {
        m_history.url_opened(url);
    }
    m_sig_update_state.emit(get_state());
}

void html_widget::render()
{
    std::shared_ptr<litebrowser::web_page> page = current_page();
    if(page)
    {
        page->render(m_draw_buffer.get_width());
        update_view_port(page);
        m_draw_buffer.redraw(get_draw_function(page));
        queue_draw();
    }
}

bool html_widget::on_close()
{
    if(m_current_page)
    {
        m_current_page->stop_loading();
    }
    if(m_next_page)
    {
        m_next_page->stop_loading();
    }
    return false;
}

void html_widget::go_forward()
{
    std::string url;
    if(m_history.forward(url))
    {
        open_url(url);
    }
}

void html_widget::go_back()
{
    std::string url;
    if(m_history.back(url))
    {
        open_url(url);
    }
}

uint32_t html_widget::get_state()
{
    uint32_t    ret = 0;
    std::string url;
    if(m_history.back(url))
    {
        ret |= page_state_has_back;
    }
    if(m_history.forward(url))
    {
        ret |= page_state_has_forward;
    }
    {
        std::lock_guard<std::mutex> lock(m_page_mutex);
        if(m_next_page)
        {
            ret |= page_state_downloading;
        }
    }
    if(!(ret & page_state_downloading))
    {
        std::shared_ptr<litebrowser::web_page> page = current_page();
        if(page)
        {
            if(page->is_downloading())
            {
                ret |= page_state_downloading;
            }
        }
    }
    return ret;
}

void html_widget::stop_download()
{
    std::lock_guard<std::mutex> lock(m_page_mutex);
    if(m_next_page)
    {
        m_next_page->stop_loading();
    } else if(m_current_page)
    {
        m_current_page->stop_loading();
    }
}

void html_widget::reload()
{
    std::shared_ptr<litebrowser::web_page> page = current_page();
    if(page)
    {
        open_url(page->url());
    }
}

std::string html_widget::get_html_source()
{
    std::lock_guard<std::mutex> lock(m_page_mutex);
    if(m_current_page)
    {
        return m_current_page->get_html_source();
    }
    return {};
}
