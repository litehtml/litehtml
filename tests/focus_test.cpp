#include "litehtml/document.h"
#include "grid_test_container.h"
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

    class focus_container : public grid_test_container
    {
      public:
        litehtml::position::vector redraws;

        const std::function<void(const litehtml::position&)> redraw = [this](const litehtml::position& box) {
            redraws.push_back(box);
        };

        void clear_redraws()
        {
            redraws.clear();
        }

        void on_anchor_click(const char* url, const litehtml::element::ptr&) override
        {
            clicked_anchors.emplace_back(url ? url : "");
        }

        std::vector<std::string> clicked_anchors;
    };

    bool focused(const litehtml::document::ptr& doc, const litehtml::element::ptr& el)
    {
        return doc->get_focused_element() == el;
    }

    bool matches(const litehtml::element::ptr& el, const char* selector)
    {
        return el->select(selector) != litehtml::select_no_match;
    }

    void click_at(const litehtml::document::ptr& doc, focus_container& container, int x, int y)
    {
        doc->on_lbutton_down(x, y, x, y, container.redraw);
        doc->on_lbutton_up(x, y, x, y, container.redraw);
    }

    void click_box(const litehtml::document::ptr& doc, focus_container& container, const litehtml::position& pos)
    {
        click_at(doc, container, static_cast<int>(pos.x + pos.width / 2), static_cast<int>(pos.y + pos.height / 2));
    }
} // namespace

