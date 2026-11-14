# Cairo reference container

`litehtml_container_cairo` is a reference implementation of part of the
`litehtml::document_container` interface using Cairo. It draws backgrounds,
gradients, borders, images and list markers, but it does not draw or measure
text. The `container_cairo` class is abstract: an application must provide
font and text callbacks as well as the remaining host-specific callbacks.

For Pango-based font measurement and text drawing, use the
[`container_cairo_pango`](../cairo_pango/container_cairo_pango.h) extension
from the `litehtml_container_pango` target. It inherits from
`container_cairo` but still requires application-specific callbacks.

## Dependencies

- C99 and C++17 compilers and CMake 3.20 or newer.
- pkg-config and the Cairo development files (`cairo`).
- litehtml, built from the same source tree.
- For the Pango extension: the Pango and PangoCairo development files
  (`pango` and `pangocairo`).

## Building

From the root of the litehtml source tree:

```sh
cmake -S . -B build-cairo -DLITEHTML_BUILD_CONTAINER_CAIRO=ON
cmake --build build-cairo --target litehtml_container_cairo
```

For the Pango extension, enable its option instead; it also enables the Cairo
container:

```sh
cmake -S . -B build-pango -DLITEHTML_BUILD_CONTAINER_PANGO=ON
cmake --build build-pango --target litehtml_container_pango
```

To use the target from an application that includes litehtml as a CMake
subdirectory, enable the option before adding litehtml and link the application
target:

```cmake
set(LITEHTML_BUILD_CONTAINER_CAIRO ON CACHE BOOL "Build the Cairo container")
add_subdirectory(path/to/litehtml)
target_link_libraries(myapp PRIVATE litehtml_container_cairo)
```

Replace `myapp` and the path with your application's target and litehtml source
path. If the option is already cached as `OFF`, reconfigure with
`-DLITEHTML_BUILD_CONTAINER_CAIRO=ON`. For Pango, enable
`LITEHTML_BUILD_CONTAINER_PANGO` and link `litehtml_container_pango` instead.
The container targets propagate their include directories and linked
dependencies.

## Integration

Derive an application container and implement its host callbacks. This is an
excerpt, not a complete class:

```cpp
#include "container_cairo.h"

class app_container : public container_cairo
{
public:
    cairo_surface_t* get_image(const std::string& url) override;
    double get_screen_dpi() const override;
    int get_screen_width() const override;
    int get_screen_height() const override;

    // Implement the remaining document_container callbacks.
};
```

The remaining callbacks include font creation, measurement and drawing;
image loading; viewport dimensions; CSS imports; and application events.
See the [`document_container` interface](../../include/litehtml/document_container.h)
for the full list. To use Pango for the font callbacks, include
`container_cairo_pango.h` and derive from `container_cairo_pango` instead.

`get_image` must return a referenced Cairo image surface, or `nullptr` if the
image is unavailable. The container destroys the returned reference after use;
retain a separate reference if your application caches the surface. The default
`make_url` ignores the base path and copies the URL unchanged; override it if
your application needs relative URL resolution.

Once the host container is complete, create and lay out a document, then pass
a valid Cairo context as the drawing handle:

```cpp
void draw_html(container_cairo& container, cairo_t* cr)
{
    auto doc = litehtml::document::createFromString(
        "<html><body>Hello</body></html>", &container);
    doc->render(800);
    doc->draw(reinterpret_cast<litehtml::uint_ptr>(cr), 0, 0, nullptr);
}
```

The container must outlive the document, and the Cairo context must remain
valid during drawing. Supply clipping on the Cairo context when needed; the
document's optional clip argument is used to select elements to draw.

For a fuller host implementation, see the
[Cairo/Pango render-to-PNG container](../render2png/render2png.cpp) and the
[litehtml integration guide](../../doc/using.md).
