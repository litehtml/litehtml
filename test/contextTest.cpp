#include <gtest/gtest.h>

#include "litehtml.h"
#include "litehtml/utf8_strings.h"

using namespace litehtml;

const char* master_css =
#include "master.css.inc"
;

TEST(ContextTest, LoadMasterStylesheet)
{
    context ctx;
    ctx.load_master_stylesheet(litehtml_from_utf8(master_css));
}
