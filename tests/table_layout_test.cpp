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

    const char* page_style = "<style>body{margin:0;font-size:16px}"
                             ".t{display:table;width:auto}"
                             ".c{display:table-cell;vertical-align:middle}"
                             ".btn{display:inline-block;padding:5px 8px;border:1px solid #000}"
                             ".spec{width:200px;height:32px}"
                             ".fixed{display:table;table-layout:fixed}"
                             "table.fixed{border-spacing:0;box-sizing:content-box}"
                             "table.fixed td{padding:0;border:0}"
                             "</style>";
} // namespace

int main()
{
    using namespace litehtml;
    bool ok = true;

    // A table with an automatic width and no percentage column widths shrinks to its content.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            std::string(page_style) +
                "<div style='width:700px'><div class=t id=t1><div class=c id=a1><div class=spec></div></div>"
                "<div class=c id=b1><span class=btn>Search</span></div></div></div>",
            &container);
        doc->render(800);

        const auto t1  = doc->root()->select_one("#t1");
        const auto a1  = doc->root()->select_one("#a1");
        const auto b1  = doc->root()->select_one("#b1");
        ok            &= check(t1 && a1 && b1, "the shrink-to-fit table should exist");
        // 200px content column + 48px label with 8px paddings and a 1px border per side.
        ok &= check(t1->get_placement().width == 266_px, "an auto-width table without percentages must shrink to fit");
        ok &= check(a1->get_placement().width == 200_px, "the content column should keep its content width");
        ok &= check(b1->get_placement().width == 66_px, "the button column should keep its content width");
    }

    // A table with an automatic width that has a percentage column width fills its containing block,
    // like browsers do, so percentage widths resolve against the available width.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            std::string(page_style) +
                "<div style='width:700px'><div class=t id=t2><div class=c id=a2><div class=spec></div></div>"
                "<div class=c id=b2 style='width:1%'><span class=btn>Search</span></div></div></div>",
            &container);
        doc->render(800);

        const auto t2  = doc->root()->select_one("#t2");
        const auto a2  = doc->root()->select_one("#a2");
        const auto b2  = doc->root()->select_one("#b2");
        ok            &= check(t2 && a2 && b2, "the percentage table should exist");
        ok            &= check(t2->get_placement().width == 700_px,
                               "an auto-width table with a percentage column must fill its block");
        ok &= check(b2->get_placement().width == 66_px, "the percentage column must not shrink below its content");
        ok &= check(a2->get_placement().width == 634_px, "the auto column must take the remaining width");
    }

    // Fixed layout: a specified table width is divided between the first-row cells;
    // the remaining columns share the rest equally.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            std::string(page_style) +
                "<div class='fixed' style='width:300px' id=t1>"
                "<div class=c style='width:100px' id=a1></div>"
                "<div class=c style='width:50px' id=b1></div>"
                "<div class=c id=c1></div></div>",
            &container);
        doc->render(800);

        const auto t1  = doc->root()->select_one("#t1");
        const auto a1  = doc->root()->select_one("#a1");
        const auto b1  = doc->root()->select_one("#b1");
        const auto c1  = doc->root()->select_one("#c1");
        ok            &= check(t1 && a1 && b1 && c1, "the fixed table should exist");
        ok &= check(t1->get_placement().width == 300_px, "a fixed table keeps its specified width");
        ok &= check(a1->get_placement().width == 100_px, "the first cell keeps its fixed width");
        ok &= check(b1->get_placement().width == 50_px, "the second cell keeps its fixed width");
        ok &= check(c1->get_placement().width == 150_px, "the auto column takes the remaining width");
    }

    // Fixed layout: a <col> width wins over a first-row cell width.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            std::string(page_style) +
                "<table class=fixed style='width:200px' id=t2><col style='width:50px'><col>"
                "<tr><td style='width:175px' id=a2></td><td id=b2></td></tr></table>",
            &container);
        doc->render(800);

        const auto t2  = doc->root()->select_one("#t2");
        const auto a2  = doc->root()->select_one("#a2");
        const auto b2  = doc->root()->select_one("#b2");
        ok            &= check(t2 && a2 && b2, "the column test table should exist");
        ok &= check(t2->get_placement().width == 200_px, "the fixed column table width is 200px");
        ok &= check(a2->get_placement().width == 50_px, "the <col> width overrides the cell width");
        ok &= check(b2->get_placement().width == 150_px, "the second column takes the remaining width");
    }

    // Fixed layout: cells after the first row do not change the column widths.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            std::string(page_style) +
                "<table class=fixed style='width:200px' id=t3>"
                "<tr><td style='width:50px' id=a3></td><td style='width:50px' id=b3></td></tr>"
                "<tr><td style='width:200px' id=c3></td><td style='width:200px' id=d3></td></tr></table>",
            &container);
        doc->render(800);

        const auto a3 = doc->root()->select_one("#a3");
        const auto b3 = doc->root()->select_one("#b3");
        const auto c3 = doc->root()->select_one("#c3");
        const auto d3 = doc->root()->select_one("#d3");
        ok           &= check(a3 && b3 && c3 && d3, "the two-row test table should exist");
        ok &= check(a3->get_placement().width == 100_px, "the first row width controls the column");
        ok &= check(b3->get_placement().width == 100_px, "the auto remainder is shared by the columns");
        ok &= check(c3->get_placement().width == 100_px, "later rows keep the first-row column width");
        ok &= check(d3->get_placement().width == 100_px, "later rows keep the first-row column width");
    }

    // Fixed layout: the table grows when the sum of pixel column widths exceeds the
    // specified width.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            std::string(page_style) +
                "<table class=fixed style='width:75px' id=t4>"
                "<tr><td style='width:25px' id=a4></td><td style='width:25px' id=b4></td>"
                "<td style='width:25px' id=c4></td><td style='width:25px' id=d4></td></tr></table>",
            &container);
        doc->render(800);

        const auto t4 = doc->root()->select_one("#t4");
        const auto a4 = doc->root()->select_one("#a4");
        const auto d4 = doc->root()->select_one("#d4");
        ok           &= check(t4 && a4 && d4, "the growing test table should exist");
        ok &= check(t4->get_placement().width == 100_px, "the table width is the sum of its columns");
        ok &= check(a4->get_placement().width == 25_px, "the pixel columns keep their widths");
        ok &= check(d4->get_placement().width == 25_px, "the last pixel column keeps its width");
    }

    // Fixed layout: a colspan cell in the first row has its width divided over the
    // columns it spans.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            std::string(page_style) +
                "<table class=fixed style='width:100px' id=t5>"
                "<tr><td colspan=2 style='width:98px' id=a5></td></tr>"
                "<tr><td id=b5></td><td id=c5></td></tr></table>",
            &container);
        doc->render(800);

        const auto a5 = doc->root()->select_one("#a5");
        const auto b5 = doc->root()->select_one("#b5");
        const auto c5 = doc->root()->select_one("#c5");
        ok           &= check(a5 && b5 && c5, "the colspan test table should exist");
        ok &= check(a5->get_placement().width == 100_px, "the spanning cell fills the table");
        ok &= check(b5->get_placement().width == 50_px, "the spanning width is divided evenly");
        ok &= check(c5->get_placement().width == 50_px, "the spanning width is divided evenly");
    }

    // Fixed layout with an auto table width falls back to automatic sizing.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            std::string(page_style) +
                "<div class=fixed id=t6><div class=c id=a6>abcd</div></div>",
            &container);
        doc->render(800);

        const auto t6 = doc->root()->select_one("#t6");
        const auto a6 = doc->root()->select_one("#a6");
        ok           &= check(t6 && a6, "the auto-width fixed table should exist");
        ok &= check(t6->get_placement().width == 32_px, "an auto-width fixed table shrinks to its content");
        ok &= check(a6->get_placement().width == 32_px, "the auto column keeps the content width");
    }

    // A height on a table cell is a minimum row height: an empty cell still
    // occupies that height.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            std::string(page_style) +
                "<table class=fixed style='width:100px' id=t7>"
                "<tr><td style='height:40px' id=a7></td></tr></table>",
            &container);
        doc->render(800);

        const auto t7 = doc->root()->select_one("#t7");
        const auto a7 = doc->root()->select_one("#a7");
        ok           &= check(t7 && a7, "the empty-cell height table should exist");
        ok &= check(t7->get_placement().height == 40_px, "an empty cell keeps its specified height");
        ok &= check(a7->get_placement().height == 40_px, "the empty cell fills the row height");
    }

    // Content taller than the specified cell height wins.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            std::string(page_style) +
                "<table class=fixed style='width:100px' id=t8>"
                "<tr><td style='height:10px' id=a8><div style='height:50px' id=b8></div></td></tr></table>",
            &container);
        doc->render(800);

        const auto t8 = doc->root()->select_one("#t8");
        const auto a8 = doc->root()->select_one("#a8");
        const auto b8 = doc->root()->select_one("#b8");
        ok           &= check(t8 && a8 && b8, "the overflowing-cell table should exist");
        ok &= check(t8->get_placement().height == 50_px, "taller content determines the row height");
        ok &= check(a8->get_placement().height == 50_px, "the cell grows to its content");
        ok &= check(b8->get_placement().height == 50_px, "the content keeps its height");
    }

    // Content shorter than the specified cell height grows the row.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            std::string(page_style) +
                "<table class=fixed style='width:100px' id=t9>"
                "<tr><td style='height:60px' id=a9><div style='height:20px' id=b9></div></td></tr></table>",
            &container);
        doc->render(800);

        const auto t9 = doc->root()->select_one("#t9");
        const auto a9 = doc->root()->select_one("#a9");
        const auto b9 = doc->root()->select_one("#b9");
        ok           &= check(t9 && a9 && b9, "the short-content table should exist");
        ok &= check(t9->get_placement().height == 60_px, "the specified cell height determines the row height");
        ok &= check(a9->get_placement().height == 60_px, "the cell fills the specified height");
        ok &= check(b9->get_placement().height == 20_px, "the content keeps its own height");
    }

    // A collapsed border is centered on the grid line (CSS 2.1 17.6.2): a wider border takes only
    // half of its extra width from the cell content, and the other half from its neighbour's.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0}"
            "table{border-collapse:collapse;table-layout:fixed;width:202px}"
            "td{padding:0;border:2px solid #000;height:10px}"
            "#w10{border-left:12px solid #000}</style>"
            "<table><tr><td id=a10></td><td id=w10></td></tr></table>"
            "<table><tr><td id=a11></td><td id=w11></td></tr></table>",
            &container);
        doc->render(800);

        const auto a10 = doc->root()->select_one("#a10");
        const auto w10 = doc->root()->select_one("#w10");
        const auto a11 = doc->root()->select_one("#a11");
        const auto w11 = doc->root()->select_one("#w11");
        ok &= check(a10 && w10 && a11 && w11, "the collapsed-border tables should exist");
        if(a10 && w10 && a11 && w11)
        {
            ok &= check(w10->get_placement().width == w11->get_placement().width - 5_px,
                        "a wider collapsed border takes only half of its extra width from the cell content");
            ok &= check(a10->get_placement().width == a11->get_placement().width - 5_px,
                        "a wider collapsed border takes the other half from the neighbouring cell content");
        }
    }

    if(ok)
    {
        std::cout << "All table layout tests passed\n";
    }
    return ok ? 0 : 1;
}
