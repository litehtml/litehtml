# render2png

`litehtml_render2png` is a library for rendering an HTML file to a PNG or
`GdkPixbuf` using litehtml and the Cairo/Pango container. It also provides
image comparison helpers. It is not a command-line executable.

The renderer starts with a white background. Images and imported CSS are
loaded from local files, resolving relative paths against the HTML file's
directory or the supplied base path; it does not fetch network resources.

## Dependencies

- C99 and C++17 compilers and CMake 3.20 or newer.
- pkg-config and development files for Cairo, Pango, PangoCairo, GTK3,
  GdkPixbuf and fontconfig (`cairo`, `pango`, `pangocairo`, `gtk-3.0`,
  `gdk-pixbuf-2.0` and `fontconfig`).
- litehtml and its Cairo/Pango container, built from the same source tree.
- Fonts available to Pango for the text being rendered.

## Building

From the root of the litehtml source tree:

```sh
cmake -S . -B build-render2png -DLITEHTML_BUILD_RENDER2PNG=ON
cmake --build build-render2png --target litehtml_render2png
```

Enabling `LITEHTML_BUILD_RENDER2PNG` also enables the Pango and Cairo
containers. To use the target from an application that includes litehtml as
a CMake subdirectory:

```cmake
set(LITEHTML_BUILD_RENDER2PNG ON CACHE BOOL "Build render2png")
add_subdirectory(path/to/litehtml)
target_link_libraries(myapp PRIVATE litehtml_render2png)
```

Replace `myapp` and the path with your application's target and litehtml source
path. If the option is already cached as `OFF`, reconfigure with
`-DLITEHTML_BUILD_RENDER2PNG=ON`. The target propagates its include directories
and linked dependencies.

## API

`html2png::converter` accepts a screen width, screen height, DPI, default font
name and Cairo font options. Their defaults are `800`, `600`, `96`, `nullptr`
and `nullptr`, respectively. A null font name selects `serif`; null font
options leave text drawing without custom options. The screen dimensions are
the initial viewport dimensions, not necessarily the output image dimensions.

Both `to_png` and `to_pixbuf` take a path to an HTML file:

```cpp
#include "render2png.h"

bool write_preview(const std::string& html_file, const std::string& png_file)
{
    html2png::converter converter;
    return converter.to_png(html_file, png_file);
}

GdkPixbuf* render_preview(const std::string& html_file)
{
    html2png::converter converter(800, 600, 96.0);
    return converter.to_pixbuf(html_file);
}
```

`to_png` returns `false` if no pixbuf is produced or the PNG cannot be saved.
`to_pixbuf` can return `nullptr`; the caller owns any returned pixbuf and must
release it with `g_object_unref`. The converter borrows a non-null font-name
pointer and font-options pointer: keep them valid while it is used.

The HTML input stream is not explicitly checked for read errors, so a missing
or unreadable input file is not guaranteed to make either method fail. Check
the input separately if that distinction matters. Configuration and
filesystem exceptions can also propagate.

### Image comparison

`html2png::pngcmp` accepts two file paths, two `GdkPixbuf` pointers, or a
pixbuf and a file path. Pixbuf pointers must be valid and non-null. It compares
dimensions and raw pixel-buffer bytes, not visual similarity:

| Result | Meaning |
| --- | --- |
| `png_diff_same` | Dimensions and pixel-buffer bytes match. |
| `png_diff_dimensions` | Width or height differs. |
| `png_diff_data` | Pixel-buffer byte lengths or contents differ. |
| `png_diff_error` | A file passed to the comparison could not be loaded. |

## `.cfg` sidecar files

For each HTML file, the converter looks for a sidecar by appending `.cfg` to
the complete HTML path. For example, `page.html` uses `page.html.cfg`. Both
`to_pixbuf` and `to_png` read this file. If it is missing or cannot be opened,
the option defaults are used.

Example `page.html.cfg`:

```text
bestfit: true
width: 640
height: 480
drawx: -10
drawy: -20
wheelx: 0
wheely: 40
wheelatx: 100
wheelaty: 100
```

All numeric values are integers in pixels.

| Key | Default | Effect |
| --- | --- | --- |
| `bestfit` | `true` | Enables a second layout when the initial best-fit width, converted to an integer, is positive. |
| `width` | Rendered document width, or `1` if it does not compare greater than zero | Sets output image width; also selects the second layout width when `bestfit` is enabled and the initial best-fit width converts to a positive integer. |
| `height` | Rendered document height, or `1` if it does not compare greater than zero | Sets output image height. |
| `drawx` | `0` | Horizontal drawing offset; negative values shift the document left. |
| `drawy` | `0` | Vertical drawing offset; negative values shift the document up. |
| `wheelx` | `0` | Requested horizontal scroll increment for a scrollable element. |
| `wheely` | `0` | Requested vertical scroll increment for a scrollable element. |
| `wheelatx` | `0` | Horizontal viewport coordinate used to find the element to scroll. |
| `wheelaty` | `0` | Vertical viewport coordinate used to find the element to scroll. |

The first layout uses the converter's screen width. Its best-fit width is
converted to an integer, truncating fractional pixels toward zero. If
`bestfit` is `true` and that integer is positive, the document is laid out
again using `width` if specified, or the converted best-fit width otherwise.
If `bestfit` is `false` or the converted best-fit width is not positive,
`width` changes the output image width but not the layout width.

The default output dimensions are the rendered document dimensions converted
to integers, also truncating fractional pixels toward zero. A dimension that
does not compare greater than zero using litehtml's pixel comparison defaults
to `1` instead. `height` changes the output image height, not the converter's
viewport height. Set output dimensions to positive values; the converter does
not validate them before creating the Cairo surface.

When `wheelx` or `wheely` is nonzero, the converter requests a scroll before
drawing. It selects a scrollable element for each nonzero axis at
`(wheelatx, wheelaty)` in viewport coordinates. The corresponding document
coordinates are `(wheelatx - drawx, wheelaty - drawy)`. The actual movement is
limited by the element's scrollable area; if no suitable element is at that
point, nothing scrolls.

### Format and parsing

- Use one `key: value` entry per line. Surrounding whitespace is trimmed, and
  keys are case-sensitive. Unknown keys have no effect; the first stored entry
  for a duplicate key wins.
- The parser splits on colons outside double-quoted segments and discards
  empty tokens before trimming. A line is stored only if it yields exactly
  two tokens. For example, `width::640` is accepted, while `width:` is
  ignored. Quotes are retained rather than removed, so do not quote values.
- There is no comment syntax. Do not append comments to entries.
- `bestfit` recognizes only the exact lowercase values `true` and `false`;
  other values use its default.
- Integer values are parsed with `std::stoi` in decimal. A valid integer prefix
  is accepted even if characters follow it. A value with no valid integer
  prefix throws `std::invalid_argument`; one outside the range of `int` throws
  `std::out_of_range`. A whitespace-only value is trimmed to an empty value and
  also throws when read as an integer.

See the [Cairo/Pango container guide](../cairo_pango/README.md) for the
underlying text-rendering integration.
