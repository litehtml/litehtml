#include <assert.h>
#include "litehtml.h"
using namespace litehtml;

extern const tchar_t master_css[];

static void Test()
{
    context ctx;
    ctx.load_master_stylesheet(master_css);
}

void contextTest()
{
    Test();
}