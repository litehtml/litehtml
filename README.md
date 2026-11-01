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

- A C++17 compiler.
- [CMake](https://cmake.org/) 3.11 or newer (to build with CMake).
- No third-party dependencies: the HTML parser (gumbo) is bundled.

**litehtml** is compatible with any platform supported by C++ and the STL. On Windows, a
recent version of Visual Studio (MSVC) is recommended. **litehtml** works with UTF-8
strings only.

## Building

Configure and build the library with CMake:

```sh
cmake -B build
cmake --build build
```

This builds the `litehtml` library together with the bundled gumbo parser.

To install the library, headers, and CMake package:

```sh
cmake --install build
```

### CMake options

| Option                   | Default | Description                                                        |
| ------------------------ | ------- | ------------------------------------------------------------------ |
| `LITEHTML_BUILD_TESTING` | `OFF`   | Download and build the litehtml test suite.                        |
| `LITEHTML_ENABLE_LINT`   | `ON`    | Run clang-tidy during the build.                                   |
| `EXTERNAL_GUMBO`         | `OFF`   | Link against a system-installed gumbo instead of the bundled copy. |

> **Note:** `LITEHTML_BUILD_TESTING` does not simply enable CTest. It fetches and builds
> the separate [litehtml-tests](https://github.com/litehtml/litehtml-tests) project.

## Directory structure

```
litehtml/
├── cmake/          CMake package configuration (litehtmlConfig.cmake)
├── containers/     Reference document_container implementations (cairo, haiku, test, windows)
├── doc/            Documentation and integration guide
├── include/
│   ├── litehtml.h  Umbrella public header
│   └── litehtml/   Public API headers
├── src/            Library implementation (.cpp files)
│   └── gumbo/      Vendored gumbo HTML parser (C99)
└── support/        Integration helpers (draw_buffer, gtkmm4, webpage)
```

The `containers/` and `support/` directories are not part of the library build; they serve
as integration examples and helpers.

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

## Documentation

- [How to use litehtml](doc/using.md)
- [document_container interface](doc/document_container.md)
- [document::createFromString](doc/document_createFromString.md)
- [Supported CSS properties and selectors](doc/css_support.md)

## License

**litehtml** is distributed under the [New BSD License](https://opensource.org/licenses/BSD-3-Clause).

**gumbo-parser** is distributed under the [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).

## Links

- [Source code](https://github.com/litehtml/litehtml)
- [Website](http://www.litehtml.com/)
- [litebrowser for Windows](https://github.com/litehtml/litebrowser)
- [litebrowser for Linux](https://github.com/litehtml/litebrowser-linux)
- [litebrowser for Haiku](https://github.com/adamfowleruk/litebrowser-haiku)
