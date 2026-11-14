# GTKMM4 widget

`litehtml_gtkmm4` provides `html_widget`, a GTK4 widget built with gtkmm. It
combines the [webpage library](../webpage/README.md) and
[draw buffer](../draw_buffer/README.md) to load, lay out and display pages.
The widget handles scrolling, mouse and keyboard input, navigation history,
and page title updates. The application supplies the window and any address
bar or navigation controls.

> **Security:** The underlying page loader currently disables TLS peer
> certificate verification. Do not rely on it to authenticate HTTPS
> connections. See the [webpage guide](../webpage/README.md).

## Dependencies

- C99 and C++17 compilers and CMake 3.20 or newer.
- pkg-config and the gtkmm 4 development files (`gtkmm-4.0`), including GTK4
  and GdkPixbuf.
- Cairo, Pango, PangoCairo and libcurl development files (`cairo`, `pango`,
  `pangocairo` and `libcurl`), POSIX threads and APIs, and fonts available to
  Pango.
- litehtml, the webpage library and the draw buffer, built from the same
  source tree.

## Building

From the root of the litehtml source tree:

```sh
cmake -S . -B build-gtkmm4 -DLITEHTML_BUILD_GTKMM4=ON
cmake --build build-gtkmm4 --target litehtml_gtkmm4
```

An application including litehtml as a CMake subdirectory can enable the
option and link the target:

```cmake
set(LITEHTML_BUILD_GTKMM4 ON CACHE BOOL "Build the GTKMM4 widget")
add_subdirectory(path/to/litehtml litehtml)
target_link_libraries(myapp PRIVATE litehtml_gtkmm4)
```

Replace `myapp` and the path with your application's target and litehtml
source path. The second `add_subdirectory` argument specifies the litehtml
build directory. If the option is already cached as `OFF`, reconfigure with
`-DLITEHTML_BUILD_GTKMM4=ON`. Enabling it also enables the webpage and draw
buffer targets and their Cairo/Pango dependencies. The target propagates its
include directories and linked dependencies.

## Embedding the widget

Initialize libcurl at program startup with
`curl_global_init(CURL_GLOBAL_DEFAULT)` and check its result before using
libcurl or starting threads. Call `curl_global_cleanup()` only after the
widgets and other libcurl users have stopped. Coordinate this process-wide
state with any other libraries using libcurl.

Create and use GTK widgets on the GTK main thread, with a running
`Gtk::Application` and main loop. The following example attaches an
`html_widget` to a window and connects an address entry and Back button:

```cpp
#include "html_widget.h"
#include <cstdint>
#include <string>

void attach_view(Gtk::Window& window, Gtk::Box& content, html_widget& page,
                 Gtk::Entry& address, Gtk::Button& back)
{
    content.append(address);
    content.append(back);
    content.append(page);
    page.set_expand(true);
    window.set_child(content);

    back.set_sensitive((page.get_state() & page_state_has_back) != 0);
    back.signal_clicked().connect(sigc::mem_fun(page, &html_widget::go_back));
    page.signal_set_address().connect([&address](const std::string& url) {
        address.set_text(url);
    });
    page.signal_update_state().connect([&back](std::uint32_t state) {
        back.set_sensitive((state & page_state_has_back) != 0);
    });
    window.signal_close_request().connect(sigc::mem_fun(page, &html_widget::on_close), false);
}
```

The window, box, page and controls must remain valid while the connections are
in use; disconnect callbacks if their targets are destroyed earlier. During
application activation, attach the controls and present the window. Then call
`page.open_url(url)` with a nonempty URL from the GTK thread. For a complete
application using the widget, see
[litebrowser-linux](https://github.com/litehtml/litebrowser-linux), including
its [window integration](https://github.com/litehtml/litebrowser-linux/blob/master/src/browser_wnd.cpp).

## Navigation and signals

- `open_url(url)` starts navigation. `go_back()` and `go_forward()` navigate
  history, `reload()` reloads the current page, and `stop_download()` stops
  the pending page if there is one, otherwise the current page's downloads.
- `signal_set_address()` supplies an address string. It can be emitted during
  navigation and after loading, so it is not a load-completion signal.
- `signal_update_state()` supplies a `uint32_t` bitmask. `get_state()` returns
  the current bitmask; test `page_state_has_back`, `page_state_has_forward`
  and `page_state_downloading` to update controls. The downloading flag
  reflects pending-page or request state, not whether loading succeeded.
- `get_html_source()` returns the current page's source, or an empty string
  when there is no current page. `run_with_document(callback)` invokes the
  callback for the current document under its lock, if one exists. Do not
  retain that document beyond the page's lifetime; call `render()` after
  changing document content when a new layout and redraw are needed.

When only the fragment of the current history URL changes, `open_url` can
scroll within the current page. A fragment on a newly loaded page is not
automatically applied by the widget's page-loaded handler. If needed, call
`show_fragment(fragment)` after the page is current, passing the fragment
without `#`.

## Loading and shutdown

Loading is asynchronous. The widget uses `Glib::Dispatcher` to deliver its
notifications to the GTK main context; this does not make all underlying
host callbacks or widget operations thread-safe. See the
[webpage guide](../webpage/README.md) for loading, error and worker-pool
details.

The close-request connection in the example calls `on_close()` while the
widget is still alive. It stops loading on the current and pending pages and
returns `false` so GTK can continue closing the window. If the widget is
destroyed by another path, call `on_close()` before destroying it. Stopping
waits for workers and can block while a stylesheet request is queued; do not
hold locks needed by callbacks while stopping.
