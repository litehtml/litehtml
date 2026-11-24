#include "litehtml/document.h"
#include "litehtml/el_button.h"
#include "litehtml/el_input.h"
#include "litehtml/el_label.h"
#include "litehtml/el_optgroup.h"
#include "litehtml/el_option.h"
#include "litehtml/el_select.h"
#include "litehtml/el_textarea.h"
#include "grid_test_container.h"
#include <algorithm>
#include <iostream>
#include <string>

namespace
{
    bool check(bool condition, const std::string& message)
    {
        if(!condition)
        {
            std::cerr << message << '\n';
        }
        return condition;
    }

    class controls_container : public grid_test_container
    {
      public:
        struct focus_event
        {
            litehtml::element::ptr el;
            std::string            id;
            litehtml::control_type type;
            litehtml::control_data data;
        };

        struct change_event
        {
            litehtml::element::ptr el;
            std::string            id;
            litehtml::control_data data;
        };

        struct border_event
        {
            litehtml::borders  borders;
            litehtml::position pos;
        };

        struct text_event
        {
            std::string         text;
            litehtml::web_color color;
            litehtml::position  pos;
        };

        litehtml::position::vector                 redraws;
        std::vector<focus_event>                   focus_events;
        std::vector<change_event>                  changes;
        std::vector<litehtml::form_data>           submits;
        std::vector<litehtml::select_control_data> popups;
        std::vector<border_event>                  border_draws;
        std::vector<text_event>                    text_draws;
        litehtml::element::ptr                     override_element;
        litehtml::text_control_data                override_data;
        bool                                       override_active = false;

        const std::function<void(const litehtml::position&)> redraw = [this](const litehtml::position& box) {
            redraws.push_back(box);
        };

        void clear()
        {
            redraws.clear();
            focus_events.clear();
            changes.clear();
            submits.clear();
            popups.clear();
            text_draws.clear();
        }

        void on_focus_change(const litehtml::element::ptr& el, const char* element_id, litehtml::control_type type,
                             const litehtml::control_data& data) override
        {
            focus_events.push_back({el, element_id ? element_id : "", type, data});
        }

        void get_text_data(const litehtml::element::ptr& el, litehtml::text_control_data& data) override
        {
            if(override_active && el == override_element)
            {
                data = override_data;
            }
        }

        void on_change(const litehtml::element::ptr& el, const char* element_id,
                       const litehtml::control_data& data) override
        {
            changes.push_back({el, element_id ? element_id : "", data});
        }

        void on_submit(const litehtml::form_data& data) override
        {
            submits.push_back(data);
        }

        bool show_select_popup(const litehtml::element::ptr& /*el*/, const char* /*element_id*/,
                               const litehtml::select_control_data& data) override
        {
            popups.push_back(data);
            return true;
        }

        void draw_borders(litehtml::uint_ptr /*hdc*/, const litehtml::borders& borders, const litehtml::position& pos,
                          bool /*root*/) override
        {
            border_draws.push_back({borders, pos});
        }

        void draw_text(litehtml::uint_ptr /*hdc*/, const char* text, litehtml::uint_ptr /*font*/,
                       litehtml::web_color color, const litehtml::position& pos) override
        {
            text_draws.push_back({text, color, pos});
        }
    };

    bool matches(const litehtml::element::ptr& el, const char* selector)
    {
        return el->select(selector) != litehtml::select_no_match;
    }

    void click_at(const litehtml::document::ptr& doc, controls_container& container, int x, int y)
    {
        doc->on_lbutton_down(x, y, x, y, container.redraw);
        doc->on_lbutton_up(x, y, x, y, container.redraw);
    }

    void click_box(const litehtml::document::ptr& doc, controls_container& container, const litehtml::element::ptr& el)
    {
        const litehtml::position pos = el->get_placement();
        click_at(doc, container, static_cast<int>(pos.x + pos.width / 2), static_cast<int>(pos.y + pos.height / 2));
    }

    litehtml::text_control_data text_data(const litehtml::element::ptr& el)
    {
        return std::get<litehtml::text_control_data>(el->get_control_data());
    }

