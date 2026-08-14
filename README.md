# litehtml

**Fast, lightweight HTML/CSS rendering engine.** No bloat, no dependencies — just parsing and layout. You bring the drawing.

---

## What is litehtml?

litehtml parses HTML/CSS and calculates element positions. That's it.

- ✅ **Zero rendering dependencies** — no font/image/graphics libraries required
- ✅ **Bring your own renderer** — implement the simple [`document_container`](https://github.com/litehtml/litehtml/wiki/document_container) callback interface
- ✅ **Pure layout engine** — HTML in, positioned elements out

> 💡 The `document_container` interface is intentionally minimal. [Check it out](https://github.com/litehtml/litehtml/wiki/document_container) — implementation is required for correct rendering.

---

## When to use litehtml

| Use Case | Recommendation |
|----------|---------------|
| HTML tooltips, formatted text, mini-browsers | ✅ **Perfect fit** |
| Full-featured browser engine | ❌ Use WebKit/Blink instead |

litehtml is **not** a WebKit replacement. It's the fast, embeddable alternative when you need HTML rendering without the weight.

---

## Under the hood

### HTML Parser
Uses [gumbo-parser](https://codeberg.org/gumbo-parser/gumbo-parser) — a pure C99 HTML5 parser with zero external dependencies. Designed as a building block for linters, validators, templating engines, and analysis tools.

### Compatibility
- Works on **any platform** with C++ and STL support
- Windows: MS Visual Studio 2013+ recommended
- **UTF-8 only**

---

## Standards support

litehtml supports **most HTML tags and CSS properties**, but is not fully standards-compliant.

📊 [Full CSS support matrix](https://docs.google.com/spreadsheet/ccc?key=0AvHXl5n24PuhdHdELUdhaUl4OGlncXhDcDJuM1JpMnc&usp=sharing)

> For simple use cases, supported features are sufficient. Complex layouts (e.g., [Bootstrap](http://getbootstrap.com/) pages) often render correctly.

---

## Try it now

Download [litebrowser](http://www.litehtml.com/download.html) to test the engine.

**Source code:**
- [Windows](https://github.com/litehtml/litebrowser)
- [Linux](https://github.com/litehtml/litebrowser-linux)
- [Haiku](https://github.com/adamfowleruk/litebrowser-haiku)

---

## License

| Component | License |
|-----------|---------|
| litehtml | [BSD 3-Clause](https://opensource.org/licenses/BSD-3-Clause) |
| gumbo-parser | [Apache 2.0](http://www.apache.org/licenses/LICENSE-2.0) |

---

## Links

-  [Source Code](https://github.com/litehtml/litehtml)
- 🌐 [Website](http://www.litehtml.com/)
