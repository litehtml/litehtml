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

    if(ok)
    {
        std::cout << "All table layout tests passed\n";
    }
    return ok ? 0 : 1;
}
