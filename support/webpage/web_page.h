#ifndef LITEBROWSER_WEB_PAGE_H
#define LITEBROWSER_WEB_PAGE_H

#include <functional>
#include <memory>
#include <optional>
#include <unistd.h>
#include <chrono>
#include <sstream>
#include <string>
#include <vector>
#include "container_cairo_pango.h"
#include "html_host.h"
#include "http_requests_pool.h"
#include "cairo_images_cache.h"
#include "litehtml/types.h"

namespace litebrowser
{
    struct key_event
    {
        enum class kind
        {
            character,
            backspace,
            delete_,
            left,
            right,
            up,
            down,
            home,
            end,
            page_up,
            page_down,
            enter,
            space,
            escape,
            select_all,
            copy,
            cut,
            paste
        };

        kind        kind = kind::character;
        std::string text;
        bool        shift = false;
        bool        ctrl  = false;
    };

    struct key_event_result
    {
        bool        handled       = false;
        bool        set_clipboard = false;
        std::string clipboard_text;
    };

    class text_file
    {
        std::mutex        wait_mutex;
        std::stringstream stream;
        bool              data_ready = false;

      public:
        text_file()
        {
            wait_mutex.lock();
        }

        void set_ready()
        {
            data_ready = true;
        }

        std::string str() const
        {
            return data_ready ? stream.str() : "";
        }
        void wait()
        {
            wait_mutex.lock();
        }
        void on_data(void* data, size_t len, size_t downloaded, size_t total);
        void on_page_downloaded(u_int32_t http_status, u_int32_t err_code, const std::string& err_text);
    };

    class image_file
    {
        int         m_fd = -1;
        std::string m_path;
        std::string m_url;
        bool        m_redraw_on_ready;

      public:
        explicit image_file(std::string url, bool redraw_on_ready);
        void on_data(void* data, size_t len, size_t downloaded, size_t total);
        void close() const
        {
            if(m_fd > 0)
            {
                ::close(m_fd);
            }
        }
        [[nodiscard]]
        const std::string& path() const
        {
            return m_path;
        }

        [[nodiscard]]
        const std::string& url() const
        {
            return m_url;
        }

        [[nodiscard]]
        bool redraw_only() const
        {
            return m_redraw_on_ready;
        }
    };

    class web_page : public container_cairo_pango, public std::enable_shared_from_this<web_page>
    {
        std::string                     m_url;
        std::string                     m_base_url;
        litehtml::document::ptr         m_html;
        mutable std::recursive_mutex    m_html_mutex;
        std::string                     m_cursor;
        std::string                     m_clicked_url;
        std::string                     m_fragment;
        html_host_interface*            m_html_host;
        cairo_images_cache              m_images;
        litebrowser::http_requests_pool m_requests_pool;
        std::string                     m_html_source;

        std::shared_ptr<browser_notify_interface> m_notify;

        litehtml::element::ptr             m_focused_control;
        litehtml::control_type             m_focused_control_type = litehtml::control_type::none;
        litehtml::text_control_data        m_text_data;
        bool                               m_has_text_data = false;
        std::optional<litehtml::form_data> m_pending_submit;

        // Listbox select keyboard state.
        int                                   m_select_anchor      = -1;
        int                                   m_select_last_active = -1;
        std::string                           m_select_typeahead;
        std::chrono::steady_clock::time_point m_select_typeahead_at{};

      public:
        explicit web_page(html_host_interface* html_host, std::shared_ptr<browser_notify_interface> notify,
                          int pool_size) :
            m_html_host(html_host),
            m_requests_pool(pool_size, [this] { on_pool_update_state(); }),
            m_notify(std::move(notify))
        {
        }

        [[nodiscard]]
        uint64_t id() const
        {
            return reinterpret_cast<uint64_t>(this);
        }

        [[nodiscard]]
        const std::string& get_html_source() const
        {
            return m_html_source;
        }

        void open(const std::string& url, const std::string& fragment);
        void open_post(const std::string& url, const litehtml::form_data& data, const std::string& fragment);

        void             get_viewport(litehtml::position& viewport) const override;
        void             on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
        void             set_cursor(const char* cursor) override;
        void             import_css(std::string& text, const std::string& url, std::string& baseurl) override;
        void             set_caption(const char* caption) override;
        void             set_base_url(const char* base_url) override;
        cairo_surface_t* get_image(const std::string& url) override;
        void             make_url(const char* url, const char* basepath, std::string& out) override;
        void             load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
        void             on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event) override;
        void   on_focus_change(const litehtml::element::ptr& el, const char* element_id, litehtml::control_type type,
                               const litehtml::control_data& data) override;
        void   get_text_data(const litehtml::element::ptr& el, litehtml::text_control_data& data) override;
        void   on_submit(const litehtml::form_data& data) override;
        bool   show_select_popup(const litehtml::element::ptr& el, const char* element_id,
                                 const litehtml::select_control_data& data) override;
        double get_screen_dpi() const override;
        int    get_screen_width() const override;
        int    get_screen_height() const override;