    const controls_container::text_event* find_text_draw(const controls_container& container, const std::string& text)
    {
        auto it = std::find_if(container.text_draws.begin(), container.text_draws.end(),
                               [&](const auto& event) { return event.text == text; });
        return it == container.text_draws.end() ? nullptr : &*it;
    }

    size_t count_text_draws(const controls_container& container, const std::string& text)
    {
        return static_cast<size_t>(std::count_if(container.text_draws.begin(), container.text_draws.end(),
                                                 [&](const auto& event) { return event.text == text; }));
    }

    std::string field_value(const litehtml::form_data& data, const std::string& name)
    {
        for(const auto& field : data.fields)
        {
            if(field.name == name)
            {
                return field.value;
            }
        }
        return {};
    }

    bool has_field(const litehtml::form_data& data, const std::string& name)
    {
        for(const auto& field : data.fields)
        {
            if(field.name == name)
            {
                return true;
            }
        }
        return false;
    }
} // namespace

int main()
{
    using namespace litehtml;
    bool ok = true;

    // Element mapping.
    {
        controls_container container;
        const auto         doc =
            document::createFromString("<form id=f><input id=t><input id=c type=checkbox><textarea id=ta></textarea>"
                                       "<select id=s><optgroup id=g label=g><option id=o>A</option></optgroup></select>"
                                       "<label id=l>L</label><button id=b>B</button></form>",
                                       &container);
        const auto t  = doc->root()->select_one("#t");
        const auto c  = doc->root()->select_one("#c");
        const auto ta = doc->root()->select_one("#ta");
        const auto s  = doc->root()->select_one("#s");
        const auto o  = doc->root()->select_one("#o");
        const auto g  = doc->root()->select_one("#g");
        const auto l  = doc->root()->select_one("#l");
        const auto b  = doc->root()->select_one("#b");

        ok &= check(dynamic_cast<el_input*>(t.get()) != nullptr, "input should create el_input");
        ok &= check(dynamic_cast<el_input*>(c.get()) != nullptr &&
                        static_cast<el_input*>(c.get())->type() == input_type::checkbox,
                    "input type=checkbox should be parsed");
        ok &= check(dynamic_cast<el_textarea*>(ta.get()) != nullptr, "textarea should create el_textarea");
        ok &= check(dynamic_cast<el_select*>(s.get()) != nullptr, "select should create el_select");
        ok &= check(dynamic_cast<el_option*>(o.get()) != nullptr, "option should create el_option");
        ok &= check(dynamic_cast<el_optgroup*>(g.get()) != nullptr, "optgroup should create el_optgroup");
        ok &= check(dynamic_cast<el_label*>(l.get()) != nullptr, "label should create el_label");
        ok &= check(dynamic_cast<el_button*>(b.get()) != nullptr, "button should create el_button");
    }

    // Default boxes and basic control state.
    {
        controls_container container;
        const auto         doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}</style>"
            "<form id=f action='/submit' method='post'>"
            "<input id=t name=text value='hello'>"
            "<input id=pw name=password type=password value=secret>"
            "<input id=num name=number type=number value=5>"
            "<input id=search name=search type=search value=q>"
            "<input id=btn type=button value=Click>"
            "<input id=img type=image src=missing.png alt=Go>"
            "<input id=cb name=check type=checkbox>"
            "<input id=r1 name=radio value=one type=radio>"
            "<input id=r2 name=radio value=two type=radio>"
            "<input id=hid name=hidden type=hidden value=secret>"
            "<textarea id=ta name=area>line1</textarea>"
            "<select id=sel name=sel><option value=a>A</option><option value=b selected>B</option></select>"
            "<input id=sub name=go type=submit value='Send'>"
            "<input id=res type=reset value=Reset>"
            "<label id=lab for=cb style='display:inline-block;width:40px;height:20px'>check</label>"
            "</form>",
            &container);
        doc->render(800);

        const auto form = doc->root()->select_one("#f");
        const auto t    = doc->root()->select_one("#t");
        const auto cb   = doc->root()->select_one("#cb");
        const auto r1   = doc->root()->select_one("#r1");
        const auto r2   = doc->root()->select_one("#r2");
        const auto hid  = doc->root()->select_one("#hid");
        const auto ta   = doc->root()->select_one("#ta");
        const auto sel  = doc->root()->select_one("#sel");
        const auto sub  = doc->root()->select_one("#sub");
        const auto res  = doc->root()->select_one("#res");
        const auto lab  = doc->root()->select_one("#lab");

        ok &= check(t && cb && r1 && r2 && hid && ta && sel && sub && res && lab, "all test controls should exist");
        ok &= check(t->get_placement().width > 0_px && t->get_placement().height > 0_px,
                    "text input should get a default box");
        ok &= check(cb->get_placement().width > 0_px && cb->get_placement().height > 0_px,
                    "checkbox should get a default box");
        ok &= check(ta->get_placement().width > 0_px && ta->get_placement().height > 0_px,
                    "textarea should get a default box");
        ok &= check(sel->get_placement().width > 0_px && sel->get_placement().height > 0_px,
                    "select should get a default box");
        ok &= check(text_data(t).text == "hello", "the input value attribute should initialize the text state");
        ok &= check(text_data(ta).text == "line1", "the textarea initial text should be captured from its content");
        ok &= check(matches(doc->root()->select_one("#sel option:last-child"), "option:checked"),
                    "the selected option should match :checked");

        doc->draw(0, 0, 0, nullptr);
        ok &= check(true, "drawing the controls should not crash");

        // Checkbox toggling and radio group exclusivity.
        container.clear();
        click_box(doc, container, cb);
        ok &= check(matches(cb, "input:checked"), "clicking a checkbox should set :checked");
        ok &= check(container.changes.size() == 1 && container.changes.back().id == "cb" &&
                        std::get<bool>(container.changes.back().data),
                    "clicking a checkbox should report on_change with bool true");

        click_box(doc, container, cb);
        ok &= check(!matches(cb, "input:checked"), "clicking a checked checkbox should clear :checked");
        ok &= check(container.changes.size() == 2 && !std::get<bool>(container.changes.back().data),
                    "unchecking should report on_change with bool false");

        click_box(doc, container, r1);
        ok &= check(matches(r1, "input:checked") && !matches(r2, "input:checked"), "selecting a radio should check it");
        click_box(doc, container, r2);
        ok &= check(!matches(r1, "input:checked") && matches(r2, "input:checked"),
                    "selecting another radio should uncheck the group");

        doc->draw(0, 0, 0, nullptr);
        ok &= check(true, "drawing checked controls should not crash");

        // Label activation via for=.
        container.clear();
        click_box(doc, container, lab);
        ok &= check(matches(cb, "input:checked"), "clicking a label should activate its control");

        // Text editing through update_focused and container-provided text data.
        container.clear();
        ok &= check(doc->set_focus(t, true, container.redraw), "the text input should be focusable");
        ok &= check(container.focus_events.size() == 1 && container.focus_events.back().id == "t" &&
                        container.focus_events.back().type == control_type::text &&
                        std::get<text_control_data>(container.focus_events.back().data).text == "hello",
                    "focus change should pass the text control type and data");

        text_control_data text;
        text.text            = "abc";
        text.caret           = 3;
        text.selection_start = 1;
        text.selection_end   = 2;
        ok &= check(doc->update_focused(text, container.redraw), "update_focused should accept the focused element");
        ok &= check(text_data(t).text == "abc" && text_data(t).caret == 3 && text_data(t).selection_start == 1 &&
                        text_data(t).selection_end == 2,
                    "update_focused should store the text state");
        ok &= check(container.changes.size() == 1 && container.changes.back().id == "t",
                    "a text value change should report on_change");
        ok &= check(!container.redraws.empty(), "update_focused should request a redraw");

        const auto change_count = container.changes.size();
        text.caret              = 1;
        text.selection_start = text.selection_end = 1;
        ok &= check(doc->update_focused(text, container.redraw), "a caret-only update should be accepted");
        ok &= check(container.changes.size() == change_count, "a caret-only update should not report on_change");

        container.override_element   = t;
        container.override_data      = text;
        container.override_data.text = "overridden";
        container.override_active    = true;
        container.clear();
        ok                        &= check(doc->submit_form(form), "submitting a form should succeed");
        ok                        &= check(container.submits.size() == 1, "on_submit should be called once");
        ok                        &= check(field_value(container.submits.back(), "text") == "overridden",
                                           "form value should come from get_text_data");
        container.override_active  = false;

        // Textarea updates.
        container.clear();
        ok                   &= check(doc->set_focus(ta, true, container.redraw), "textarea should be focusable");
        text.text             = "x\ny";
        text.caret            = 3;
        text.selection_start = text.selection_end = 0;
        ok &= check(doc->update_focused(text, container.redraw), "textarea update_focused should be accepted");
        ok &= check(text_data(ta).text == "x\ny", "textarea should store the updated text");
        doc->draw(0, 0, 0, nullptr);
        ok &= check(true, "drawing an edited textarea should not crash");

        // Select popup and applying the chosen option.
        container.clear();
        click_box(doc, container, sel);
        ok &= check(container.popups.size() == 1 && container.focus_events.back().type == control_type::select,
                    "clicking a select should request a popup and focus it");
        auto select_data = container.popups.back();
        bool found_b     = false;
        for(auto& group : select_data.groups)
        {
            for(auto& option : group.options)
            {
                option.selected = option.value == "a";
                found_b         = found_b || option.selected;
            }
        }
        ok &= check(found_b, "the select popup data should contain the options");
        ok &= check(doc->update_focused(select_data, container.redraw), "applying a select choice should succeed");
        ok &= check(!container.changes.empty() && container.changes.back().id == "sel",
                    "changing the select should report on_change");
        container.clear();
        ok &= check(doc->submit_form(form) && field_value(container.submits.back(), "sel") == "a",
                    "the select should use the applied choice");

        // Reset restores initial values.
        container.clear();
        click_box(doc, container, res);
        ok &= check(!container.redraws.empty(), "reset should request a redraw of the changed controls");
        ok &= check(text_data(t).text == "hello", "reset should restore the input value");
        ok &= check(text_data(ta).text == "line1", "reset should restore the textarea value");
        ok &= check(!matches(cb, "input:checked"), "reset should restore the checkbox state");
        ok &= check(matches(doc->root()->select_one("#sel option:last-child"), "option:checked"),
                    "reset should restore the selected option");
    }

    // Listbox selects: multiple or size > 1 render as rows and are selected in place.
    {
        controls_container container;
        const auto         doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}</style>"
            "<form id=f>"
            "<select id=lm name=lm multiple>"
            "<option value=a>A</option><option value=b>B</option><option value=c selected>C</option>"
            "<option value=d disabled>D</option><option value=e>E</option></select>"
            "<select id=ls name=ls size=3>"
            "<option value=x>X</option><option value=y>Y</option><option value=z>Z</option></select>"
            "<select id=lm2 name=lm2 multiple size=2>"
            "<option value=p>P</option><option value=q>Q</option>"
            "<option value=r>R</option><option value=s>S</option></select>"
            "</form>",
            &container);
        doc->render(800);

        const auto lm  = doc->root()->select_one("#lm");
        const auto ls  = doc->root()->select_one("#ls");
        const auto lm2 = doc->root()->select_one("#lm2");

        const auto selected_values = [](const element::ptr& el) {
            std::vector<std::string> values;
            for(const auto& group : std::static_pointer_cast<el_select>(el)->select_data().groups)
            {
                for(const auto& option : group.options)
                {
                    if(option.selected)
                    {
                        values.push_back(option.value);
                    }
                }
            }
            return values;
        };
        const auto pitch     = lm->css().line_height().computed_value + 1_px;
        const auto click_row = [&](const element::ptr& el, int row) {
            const auto pos = el->get_placement();
            click_at(doc, container, static_cast<int>(pos.x + 5_px),
                     static_cast<int>(pos.y + pixel_t(row) * pitch + pitch / 2_px));
        };
        const auto scroll_at = [&](const element::ptr& el, pixel_t dy) {
            const auto pos = el->get_placement();
            return doc->on_scroll(0_px, dy, pos.x + 5_px, pos.y + 5_px, pos.x + 5_px, pos.y + 5_px);
        };

        ok &= check(std::static_pointer_cast<el_select>(lm)->select_data().size == 4,
                    "a multiple select should default to 4 visible rows");
        ok &= check(lm->get_placement().height == 4 * pitch, "a multiple select should be as tall as its visible rows");
        ok &= check(std::static_pointer_cast<el_select>(lm2)->select_data().size == 2,
                    "the size attribute should set the visible rows");
        ok &= check(std::static_pointer_cast<el_select>(ls)->select_data().size == 3,
                    "a sized single select should use its visible row count");
        ok &= check(ls->get_placement().height == 3 * pitch, "a sized select should be as tall as its rows");
        ok &= check(selected_values(lm) == std::vector<std::string>{"c"},
                    "a multiple select should keep only the explicitly selected option");
        ok &= check(selected_values(ls).empty(), "a listbox without a selected option should stay unselected");

        doc->draw(0, 0, 0, nullptr);
        ok &= check(true, "drawing listbox selects should not crash");

        // Multiple listboxes toggle rows; single listboxes replace the selection.
        container.clear();
        click_row(lm, 0);
        ok &= check(selected_values(lm) == std::vector<std::string>{"a", "c"},
                    "clicking a multiple listbox row should toggle it on");
        ok &= check(container.changes.size() == 1 && container.changes.back().id == "lm",
                    "a listbox selection should report on_change");

        click_row(lm, 0);
        ok &= check(selected_values(lm) == std::vector<std::string>{"c"},
                    "clicking a selected multiple row should toggle it off");

        container.clear();
        click_row(lm, 3);
        ok &= check(selected_values(lm) == std::vector<std::string>{"c"} && container.changes.empty(),
                    "clicking a disabled listbox row should do nothing");

        container.clear();
        click_row(ls, 1);
        ok &= check(selected_values(ls) == std::vector<std::string>{"y"},
                    "clicking a single listbox row should select only it");
        click_row(ls, 2);
        ok &= check(selected_values(ls) == std::vector<std::string>{"z"},
                    "clicking another single listbox row should replace the selection");

        // Mouse wheel scrolls the option rows and reveals them for clicks.
        auto scrolled  = scroll_at(lm, 30_px);
        ok            &= check(scrolled.size() == 1 && scrolled.back().dy == pitch,
                               "the wheel should scroll a listbox by the available row height");
        click_row(lm, 0);
        ok &= check(selected_values(lm) == std::vector<std::string>{"b", "c"},
                    "clicking after scrolling should select the revealed row");

        scrolled  = scroll_at(lm, -30_px);
        ok       &= check(scrolled.size() == 1 && scrolled.back().dy == -pitch,
                          "scrolling up should move back to the first row");
        ok       &= check(scroll_at(lm, 100_px).size() == 1, "scrolling down should be clamped to the last row");
        ok       &= check(scroll_at(lm, 10_px).empty(), "a listbox at the bottom should not consume more scrolling");

        // Revealing an option from the keyboard adjusts the visible rows.
        auto* const widget = dynamic_cast<el_select*>(lm.get());
        widget->scroll_option_into_view(0);
        container.clear();
        click_row(lm, 0);
        ok &= check(selected_values(lm) == std::vector<std::string>{"a", "b", "c"},
                    "revealing the first option should reset the visible rows");

        widget->scroll_option_into_view(4);
        container.clear();
        click_row(lm, 0);
        ok &= check(selected_values(lm) == std::vector<std::string>{"a", "c"},
                    "revealing the last option should scroll the rows");
    }

    // The active listbox option is outlined only while it is not selected.
    {
        controls_container container;
        const auto         doc =
            document::createFromString("<style>body{margin:0;font-size:16px;line-height:20px}</style>"
                                       "<select id=lm multiple size=4><option>one</option><option selected>two</option>"
                                       "<option>three</option><option>four</option></select>",
                                       &container);
        doc->render(800);

        const auto lm  = doc->root()->select_one("#lm");
        auto*      el  = dynamic_cast<el_select*>(lm.get());
        ok            &= check(el != nullptr, "the listbox select should exist");
        ok            &= check(doc->set_focus(lm, true, container.redraw), "the listbox should be focusable");

        const auto outline_count = [&container]() {
            return static_cast<size_t>(std::count_if(container.border_draws.begin(), container.border_draws.end(),
                                                     [](const controls_container::border_event& draw) {
                                                         return draw.borders.top.width == 2_px &&
                                                                draw.borders.top.color ==
                                                                    litehtml::web_color(16, 16, 16);
                                                     }));
        };

        el->set_active_option(0);
        container.border_draws.clear();
        doc->draw(0, 0, 0, nullptr);
        ok &= check(outline_count() == 1, "an active unselected option should draw an outline");

        el->set_active_option(1);
        container.border_draws.clear();
        doc->draw(0, 0, 0, nullptr);
        ok &= check(outline_count() == 0, "an active selected option should not draw an outline");
    }

    // Attribute-mapped sizes must survive a focus style refresh.
    {
        controls_container container;
        const auto         doc = document::createFromString("<style>body{margin:0;font-size:16px;line-height:20px}"
                                                            "textarea:focus{background:#eef}</style>"
                                                            "<textarea id=ta rows=4 cols=30>text</textarea>",
                                                            &container);
        doc->render(800);

        const auto ta            = doc->root()->select_one("#ta");
        const auto size_before   = ta->get_placement();
        const auto height_before = ta->css().get_height().to_string();
        const auto width_before  = ta->css().get_width().to_string();
        ok                    &= check(doc->set_focus(ta, true, container.redraw), "the textarea should be focusable");
        const auto size_after  = ta->get_placement();
        ok &=
            check(size_after.height == size_before.height, "focusing a textarea must not change its rows-based height");
        ok &= check(size_after.width == size_before.width, "focusing a textarea must not change its cols-based width");
        ok &= check(ta->css().get_height().to_string() == height_before,
                    "focusing a textarea must keep the height declaration from the rows attribute");
        ok &= check(ta->css().get_width().to_string() == width_before,
                    "focusing a textarea must keep the width declaration from the cols attribute");
        ok &= check(ta->css().get_bg().m_color == web_color(238, 238, 255), "the :focus style should still be applied");
    }

    // Author rules still win over presentational attributes after a refresh.
    {
        controls_container container;
        const auto         doc =
            document::createFromString("<style>body{margin:0;font-size:16px;line-height:20px}"
                                       "textarea{height:120px;width:250px}textarea:focus{color:#00f}</style>"
                                       "<textarea id=ta rows=2 cols=10>text</textarea>",
                                       &container);
        doc->render(800);

        const auto ta  = doc->root()->select_one("#ta");
        ok            &= check(doc->set_focus(ta, true, container.redraw), "the textarea should be focusable");
        ok            &= check(ta->get_placement().height == 120_px, "author height must win over the rows attribute");
        ok            &= check(ta->get_placement().width == 250_px, "author width must win over the cols attribute");
    }

    // Form submission order and implicit submitter.
    {
        controls_container container;
        const auto         doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}</style>"
            "<form id=f action='/submit' method='POST' enctype='multipart/form-data' target='_blank'>"
            "<input id=t name=text value='hello'>"
            "<input id=cb name=check type=checkbox checked>"
            "<input id=cb2 name=unchecked type=checkbox>"
            "<input id=r1 name=radio value=one type=radio checked>"
            "<input id=hid name=hidden type=hidden value=secret>"
            "<textarea id=ta name=area>line1</textarea>"
            "<select id=sel name=sel><option value=a>A</option><option value=b selected>B</option></select>"
            "<input id=sub name=go type=submit value='Send'>"
            "<input id=res type=reset value=Reset>"
            "</form>",
            &container);
        doc->render(800);

        const auto form = doc->root()->select_one("#f");
        container.clear();
        ok               &= check(doc->submit_form(form), "form submission should succeed");
        ok               &= check(container.submits.size() == 1, "one submission should be recorded");
        const auto& data  = container.submits.back();
        ok &= check(data.action == "/submit" && data.method == "post" && data.enctype == "multipart/form-data" &&
                        data.target == "_blank",
                    "form attributes should be copied to form_data");
        ok &= check(data.fields.size() == 7, "all successful controls should be collected");
        ok &= check(field_value(data, "text") == "hello", "text field should be collected");
        ok &= check(field_value(data, "check") == "on", "checked checkbox should be collected");
        ok &= check(!has_field(data, "unchecked"), "unchecked checkbox should be omitted");
        ok &= check(field_value(data, "radio") == "one", "checked radio should be collected");
        ok &= check(field_value(data, "hidden") == "secret", "hidden input should be collected");
        ok &= check(field_value(data, "area") == "line1", "textarea should be collected");
        ok &= check(field_value(data, "sel") == "b", "select should be collected");
        ok &= check(field_value(data, "go") == "Send", "the implicit submit button should be collected");
    }

    // Button submission and focus removal notification.
    {
        controls_container container;
        const auto doc = document::createFromString("<style>body{margin:0;font-size:16px;line-height:20px}</style>"
                                                    "<form id=f action='/go'>"
                                                    "<input id=t name=text value=hello>"
                                                    "<button id=b name=button value=value>Go</button>"
                                                    "</form>",
                                                    &container);
        doc->render(800);

        const auto form = doc->root()->select_one("#f");
        const auto b    = doc->root()->select_one("#b");
        container.clear();
        click_box(doc, container, b);
        ok &= check(container.submits.size() == 1 && field_value(container.submits.back(), "button") == "value",
                    "clicking a button should submit the form");
        ok &= check(container.focus_events.back().el == b, "the button should receive focus");

        container.clear();
        ok &= check(doc->set_focus(nullptr, false, container.redraw), "focus should be removable");
        ok &= check(container.focus_events.size() == 1 && !container.focus_events.back().el,
                    "removing the focus should notify the container with a null element");
    }

    // A button with child elements lays its content out instead of flattening it into a label.
    {
        controls_container container;
        const auto         doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}</style>"
            "<button id=b><img src=dot.png alt=Send><span id=s>Send <strong>now</strong></span></button>"
            "<button id=multi>a<br>b</button>"
            "<button id=single>a b</button>",
            &container);
        doc->render(800);

        const auto b      = doc->root()->select_one("#b");
        const auto s      = doc->root()->select_one("#s");
        const auto multi  = doc->root()->select_one("#multi");
        const auto single = doc->root()->select_one("#single");

        ok &= check(b && s && multi && single, "the button and its content should create elements");
        ok &= check(b && !b->is_replaced() && multi && !multi->is_replaced(),
                    "a button with child elements should not be replaced");
        ok &= check(single && single->is_replaced(), "a text-only button should stay replaced");
        ok &= check(s && s->parent() == b, "the span should stay inside the button");
        ok &= check(multi && single && multi->get_placement().height > single->get_placement().height,
                    "a line break inside a button should lay out two lines");
    }

    // Clicking the content of a submit button submits the form through the button.
    {
        controls_container container;
        const auto         doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}</style>"
            "<form id=f action='/go'>"
            "<button id=b name=button value=value><span id=s>Go <strong>now</strong></span></button>"
            "</form>",
            &container);
        doc->render(800);

        const auto b   = doc->root()->select_one("#b");
        const auto box = b ? b->get_placement() : position{};
        const int  x   = static_cast<int>(box.x + box.width / 2);
        const int  y   = static_cast<int>(box.y + box.height / 2);

        container.clear();
        doc->on_lbutton_down(x, y, x, y, container.redraw);
        const auto over  = doc->get_over_element();
        ok              &= check(over && over.get() != b.get() && over->is_ancestor(b),
                                 "the button content should be hit instead of the button itself");
        doc->on_lbutton_up(x, y, x, y, container.redraw);

        ok &= check(container.submits.size() == 1 && field_value(container.submits.back(), "button") == "value",
                    "clicking content inside a button should submit the form");
        ok &= check(!container.focus_events.empty() && container.focus_events.back().el == b,
                    "clicking content inside a button should focus the button");
    }

    // Placeholder rendering, ::placeholder styles and :placeholder-shown.
    {
        controls_container container;
        const auto         doc = document::createFromString("<style>body{margin:0;font-size:16px;line-height:20px}"
                                                            "input::placeholder{color:rgb(0,128,0)}"
                                                            "textarea::placeholder{color:rgb(0,0,255)}</style>"
                                                            "<input id=t placeholder='Type here'>"
                                                            "<input id=v placeholder='Type here' value='abc'>"
                                                            "<input id=pw type=password placeholder='Secret'>"
                                                            "<input id=num type=number placeholder='Number'>"
                                                            "<input id=cb type=checkbox placeholder='Check'>"
                                                            "<textarea id=ta placeholder='Notes'></textarea>",
                                                            &container);
        doc->render(800);

        const auto t = doc->root()->select_one("#t");

        container.clear();
        doc->draw(0, 0, 0, nullptr);

        const auto* placeholder  = find_text_draw(container, "Type here");
        ok                      &= check(placeholder != nullptr, "an empty text input should draw its placeholder");
        ok                      &= check(placeholder && placeholder->color == web_color(0, 128, 0),
                                         "input::placeholder colour should be applied");
        ok &= check(count_text_draws(container, "Type here") == 1, "only the empty input should draw the placeholder");
        ok &= check(find_text_draw(container, "abc") != nullptr, "a filled input should draw its value");
        ok &= check(find_text_draw(container, "Secret") != nullptr, "a password placeholder should not be masked");
        ok &= check(find_text_draw(container, "Number") == nullptr, "a number input should not draw a placeholder");
        ok &= check(find_text_draw(container, "Check") == nullptr, "a checkbox should not draw a placeholder");
        const auto* notes  = find_text_draw(container, "Notes");
        ok                &= check(notes != nullptr, "an empty textarea should draw its placeholder");
        ok &= check(notes && notes->color == web_color(0, 0, 255), "textarea::placeholder colour should be applied");

        ok &= check(matches(t, "input:placeholder-shown"),
                    "an empty input with a placeholder should match :placeholder-shown");
        ok &= check(!matches(doc->root()->select_one("#v"), "input:placeholder-shown"),
                    "a filled input should not match :placeholder-shown");
        ok &= check(matches(doc->root()->select_one("#ta"), "textarea:placeholder-shown"),
                    "an empty textarea with a placeholder should match :placeholder-shown");

        ok &= check(doc->set_focus(t, true, container.redraw), "the placeholder input should be focusable");
        text_control_data text;
        text.text   = "hello";
        text.caret  = 5;
        ok         &= check(static_cast<el_input*>(t.get())->update_focused(text),
                            "hiding the placeholder should report a style change");
        ok &= check(!matches(t, "input:placeholder-shown"), "a filled input should stop matching :placeholder-shown");
        container.clear();
        doc->draw(0, 0, 0, nullptr);
        ok &= check(find_text_draw(container, "Type here") == nullptr, "the placeholder should hide while typing");
        ok &= check(find_text_draw(container, "hello") != nullptr, "the typed value should be drawn");

        text.text.clear();
        text.caret  = 0;
        ok         &= check(static_cast<el_input*>(t.get())->update_focused(text),
                            "showing the placeholder should report a style change");
        ok &= check(matches(t, "input:placeholder-shown"), "clearing the value should restore :placeholder-shown");
        container.clear();
        doc->draw(0, 0, 0, nullptr);
        ok &= check(find_text_draw(container, "Type here") != nullptr,
                    "clearing the value should restore the placeholder");
    }

    // UA placeholder colour and focus-dependent ::placeholder rules.
    {
        controls_container container;
        const auto         doc = document::createFromString("<style>body{margin:0;font-size:16px;line-height:20px}"
                                                            "input:focus::placeholder{color:rgb(1,2,3)}</style>"
                                                            "<input id=t placeholder='Default'>",
                                                            &container);
        doc->render(800);

        const auto t = doc->root()->select_one("#t");

        container.clear();
        doc->draw(0, 0, 0, nullptr);
        const auto* default_draw  = find_text_draw(container, "Default");
        ok                       &= check(default_draw != nullptr, "the UA placeholder should be drawn");
        ok                       &= check(default_draw && default_draw->color == web_color(169, 169, 169),
                                          "the master stylesheet should give the placeholder the darkgray colour");

        ok &= check(doc->set_focus(t, true, container.redraw), "the input should be focusable");
        container.clear();
        doc->draw(0, 0, 0, nullptr);
        const auto* focused_draw  = find_text_draw(container, "Default");
        ok                       &= check(focused_draw && focused_draw->color == web_color(1, 2, 3),
                                          "input:focus::placeholder should override the UA colour");
    }

    if(ok)
    {
        std::cout << "All controls tests passed\n";
    }
    return ok ? 0 : 1;
}
