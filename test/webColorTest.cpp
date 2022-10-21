#include <assert.h>
#include "litehtml.h"
#include "test/container_test.h"
using namespace litehtml;

static void WebColorParseTest() {
  container_test container;
  web_color c;
  c = web_color::from_string("", &container), assert(c.red == 0), assert(c.green == 0), assert(c.blue == 0);
  c = web_color::from_string("#f0f", &container), assert(c.red == 255), assert(c.green == 0), assert(c.blue == 255);
  c = web_color::from_string("#ff00ff", &container), assert(c.red == 255), assert(c.green == 0), assert(c.blue == 255);
  c = web_color::from_string("rgb()", &container), assert(c.red == 0), assert(c.green == 0), assert(c.blue == 0);
  c = web_color::from_string("rgb(255,0,255)", &container), assert(c.red == 255), assert(c.green == 0), assert(c.blue == 255);
  c = web_color::from_string("red", &container), assert(c.red == 255), assert(c.green == 0), assert(c.blue == 0);
  c = web_color::from_string("unknown", &container), assert(c.red == 0), assert(c.green == 0), assert(c.blue == 0);
}

void webColorTest() {
  WebColorParseTest();
}