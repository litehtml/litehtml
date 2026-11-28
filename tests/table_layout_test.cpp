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

    // CSS 2.1 17.2: the first header group is placed before all other rows and the first footer
    // group after them, whatever their document order is.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0}"
            ".t{display:table;border-spacing:0}"
            ".rg{display:table-row-group}"
            ".hg{display:table-header-group}"
            ".fg{display:table-footer-group}"
            ".r{display:table-row}"
            ".c{display:table-cell;width:10px;height:10px}</style>"
            "<div class=t id=t12>"
            "<div class=fg><div class=r><div class=c id=f12></div></div></div>"
            "<div class=rg><div class=r><div class=c id=b12a></div></div></div>"
            "<div class=hg><div class=r><div class=c id=h12></div></div></div>"
            "<div class=rg><div class=r><div class=c id=b12b></div></div></div>"
            "</div>",
            &container);
        doc->render(800);

        const auto t12  = doc->root()->select_one("#t12");
        const auto h12  = doc->root()->select_one("#h12");
        const auto b12a = doc->root()->select_one("#b12a");
        const auto b12b = doc->root()->select_one("#b12b");
        const auto f12  = doc->root()->select_one("#f12");
        ok &= check(t12 && h12 && b12a && b12b && f12, "the header/footer group table should exist");
        if(h12 && b12a && b12b && f12)
        {
            ok &= check(h12->get_placement().y < b12a->get_placement().y, "the header group is placed first");
            ok &= check(b12a->get_placement().y < b12b->get_placement().y, "the row groups keep their order");
            ok &= check(b12b->get_placement().y < f12->get_placement().y, "the footer group is placed last");
            ok &= check(t12->get_placement().height == 40_px, "every row of the groups keeps its height");
        }
    }

    // Only the first header and the first footer group are used that way; further ones behave as
    // ordinary row groups and stay where they are (CSS 2.1 17.2.1).
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0}"
            ".t{display:table}"
            ".rg{display:table-row-group}"
            ".hg{display:table-header-group}"
            ".fg{display:table-footer-group}"
            ".r{display:table-row}"
            ".c{display:table-cell;width:10px;height:10px}</style>"
            "<div class=t>"
            "<div class=rg><div class=r><div class=c id=b13></div></div></div>"
            "<div class=fg><div class=r><div class=c id=f13a></div></div></div>"
            "<div class=fg><div class=r><div class=c id=f13b></div></div></div>"
            "<div class=hg><div class=r><div class=c id=h13a></div></div></div>"
            "<div class=hg><div class=r><div class=c id=h13b></div></div></div>"
            "</div>",
            &container);
        doc->render(800);

        const auto b13  = doc->root()->select_one("#b13");
        const auto f13a = doc->root()->select_one("#f13a");
        const auto f13b = doc->root()->select_one("#f13b");
        const auto h13a = doc->root()->select_one("#h13a");
        const auto h13b = doc->root()->select_one("#h13b");
        ok &= check(b13 && f13a && f13b && h13a && h13b, "the multiple group table should exist");
        if(b13 && f13a && f13b && h13a && h13b)
        {
            ok &= check(h13a->get_placement().y < b13->get_placement().y, "the first header group is placed first");
            ok &= check(b13->get_placement().y < f13b->get_placement().y, "a second footer group stays in place");
            ok &= check(f13b->get_placement().y < h13b->get_placement().y, "a second header group stays in place");
            ok &= check(h13b->get_placement().y < f13a->get_placement().y, "the first footer group is placed last");
        }
    }

    // CSS 2.1 17.5.3: cells aligned at the baseline have their first baseline put on the baseline of
    // the row, which is the largest distance from the cell top to the baseline.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0}"
            ".t{display:table;border-spacing:0}"
            ".r{display:table-row}"
            ".c{display:table-cell;vertical-align:baseline;padding:0}"
            "</style>"
            "<div class=t><div class=r>"
            "<div class=c id=small14><div style='font-size:10px' id=small14in>x</div></div>"
            "<div class=c id=big14><div style='font-size:30px' id=big14in>x</div></div>"
            "</div></div>",
            &container);
        doc->render(800);

        const auto small = doc->root()->select_one("#small14in");
        const auto big   = doc->root()->select_one("#big14in");
        ok &= check(small && big, "the baseline cells should exist");
        if(small && big)
        {
            ok &= check(small->get_placement().y == 20_px,
                        "a baseline cell is moved down to the baseline of the row");
            ok &= check(big->get_placement().y == 0_px, "the tallest baseline cell sets the row baseline");
        }
    }

    // CSS 2.1 17.5.3: sub, super, text-top, text-bottom and <length>/<percentage> do not apply to
    // cells; the cell is aligned at the baseline instead.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0}"
            ".t{display:table;border-spacing:0}"
            ".r{display:table-row}"
            ".c{display:table-cell;padding:0}"
            ".sub{vertical-align:sub}"
            ".len{vertical-align:20px}"
            "</style>"
            "<div class=t><div class=r>"
            "<div class='c sub' id=sub15><div style='font-size:10px' id=sub15in>x</div></div>"
            "<div class='c len' id=len15><div style='font-size:10px' id=len15in>x</div></div>"
            "<div class=c id=base15><div style='font-size:30px' id=base15in>x</div></div>"
            "</div></div>",
            &container);
        doc->render(800);

        const auto sub  = doc->root()->select_one("#sub15in");
        const auto len  = doc->root()->select_one("#len15in");
        const auto base = doc->root()->select_one("#base15in");
        ok &= check(sub && len && base, "the fallback vertical-align cells should exist");
        if(sub && len && base)
        {
            ok &= check(sub->get_placement().y == 20_px, "'vertical-align: sub' on a cell aligns it at the baseline");
            ok &= check(len->get_placement().y == 20_px, "a length on a cell aligns it at the baseline");
            ok &= check(base->get_placement().y == 0_px, "the tallest baseline cell sets the row baseline");
        }
    }

    // CSS 2.1 17.5.3, 10.8.1: the baseline of a cell is the baseline of the first in-flow table row
    // in it, and the baseline of a table is the baseline of its first row.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0}"
            ".t{display:table;border-spacing:0}"
            ".r{display:table-row}"
            ".c{display:table-cell;vertical-align:baseline;padding:0}"
            "</style>"
            "<div class=t><div class=r>"
            "<div class=c id=nested16>"
            "<div class=t><div class=r><div class=c><span style='font-size:30px' id=inner16>x</span></div></div></div>"
            "</div>"
            "<div class=c><span style='font-size:30px' id=ref16>x</span></div>"
            "</div></div>",
            &container);
        doc->render(800);

        const auto inner = doc->root()->select_one("#inner16");
        const auto ref   = doc->root()->select_one("#ref16");
        ok &= check(inner && ref, "the nested table cells should exist");
        if(inner && ref)
        {
            ok &= check(inner->get_placement().y == ref->get_placement().y,
                        "a table inside a cell provides the cell baseline from its first row");
        }
    }

    // CSS 2.1 6.2: the font properties are inherited into tables.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0}.t{display:table}.r{display:table-row}.c{display:table-cell}</style>"
            "<div style='font-size:20px'><div class=t><div class=r><div class=c id=cell17>x</div></div></div></div>",
            &container);
        doc->render(800);

        const auto cell = doc->root()->select_one("#cell17");
        ok &= check(cell != nullptr, "the inherited font table should exist");
        if(cell)
        {
            ok &= check(cell->css().get_font_size() == 20_px, "a table cell inherits the font size of its ancestors");
        }
    }

    // A whitespace-only anonymous context before a nested table has no baseline of its own.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0;font:20px/20px sans-serif}"
            "table,.inner,.lines{border-spacing:0}"
            "td,.inner>div{display:table-cell;vertical-align:baseline;padding:0}"
            ".inner,.lines{display:table}.lines>span{display:table-row}</style>"
            "<table><tr><td><div class=inner>"
            "<div> <a><span class=lines><span><span id=label18>array</span></span></span></a> </div>"
            "<div><span class=lines><span><span id=marker18>tag</span></span></span></div>"
            "</div></td></tr><tr><td><div id=next18>vector</div></td></tr></table>",
            &container);
        doc->render(800);

        const auto label  = doc->root()->select_one("#label18");
        const auto marker = doc->root()->select_one("#marker18");
        const auto next   = doc->root()->select_one("#next18");
        ok &= check(label && marker && next, "the nested baseline table should exist");
        if(label && marker && next)
        {
            ok &= check(label->get_placement().y == marker->get_placement().y,
                        "empty anonymous contexts must not replace the nested table baseline");
            ok &= check(next->get_placement().y >= label->get_placement().y + label->get_placement().height,
                        "the next row must not overlap the nested table label");
        }
    }

    // Empty and out-of-flow descendants do not provide a cell's first in-flow baseline.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0}table{border-spacing:0}td{padding:0;vertical-align:baseline}"
            ".text{font-size:20px;line-height:20px}</style>"
            "<table><tr><td><div></div><div style='position:absolute;font-size:40px'>a</div>"
            "<div style='float:left;font-size:40px'>b</div><div class=text><span id=flow19>x</span></div></td>"
            "<td><div class=text><span id=ref19>x</span></div></td></tr></table>",
            &container);
        doc->render(800);

        const auto flow = doc->root()->select_one("#flow19");
        const auto ref  = doc->root()->select_one("#ref19");
        ok &= check(flow && ref, "the in-flow baseline cells should exist");
        if(flow && ref)
        {
            ok &= check(flow->get_placement().y == ref->get_placement().y,
                        "empty, positioned and floating boxes must not supply a cell baseline");
        }
    }

    // A line box inside a zero-height block is still a valid first baseline.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0}table{border-spacing:0}td{padding:0;vertical-align:baseline}</style>"
            "<table><tr><td><div style='height:0;overflow:hidden;font-size:30px;line-height:30px'>x</div>"
            "<div style='font-size:10px;line-height:10px'>later</div></td>"
            "<td><div id=ref20 style='font-size:20px;line-height:20px'>x</div></td></tr></table>",
            &container);
        doc->render(800);

        const auto ref = doc->root()->select_one("#ref20");
        ok &= check(ref != nullptr, "the zero-height baseline reference should exist");
        if(ref)
        {
            ok &= check(ref->get_placement().y == 10_px, "a zero-height block's first line still sets the baseline");
        }
    }

    // A shifted non-spanning cell can determine the row's minimum height.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0;font:20px/20px sans-serif}table{border-spacing:0}"
            "td{padding:0;vertical-align:baseline}</style>"
            "<table id=table21><tr><td><div id=tall21>a<br>b</div></td>"
            "<td style='padding-top:40px'><div id=anchor21>x</div></td></tr>"
            "<tr><td><div id=next21>next</div></td></tr></table>",
            &container);
        doc->render(800);

        const auto table  = doc->root()->select_one("#table21");
        const auto tall   = doc->root()->select_one("#tall21");
        const auto anchor = doc->root()->select_one("#anchor21");
        const auto next   = doc->root()->select_one("#next21");
        ok &= check(table && tall && anchor && next, "the shifted row height table should exist");
        if(table && tall && anchor && next)
        {
            ok &= check(tall->get_placement().y == anchor->get_placement().y, "the first line baselines must align");
            ok &= check(next->get_placement().y >= tall->get_placement().y + tall->get_placement().height,
                        "a row must include the full shifted height of its baseline cells");
            ok &= check(table->get_placement().height == 100_px, "the table includes the expanded row and next row");
        }
    }

    // A specified cell height is a minimum, not extra space to move with the content.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0}table{border-spacing:0}"
            "td{padding:0;vertical-align:baseline}"
            ".small{font-size:10px;line-height:10px}"
            ".large{font-size:30px;line-height:30px}</style>"
            "<table id=single22><tr><td style='height:100px'><div class=small id=small22>x</div></td>"
            "<td><div class=large id=large22>x</div></td></tr></table>"
            "<table id=span22><tr><td rowspan=2 style='height:100px'><div class=small id=spantext22>x</div></td>"
            "<td><div class=large id=spanref22>x</div></td></tr>"
            "<tr><td><div class=small>x</div></td></tr></table>",
            &container);
        doc->render(800);

        const auto single   = doc->root()->select_one("#single22");
        const auto span     = doc->root()->select_one("#span22");
        const auto small    = doc->root()->select_one("#small22");
        const auto large    = doc->root()->select_one("#large22");
        const auto spantext = doc->root()->select_one("#spantext22");
        const auto spanref  = doc->root()->select_one("#spanref22");
        ok &= check(single && span && small && large && spantext && spanref, "the minimum height tables should exist");
        if(single && span && small && large && spantext && spanref)
        {
            ok &= check(single->get_placement().height == 100_px,
                        "a baseline shift must not increase a fitting cell minimum");
            ok &= check(span->get_placement().height == 100_px,
                        "a baseline shift must not increase a fitting rowspan minimum");
            ok &= check(small->get_placement().y == large->get_placement().y + 20_px,
                        "the single-row cell content remains baseline-aligned");
            ok &= check(spantext->get_placement().y == spanref->get_placement().y + 20_px,
                        "the spanning cell content remains baseline-aligned");
        }
    }

    // Spanning cells use the rows' post-alignment heights and include inter-row border spacing.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0}table{border-spacing:0 4px}td{padding:0;vertical-align:baseline}"
            ".small{font:10px/10px sans-serif}.large{font:30px/30px sans-serif}"
            "#collapsed23{border-collapse:collapse}</style>"
            "<table id=baseline23><tr><td><div class=small>x</div></td>"
            "<td rowspan=2><div class=large>x</div></td></tr><tr><td></td></tr></table>"
            "<table id=top23><tr><td rowspan=2 style='height:30px;vertical-align:top'></td>"
            "<td style='height:10px;vertical-align:top'></td></tr>"
            "<tr><td style='height:10px;vertical-align:top'></td></tr></table>"
            "<table id=collapsed23><tr><td><div class=small>x</div></td>"
            "<td rowspan=2><div class=large>x</div></td></tr><tr><td></td></tr></table>",
            &container);
        doc->render(800);

        const auto baseline  = doc->root()->select_one("#baseline23");
        const auto top       = doc->root()->select_one("#top23");
        const auto collapsed = doc->root()->select_one("#collapsed23");
        ok &= check(baseline && top && collapsed, "the spanning-row tables should exist");
        if(baseline && top && collapsed)
        {
            ok &= check(baseline->get_placement().height == 42_px,
                        "baseline growth in the first row must not be added again to the last row");
            ok &= check(top->get_placement().height == 38_px,
                        "inter-row spacing counts toward a top-aligned cell's spanning height");
            ok &= check(collapsed->get_placement().height == 30_px,
                        "collapsed rows must fit the spanning cell without separate-border spacing");
        }
    }

    // CSS 2.1 17.2.1: consecutive row groups share one anonymous table, so their columns line up.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0;font-size:16px}"
            ".rg{display:table-row-group}.r{display:table-row}.c{display:table-cell}</style>"
            "<div><div class=rg><div class=r><div class=c id=ga>abcdef</div></div></div>"
            "<div class=rg><div class=r><div class=c id=gb>a</div></div></div></div>",
            &container);
        doc->render(800);

        const auto ga = doc->root()->select_one("#ga");
        const auto gb = doc->root()->select_one("#gb");
        ok           &= check(ga && gb, "the row group cells should exist");
        if(ga && gb)
        {
            ok &= check(ga->get_placement().width == 48_px && gb->get_placement().width == 48_px,
                        "row groups of one block must share the anonymous table columns");
            ok &= check(ga->get_placement().y < gb->get_placement().y, "the second row group is below the first");
        }
    }

    // CSS 2.1 17.2.1: a table nested between two cells becomes a cell of the same row.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0;font-size:16px}"
            ".t{display:table}.r{display:table-row}.c{display:table-cell}</style>"
            "<div class=t><div class=c id=na>aa</div>"
            "<div class=t id=nested><div class=r><div class=c>bb</div></div></div>"
            "<div class=c id=nc>cc</div></div>",
            &container);
        doc->render(800);

        const auto na     = doc->root()->select_one("#na");
        const auto nested = doc->root()->select_one("#nested");
        const auto nc     = doc->root()->select_one("#nc");
        ok               &= check(na && nested && nc, "the nested table should exist");
        if(na && nested && nc)
        {
            ok &= check(na->get_placement().y == nc->get_placement().y,
                        "a nested table between two cells must not break the row");
            ok &= check(na->get_placement().x < nested->get_placement().x &&
                            nested->get_placement().x < nc->get_placement().x,
                        "the nested table sits between the two cells");
        }
    }

    // CSS 2.1 17.2.1: cells inside an inline box generate an inline-table, so the text around
    // them stays on the same line.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0;font-size:16px}.c{display:table-cell}</style>"
            "<div id=line>a<span><span class=c id=ia>b</span><span class=c id=ic>c</span></span>d</div>",
            &container);
        doc->render(800);

        const auto line = doc->root()->select_one("#line");
        const auto ia   = doc->root()->select_one("#ia");
        const auto ic   = doc->root()->select_one("#ic");
        ok             &= check(line && ia && ic, "the inline table cells should exist");
        if(line && ia && ic)
        {
            ok &= check(line->get_placement().height == 16_px,
                        "cells inside an inline box must not turn the content into blocks");
            ok &= check(ia->get_placement().y == ic->get_placement().y, "the cells share one row");
            ok &= check(ia->get_placement().x < ic->get_placement().x, "the cells are laid out in order");
        }
    }

    // The cellpadding attribute maps to the padding of the cells, overriding the 1px default.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0;font-size:16px}</style>"
            "<table cellpadding=0 id=pad0><tr><td id=p0>ab</td></tr></table>"
            "<table id=pad1><tr><td id=p1>ab</td></tr></table>",
            &container);
        doc->render(800);

        const auto pad0 = doc->root()->select_one("#pad0");
        const auto pad1 = doc->root()->select_one("#pad1");
        ok             &= check(pad0 && pad1, "the cellpadding tables should exist");
        if(pad0 && pad1)
        {
            ok &= check(pad0->get_placement().width == 20_px,
                        "cellpadding=0 removes the default cell padding (2px spacing + 16px text)");
            ok &= check(pad1->get_placement().width == 22_px,
                        "without cellpadding the cells keep the 1px default padding");
        }
    }

    // White space of a `white-space: pre` row is content and gets a cell of its own; collapsible
    // white space between cells is dropped.
    {
        grid_test_container container;
        const auto          doc = document::createFromString(
            "<style>body{margin:0;font-size:16px}"
            ".t{display:table}.r{display:table-row}.c{display:table-cell}</style>"
            "<div class=t id=pre><div class='r' style='white-space:pre'>"
            "<div class=c>a</div> b <div class=c>c</div></div></div>"
            "<div class=t id=normal><div class='r'>"
            "<div class=c>a</div> b <div class=c>c</div></div></div>",
            &container);
        doc->render(800);

        const auto pre    = doc->root()->select_one("#pre");
        const auto normal = doc->root()->select_one("#normal");
        ok               &= check(pre && normal, "the white space tables should exist");
        if(pre && normal)
        {
            ok &= check(normal->get_placement().width == 24_px,
                        "collapsible white space between cells is dropped (three 8px cells)");
            ok &= check(pre->get_placement().width == 40_px,
                        "white-space: pre keeps the spaces around the anonymous cell (8px + 24px + 8px)");
        }
    }

    if(ok)
    {
        std::cout << "All table layout tests passed\n";
    }
    return ok ? 0 : 1;
}
