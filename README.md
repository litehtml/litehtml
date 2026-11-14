# litehtml

**litehtml** is a lightweight HTML rendering engine with CSS2/CSS3 support.

Note that **litehtml** itself does not draw any text, images, or other graphics, and it
does not depend on any image, drawing, or font library. You are free to use any library
to draw images, fonts, and other graphics. **litehtml** only parses HTML/CSS and computes
the layout (that is, it places the HTML elements in the correct positions).

To draw the rendered elements you have to implement the simple
[document_container](doc/document_container.md) callback interface. This interface is
deliberately small and easy to implement, and it is required to render HTML correctly.

## Table of contents

- [Where litehtml can be used](#where-litehtml-can-be-used)
- [Requirements](#requirements)
- [Building](#building)
  - [CMake options](#cmake-options)
  - [Optional libraries](#optional-libraries)
- [Directory structure](#directory-structure)
- [Quick example](#quick-example)
- [Testing](#testing)
- [HTML Parser](#html-parser)
- [Supported standards](#supported-standards)
- [Documentation](#documentation)
- [License](#license)
- [Links](#links)

## Where litehtml can be used

**litehtml** is a good fit when you need to display HTML-formatted text or even build a
mini-browser. Using it as a full-featured browser engine is not recommended.

For simple HTML tooltips or HTML-formatted text you usually don't need something as heavy
as WebKit. **litehtml** is much lighter and easier to integrate into your application.

## Requirements

- C++17 and C99 compilers.
- [CMake](https://cmake.org/) 3.20 or newer (to build with CMake).
- No external libraries for the default core build: the gumbo HTML parser is
  bundled.

The core **litehtml** library is compatible with platforms supported by C++ and the STL.
On Windows, a recent version of Visual Studio (MSVC) is recommended. **litehtml** works
with UTF-8 strings only. The optional libraries have additional dependencies and may
have platform-specific requirements.

## Building

From the root of the litehtml source tree, configure and build with CMake:

```sh
cmake -S . -B build
cmake --build build
```

With the default options, this builds the `litehtml` library together with the bundled
gumbo parser, without any optional libraries.

To install the core library, headers, and CMake package:

```sh
cmake --install build
```

The install rules include bundled gumbo when it is used, but do not install or export
the optional library targets. See their guides below for source-tree integration.

### CMake options

| Option | Default | Description |
| --- | --- | --- |
| `LITEHTML_BUILD_TESTING` | `OFF` | Download and build the separate litehtml test project. |
| `LITEHTML_BUILD_GRID_TESTS` | `OFF` | Build the local CSS Grid regression tests. |
| `LITEHTML_BUILD_MATH_TESTS` | `OFF` | Build the local CSS calculation regression tests. |
| `LITEHTML_ENABLE_LINT` | `ON` | Run clang-tidy for litehtml and local tests when it is available. |
| `EXTERNAL_GUMBO` | `OFF` | Link against a system-installed gumbo instead of the bundled copy. |

`LITEHTML_BUILD_TESTING=ON` configures the separate
[litehtml-tests](https://github.com/litehtml/litehtml-tests) project rather than the
regular core, optional-library and local-test targets in that build tree. Use a separate
build tree for the library and local tests. The external project is downloaded when it
is built.

### Optional libraries

The following options default to `OFF`. They require pkg-config and the listed
development libraries; the guides describe additional integration and platform
requirements.

| Option | Library and purpose | Dependencies |
| --- | --- | --- |
| `LITEHTML_BUILD_CONTAINER_CAIRO` | [`litehtml_container_cairo`](containers/cairo/README.md): Cairo reference container, without text drawing | Cairo |
| `LITEHTML_BUILD_CONTAINER_PANGO` | [`litehtml_container_pango`](containers/cairo_pango/README.md): Cairo container with Pango text | Cairo, Pango, PangoCairo |
| `LITEHTML_BUILD_RENDER2PNG` | [`litehtml_render2png`](containers/render2png/README.md): render local HTML to PNG or GdkPixbuf | Cairo, Pango, PangoCairo, GTK4, GdkPixbuf, fontconfig |
| `LITEHTML_BUILD_DRAW_BUFFER` | [`litehtml_draw_buffer`](support/draw_buffer/README.md): Cairo-backed drawing buffer | Cairo |
| `LITEHTML_BUILD_WEBPAGE` | [`litehtml_webpage`](support/webpage/README.md): page loading and history | Cairo, Pango, PangoCairo, libcurl, POSIX threads and APIs |
| `LITEHTML_BUILD_GTKMM4` | [`litehtml_gtkmm4`](support/gtkmm4/README.md): GTKMM4 page widget | Cairo, Pango, PangoCairo, libcurl, gtkmm 4, POSIX threads and APIs |

Pango-dependent libraries also need suitable fonts available at runtime. Enabling the
Pango container also enables the Cairo container; enabling render2png or webpage enables
both containers. GTKMM4 enables webpage and draw_buffer, including their required
containers. These dependencies are enabled for the build even if their cached options
still show `OFF`.

For example, to build the Pango container and its dependencies:

```sh
cmake -S . -B build-pango -DLITEHTML_BUILD_CONTAINER_PANGO=ON
cmake --build build-pango --target litehtml_container_pango
```

> **Security:** The webpage loader disables TLS peer certificate verification, including
> when used through the GTKMM4 widget. See the [webpage guide](support/webpage/README.md).

## Directory structure

```
litehtml/
├── cmake/          CMake package configuration (litehtmlConfig.cmake)
├── containers/     Reference containers and optional rendering libraries
├── doc/            Documentation and integration guide
├── include/
│   ├── litehtml.h  Umbrella public header
│   └── litehtml/   Public API headers
├── src/            Library implementation (.cpp files)
│   └── gumbo/      Vendored gumbo HTML parser (C99)
└── support/        Optional integration helpers (draw_buffer, gtkmm4, webpage)
```

The `containers/` and `support/` directories contain reference code and optional
libraries; their CMake targets are built when the corresponding options are enabled.

## Quick example

Integrating litehtml means implementing `litehtml::document_container` and creating a
document:

```cpp
#include "litehtml.h"

// Implement litehtml::document_container to draw text and images and to load resources.
class my_container : public litehtml::document_container
{
    // ...
};

int main()
{
    my_container container;

    auto doc = litehtml::document::createFromString(
        "<html><body>Hello, <b>world</b>!</body></html>", &container);

    doc->render(800); // returns the optimal document width
    doc->draw(nullptr, 0, 0, nullptr);

    return 0;
}
```

For a complete walkthrough of the document lifecycle (render → width/height → draw →
mouse/scroll/anchors), see [doc/using.md](doc/using.md).

## Testing

The rendering tests live in a separate repository,
[litehtml-tests](https://github.com/litehtml/litehtml-tests). Refer to that repository for
instructions on running the test suite.

To run the local Grid tests:

```sh
cmake -S . -B build-grid -DLITEHTML_BUILD_GRID_TESTS=ON
cmake --build build-grid --target litehtml-grid-tests
ctest --test-dir build-grid -R litehtml-grid-tests --output-on-failure
```

To run the local CSS math tests, including headless layout checks:

```sh
cmake -S . -B build-math -DLITEHTML_BUILD_MATH_TESTS=ON
cmake --build build-math --target litehtml-math-tests
ctest --test-dir build-math -R litehtml-math-tests --output-on-failure
```

## HTML Parser

**litehtml** uses [gumbo-parser](https://codeberg.org/gumbo-parser/gumbo-parser) to parse
HTML. Gumbo is an implementation of the HTML5 parsing algorithm written as a pure C99
library with no outside dependencies. It is designed to serve as a building block for
other tools and libraries such as linters, validators, templating languages, and
refactoring and analysis tools.

## Supported standards

**litehtml** is not yet fully compatible with the HTML/CSS standards. There is still a lot
of work to do to match modern browsers, but **litehtml** supports most HTML tags and CSS
properties. See [doc/css_support.md](doc/css_support.md) for the list of supported CSS
properties and selectors.

CSS Grid has partial support; see [doc/css_support.md](doc/css_support.md#grid) for details.
CSS length calculations support `calc()`, `min()`, `max()`, and `clamp()`;
see [CSS math values](doc/css_support.md#css-math-values) for scope and limits.

## Documentation

- [How to use litehtml](doc/using.md)
- [document_container interface](doc/document_container.md)
- [document::createFromString](doc/document_createFromString.md)
- [Supported CSS properties and selectors](doc/css_support.md)
- [Optional library guides](#optional-libraries)

## License

**litehtml** is distributed under the [New BSD License](https://opensource.org/licenses/BSD-3-Clause).

**gumbo-parser** is distributed under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).

## Links

- [Source code](https://github.com/litehtml/litehtml)
- [Website](http://www.litehtml.com/)
- [litebrowser for Windows](https://github.com/litehtml/litebrowser)
- [litebrowser for Linux](https://github.com/litehtml/litebrowser-linux)
- [litebrowser for Haiku](https://github.com/adamfowleruk/litebrowser-haiku)