        void show_fragment(const std::string& fragment);
        void show_fragment_and_reset()
        {
            if(!m_fragment.empty() && m_html)
            {
                show_fragment(m_fragment);
                m_fragment = "";
            }
        }

        void on_mouse_over(int x, int y, int client_x, int client_y,
                           const std::function<void(const litehtml::position&)>& redraw_box);
        void on_lbutton_down(int x, int y, int client_x, int client_y,
                             const std::function<void(const litehtml::position&)>& redraw_box);
        void on_lbutton_up(int x, int y, int client_x, int client_y,
                           const std::function<void(const litehtml::position&)>& redraw_box);
        std::optional<litehtml::position> focus_next(const std::function<void(const litehtml::position&)>& redraw_box);
        std::optional<litehtml::position> focus_prev(const std::function<void(const litehtml::position&)>& redraw_box);
        void                              update_focused(const litehtml::control_data& data);
        bool                              has_text_focus() const;
        void                              set_caret_visible(bool visible);
        key_event_result                  on_key_event(const key_event& event);
        void                              submit_focused_form();
        void                              show_focused_select_popup();

        [[nodiscard]]
        const std::string& get_cursor() const
        {
            return m_cursor;
        }

        void draw(litehtml::uint_ptr hdc, int x, int y, const litehtml::position* clip)
        {
            std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
            if(m_html)
            {
                m_html->draw(hdc, x, y, clip);
            }
        }

        int render(int max_width)
        {
            std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
            return m_html ? m_html->render(max_width).value() : 0;
        }

        [[nodiscard]]
        const std::string& url() const
        {
            return m_url;
        }

        [[nodiscard]]
        int width() const
        {
            std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
            return m_html ? m_html->width().value() : 0;
        }

        [[nodiscard]]
        int height() const
        {
            std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
            return m_html ? m_html->height().value() : 0;
        }

        bool media_changed()
        {
            std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
            return m_html && m_html->media_changed();
        }

        litehtml::position::vector get_fixed_boxes()
        {
            litehtml::position::vector            ret;
            std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
            if(m_html)
            {
                m_html->get_fixed_boxes(ret);
            }
            return ret;
        }

        void stop_loading()
        {
            m_requests_pool.stop();
        }

        [[nodiscard]]
        bool is_downloading()
        {
            return m_requests_pool.is_downloading();
        }

        std::vector<litehtml::scroll_values> on_scroll(litehtml::pixel_t dx, litehtml::pixel_t dy, int x, int y,
                                                       int client_x, int client_y)
        {
            std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
            return m_html ? m_html->on_scroll(dx, dy, x, y, client_x, client_y)
                          : std::vector<litehtml::scroll_values>();
        }

        void run_with_document(const std::function<void(const std::shared_ptr<litehtml::document>)>& func)
        {
            std::lock_guard<std::recursive_mutex> html_lock(m_html_mutex);
            if(m_html)
            {
                func(m_html);
            }
        }

      private:
        void http_request(
            const std::string&                                                                  url,
            const std::function<void(void* data, size_t len, size_t downloaded, size_t total)>& cb_on_data,
            const std::function<void(u_int32_t http_status, u_int32_t err_code, const std::string& err_text,
                                     const std::string& url)>&                                  cb_on_finish);
        void on_page_downloaded(const std::shared_ptr<text_file>& data, u_int32_t http_status, u_int32_t err_code,
                                const std::string& err_text, const std::string& url);
        void on_image_downloaded(const std::shared_ptr<image_file>& data, u_int32_t http_status, u_int32_t err_code,
                                 const std::string& err_text, const std::string& url);
        void on_pool_update_state();
        void open_internal(const std::string& url, const std::string& fragment,
                           const std::vector<litehtml::form_field>* post_data);
        void process_pending_submit();
        void apply_select(const litehtml::select_control_data& data);
        bool handle_listbox_key(const key_event& event, key_event_result& result,
                                const std::function<void(const litehtml::position&)>& redraw);
    };
} // namespace litebrowser

#endif // LITEBROWSER_WEB_PAGE_H