int main()
{
    using namespace litehtml;
    bool ok = true;

    // can_be_focused/tab_index defaults, tab order and wrapping.
    {
        focus_container container;
        const auto      doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}</style>"
            "<div id=wrap><a id=a href='#'>a</a><div id=b tabindex='1'>b</div><a id=c href='#'>c</a>"
            "<div id=d tabindex='0'>d</div><div id=e tabindex='-1'>e</div><a id=f>f</a>"
            "<div id=g tabindex='bad'>g</div><a id=hidden href='#' style='display:none'>h</a></div>",
            &container);
        doc->render(800);

        const auto wrap   = doc->root()->select_one("#wrap");
        const auto a      = doc->root()->select_one("#a");
        const auto b      = doc->root()->select_one("#b");
        const auto c      = doc->root()->select_one("#c");
        const auto d      = doc->root()->select_one("#d");
        const auto e      = doc->root()->select_one("#e");
        const auto f      = doc->root()->select_one("#f");
        const auto g      = doc->root()->select_one("#g");
        const auto hidden = doc->root()->select_one("#hidden");

        ok &= check(a && b && c && d && e && f && g && hidden && wrap, "all test elements should exist");
        ok &= check(!doc->get_focused_element(), "a new document has no focused element");
        ok &= check(a->can_be_focused() && !a->tab_index(), "a link with href is focusable without a tabindex");
        ok &= check(b->can_be_focused() && b->tab_index().value_or(0) == 1, "an element with tabindex is focusable");
        ok &= check(e->can_be_focused() && e->tab_index().value_or(0) == -1,
                    "tabindex=-1 is focusable but not sequentially focusable");
        ok &= check(!f->can_be_focused(), "a link without href is not focusable");
        ok &= check(!g->can_be_focused() && !g->tab_index(), "an invalid tabindex does not make an element focusable");
        ok &= check(hidden->can_be_focused(), "a hidden link still can_be_focused, but is skipped by navigation");
        ok &=
            check(!doc->set_focus(hidden, true, container.redraw), "set_focus should not focus a display:none element");

        // Positive tabindex values come first, then document order; tabindex=-1 and display:none are skipped.
        const auto expect_next = [&](const element::ptr& expected) {
            container.clear_redraws();
            const auto     pos       = doc->focus_next(container.redraw);
            const position placement = expected->get_placement();
            // Block elements report the exact border box; inline elements report the line box fragments
            // inside the placement.
            const bool position_ok =
                pos && (placement.height > 0_px ? *pos == placement
                                                : (pos->x == placement.x && pos->y >= placement.y &&
                                                   pos->width > 0_px && pos->height > 0_px));
            return check(pos && focused(doc, expected) && position_ok,
                         "focus_next should focus the expected element and return its position");
        };

        ok &= expect_next(b);
        ok &= check(b->select("div:focus") != select_no_match, "the focused element matches :focus");
        ok &= expect_next(a);
        ok &= check(!matches(b, "div:focus"), "the previously focused element loses :focus");
        ok &= expect_next(c);
        ok &= expect_next(d);
        ok &= expect_next(b); // wraps around

        const auto pos  = doc->focus_prev(container.redraw);
        ok             &= check(pos && focused(doc, d) && *pos == d->get_placement(),
                                "focus_prev from the first element should wrap to the last one");

        // focus_prev from the last element goes backwards through the same order.
        ok &= check(doc->focus_prev(container.redraw) && focused(doc, c), "focus_prev should move backwards");

        // Without a focused element focus_prev starts at the last tabbable element.
        ok &= check(doc->set_focus(nullptr, false, container.redraw) && !doc->get_focused_element(),
                    "the focus should be cleared");
        ok &= check(doc->focus_prev(container.redraw) && focused(doc, d),
                    "focus_prev without a focused element should focus the last element");
    }

    // :focus, :focus-visible and :focus-within.
    {
        focus_container container;
        const auto      doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}"
            "#wrap:focus-within{background:#ddd}#a:focus{color:#f00}#a:focus-visible{color:#0f0}"
            "#b:focus{color:#00f}#other{background:#fff}</style>"
            "<div id=wrap><a id=a href='#'>a</a><a id=b href='#'>b</a></div><div id=other>other</div>",
            &container);
        doc->render(800);

        const auto wrap  = doc->root()->select_one("#wrap");
        const auto a     = doc->root()->select_one("#a");
        const auto b     = doc->root()->select_one("#b");
        const auto other = doc->root()->select_one("#other");

        container.clear_redraws();
        const auto a_pos  = doc->focus_next(container.redraw);
        ok               &= check(a_pos && focused(doc, a), "focus_next should focus the first link");
        ok               &= check(a_pos && a_pos->width == 8_px && a_pos->height == 16_px && a_pos->y == 2_px,
                                  "the returned position should cover the inline link");
        ok               &= check(matches(a, "a:focus") && matches(a, "a:focus-visible"),
                                  "keyboard focus sets both :focus and :focus-visible");
        ok &= check(a->css().get_color() == web_color(0, 255, 0), "the :focus-visible style should win over :focus");
        ok &= check(a->css().get_outline().style == border_style_auto && a->css().get_outline().width.val() == 1.0f &&
                        a->css().get_outline().color == web_color(229, 151, 0) &&
                        a->css().get_outline_offset().val() == 1.0f,
                    "a keyboard-focused link should use the UA focus ring with the link offset");
        ok &= check(matches(wrap, "div:focus-within") && !matches(other, "div:focus-within"),
                    ":focus-within should match only the ancestors of the focused element");
        ok &= check(!container.redraws.empty(), "a focus style change should request a redraw");

        const auto b_pos  = doc->focus_next(container.redraw);
        ok               &= check(b_pos && focused(doc, b), "focus_next should move to the second link");
        ok               &= check(!matches(a, "a:focus") && !matches(a, "a:focus-visible"),
                                  "the previous element loses both focus pseudo classes");
        ok               &= check(matches(b, "a:focus") && matches(b, "a:focus-visible"),
                                  "the newly focused element matches the focus pseudo classes");

        // A mouse click focuses without :focus-visible, even for an already focused element.
        if(b_pos)
        {
            click_box(doc, container, *b_pos);
        }
        ok &= check(focused(doc, b), "a click should keep the focus on the clicked link");
        ok &= check(matches(b, "a:focus") && !matches(b, "a:focus-visible"),
                    "a mouse focus does not match :focus-visible");
        ok &= check(b->css().get_color() == web_color(0, 0, 255), "the :focus only style should be applied");
        ok &= check(b->css().get_outline().style == border_style_none && b->css().outline_extent() == 0_px,
                    "a mouse-focused link should not use the UA focus ring");

        // Clicking outside any focusable element clears the focus.
        doc->on_lbutton_down(500, 500, 500, 500, container.redraw);
        doc->on_lbutton_up(500, 500, 500, 500, container.redraw);
        ok &= check(!doc->get_focused_element(), "a click outside any focusable element clears the focus");
        ok &= check(!matches(b, "a:focus") && !matches(b, "a:focus-visible") && !matches(wrap, "div:focus-within"),
                    "clearing the focus removes all focus pseudo classes");
    }

    // Text-entry controls always match :focus-visible, even when the focus came from the mouse or script.
    {
        focus_container container;
        const auto      doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}</style>"
            "<input id=t type=text><input id=n type=number><input id=p type=password>"
            "<input id=se type=search><textarea id=ta></textarea>"
            "<select id=sel><option>x</option></select>"
            "<input id=c type=checkbox><input id=r type=radio><button id=b>b</button><a id=l href='#'>l</a>",
            &container);
        doc->render(800);

        for(const char* id : {"#t", "#n", "#p", "#se", "#ta", "#sel"})
        {
            const auto el  = doc->root()->select_one(id);
            ok            &= check(el && doc->set_focus(el, false, container.redraw),
                                   std::string("a text-entry control should be focusable: ") + id);
            ok            &= check(el && matches(el, ":focus-visible"),
                                   std::string("mouse-style focus should keep :focus-visible for ") + id);
            ok            &= check(el && el->css().get_outline().style == border_style_auto,
                                   std::string("mouse-style focus should apply the UA focus ring for ") + id);
        }
        for(const char* id : {"#c", "#r", "#b", "#l"})
        {
            const auto el  = doc->root()->select_one(id);
            ok            &= check(el && doc->set_focus(el, false, container.redraw),
                                   std::string("a non-text control should be focusable: ") + id);
            ok            &= check(el && !matches(el, ":focus-visible"),
                                   std::string("mouse-style focus should not set :focus-visible for ") + id);
            ok            &= check(el && el->css().outline_extent() == 0_px,
                                   std::string("mouse-style focus should not apply the UA focus ring for ") + id);
        }

        const auto c = doc->root()->select_one("#c");
        ok &= check(c && doc->set_focus(c, true, container.redraw) && c->css().get_outline_offset().val() == 2.0f,
                    "a keyboard-focused checkbox should use the 2px focus ring offset");
        const auto l = doc->root()->select_one("#l");
        ok &= check(l && doc->set_focus(l, true, container.redraw) && l->css().get_outline_offset().val() == 1.0f,
                    "a keyboard-focused link should use the 1px focus ring offset");
    }

    // Programmatic focus.
    {
        focus_container container;
        const auto      doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}#a:focus{color:#f00}</style>"
            "<a id=a href='#'>a</a><div id=plain>plain</div>",
            &container);
        doc->render(800);

        const auto a     = doc->root()->select_one("#a");
        const auto plain = doc->root()->select_one("#plain");

        ok &= check(doc->set_focus(a, false, container.redraw) && focused(doc, a) && matches(a, "a:focus"),
                    "set_focus should focus the element");
        ok &= check(!matches(a, "a:focus-visible"), "set_focus with focus_visible=false should not set :focus-visible");
        ok &= check(!doc->set_focus(plain, true, container.redraw) && focused(doc, a),
                    "set_focus should ignore elements that cannot be focused");
        ok &= check(doc->set_focus(nullptr, false, container.redraw) && !doc->get_focused_element(),
                    "set_focus(nullptr) should clear the focus");
    }

    // inert and click suppression.
    {
        focus_container container;
        const auto      doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}#x{display:block;width:50px;height:20px}</style>"
            "<div id=inertwrap inert><a id=x href='#'>x</a><div id=y tabindex='0'>y</div></div>"
            "<a id=z href='#'>z</a>",
            &container);
        doc->render(800);

        const auto x = doc->root()->select_one("#x");
        const auto y = doc->root()->select_one("#y");
        const auto z = doc->root()->select_one("#z");

        ok &= check(x && y && z, "the inert test elements should exist");
        ok &= check(x->is_inert() && y->is_inert() && !z->is_inert(),
                    "is_inert should cover the element and its descendants");
        ok &= check(!x->can_be_focused() && !y->can_be_focused(),
                    "links and tabindex elements inside an inert subtree cannot be focused");

        const auto pos  = doc->focus_next(container.redraw);
        ok             &= check(pos && focused(doc, z), "focus navigation should skip inert subtrees");

        const position x_pos = x->get_placement();
        const int      x_cx  = static_cast<int>(x_pos.x + x_pos.width / 2);
        const int      x_cy  = static_cast<int>(x_pos.y + x_pos.height / 2);
        doc->on_lbutton_down(x_cx, x_cy, x_cx, x_cy, container.redraw);
        ok &= check(!matches(x, "a:active"), "an inert element should not get :active");
        doc->on_lbutton_up(x_cx, x_cy, x_cx, x_cy, container.redraw);
        ok &= check(container.clicked_anchors.empty(), "a click inside an inert subtree should not activate the link");
        ok &= check(!doc->get_focused_element(), "a click inside an inert subtree should not focus anything");

        if(pos)
        {
            click_box(doc, container, *pos);
        }
        ok &= check(container.clicked_anchors.size() == 1 && container.clicked_anchors.front() == "#",
                    "a click on a normal link should still activate it");
        ok &= check(focused(doc, z), "a click on a normal link should focus it");
    }

    // No focusable elements at all.
    {
        focus_container container;
        const auto      doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}</style><div>plain text</div>", &container);
        doc->render(800);

        ok &= check(!doc->focus_next(container.redraw), "focus_next returns nothing when no element can be focused");
        ok &= check(!doc->focus_prev(container.redraw), "focus_prev returns nothing when no element can be focused");
    }

    if(ok)
    {
        std::cout << "All focus tests passed\n";
    }
    return ok ? 0 : 1;
}
