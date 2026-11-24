#include <litehtml/url_path.h>
#include <litehtml/url.h>
#include <litehtml/el_select.h>
#include "control_text_layout.h"
#include "web_page.h"
#include <algorithm>
#include <cctype>
#include <cmath>

using litehtml::operator""_px;

namespace
{
    size_t utf8_next(const std::string& text, size_t pos)
    {
        if(pos >= text.size())
        {
            return text.size();
        }
        int index = static_cast<int>(pos);
        litehtml::read_utf8_char(text, index);
        return static_cast<size_t>(index);
    }

    size_t utf8_prev(const std::string& text, size_t pos)
    {
        if(pos == 0)
        {
            return 0;
        }
        int index = static_cast<int>(std::min(pos, text.size()));
        litehtml::prev_utf8_char(text, index);
        return static_cast<size_t>(index);
    }

    bool has_selection(const litehtml::text_control_data& data)
    {
        return data.selection_start != data.selection_end;
    }

    void erase_selection(litehtml::text_control_data& data)
    {
        if(!has_selection(data))
        {
            return;
        }
        auto start = std::min(data.selection_start, data.selection_end);
        auto end   = std::max(data.selection_start, data.selection_end);
        data.text.erase(start, end - start);
        data.caret           = start;
        data.selection_start = data.selection_end = start;
    }

    void insert_text(litehtml::text_control_data& data, const std::string& text)
    {
        if(text.empty())
        {
            return;
        }
        erase_selection(data);
        data.text.insert(data.caret, text);
        data.caret           += text.size();
        data.selection_start = data.selection_end = data.caret;
    }

    size_t offset_for_x(const litehtml::document& doc, litehtml::uint_ptr font, const std::string& text,
                        const litehtml::control_text::line& line, litehtml::pixel_t x)
    {
        size_t best      = line.start;
        float  best_diff = 1e30f;
        for(size_t pos = line.start; pos <= line.end; pos = utf8_next(text, pos))
        {
            const auto width = litehtml::control_text::measure(doc, font, text, line.start, pos);
            const auto diff  = std::abs(static_cast<float>(width - x));
            if(diff < best_diff)
            {
                best_diff = diff;
                best      = pos;
            }
            if(pos == line.end)
            {
                break;
            }
        }
        return best;
    }

    struct listbox_item
    {
        std::string label;
        int         index    = -1;
        bool        group    = false;
        bool        disabled = false;
    };

    std::string lower_ascii(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return text;
    }

    std::vector<listbox_item> listbox_items(const litehtml::select_control_data& data)
    {
        std::vector<listbox_item> items;
        for(const auto& group : data.groups)
        {
            if(!group.label.empty())
            {
                listbox_item item;
                item.label    = group.label;
                item.group    = true;
                item.disabled = group.disabled;
                items.push_back(std::move(item));
            }
            for(const auto& option : group.options)
            {
                listbox_item item;
                item.label    = option.label;
                item.index    = option.index;
                item.disabled = option.disabled || group.disabled;
                items.push_back(std::move(item));
            }
        }
        return items;
    }

    int first_enabled_from(const std::vector<listbox_item>& items, int start, int direction)
    {
        if(items.empty())
        {
            return -1;
        }
        if(direction >= 0)
        {
            for(int i = std::max(0, start); i < static_cast<int>(items.size()); ++i)
            {
                if(!items[static_cast<size_t>(i)].group && !items[static_cast<size_t>(i)].disabled)
                {
                    return items[static_cast<size_t>(i)].index;
                }
            }
        } else
        {
            for(int i = std::min(start, static_cast<int>(items.size()) - 1); i >= 0; --i)
            {
                if(!items[static_cast<size_t>(i)].group && !items[static_cast<size_t>(i)].disabled)
                {
                    return items[static_cast<size_t>(i)].index;
                }
            }
        }
        return -1;
    }

