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

    class reflow_container : public grid_test_container
    {
      public:
        // Real containers create a new handle for every font description, so style changes that
        // only affect painting (text decoration) still produce a new handle.
        litehtml::uint_ptr create_font(const litehtml::font_description& description, const litehtml::document*,
                                       litehtml::font_metrics*           metrics) override
        {
            grid_test_container::create_font(description, nullptr, metrics);
            return next_font++;
        }

        // Boxes reported through the redraw callback of the last mouse event.
        litehtml::position::vector redraws;

        const std::function<void(const litehtml::position&)> redraw = [this](const litehtml::position& box) {
            redraws.push_back(box);
        };

        void clear_redraws()
        {
            redraws.clear();
        }

      private:
        litehtml::uint_ptr next_font = 1;
    };

    // Every reported box must stay inside the given rectangle.
    bool redraws_within(const litehtml::position::vector& boxes, const litehtml::position& area)
    {
        for(const auto& box : boxes)
        {
            if(box.right() > area.right() || box.bottom() > area.bottom() || box.x < area.x || box.y < area.y)
            {
                return false;
            }
        }
        return true;
    }
} // namespace

int main()
{
    using namespace litehtml;
    bool ok = true;

    // A layout change that is contained in a block with definite sizes re-lays out only that block.
    {
        reflow_container container;
        const auto       doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}"
            "#wrapper{width:200px;height:100px}#box{width:100px;height:20px;background:#ddd}"
            "#box:hover{padding:10px;background:#333}#after{width:10px;height:10px}#outside{width:20px;height:20px}"
            "</style><div id=wrapper><div id=box></div><div id=after></div></div><div id=outside></div>",
            &container);
        doc->render(800);
        const auto box     = doc->root()->select_one("#box");
        const auto after   = doc->root()->select_one("#after");
        const auto outside = doc->root()->select_one("#outside");
        const auto wrapper = doc->root()->select_one("#wrapper");

        ok &= check(box && after && outside && wrapper && box->get_placement().height == 20_px,
                    "the hovered box should have its initial height");

        const bool hovered = doc->on_mouse_over(10, 10, 10, 10, container.redraw);
        ok &=
            check(hovered && !container.redraws.empty(), "hovering a layout-changing element should request a redraw");
        ok &= check(box->get_placement().y == 10_px, "hovering should apply the new padding");
        ok &= check(after->get_placement().y == 40_px, "the sibling inside the wrapper should move below the box");
        ok &= check(outside->get_placement().y == 100_px, "content outside the wrapper must not move");
        ok &= check(redraws_within(container.redraws, position(0_px, 0_px, 200_px, 100_px)),
                    "a contained layout change should only report boxes inside the definite-size block");

        container.clear_redraws();
        doc->on_mouse_leave(container.redraw);
        ok &= check(box->get_placement().height == 20_px && after->get_placement().y == 20_px,
                    "leaving the hover should restore the previous layout");
        ok &= check(redraws_within(container.redraws, position(0_px, 0_px, 200_px, 100_px)),
                    "leaving the hover should only report boxes inside the definite-size block");
    }

    // A paint only change reports the area but leaves the geometry alone.
    {
        reflow_container container;
        const auto       doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}"
            "#wrapper{width:200px;height:100px}#box{width:100px;height:20px}#box:hover{color:#f00;background:#333}"
            "#after{width:10px;height:10px}</style>"
            "<div id=wrapper><div id=box>text</div><div id=after></div></div>",
            &container);
        doc->render(800);
        const auto box   = doc->root()->select_one("#box");
        const auto after = doc->root()->select_one("#after");

        const bool hovered = doc->on_mouse_over(10, 10, 10, 10, container.redraw);
        ok &= check(hovered && !container.redraws.empty(), "a paint-only hover should request a redraw");
        ok &= check(box->get_placement().height == 20_px && after->get_placement().y == 20_px,
                    "a paint-only hover must not change the layout");
        ok &= check(redraws_within(container.redraws, position(0_px, 0_px, 200_px, 100_px)),
                    "a paint-only hover should only report the changed box");
    }

    // A paint-only change never lays the document out again, not even without a definite-size ancestor.
    {
        reflow_container container;
        const auto       doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}#box{width:100px;height:20px}"
            "#box:hover{background:#333}</style><div id=box></div>",
            &container);
        doc->render(800);

        const bool hovered = doc->on_mouse_over(10, 10, 10, 10, container.redraw);
        ok &= check(hovered && !container.redraws.empty(), "a paint-only hover should request a redraw");
        ok &= check(redraws_within(container.redraws, position(0_px, 0_px, 100_px, 20_px)),
                    "a paint-only hover must not lay out the whole document again");
    }

    // A paint-only change inside an inline box must not lay out the document again: the font handle
    // changes for text-decoration, but the layout does not.
    {
        reflow_container container;
        const auto doc = document::createFromString("<style>a{text-decoration:none}a:hover{text-decoration:underline}"
                                                    "p{margin:0;font-size:16px;line-height:20px}</style>"
                                                    "<p>text <a href='#'>link</a> tail</p>",
                                                    &container);
        doc->render(800);
        const auto text = doc->root()->select_one("p");

        const bool hovered = doc->on_mouse_over(48, 10, 48, 10, container.redraw);
        ok &= check(hovered && !container.redraws.empty(), "underlining a hovered link should request a redraw");
        ok &= check(redraws_within(container.redraws, position(0_px, 0_px, 400_px, 40_px)),
                    "a text-decoration hover must not lay out the whole document again");
        ok &=
            check(text && text->get_placement().height == 20_px, "a text-decoration hover must not change the layout");
    }

    // Without a definite-size ancestor the whole document is laid out again.
    {
        reflow_container container;
        const auto       doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}#box{width:100px;height:20px}"
            "#box:hover{padding:10px}#after{width:10px;height:10px}</style>"
            "<div id=box></div><div id=after></div>",
            &container);
        doc->render(800);
        const auto box   = doc->root()->select_one("#box");
        const auto after = doc->root()->select_one("#after");

        const bool hovered  = doc->on_mouse_over(10, 10, 10, 10, container.redraw);
        ok                 &= check(hovered && box->get_placement().y == 10_px,
                                    "a hover without a definite-size ancestor should still apply the new padding");
        ok                 &= check(after->get_placement().y == 40_px,
                                    "the following sibling should move when the document is laid out again");
    }

    // The relative shift of a re-laid-out block is preserved.
    {
        reflow_container container;
        const auto doc = document::createFromString("<style>body{margin:0;font-size:16px;line-height:20px}"
                                                    "#wrapper{position:relative;left:20px;width:200px;height:100px}"
                                                    "#child{width:50px;height:20px}#child:hover{padding:5px}</style>"
                                                    "<div id=wrapper><div id=child></div></div>",
                                                    &container);
        doc->render(800);
        const auto wrapper = doc->root()->select_one("#wrapper");
        const auto child   = doc->root()->select_one("#child");

        doc->on_mouse_over(25, 10, 25, 10, container.redraw);
        ok &= check(wrapper->get_placement().x == 20_px && child->get_placement().x == 25_px,
                    "a relatively positioned re-layout root should keep its shift");
        ok &= check(child->get_placement().y == 5_px, "the hovered padding should be applied");
    }

    // A hover on an ancestor selector changes a descendant.
    {
        reflow_container container;
        const auto       doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}#outer{width:300px;height:100px}"
            "#inner{width:100px;height:20px}#outer:hover #inner{width:150px}</style>"
            "<div id=outer><div id=inner></div></div>",
            &container);
        doc->render(800);
        const auto inner = doc->root()->select_one("#inner");

        doc->on_mouse_over(10, 10, 10, 10, container.redraw);
        ok &= check(inner->get_placement().width == 150_px,
                    "an ancestor hover should apply the new size of the descendant");
    }

    // A box with a definite height but an auto width still contains the change.
    {
        reflow_container container;
        const auto       doc = document::createFromString(
            "<style>body{margin:0;font-size:16px;line-height:20px}"
            "#wrapper{height:100px}#box{width:100px;height:20px}#box:hover{padding:10px}</style>"
            "<div id=wrapper><div id=box></div><div id=after>x</div></div>",
            &container);
        doc->render(800);
        const auto after = doc->root()->select_one("#after");

        doc->on_mouse_over(10, 10, 10, 10, container.redraw);
        ok &= check(after && after->get_placement().y == 40_px,
                    "a change inside an auto-width block should be laid out again");
    }

    // Content of an absolutely positioned box is laid out again as well.
    {
        reflow_container container;
        const auto       doc = document::createFromString(
            "<style>body{margin:0}#wrapper{position:relative;width:200px;height:100px}"
            "#box{position:absolute;left:10px;top:10px;width:100px;height:60px;font-size:16px;line-height:20px}"
            "#box:hover{font-size:32px;line-height:40px}</style>"
            "<div id=wrapper><div id=box>text<div id=inner></div></div></div>",
            &container);
        doc->render(800);
        const auto inner = doc->root()->select_one("#inner");

        doc->on_mouse_over(20, 20, 20, 20, container.redraw);
        ok &= check(inner && inner->get_placement().y == 50_px,
                    "a positioned box should lay out its content again after a style change");
    }

    return ok ? 0 : 1;
}
