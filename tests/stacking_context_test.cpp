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

    class click_container : public grid_test_container
    {
      public:
        std::string                      clicked;
        std::vector<litehtml::web_color> fills;

        void on_anchor_click(const char* url, const litehtml::element::ptr&) override
        {
            clicked = url;
        }
        void draw_solid_fill(litehtml::uint_ptr, const litehtml::background_layer&,
                             const litehtml::web_color& color) override
        {
            fills.push_back(color);
        }
    };

    void click(const litehtml::document::ptr& doc, litehtml::pixel_t x, litehtml::pixel_t y)
    {
        auto redraw = [](const litehtml::position&) {};
        doc->on_lbutton_down(x, y, x, y, redraw);
        doc->on_lbutton_up(x, y, x, y, redraw);
    }

    std::string hover_id(const litehtml::document::ptr& doc, litehtml::pixel_t x, litehtml::pixel_t y)
    {
        auto redraw = [](const litehtml::position&) {};
        doc->on_mouse_over(x, y, x, y, redraw);
        auto over = doc->get_over_element();
        if(!over)
        {
            return {};
        }
        auto id = over->get_attr("id");
        return id ? id : over->get_tagName();
    }
} // namespace

int main()
{
    using namespace litehtml;
    bool ok = true;

    {
        click_container container;
        auto            doc = document::createFromString(
            "<!doctype html><style>body{margin:0}.page{position:relative;width:80px;height:80px}"
            "a{position:absolute;left:0;top:0;width:40px;height:40px}"
            "#under{z-index:-1;background:#ff0000}#first{z-index:2;background:#00ff00}"
            "#last{z-index:2;background:#0000ff}</style>"
            "<div class=page><div><a id=first href=/first></a></div>"
            "<a id=under href=/under></a><div><a id=last href=/last></a></div></div>",
            &container);
        for(int i = 0; i < 2; ++i)
        {
            doc->render(800);
            container.fills.clear();
            doc->draw(0, 0, 0, nullptr);
            ok &= check(container.fills ==
                            std::vector<web_color>{web_color(255, 0, 0), web_color(0, 255, 0), web_color(0, 0, 255)},
                        "positioned paint order follows z-index and tree order across repeated layouts");
            ok &= check(hover_id(doc, 10, 10) == "last", "later equal-z positioned sibling wins hit-testing");
        }
    }

    for(const auto* layout : {"flex", "grid"})
    {
        click_container container;
        auto            doc = document::createFromString(
            std::string("<!doctype html><style>body{margin:0}.page{position:relative;width:200px;height:100px}"
                        ".layout{display:") +
                layout +
                ";width:200px;height:100px;grid-template-columns:80px;grid-template-rows:100px}"
                ".item{width:80px;height:100px;z-index:1}"
                "a{position:absolute;left:50%;top:10px;width:40px;height:40px}"
                "#inner{z-index:20;background:#ff0000}#sibling{z-index:2;background:#0000ff}</style>"
                "<div class=page><div class=layout><div class=item><div><a id=inner href=/inner></a></div>"
                "</div></div><a id=sibling href=/sibling></a></div>",
            &container);
        const auto inner = doc->root()->select_one("#inner");
        for(const auto mode : {render_all, render_no_fixed, render_fixed_only})
        {
            doc->render(800, mode);
            container.fills.clear();
            doc->draw(0, 0, 0, nullptr);
            ok &= check(inner && inner->get_placement().x == 100_px &&
                            container.fills == std::vector<web_color>{web_color(255, 0, 0), web_color(0, 0, 255)} &&
                            hover_id(doc, 110, 20) == "sibling",
                        std::string("static ") + layout +
                            " stacking context must not replace the positioned containing block");
        }
    }

    {
        click_container container;
        auto            doc = document::createFromString(
            "<!doctype html><style>body{margin:0}.page{position:relative;width:100px;height:100px}"
            "#wrapper{position:relative;z-index:1;width:80px;height:80px}"
            "a{position:absolute;left:10px;top:10px;width:40px;height:40px}"
            "#inner{z-index:20;background:#ff0000}#sibling{z-index:2;background:#0000ff}</style>"
            "<div class=page><div id=wrapper><a id=inner href=/inner></a></div>"
            "<a id=sibling href=/sibling></a></div>",
            &container);
        const auto wrapper  = doc->root()->select_one("#wrapper");
        const auto inner    = doc->root()->select_one("#inner");
        ok                 &= check(wrapper && inner, "changing stacking-context test elements exist");
        if(wrapper && inner)
        {
            struct state
            {
                element_position position;
                render_type      mode;
            };
            for(const auto current : {
                    state{element_position_relative, render_all       },
                    state{element_position_static,   render_fixed_only},
                    state{element_position_relative, render_fixed_only},
                    state{element_position_static,   render_no_fixed  },
                    state{element_position_relative, render_all       }
            })
            {
                wrapper->css().set_position(current.position);
                doc->render(800, current.mode);
                container.fills.clear();
                doc->draw(0, 0, 0, nullptr);
                const bool confined = current.position == element_position_relative;
                const auto expected = confined ? std::vector<web_color>{web_color(255, 0, 0), web_color(0, 0, 255)}
                                               : std::vector<web_color>{web_color(0, 0, 255), web_color(255, 0, 0)};
                ok &= check(container.fills == expected && inner->get_placement().x == 10_px &&
                                inner->get_placement().y == 10_px &&
                                hover_id(doc, 20, 20) == (confined ? "sibling" : "inner"),
                            "every render mode refreshes changed stacking ownership without changing geometry");
                click(doc, 20, 20);
                ok &= check(container.clicked == (confined ? "/sibling" : "/inner"),
                            "clicks follow refreshed stacking ownership");
            }
        }
    }

    {
        // Links inside position:absolute; z-index:auto containers must remain
        // clickable, even though the positioned descendants are hoisted into the
        // root stacking context.
        click_container container;
        auto            doc = document::createFromString(
            "<style>"
            "body{margin:0;font-size:16px;line-height:20px}"
            ".page{position:relative;height:100px}"
            ".navbar{position:absolute;top:0;left:0;height:22px}"
            ".head{float:left;padding:2px 4px}"
            ".tools{position:absolute;bottom:0;left:0}"
            ".tools a{display:block;padding:4px}"
            "</style>"
            "<div class='page'>"
            "<div class='navbar'><div class='head'><a id='cpp' href='/cpp'>C++</a></div></div>"
            "<div class='tools'><a id='discussion' href='/talk'>Discussion</a></div>"
            "</div>",
            &container);
        doc->render(800);

        ok &= check(hover_id(doc, 6, 4) == "cpp", "absolute navbar link should be the hover target");
        click(doc, 6, 4);
        ok &= check(container.clicked == "/cpp", "absolute navbar link should activate on click");

        container.clicked.clear();
        ok &= check(hover_id(doc, 6, 96) == "discussion", "absolute tools link should be the hover target");
        click(doc, 6, 96);
        ok &= check(container.clicked == "/talk", "absolute tools link should activate on click");
    }

    {
        // A relative z-index:auto wrapper does not create a stacking context, so
        // its absolutely positioned link belongs to the root context and must be
        // reachable behind the wrapper's own box.
        click_container container;
        auto            doc = document::createFromString(
            "<style>"
            "body{margin:0;font-size:16px;line-height:20px}"
            ".page{position:relative;height:100px}"
            ".wrapper{position:relative;z-index:auto;width:80px;height:80px;background:#eee}"
            ".wrapper a{position:absolute;left:10px;top:10px;width:60px;height:20px}"
            "</style>"
            "<div class='page'><div class='wrapper'><a id='inner' href='/inner'>inner</a></div></div>",
            &container);
        doc->render(800);

        ok &=
            check(hover_id(doc, 15, 15) == "inner", "link in relative z-index:auto wrapper should be the hover target");
        click(doc, 15, 15);
        ok &= check(container.clicked == "/inner", "link in relative z-index:auto wrapper should activate on click");
    }

    {
        // A positioned element with a real z-index establishes a stacking
        // context, so a higher-z sibling must win over its positioned child.
        click_container container;
        auto            doc = document::createFromString(
            "<style>"
            "body{margin:0;font-size:16px;line-height:20px}"
            ".page{position:relative;height:100px}"
            ".wrapper{position:relative;z-index:2;width:80px;height:80px;background:#eee}"
            ".wrapper a{position:absolute;left:10px;top:10px;width:40px;height:20px}"
            ".sibling{position:absolute;left:15px;top:15px;width:40px;height:40px;z-index:5}"
            "</style>"
            "<div class='page'><div class='wrapper'><a id='inner' href='/inner'>inner</a></div>"
            "<a id='sibling' class='sibling' href='/sibling'>sibling</a></div>",
            &container);
        doc->render(800);

        ok &= check(hover_id(doc, 20, 20) == "sibling",
                    "higher-z sibling should be the hover target over a stacking-context child");
        click(doc, 20, 20);
        ok &= check(container.clicked == "/sibling", "higher-z sibling should activate on click");
    }

    {
        // A flex item with z-index creates a stacking context and paints above a
        // later flex item it overlaps.
        click_container container;
        auto doc = document::createFromString("<style>"
                                              "body{margin:0;font-size:16px;line-height:20px}"
                                              ".flex{display:flex;width:200px;height:100px}"
                                              ".item1{width:120px;height:100px;z-index:1}"
                                              ".item2{width:120px;height:100px;margin-left:-60px}"
                                              "</style>"
                                              "<div class='flex'><a id='a1' class='item1' href='/a1'>one</a><a id='a2' "
                                              "class='item2' href='/a2'>two</a></div>",
                                              &container);
        doc->render(800);

        ok &= check(hover_id(doc, 80, 50) == "a1", "z-index flex item should be the hover target in the overlap");
        click(doc, 80, 50);
        ok &= check(container.clicked == "/a1", "z-index flex item should activate on click");
    }

    {
        // A grid item with z-index creates a stacking context and paints above a
        // later grid item in the same cell.
        click_container container;
        auto            doc = document::createFromString(
            "<style>"
            "body{margin:0;font-size:16px;line-height:20px}"
            ".grid{display:grid;width:200px;height:100px;grid-template-columns:200px;grid-template-rows:100px}"
            ".item1{grid-column:1;grid-row:1;z-index:1}"
            ".item2{grid-column:1;grid-row:1}"
            "</style>"
            "<div class='grid'><a id='g1' class='item1' href='/g1'>one</a><a id='g2' class='item2' "
            "href='/g2'>two</a></div>",
            &container);
        doc->render(800);

        ok &= check(hover_id(doc, 50, 50) == "g1", "z-index grid item should be the hover target in the overlap");
        click(doc, 50, 50);
        ok &= check(container.clicked == "/g1", "z-index grid item should activate on click");
    }

    {
        // A table caption shifts the cells; hit-testing must follow the table grid
        // (like draw_children does) instead of the render-tree child positions.
        click_container container;
        auto            doc = document::createFromString(
            "<style>"
            "body{margin:0;font-size:16px;line-height:20px}"
            "table{width:220px;border:1px solid #999;background:#f8f9fa}"
            "td{padding:4px}"
            "</style>"
            "<table><caption>Title</caption>"
            "<tbody><tr><td><a id='cell-a' href='/cell-a'>First link</a></td></tr>"
            "<tr><td><a id='cell-b' href='/cell-b'>Second link</a></td></tr></tbody></table>",
            &container);
        doc->render(800);

        ok &= check(hover_id(doc, 10, 30) == "cell-a",
                    "link in the first table cell below a caption should be the hover target");
        click(doc, 10, 30);
        ok &= check(container.clicked == "/cell-a",
                    "link in the first table cell below a caption should activate on click");
    }

    {
        // Links nested inside inline elements (sup, spans) must hit-test through
        // to the anchor instead of returning the outer inline element.
        click_container container;
        auto            doc = document::createFromString(
            "<style>"
            "body{margin:0;font-size:16px;line-height:20px}"
            "table{width:300px}td{padding:4px}sup{font-size:12px}"
            "</style>"
            "<p>Text with reference <sup><a id='ref' href='#cite-12'>"
            "<span class='mw-reflink-text'><span class='cite-bracket'>[</span>12<span class='cite-bracket'>]</span>"
            "</span></a></sup> end</p>"
            "<table><tbody>"
            "<tr><td><span class='plainlinks'><a id='coord' href='/coord'>"
            "<span class='geo-default'><span class='geo-dms'>42deg</span></span></a></span></td></tr>"
            "<tr><td><span lang='la'><a id='veritas' href='/veritas'>Veritas</a></span></td></tr>"
            "</tbody></table>",
            &container);
        doc->render(800);

        click(doc, 160, 22);
        ok &= check(container.clicked == "#cite-12", "superscript reference link should activate on click");

        container.clicked.clear();
        ok &= check(hover_id(doc, 6, 92) == "veritas",
                    "link wrapped in a span should be the hover target, not the outer span");
        click(doc, 6, 92);
        ok &= check(container.clicked == "/veritas", "link wrapped in a span should activate on click");

        container.clicked.clear();
        click(doc, 6, 62);
        ok &= check(container.clicked == "/coord", "nested-span coordinate link should activate on click");
    }

    return ok ? 0 : 1;
}