    int option_position(const std::vector<listbox_item>& items, int option_index)
    {
        for(size_t i = 0; i < items.size(); ++i)
        {
            if(items[i].index == option_index && !items[i].group)
            {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    int first_selected_option(const litehtml::select_control_data& data)
    {
        for(const auto& group : data.groups)
        {
            for(const auto& option : group.options)
            {
                if(option.selected)
                {
                    return option.index;
                }
            }
        }
        return -1;
    }

    void select_only(litehtml::select_control_data& data, int option_index)
    {
        for(auto& group : data.groups)
        {
            for(auto& option : group.options)
            {
                option.selected = option.index == option_index && !option.disabled && !group.disabled;
            }
        }
    }

    void toggle_option(litehtml::select_control_data& data, int option_index)
    {
        for(auto& group : data.groups)
        {
            for(auto& option : group.options)
            {
                if(option.index == option_index && !option.disabled && !group.disabled)
                {
                    option.selected = !option.selected;
                    return;
                }
            }
        }
    }

    void select_range(litehtml::select_control_data& data, int anchor, int active)
    {
        const int first = std::min(anchor, active);
        const int last  = std::max(anchor, active);
        for(auto& group : data.groups)
        {
            for(auto& option : group.options)
            {
                const bool in_range = option.index >= first && option.index <= last;
                option.selected     = in_range && !option.disabled && !group.disabled;
            }
        }
    }
} // namespace

void litebrowser::text_file::on_data(void* data, size_t len, size_t /*downloaded*/, size_t /*total*/)
{
    stream.write(static_cast<const char*>(data), static_cast<std::streamsize>(len));
}

void litebrowser::text_file::on_page_downloaded(u_int32_t http_status, u_int32_t err_code,
                                                const std::string& /*err_text*/)
{
    data_ready = err_code == 0 && (http_status == 200 || http_status == 0);
    wait_mutex.unlock();
}

void litebrowser::web_page::open(const std::string& url, const std::string& fragment)
{
    open_internal(url, fragment, nullptr);
}

void litebrowser::web_page::open_post(const std::string& url, const litehtml::form_data& data,
                                      const std::string& fragment)
{
    open_internal(url, fragment, &data.fields);
}

void litebrowser::web_page::open_internal(const std::string& url, const std::string& fragment,
                                          const std::vector<litehtml::form_field>* post_data)
{
    litehtml::url l_url(url);

    if(!l_url.has_scheme())
    {
        if(url.front() != '/')
        {
            m_url = "http://" + url;
        } else
        {
            m_url = url;
        }
    } else
    {
        m_url = url;
    }
    m_base_url = m_url;
    m_fragment = fragment;

    auto data       = std::make_shared<text_file>();
    auto cb_on_data = [data](void* in_data, size_t len, size_t /*downloaded*/, size_t /*total*/) {
        data->on_data(in_data, len, 0, 0);
    };
    auto weak_this    = weak_from_this();
    auto cb_on_finish = [weak_this, data](u_int32_t http_status, u_int32_t err_code, const std::string& err_text,
                                          const std::string& url) {
        if(auto shared_this = weak_this.lock())
        {
            shared_this->on_page_downloaded(data, http_status, err_code, err_text, url);
        }
    };

    if(post_data)
    {
        std::vector<std::pair<std::string, std::string>> fields;
        fields.reserve(post_data->size());
        for(const auto& field : *post_data)
        {
            fields.emplace_back(litehtml::url::encode(field.name), litehtml::url::encode(field.value));
        }
        m_requests_pool.enqueue(m_url, fields, cb_on_data, cb_on_finish);
    } else
    {
        m_requests_pool.enqueue(m_url, cb_on_data, cb_on_finish);
    }
}

void litebrowser::web_page::get_viewport(litehtml::position& viewport) const
{
    m_html_host->get_viewport(viewport);
}

void litebrowser::web_page::on_anchor_click(const char* url, const litehtml::element::ptr& /*el*/)
{
    if(url)
    {
        make_url(url, m_base_url.c_str(), m_clicked_url);
    }
}

void litebrowser::web_page::set_cursor(const char* cursor)
{
    if(cursor)
    {
        if(m_cursor != cursor)
        {
            m_cursor = cursor;
            m_html_host->update_cursor();
        }
    }
}

void litebrowser::web_page::import_css(std::string& text, const std::string& url, std::string& baseurl)
{
    std::string css_url;
    make_url(url.c_str(), baseurl.c_str(), css_url);

    auto data       = std::make_shared<text_file>();
    auto cb_on_data = [data](void* in_data, size_t len, size_t /*downloaded*/, size_t /*total*/) {
        data->on_data(in_data, len, 0, 0);
    };
    auto cb_on_finish = [data](u_int32_t http_status, u_int32_t err_code, const std::string& err_text,
                               const std::string& /*url*/) {
        data->on_page_downloaded(http_status, err_code, err_text);
    };
    http_request(css_url, cb_on_data, cb_on_finish);
    data->wait();
    text = data->str();
    if(!text.empty())
    {
        baseurl = css_url;
    }
}

void litebrowser::web_page::set_caption(const char* caption)
{
    m_notify->on_set_caption(caption);
}

void litebrowser::web_page::set_base_url(const char* base_url)
{
    if(base_url)
    {
        m_base_url = litehtml::resolve(litehtml::url(m_url), litehtml::url(base_url)).str();
    } else
    {
        m_base_url = m_url;
    }
}

cairo_surface_t* litebrowser::web_page::get_image(const std::string& url)
{
    return m_images.get_image(url);
}

void litebrowser::web_page::show_fragment(const std::string& fragment)
{
    std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
    if(fragment.empty() || !m_html)
    {
        m_html_host->scroll_to(0, 0);
    } else
    {
        auto                   escaped_hash = litehtml::get_escaped_string(fragment);
        std::string            selector     = ":is([id=\"" + escaped_hash + "\"],[name=\"" + escaped_hash + "\"])";
        litehtml::element::ptr el           = m_html->root()->select_one(selector);
        if(el)
        {
            litehtml::position pos = el->get_placement();
            m_html_host->scroll_to(0, pos.top());
        }
    }
}

void litebrowser::web_page::make_url(const char* url, const char* basepath, std::string& out)
{
    if(!basepath || !basepath[0])
    {
        if(!m_base_url.empty())
        {
            out = litehtml::resolve(litehtml::url(m_base_url), litehtml::url(url)).str();
        } else
        {
            out = url;
        }
    } else
    {
        out = litehtml::resolve(litehtml::url(basepath), litehtml::url(url)).str();
    }
}

void litebrowser::web_page::on_mouse_over(int x, int y, int client_x, int client_y,
                                          const std::function<void(const litehtml::position&)>& redraw_box)
{
    std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
    if(m_html)
    {
        litehtml::position::vector redraw_boxes;
        if(m_html->on_mouse_over(x, y, client_x, client_y, redraw_box))
        {
            m_html_host->redraw_boxes(redraw_boxes);
        }
    }
}

void litebrowser::web_page::on_lbutton_down(int x, int y, int client_x, int client_y,
                                            const std::function<void(const litehtml::position&)>& redraw_box)
{
    std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
    if(m_html)
    {
        litehtml::position::vector redraw_boxes;
        if(m_html->on_lbutton_down(x, y, client_x, client_y, redraw_box))
        {
            m_html_host->redraw_boxes(redraw_boxes);
        }
    }
}

void litebrowser::web_page::on_lbutton_up(int x, int y, int client_x, int client_y,
                                          const std::function<void(const litehtml::position&)>& redraw_box)
{
    if(m_html)
    {
        {
            std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);

            litehtml::position::vector redraw_boxes;
            m_clicked_url.clear();
            if(m_html->on_lbutton_up(x, y, client_x, client_y, redraw_box))
            {
                m_html_host->redraw_boxes(redraw_boxes);
            }
        }
        if(!m_clicked_url.empty())
        {
            m_html_host->open_url(m_clicked_url);
        }
    }
    process_pending_submit();
}

std::optional<litehtml::position> litebrowser::web_page::focus_next(
    const std::function<void(const litehtml::position&)>& redraw_box)
{
    std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
    return m_html ? m_html->focus_next(redraw_box) : std::nullopt;
}

std::optional<litehtml::position> litebrowser::web_page::focus_prev(
    const std::function<void(const litehtml::position&)>& redraw_box)
{
    std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
    return m_html ? m_html->focus_prev(redraw_box) : std::nullopt;
}

void litebrowser::web_page::update_focused(const litehtml::control_data& data)
{
    litehtml::position::vector boxes;
    const auto                 redraw = [&boxes](const litehtml::position& box) { boxes.push_back(box); };
    {
        std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
        if(const auto* text = std::get_if<litehtml::text_control_data>(&data))
        {
            m_text_data     = *text;
            m_has_text_data = true;
        }
        if(m_html)
        {
            m_html->update_focused(data, redraw);
        }
    }
    if(!boxes.empty())
    {
        m_html_host->redraw_boxes(boxes);
    }
}

bool litebrowser::web_page::has_text_focus() const
{
    std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
    return m_has_text_data;
}

void litebrowser::web_page::set_caret_visible(bool visible)
{
    litehtml::position::vector boxes;
    const auto                 redraw = [&boxes](const litehtml::position& box) { boxes.push_back(box); };
    {
        std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
        if(!m_html || !m_has_text_data || m_text_data.caret_visible == visible)
        {
            return;
        }
        m_text_data.caret_visible = visible;
        m_html->update_focused(m_text_data, redraw);
    }
    if(!boxes.empty())
    {
        m_html_host->redraw_boxes(boxes);
    }
}

litebrowser::key_event_result litebrowser::web_page::on_key_event(const key_event& event)
{
    key_event_result           result;
    litehtml::position::vector boxes;
    const auto                 redraw = [&boxes](const litehtml::position& box) { boxes.push_back(box); };

    {
        std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
        if(!m_html || !m_focused_control)
        {
            return result;
        }

        const bool is_textarea = m_focused_control_type == litehtml::control_type::textarea;
        if(m_has_text_data)
        {
            auto& data        = m_text_data;
            bool  text_edited = false;
            bool  moved       = false;
            auto  old_caret   = data.caret;

            const auto selection_min = [&data]() { return std::min(data.selection_start, data.selection_end); };
            const auto selection_max = [&data]() { return std::max(data.selection_start, data.selection_end); };

            const auto move_to = [&](size_t pos) {
                old_caret  = data.caret;
                data.caret = std::min(pos, data.text.size());
                if(event.shift)
                {
                    if(!has_selection(data))
                    {
                        data.selection_start = old_caret;
                    }
                    data.selection_end = data.caret;
                } else
                {
                    data.selection_start = data.selection_end = data.caret;
                }
                moved = true;
            };

            switch(event.kind)
            {
            case key_event::kind::character:
            case key_event::kind::space:
            case key_event::kind::paste:
                {
                    std::string insert = event.text;
                    if(event.kind == key_event::kind::space)
                    {
                        insert = " ";
                    }
                    if(!is_textarea)
                    {
                        insert.erase(std::remove_if(insert.begin(), insert.end(),
                                                    [](char ch) { return ch == '\n' || ch == '\r'; }),
                                     insert.end());
                    }
                    insert_text(data, insert);
                    text_edited = true;
                }
                break;
            case key_event::kind::backspace:
                if(has_selection(data))
                {
                    erase_selection(data);
                    text_edited = true;
                } else if(data.caret > 0)
                {
                    const auto prev = utf8_prev(data.text, data.caret);
                    data.text.erase(prev, data.caret - prev);
                    data.caret           = prev;
                    data.selection_start = data.selection_end = prev;
                    text_edited                               = true;
                }
                break;
            case key_event::kind::delete_:
                if(has_selection(data))
                {
                    erase_selection(data);
                    text_edited = true;
                } else if(data.caret < data.text.size())
                {
                    const auto next = utf8_next(data.text, data.caret);
                    data.text.erase(data.caret, next - data.caret);
                    data.selection_start = data.selection_end = data.caret;
                    text_edited                               = true;
                }
                break;
            case key_event::kind::left:
                if(has_selection(data) && !event.shift)
                {
                    move_to(selection_min());
                } else
                {
                    move_to(utf8_prev(data.text, data.caret));
                }
                break;
            case key_event::kind::right:
                if(has_selection(data) && !event.shift)
                {
                    move_to(selection_max());
                } else
                {
                    move_to(utf8_next(data.text, data.caret));
                }
                break;
            case key_event::kind::home:
            case key_event::kind::end:
            case key_event::kind::up:
            case key_event::kind::down:
                {
                    const auto font  = m_focused_control->css().get_font();
                    const auto width = std::max(1_px, m_focused_control->get_placement().width);
                    const auto lines =
                        litehtml::control_text::layout_lines(*m_html, font, data.text, is_textarea ? width : 0_px);
                    const auto caret =
                        litehtml::control_text::caret_position(*m_html, font, lines, data.text, data.caret);

                    if(event.kind == key_event::kind::home)
                    {
                        move_to(lines[caret.line].start);
                    } else if(event.kind == key_event::kind::end)
                    {
                        move_to(lines[caret.line].end);
                    } else if(event.kind == key_event::kind::up && caret.line > 0)
                    {
                        move_to(offset_for_x(*m_html, font, data.text, lines[caret.line - 1], caret.x));
                    } else if(event.kind == key_event::kind::down && caret.line + 1 < lines.size())
                    {
                        move_to(offset_for_x(*m_html, font, data.text, lines[caret.line + 1], caret.x));
                    } else
                    {
                        moved = true;
                    }
                }
                break;
            case key_event::kind::enter:
                if(is_textarea)
                {
                    insert_text(data, "\n");
                    text_edited = true;
                } else
                {
                    m_html->submit_focused_form(redraw);
                    result.handled = true;
                }
                break;
            case key_event::kind::select_all:
                data.selection_start = 0;
                data.selection_end   = data.text.size();
                data.caret           = data.text.size();
                moved                = true;
                break;
            case key_event::kind::copy:
                if(has_selection(data))
                {
                    result.clipboard_text = data.text.substr(selection_min(), selection_max() - selection_min());
                    result.set_clipboard  = true;
                    result.handled        = true;
                    return result;
                }
                break;
            case key_event::kind::cut:
                if(has_selection(data))
                {
                    result.clipboard_text = data.text.substr(selection_min(), selection_max() - selection_min());
                    result.set_clipboard  = true;
                    erase_selection(data);
                    text_edited = true;
                }
                break;
            case key_event::kind::escape:
                data.selection_start = data.selection_end = data.caret;
                moved                                     = true;
                break;
            case key_event::kind::page_up:
            case key_event::kind::page_down:
                // Text controls keep the previous page-scrolling behavior.
                break;
            }

            if(text_edited || moved)
            {
                data.caret_visible   = true;
                data.caret           = std::min(data.caret, data.text.size());
                data.selection_start = std::min(data.selection_start, data.text.size());
                data.selection_end   = std::min(data.selection_end, data.text.size());
                m_html->update_focused(data, redraw);
                result.handled = true;
            } else if(result.handled)
            {
                // submit handled above
            }
        } else
        {
            const bool activate = event.kind == key_event::kind::space || event.kind == key_event::kind::enter;
            switch(m_focused_control_type)
            {
            case litehtml::control_type::checkbox:
            case litehtml::control_type::radio:
                if(event.kind == key_event::kind::space)
                {
                    bool       checked = false;
                    const auto current = m_focused_control->get_control_data();
                    if(const auto* checked_data = std::get_if<bool>(&current))
                    {
                        checked = *checked_data;
                    }
                    m_html->update_focused(m_focused_control_type == litehtml::control_type::checkbox ? !checked : true,
                                           redraw);
                    result.handled = true;
                }
                break;
            case litehtml::control_type::select:
                if(!handle_listbox_key(event, result, redraw) &&
                   (event.kind == key_event::kind::space || event.kind == key_event::kind::enter ||
                    event.kind == key_event::kind::down))
                {
                    m_focused_control->on_click();
                    result.handled = true;
                }
                break;
            case litehtml::control_type::button:
            case litehtml::control_type::submit:
            case litehtml::control_type::reset:
                if(activate)
                {
                    m_focused_control->on_click();
                    m_html->flush_redraw(redraw);
                    result.handled = true;
                }
                break;
            default:
                break;
            }
        }
    }

    if(!boxes.empty())
    {
        m_html_host->redraw_boxes(boxes);
    }
    process_pending_submit();
    return result;
}

bool litebrowser::web_page::handle_listbox_key(const key_event& event, key_event_result& result,
                                               const std::function<void(const litehtml::position&)>& redraw)
{
    if(!m_html || !m_focused_control)
    {
        return false;
    }

    const auto  current_control = m_focused_control->get_control_data();
    const auto* select          = std::get_if<litehtml::select_control_data>(&current_control);
    if(!select || (!select->multiple && select->size <= 1))
    {
        return false;
    }

    auto*     widget      = dynamic_cast<litehtml::el_select*>(m_focused_control.get());
    const int core_active = widget ? widget->active_option() : -1;
    if(core_active != m_select_last_active)
    {
        // The active option was changed outside the keyboard handler, for example by a mouse click.
        m_select_anchor      = core_active;
        m_select_last_active = core_active;
        m_select_typeahead.clear();
    }

    int active = core_active;
    if(active < 0)
    {
        active = first_selected_option(*select);
    }

    const auto items = listbox_items(*select);
    if(items.empty())
    {
        result.handled = true;
        return true;
    }

    const auto step_enabled = [&items](int current, int direction) {
        if(current < 0)
        {
            return first_enabled_from(items, direction > 0 ? 0 : static_cast<int>(items.size()) - 1, direction);
        }
        const int pos = option_position(items, current);
        if(pos < 0)
        {
            return first_enabled_from(items, direction > 0 ? 0 : static_cast<int>(items.size()) - 1, direction);
        }
        const int found = first_enabled_from(items, pos + direction, direction);
        return found >= 0 ? found : current;
    };

    const auto page_enabled = [&items, select](int current, int direction) {
        const int step = std::max(1, select->size - 1);
        int       pos  = 0;
        if(current < 0)
        {
            pos = direction > 0 ? 0 : static_cast<int>(items.size()) - 1;
        } else
        {
            const int current_pos = option_position(items, current);
            pos                   = (current_pos < 0 ? 0 : current_pos) + direction * step;
            pos                   = std::clamp(pos, 0, static_cast<int>(items.size()) - 1);
        }
        int found = first_enabled_from(items, pos, direction);
        if(found < 0)
        {
            found = first_enabled_from(items, direction > 0 ? static_cast<int>(items.size()) - 1 : 0, -direction);
        }
        return found;
    };

    litehtml::select_control_data updated    = *select;
    int                           new_active = active;

    switch(event.kind)
    {
    case key_event::kind::up:
    case key_event::kind::down:
    case key_event::kind::home:
    case key_event::kind::end:
    case key_event::kind::page_up:
    case key_event::kind::page_down:
        {
            m_select_typeahead.clear();
            switch(event.kind)
            {
            case key_event::kind::up:
                new_active = step_enabled(active, -1);
                break;
            case key_event::kind::down:
                new_active = step_enabled(active, 1);
                break;
            case key_event::kind::home:
                new_active = first_enabled_from(items, 0, 1);
                break;
            case key_event::kind::end:
                new_active = first_enabled_from(items, static_cast<int>(items.size()) - 1, -1);
                break;
            case key_event::kind::page_up:
                new_active = page_enabled(active, -1);
                break;
            case key_event::kind::page_down:
                new_active = page_enabled(active, 1);
                break;
            default:
                break;
            }

            result.handled = true;
            if(new_active < 0)
            {
                return true;
            }

            const bool ctrl_only = event.ctrl && !event.shift && select->multiple;
            const bool extending = event.shift && select->multiple;
            if(ctrl_only)
            {
                // Move the keyboard cursor without changing the selection.
            } else if(extending)
            {
                int anchor = m_select_anchor;
                if(anchor < 0)
                {
                    anchor = active >= 0 ? active : new_active;
                }
                select_range(updated, anchor, new_active);
                m_select_anchor = anchor;
            } else
            {
                select_only(updated, new_active);
                m_select_anchor = new_active;
            }
            break;
        }

    case key_event::kind::space:
        m_select_typeahead.clear();
        result.handled = true;
        if(active < 0)
        {
            return true;
        }
        if(select->multiple)
        {
            toggle_option(updated, active);
        } else
        {
            select_only(updated, active);
        }
        m_select_anchor = active;
        break;

    case key_event::kind::enter:
    case key_event::kind::escape:
        m_select_typeahead.clear();
        result.handled = true;
        return true;

    case key_event::kind::character:
        {
            if(event.text.empty())
            {
                return false;
            }

            const auto now = std::chrono::steady_clock::now();
            if(m_select_typeahead.empty() || now - m_select_typeahead_at > std::chrono::seconds(1))
            {
                m_select_typeahead = event.text;
            } else
            {
                m_select_typeahead += event.text;
            }
            m_select_typeahead_at = now;

            const auto find_match = [&items](const std::string& prefix) {
                const auto lowered = lower_ascii(prefix);
                for(const auto& item : items)
                {
                    if(item.group || item.disabled)
                    {
                        continue;
                    }
                    const auto label = lower_ascii(item.label);
                    if(label.size() >= lowered.size() && label.compare(0, lowered.size(), lowered) == 0)
                    {
                        return item.index;
                    }
                }
                return -1;
            };

            int match = find_match(m_select_typeahead);
            if(match < 0 && m_select_typeahead.size() > event.text.size())
            {
                m_select_typeahead = event.text;
                match              = find_match(m_select_typeahead);
            }

            result.handled = true;
            if(match < 0)
            {
                return true;
            }

            select_only(updated, match);
            m_select_anchor      = match;
            m_select_last_active = match;
            if(widget)
            {
                widget->set_active_option(match);
                widget->scroll_option_into_view(match);
            }
            m_html->update_focused(updated, redraw);
            return true;
        }

    default:
        return false;
    }

    if(widget)
    {
        widget->set_active_option(new_active);
        widget->scroll_option_into_view(new_active);
    }
    m_select_last_active = new_active;
    m_html->update_focused(updated, redraw);
    return true;
}

void litebrowser::web_page::submit_focused_form()
{
    litehtml::position::vector boxes;
    const auto                 redraw = [&boxes](const litehtml::position& box) { boxes.push_back(box); };
    {
        std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
        if(m_html)
        {
            m_html->submit_focused_form(redraw);
        }
    }
    if(!boxes.empty())
    {
        m_html_host->redraw_boxes(boxes);
    }
    process_pending_submit();
}

void litebrowser::web_page::show_focused_select_popup()
{
    std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
    if(m_focused_control && m_focused_control_type == litehtml::control_type::select)
    {
        m_focused_control->on_click();
    }
}

void litebrowser::web_page::apply_select(const litehtml::select_control_data& data)
{
    update_focused(data);
}

void litebrowser::web_page::process_pending_submit()
{
    std::optional<litehtml::form_data> data;
    {
        std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
        data = std::move(m_pending_submit);
        m_pending_submit.reset();
    }
    if(!data)
    {
        return;
    }

    if(data->method.empty())
    {
        data->method = "get";
    }
    if(data->enctype.empty())
    {
        data->enctype = "application/x-www-form-urlencoded";
    }

    if(data->action.empty())
    {
        data->action = m_url;
    } else
    {
        std::string resolved;
        make_url(data->action.c_str(), m_base_url.c_str(), resolved);
        data->action = resolved;
    }

    m_html_host->submit_form(*data);
}

void litebrowser::web_page::on_page_downloaded(const std::shared_ptr<text_file>& data, u_int32_t /*http_status*/,
                                               u_int32_t err_code, const std::string& err_text, const std::string& url)
{
    if(err_code == 0)
    {
        m_url = url;
        std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
        data->set_ready();
        m_html_source = data->str();
    } else
    {
        std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
        std::stringstream                     ss;
        ss << "<h1>Impossible to load page</h1>" << std::endl;
        ss << "<p>Error #" << err_code << ": " << err_text << "</p>" << std::endl;
        m_html_source = ss.str();
    }

    m_html = litehtml::document::createFromString(m_html_source, this);

    {
        std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
        m_focused_control.reset();
        m_focused_control_type = litehtml::control_type::none;
        m_text_data            = {};
        m_has_text_data        = false;
        m_pending_submit.reset();
        m_select_anchor      = -1;
        m_select_last_active = -1;
        m_select_typeahead.clear();
    }

    if(m_html)
    {
        std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
        int                                   render_width = m_html_host->get_render_width();
        m_html->render(render_width);
    }
    m_notify->on_page_loaded(id());
}

void litebrowser::web_page::on_image_downloaded(const std::shared_ptr<image_file>& data, u_int32_t http_status,
                                                u_int32_t err_code, const std::string& /*err_text*/,
                                                const std::string& /*url*/)
{
    data->close();
    if(!data->path().empty() && !err_code && (http_status == 200 || http_status == 0))
    {
        auto ptr = m_html_host->load_image(data->path());
        if(ptr)
        {
            m_images.add_image(data->url(), ptr);
            if(data->redraw_only())
            {
                m_notify->redraw();
            } else
            {
                m_notify->render();
            }
        }
    }
    unlink(data->path().c_str());
}

void litebrowser::web_page::load_image(const char* src, const char* baseurl, bool redraw_on_ready)
{
    std::string url;
    make_url(src, baseurl, url);

    if(m_images.reserve(url))
    {
        auto data       = std::make_shared<image_file>(url, redraw_on_ready);
        auto cb_on_data = [data](void* in_data, size_t len, size_t /*downloaded*/, size_t /*total*/) {
            data->on_data(in_data, len, 0, 0);
        };
        auto weak_this    = weak_from_this();
        auto cb_on_finish = [weak_this, data](u_int32_t http_status, u_int32_t err_code, const std::string& err_text,
                                              const std::string& url) {
            if(auto shared_this = weak_this.lock())
            {
                shared_this->on_image_downloaded(data, http_status, err_code, err_text, url);
            }
        };

        http_request(url, cb_on_data, cb_on_finish);
    }
}

void litebrowser::web_page::http_request(
    const std::string& url, const std::function<void(void*, size_t, size_t, size_t)>& cb_on_data,
    const std::function<void(u_int32_t, u_int32_t, const std::string&, const std::string&)>& cb_on_finish)
{
    m_requests_pool.enqueue(url, cb_on_data, cb_on_finish);
}

void litebrowser::web_page::on_pool_update_state()
{
    m_notify->update_state();
}

double litebrowser::web_page::get_screen_dpi() const
{
    return m_html_host->get_dpi();
}

int litebrowser::web_page::get_screen_width() const
{
    return m_html_host->get_screen_width();
}

int litebrowser::web_page::get_screen_height() const
{
    return m_html_host->get_screen_height();
}

void litebrowser::web_page::on_mouse_event(const litehtml::element::ptr& /*el*/, litehtml::mouse_event /*event*/) {}

void litebrowser::web_page::on_focus_change(const litehtml::element::ptr& el, const char* /*element_id*/,
                                            litehtml::control_type type, const litehtml::control_data& data)
{
    std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
    m_focused_control      = el;
    m_focused_control_type = type;
    m_has_text_data        = false;
    m_select_anchor        = -1;
    m_select_last_active   = -1;
    m_select_typeahead.clear();
    if(el)
    {
        if(const auto* text = std::get_if<litehtml::text_control_data>(&data))
        {
            m_text_data     = *text;
            m_has_text_data = true;
        } else if(const auto* select = std::get_if<litehtml::select_control_data>(&data))
        {
            if(select->multiple || select->size > 1)
            {
                const int active = first_selected_option(*select);
                if(auto* widget = dynamic_cast<litehtml::el_select*>(el.get()))
                {
                    widget->set_active_option(active);
                }
                m_select_anchor      = active;
                m_select_last_active = active;
            }
        }
    } else
    {
        m_focused_control_type = litehtml::control_type::none;
    }
}

void litebrowser::web_page::get_text_data(const litehtml::element::ptr& el, litehtml::text_control_data& data)
{
    std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
    if(m_has_text_data && el == m_focused_control)
    {
        data = m_text_data;
    }
}

void litebrowser::web_page::on_submit(const litehtml::form_data& data)
{
    std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
    m_pending_submit = data;
}

bool litebrowser::web_page::show_select_popup(const litehtml::element::ptr& el, const char* element_id,
                                              const litehtml::select_control_data& data)
{
    if(!m_html_host)
    {
        return false;
    }
    auto weak_this = weak_from_this();
    m_html_host->show_select_popup(el, element_id, data, [weak_this](const litehtml::select_control_data& selected) {
        if(auto self = weak_this.lock())
        {
            self->apply_select(selected);
        }
    });
    return true;
}

//////////////////////////////////////////////////////////

litebrowser::image_file::image_file(std::string url, bool redraw_on_ready) :
    m_url(std::move(url)),
    m_redraw_on_ready(redraw_on_ready)
{
}

void litebrowser::image_file::on_data(void* data, size_t len, size_t /*downloaded*/, size_t /*total*/)
{
    if(m_fd < 0)
    {
        char nameBuff[] = "/tmp/litebrowser-XXXXXX";
        m_fd            = mkstemp(nameBuff);
        if(m_fd >= 0)
        {
            m_path = nameBuff;
        }
    }
    if(m_fd >= 0)
    {
        write(m_fd, data, len);
    }
}
