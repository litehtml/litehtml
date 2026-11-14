# Cairo/Pango reference container

`litehtml_container_pango` extends the
[Cairo reference container](../cairo/README.md) with Pango font creation,
measurement and text drawing. Its `container_cairo_pango` class inherits
the Cairo drawing callbacks for backgrounds, borders, images and other
graphics.

The class is still abstract: applications must implement the remaining
host-specific callbacks, including image loading, viewport information and
application events.

## Dependencies

- C99 and C++17 compilers and CMake 3.20 or newer.
- pkg-config and development files for Cairo, Pango and PangoCairo
  (`cairo`, `pango` and `pangocairo`).
- litehtml and its Cairo container, built from the same source tree.
- Fonts available to Pango for the text the application needs to render.

## Building

From the root of the litehtml source tree:

```sh
cmake -S . -B build-pango -DLITEHTML_BUILD_CONTAINER_PANGO=ON
cmake --build build-pango --target litehtml_container_pango
```

Enabling `LITEHTML_BUILD_CONTAINER_PANGO` also enables
`litehtml_container_cairo`. To use the target from an application that
includes litehtml as a CMake subdirectory:

```cmake
set(LITEHTML_BUILD_CONTAINER_PANGO ON CACHE BOOL "Build the Pango container")
add_subdirectory(path/to/litehtml)
target_link_libraries(myapp PRIVATE litehtml_container_pango)
```

Replace `myapp` and the path with your application's target and litehtml source
path. If the option is already cached as `OFF`, reconfigure with
`-DLITEHTML_BUILD_CONTAINER_PANGO=ON`. The target propagates its include
directories and linked dependencies, including the Cairo container.

## Integration

Derive an application container and implement its host callbacks. This is an
excerpt, not a complete class:

```cpp
#include "container_cairo_pango.h"

class app_container : public container_cairo_pango
{
public:
    cairo_surface_t* get_image(const std::string& url) override;
    double get_screen_dpi() const override;
    int get_screen_width() const override;
    int get_screen_height() const override;

    // Implement the remaining document_container callbacks.
};
```

The Pango extension implements `create_font`, `delete_font`, `text_width` and
`draw_text`; the application still supplies callbacks such as `load_image`,
`get_viewport`, `import_css` and event handlers. See the
[`document_container` interface](../../include/litehtml/document_container.h)
and the [Cairo container guide](../cairo/README.md) for the remaining
requirements, including image-surface ownership and URL handling.

With a complete container, create and lay out a document, then pass a valid
Cairo context as the drawing handle:

```cpp
void draw_html(container_cairo_pango& container, cairo_t* cr)
{
    auto doc = litehtml::document::createFromString(
        "<html><body>Hello, Pango!</body></html>", &container);
    doc->render(800);
    doc->draw(reinterpret_cast<litehtml::uint_ptr>(cr), 0, 0, nullptr);
}
```

The container must outlive the document, and the Cairo context must remain
valid during drawing. Apply clipping to the Cairo context when needed; the
document's optional clip argument selects elements to draw.

### Font options

The optional `set_font_options` method configures antialiasing, hint style and
subpixel order for text drawing. For example:

```cpp
void configure_text(container_cairo_pango& container)
{
    container.set_font_options(CAIRO_ANTIALIAS_GRAY, CAIRO_HINT_STYLE_SLIGHT,
                               CAIRO_SUBPIXEL_ORDER_DEFAULT);
}
```

Choose options appropriate for the host environment. By default,
`get_font_options()` returns `nullptr`; it can also be overridden to supply
host-managed Cairo font options. These options are applied when drawing text,
not by the separate font measurement callbacks.

On construction, the container attempts to create a FreeType-backed
PangoCairo font map and sets it as the default PangoCairo font map if available.

For a fuller host implementation, see the
[Cairo/Pango render-to-PNG container](../render2png/render2png.cpp) and the
[litehtml integration guide](../../doc/using.md).
