#include <gtest/gtest.h>

#include "litehtml.h"
#include "test/container_test.h"

using namespace litehtml;

TEST(LayoutGlobal, Smoke) {
  container_test container;
  litehtml::document::ptr doc = document::createFromString("<html>Body</html>", &container);
  doc->render(50, render_all);
}
