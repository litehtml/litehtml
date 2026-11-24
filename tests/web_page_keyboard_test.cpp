#include "html_host.h"
#include "web_page.h"

#include "litehtml/el_select.h"

#include <curl/curl.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using litehtml::operator""_px;

namespace
{
    bool check(bool condition, const char* message)
    {
        if(!condition)
        {
            std::cerr << message << '\n';
        }
        return condition;
    }

    class test_host : public litebrowser::html_host_interface
    {
      public:
        int popup_count  = 0;
        int submit_count = 0;

        void open_url(const std::string& /*url*/) override {}
        void update_cursor() override {}
        void scroll_to(int /*x*/, int /*y*/) override {}
        void get_viewport(litehtml::position& viewport) const override
        {
            viewport = litehtml::position(0_px, 0_px, 800_px, 600_px);
        }
        void redraw_boxes(const litehtml::position::vector& /*boxes*/) override {}
        int  get_render_width() override
        {
            return 800;
        }
        double get_dpi() override
        {
            return 96.0;
        }
        int get_screen_width() override
        {
            return 800;
        }
        int get_screen_height() override
        {
            return 600;
        }
        cairo_surface_t* load_image(const std::string& /*path*/) override
        {
            return nullptr;
        }
        void show_select_popup(
            const litehtml::element::ptr& /*el*/, const char* /*element_id*/,
            const litehtml::select_control_data& /*data*/,
            const std::function<void(const litehtml::select_control_data&)>& /*on_selected*/) override
        {
            ++popup_count;
        }
        void submit_form(const litehtml::form_data& /*data*/) override
        {
            ++submit_count;
        }
    };

    class test_notifier : public litebrowser::browser_notify_interface
    {
      public:
        std::promise<void> loaded;

        void redraw() override {}
        void render() override {}
        void update_state() override {}
        void on_set_caption(const std::string& /*caption*/) override {}
        void on_page_loaded(uint64_t /*web_page_id*/) override
        {
            try
            {
                loaded.set_value();
            }
            catch(const std::future_error&)
            {
            }
        }
    };

    std::vector<int> selected_flags(const litehtml::element::ptr& el)
    {
        std::vector<int> flags;
        const auto       data = el->get_control_data();
        if(const auto* select = std::get_if<litehtml::select_control_data>(&data))
        {
            for(const auto& group : select->groups)
            {
                for(const auto& option : group.options)
                {
                    flags.push_back(option.selected ? 1 : 0);
                }
            }
        }
        return flags;
    }

    litebrowser::key_event_result press(const std::shared_ptr<litebrowser::web_page>& page,
                                        decltype(litebrowser::key_event::kind) kind, bool shift = false,
                                        bool ctrl = false)
    {
        litebrowser::key_event event;
        event.kind  = kind;
        event.shift = shift;
        event.ctrl  = ctrl;
        return page->on_key_event(event);
    }

    litebrowser::key_event_result type_text(const std::shared_ptr<litebrowser::web_page>& page, const std::string& text)
    {
        litebrowser::key_event event;
        event.kind = litebrowser::key_event::kind::character;
        event.text = text;
        return page->on_key_event(event);
    }
} // namespace

