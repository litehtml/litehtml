# webpage

`litehtml_webpage` provides [`litebrowser::web_page`](web_page.h), a
Cairo/Pango-based litehtml container that loads pages and resources with
libcurl. It also includes a request pool and a separate navigation-history
helper. The host supplies the viewport, image decoding, application callbacks
and presentation; the library does not provide a window or GUI event loop.

> **Security:** The current [request implementation](http_request.cpp)
> disables TLS peer certificate verification. Do not rely on it to
> authenticate HTTPS connections.

## Dependencies

- C99 and C++17 compilers and CMake 3.20 or newer.
- pkg-config and development files for Cairo, Pango, PangoCairo and libcurl
  (`cairo`, `pango`, `pangocairo` and `libcurl`).
- litehtml and its Cairo/Pango container, built from the same source tree.
- POSIX threads and APIs such as `unistd.h`, `mkstemp` and `unlink`, and fonts
  available to Pango.

GTK is not required. The [`html_widget`](../gtkmm4/html_widget.cpp) uses this
library with the [draw buffer](../draw_buffer/README.md), and
[litebrowser-linux](https://github.com/litehtml/litebrowser-linux) uses that
widget.

## Building

From the root of the litehtml source tree:

```sh
cmake -S . -B build-webpage -DLITEHTML_BUILD_WEBPAGE=ON
cmake --build build-webpage --target litehtml_webpage
```

An application including litehtml as a CMake subdirectory can enable the
option and link the target:

```cmake
set(LITEHTML_BUILD_WEBPAGE ON CACHE BOOL "Build webpage support")
add_subdirectory(path/to/litehtml)
target_link_libraries(myapp PRIVATE litehtml_webpage)
```

Replace `myapp` and the path with your application's target and litehtml
source path. If the option is already cached as `OFF`, reconfigure with
`-DLITEHTML_BUILD_WEBPAGE=ON`. The target propagates its include directories
and linked dependencies. Enabling `LITEHTML_BUILD_GTKMM4` also enables
`litehtml_webpage`.

## Integrating a page

At program startup, initialize libcurl with
`curl_global_init(CURL_GLOBAL_DEFAULT)` and check its result before using
libcurl or starting threads. Call `curl_global_cleanup()` only after all pages
and other libcurl users have stopped. Coordinate this process-wide state with
any other libraries using libcurl.

Implement all methods of both interfaces in [`html_host.h`](html_host.h):

- `html_host_interface` supplies `get_viewport`, `get_render_width`, `get_dpi`,
  `get_screen_width`, `get_screen_height` and `load_image`. `get_viewport`
  includes the viewport origin in document coordinates as well as its size.
  Implement `open_url`, `update_cursor`, `scroll_to` and `redraw_boxes` to
  handle the corresponding application events. `get_cursor()` provides the
  current cursor name.
- `browser_notify_interface` reports requests through `redraw`, `render`,
  `update_state` and `on_set_caption`, and announces a page load with
  `on_page_loaded(page_id)`. The host must perform the corresponding
  application or widget updates.

The host interface receives synchronous calls, including `get_render_width`
and `load_image` from worker threads. Provide thread-safe results; methods
that return values cannot simply be deferred to the GUI thread. Notifications
can also arrive from workers, so dispatch GUI updates to the GUI thread.
See the GTK widget's [notifier](../gtkmm4/html_widget.h) for one approach.

With an application-provided host and notifier, create the page with shared
ownership before calling `open`:

```cpp
#include "web_page.h"
#include <memory>
#include <string>

std::shared_ptr<litebrowser::web_page> make_page(
    litebrowser::html_host_interface& host,
    const std::shared_ptr<litebrowser::browser_notify_interface>& notifier)
{
    return std::make_shared<litebrowser::web_page>(&host, notifier, 10);
}

void start_page(const std::shared_ptr<litebrowser::web_page>& page,
                const std::string& url, const std::string& fragment)
{
    page->open(url, fragment);
}
```

Store the returned page in your application before calling `start_page`. The
example assumes a non-null page and notifier and a nonempty URL. Pass the
fragment without `#`, or an empty string. The final constructor argument is
the number of request workers; it must be positive. The host must remain
valid while the page and its outstanding work use it. The page stores the
notifier as a `shared_ptr`. Use an explicit scheme for network URLs: if a URL
has no scheme and does not start with `/`, `open` prefixes it with `http://`.
An absolute path is treated as a local file.

`open` queues the page request without waiting; a worker may finish before
`open` returns. The notifier's `on_page_loaded` is called after the page
request finishes and an initial document creation and layout have been
attempted. It also fires for a download error, for which the implementation
creates error HTML, and does not mean that all images have finished loading.
Check the page ID when handling queued notifications so an older page cannot
replace a newer one. After the matching notification and layout, call
`show_fragment_and_reset()` on the host's UI thread to apply the fragment
passed to `open`; `show_fragment(fragment)` can be used separately.

## Layout, drawing and input

For a loaded page, update the host's viewport before applying media changes or
laying it out. For example, after a resize:

```cpp
#include "web_page.h"

void resize_page(litebrowser::web_page& page, int width)
{
    page.media_changed();
    page.render(width);
}

void draw_page(litebrowser::web_page& page, cairo_t* cr, int left, int top)
{
    page.draw(reinterpret_cast<litehtml::uint_ptr>(cr), -left, -top, nullptr);
}
```

Here `cr` is a valid Cairo context and `left` and `top` are the viewport origin
in document coordinates. A null clip requests drawing without a clip rectangle;
the host remains responsible for the Cairo context's clipping and scale.
`render` lays out the document but does not schedule a repaint. Handle
`render` and `redraw` notifications by updating the backing image and
requesting presentation as appropriate. For media-only changes,
`media_changed()` reports whether a re-render is needed.

Mouse methods take both document and client coordinates and a callback for
invalidated document rectangles. For example:

```cpp
#include "web_page.h"

litehtml::position::vector mouse_move(litebrowser::web_page& page,
                                      int client_x, int client_y,
                                      int left, int top)
{
    litehtml::position::vector dirty;
    page.on_mouse_over(client_x + left, client_y + top, client_x, client_y,
                       [&dirty](const litehtml::position& box) {
                           dirty.push_back(box);
                       });
    return dirty;
}
```

Redraw the returned regions and schedule presentation. `on_lbutton_down` and
`on_lbutton_up` use the same coordinate and callback convention. `on_scroll`
handles scrollable elements and returns affected scroll boxes; the host also
manages scrolling of the outer viewport. The draw buffer guide describes
converting and presenting its actual, potentially snapped viewport position.

`id()` identifies the page while it is alive; `width()` and `height()` report
the laid-out document size, or zero when no document exists. `url()`,
`get_html_source()` and `get_cursor()` return references to page-owned strings;
do not read them concurrently with updates. `run_with_document()` invokes a
callback under the document lock if a document exists. Do not retain the
document beyond the page's lifetime.

## Loading and lifetime

- `load_image(path)` receives the path of a downloaded temporary file. Decode
  it during the call and return an owned Cairo surface reference, or `nullptr`
  on failure. The page takes ownership of that reference and removes the file
  after the call. Check the Cairo surface status before returning it.
- `is_downloading()` reports whether the request pool has queued or active
  work; it is not a success indicator. Image completion can request either a
  redraw or a new layout through the notifier.
- Call `stop_loading()` from an owner thread while the page, host and notifier
  are still valid, without holding locks needed by their callbacks. It cancels
  active requests and joins the workers; do not call it from a worker
  notification. A stopped pool cannot be restarted, so create a new page for
  later loading.

Stylesheet imports wait for requests in the same pool. A single worker can
deadlock when a page loads an external stylesheet; multiple workers are needed
for that case but do not guarantee freedom from starvation. Stopping can also
block if a worker is waiting for a queued stylesheet. The stopped pool does
not drain queued requests, whose callbacks can retain the page; after stopping,
`is_downloading()` returns `false` even if such requests remain. Guard or
disconnect queued UI notifications before destroying their targets.

The optional global [`web_history`](web_history.h) class tracks URLs
separately from the page. `back` and `forward` return a candidate URL without
changing the current entry; `url_opened` records navigation.
[`http_request`](http_request.h) and [`http_requests_pool`](http_requests_pool.h)
are lower-level transfer helpers used by `web_page`.
