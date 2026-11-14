# draw_buffer

`litehtml_draw_buffer` provides `litebrowser::draw_buffer`, a Cairo-backed
buffer for drawing a page into an image surface and presenting it in a widget
or window. On scroll, it can reuse the overlapping portion of the previous
image and redraw exposed areas and fixed-position boxes.

The buffer does not lay out HTML, handle input, or schedule window repaints.
The host supplies a drawing callback and manages the document and its viewport.

## Dependencies

- C99 and C++17 compilers and CMake 3.20 or newer.
- pkg-config and the Cairo development files (`cairo`).
- litehtml, built from the same source tree.

GTK is not required to use the buffer. The
[`litehtml_gtkmm4` library](../gtkmm4/) includes
[`html_widget`](../gtkmm4/html_widget.cpp), an example of a host that uses it.
[litebrowser-linux](https://github.com/litehtml/litebrowser-linux) is a
litehtml-based browser that uses the widget and, through it, this library.

## Building

From the root of the litehtml source tree:

```sh
cmake -S . -B build-draw-buffer -DLITEHTML_BUILD_DRAW_BUFFER=ON
cmake --build build-draw-buffer --target litehtml_draw_buffer
```

An application including litehtml as a CMake subdirectory can enable the
option and link the target:

```cmake
set(LITEHTML_BUILD_DRAW_BUFFER ON CACHE BOOL "Build draw_buffer")
add_subdirectory(path/to/litehtml)
target_link_libraries(myapp PRIVATE litehtml_draw_buffer)
```

Replace `myapp` and the path with your application's target and litehtml
source path. If the option is already cached as `OFF`, reconfigure with
`-DLITEHTML_BUILD_DRAW_BUFFER=ON`. The target propagates its include
directories and linked dependencies. Enabling `LITEHTML_BUILD_GTKMM4` also
enables the draw buffer.

## Integration

The callback receives a Cairo context, the page drawing offset, and a clip
rectangle. This example assumes a valid document and a host container that
reports the current viewport:

```cpp
#include "draw_buffer.h"
#include <litehtml.h>
#include <memory>

litebrowser::draw_buffer::draw_page_function_t make_page_drawer(
    const std::shared_ptr<litehtml::document>& document)
{
    return [document](cairo_t* cr, int x, int y, const litehtml::position* clip) {
        document->draw(reinterpret_cast<litehtml::uint_ptr>(cr), x, y, clip);
    };
}

void resize_view(litebrowser::draw_buffer& buffer,
                 const std::shared_ptr<litehtml::document>& document,
                 int width, int height)
{
    auto draw = make_page_drawer(document);
    buffer.on_size_allocate(draw, width, height);
    document->media_changed();
    document->render(width);
    buffer.redraw(draw);
}

void scroll_view(litebrowser::draw_buffer& buffer,
                 const std::shared_ptr<litehtml::document>& document,
                 int left, int top)
{
    litehtml::position::vector fixed_boxes;
    document->get_fixed_boxes(fixed_boxes);
    buffer.on_scroll(make_page_drawer(document), left, top, fixed_boxes);
}
```

Update the host's viewport before applying media changes and laying out the
document. `on_size_allocate` redraws immediately when it creates a buffer;
the explicit redraw above updates it after layout. If only content changes,
use `redraw` or `redraw_area` and request a widget repaint separately.
`on_scroll` takes the new absolute viewport origin, not a delta. Pass the
document's current fixed boxes, or an empty vector if there are none.
Keep the document's container alive while the document is used; callbacks are
passed to each operation rather than stored by the buffer.

To present the current buffer on a Cairo destination using logical pixel
coordinates:

```cpp
#include "draw_buffer.h"
#include <cmath>

cairo_status_t paint_buffer(const litebrowser::draw_buffer& buffer, cairo_t* cr)
{
    auto* surface = buffer.get_cairo_surface();
    if(!cr || !surface)
    {
        return CAIRO_STATUS_NULL_POINTER;
    }
    const auto status = cairo_surface_status(surface);
    if(status != CAIRO_STATUS_SUCCESS)
    {
        return status;
    }
    const double scale = buffer.get_scale_factor();
    if(!std::isfinite(scale) || scale <= 0.0)
    {
        return CAIRO_STATUS_INVALID_MATRIX;
    }
    cairo_save(cr);
    cairo_scale(cr, 1.0 / scale, 1.0 / scale);
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);
    return cairo_status(cr);
}
```

The application should handle a non-success status and schedule presentation
after buffer updates. The buffer does not clamp scroll positions to the
document bounds.

## Buffer operations

- `on_size_allocate(draw, width, height)` creates or resizes the buffer and
  redraws it when needed. If a surface already exists at the same size, it
  does not redraw.
- `create_draw_buffer(width, height)` changes the buffer size without drawing.
  It returns `true` when recreation is attempted, including when a nonpositive
  dimension leaves no surface; `false` means the existing surface was reused.
- `redraw(draw)` redraws the whole buffer. `redraw_area(draw, x, y, width,
  height)` redraws a document-coordinate area. Before each callback, the
  affected area is clipped and cleared to white.
- `on_scroll(draw, left, top, fixed_boxes)` updates the viewport origin. It
  reuses overlapping content when possible and redraws exposed and fixed
  areas; a scroll with no overlap redraws the buffer. Without a surface or
  positive dimensions, it does nothing.
- `set_scale_factor(draw, scale)` changes the scale, recreates the surface and
  redraws when the value changes and a surface exists. The default scale is
  `1.0`.

The callback can be invoked multiple times during one operation. It receives
`x = -get_left()` and `y = -get_top()` in logical pixels; its clip is in
buffer-relative logical coordinates and can be adjusted for scale alignment.
The Cairo context is already scaled and clipped. The context and clip pointer
are valid only for the callback; do not destroy or retain them.

## Scaling and ownership

Sizes and positions passed to the buffer are unscaled logical pixels. The
underlying `CAIRO_FORMAT_RGB24` image surface has dimensions
`ceil(width * scale)` by `ceil(height * scale)` in pixels. `get_width()` and
`get_height()` return the unscaled dimensions. Supply a positive, finite scale
and sizes for which `scale * 100` and the scaled dimensions fit in `int` and
are accepted by Cairo; the class does not validate them for you.

Scroll positions are snapped to a scale-dependent grid. The grid is derived
from `int(scale * 100) / 100`, and integer positions are snapped toward zero.
For example, at scale `1.25`, requested positions `10, 11` become `8, 8`.
Use `get_left()` and `get_top()` for the actual position, including when
converting input coordinates from the widget to the document.

`get_cairo_surface()` returns a borrowed pointer, or `nullptr` when no surface
exists. Resizing, changing scale, scrolling, or destroying the buffer can
invalidate it, so fetch it again after updates. Do not destroy the borrowed
pointer. Cairo can return a non-null error surface; check
`cairo_surface_status()` before using it. The static `make_surface()` instead
returns a surface owned by the caller, which must be released with
`cairo_surface_destroy()`.

Do not copy or move `draw_buffer` objects: their generated copy operations
share the raw surface pointer without taking a reference. Use the buffer from
one thread at a time, or provide your own synchronization.