int main()
{
    if(curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
    {
        std::cerr << "curl_global_init failed\n";
        return 1;
    }

    bool ok = true;

    const auto html_path =
        std::filesystem::temp_directory_path() / ("litehtml-webpage-keyboard-" + std::to_string(getpid()) + ".html");

    {
        std::ofstream html(html_path);
        html << "<!doctype html><style>body{margin:0;font-size:16px;line-height:20px}</style>"
                "<select id=s size=4><option>one</option><option>two</option><option>three</option>"
                "<option disabled>four</option><option>five</option><option>six</option></select>"
                "<select id=m multiple size=4><option>one</option><option>two</option><option>three</option>"
                "<option disabled>four</option><option>five</option><option>six</option></select>"
                "<select id=g multiple size=3><option>top</option><optgroup label=\"Group A\">"
                "<option>a1</option><option>a2</option></optgroup><optgroup label=\"Group B\">"
                "<option>b1</option></optgroup></select>"
                "<select id=d><option>alpha</option><option>beta</option></select>";
    }

    auto host     = std::make_shared<test_host>();
    auto notifier = std::make_shared<test_notifier>();
    auto page     = std::make_shared<litebrowser::web_page>(host.get(), notifier, 1);

    auto loaded = notifier->loaded.get_future();
    page->open(html_path.string(), "");
    if(loaded.wait_for(std::chrono::seconds(10)) != std::future_status::ready)
    {
        std::cerr << "web page did not load\n";
        std::filesystem::remove(html_path);
        curl_global_cleanup();
        return 1;
    }

    const auto focus = [&](const char* id) {
        litehtml::element::ptr element;
        page->run_with_document([&](const std::shared_ptr<litehtml::document>& doc) {
            element = doc->root()->select_one(id);
            doc->set_focus(element, true, [](const litehtml::position&) {});
        });
        return element;
    };

    const auto single  = focus("#s");
    ok                &= check(single != nullptr, "single listbox should exist");

    ok &= check(press(page, litebrowser::key_event::kind::down).handled, "Down should be consumed by a listbox");
    ok &= check(selected_flags(single) == std::vector<int>({1, 0, 0, 0, 0, 0}), "Down should select the first option");
    press(page, litebrowser::key_event::kind::down);
    ok &= check(selected_flags(single) == std::vector<int>({0, 1, 0, 0, 0, 0}), "Down should move to the next option");
    press(page, litebrowser::key_event::kind::up);
    ok &=
        check(selected_flags(single) == std::vector<int>({1, 0, 0, 0, 0, 0}), "Up should move to the previous option");
    press(page, litebrowser::key_event::kind::end);
    ok &= check(selected_flags(single) == std::vector<int>({0, 0, 0, 0, 0, 1}), "End should select the last option");
    press(page, litebrowser::key_event::kind::home);
    ok &= check(selected_flags(single) == std::vector<int>({1, 0, 0, 0, 0, 0}), "Home should select the first option");
    press(page, litebrowser::key_event::kind::page_down);
    ok &= check(selected_flags(single) == std::vector<int>({0, 0, 0, 0, 1, 0}),
                "PageDown should skip the disabled option");
    press(page, litebrowser::key_event::kind::page_up);
    ok &= check(selected_flags(single) == std::vector<int>({0, 1, 0, 0, 0, 0}), "PageUp should move back by a page");
    press(page, litebrowser::key_event::kind::down, false, true);
    ok &= check(selected_flags(single) == std::vector<int>({0, 0, 1, 0, 0, 0}),
                "Ctrl+Down should still move a single-selection listbox");
    press(page, litebrowser::key_event::kind::down, true, false);
    ok &= check(selected_flags(single) == std::vector<int>({0, 0, 0, 0, 1, 0}),
                "Shift+Down should move a single-selection listbox");

    const auto multi         = focus("#m");
    const auto multi_widget  = dynamic_cast<litehtml::el_select*>(multi.get());
    ok                      &= check(multi != nullptr && multi_widget != nullptr, "multiple listbox should exist");

    ok &=
        check(press(page, litebrowser::key_event::kind::down).handled, "Down should be consumed by a multiple listbox");
    ok &= check(selected_flags(multi) == std::vector<int>({1, 0, 0, 0, 0, 0}),
                "Down should select only the first option");
    press(page, litebrowser::key_event::kind::down);
    ok &=
        check(selected_flags(multi) == std::vector<int>({0, 1, 0, 0, 0, 0}), "Down should select only the next option");
    press(page, litebrowser::key_event::kind::down, true, false);
    ok &=
        check(selected_flags(multi) == std::vector<int>({0, 1, 1, 0, 0, 0}), "Shift+Down should extend the selection");
    press(page, litebrowser::key_event::kind::down, false, true);
    ok &= check(selected_flags(multi) == std::vector<int>({0, 1, 1, 0, 0, 0}),
                "Ctrl+Down should not change the selection");
    ok &= check(multi_widget->active_option() == 4, "Ctrl+Down should move the active option");

    press(page, litebrowser::key_event::kind::space);
    ok &= check(selected_flags(multi) == std::vector<int>({0, 1, 1, 0, 1, 0}),
                "Space should toggle the active option on");
    press(page, litebrowser::key_event::kind::home, false, true);
    ok &= check(selected_flags(multi) == std::vector<int>({0, 1, 1, 0, 1, 0}),
                "Ctrl+Home should not change the selection");
    press(page, litebrowser::key_event::kind::space);
    ok &= check(selected_flags(multi) == std::vector<int>({1, 1, 1, 0, 1, 0}),
                "Space should add the active option to the selection");
    press(page, litebrowser::key_event::kind::space);
    ok &= check(selected_flags(multi) == std::vector<int>({0, 1, 1, 0, 1, 0}),
                "Space should toggle an active selected option off");

    ok &= check(type_text(page, "t").handled, "type-ahead should be consumed");
    ok &= check(selected_flags(multi) == std::vector<int>({0, 1, 0, 0, 0, 0}),
                "type-ahead should select a matching option");
    type_text(page, "h");
    ok &= check(selected_flags(multi) == std::vector<int>({0, 0, 1, 0, 0, 0}), "type-ahead should refine the prefix");
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    type_text(page, "f");
    ok &= check(selected_flags(multi) == std::vector<int>({0, 0, 0, 0, 1, 0}),
                "type-ahead should skip disabled matches and reset after a pause");

    press(page, litebrowser::key_event::kind::home);
    press(page, litebrowser::key_event::kind::end, true, false);
    ok &= check(selected_flags(multi) == std::vector<int>({1, 1, 1, 0, 1, 1}),
                "Shift+End should select the range and skip disabled options");

    const auto grouped  = focus("#g");
    ok                 &= check(grouped != nullptr, "grouped listbox should exist");
    press(page, litebrowser::key_event::kind::down);
    ok &= check(selected_flags(grouped) == std::vector<int>({1, 0, 0, 0}), "Down should select the first row");
    press(page, litebrowser::key_event::kind::page_down);
    ok &= check(selected_flags(grouped) == std::vector<int>({0, 1, 0, 0}),
                "PageDown should count the optgroup label row");
    press(page, litebrowser::key_event::kind::page_down);
    ok &= check(selected_flags(grouped) == std::vector<int>({0, 0, 0, 1}),
                "PageDown should skip the second optgroup label row");
    press(page, litebrowser::key_event::kind::page_up);
    ok &=
        check(selected_flags(grouped) == std::vector<int>({0, 0, 1, 0}), "PageUp should count the optgroup label row");
    press(page, litebrowser::key_event::kind::home);
    ok &= check(selected_flags(grouped) == std::vector<int>({1, 0, 0, 0}), "Home should select the first option");
    press(page, litebrowser::key_event::kind::end);
    ok &= check(selected_flags(grouped) == std::vector<int>({0, 0, 0, 1}), "End should select the last option");

    const auto dropdown  = focus("#d");
    ok                  &= check(dropdown != nullptr, "dropdown should exist");
    host->popup_count    = 0;
    ok &= check(press(page, litebrowser::key_event::kind::down).handled, "Down should open a dropdown popup");
    ok &= check(host->popup_count == 1, "Down should request the dropdown popup");
    press(page, litebrowser::key_event::kind::space);
    ok &= check(host->popup_count == 2, "Space should request the dropdown popup");
    ok &= check(!press(page, litebrowser::key_event::kind::page_down).handled,
                "PageDown should not be consumed by a dropdown");
    ok &= check(host->popup_count == 2, "PageDown should not open a dropdown popup");

    page->stop_loading();
    std::filesystem::remove(html_path);
    curl_global_cleanup();

    if(!ok)
    {
        return 1;
    }
    std::cout << "All webpage keyboard tests passed\n";
    return 0;
}
