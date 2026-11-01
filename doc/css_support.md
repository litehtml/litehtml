# CSS support

## Table of contents

- [Properties](#properties)
  - [Summary](#summary)
  - [Box model & layout](#box-model--layout)
  - [Background](#background)
  - [Border](#border)
  - [Text & font](#text--font)
  - [List](#list)
  - [Flexbox](#flexbox)
  - [Table](#table)
  - [Generated content & counters](#generated-content--counters)
  - [UI & misc](#ui--misc)
- [Selectors](#selectors)
  - [Type & universal selectors](#type--universal-selectors)
  - [ID & class selectors](#id--class-selectors)
  - [Attribute selectors](#attribute-selectors)
  - [Combinators](#combinators)
  - [Selector lists & specificity](#selector-lists--specificity)
  - [Test coverage](#test-coverage)
- [Selectors: pseudo-classes & pseudo-elements](#selectors-pseudo-classes--pseudo-elements)
  - [Pseudo-classes that actually match](#pseudo-classes-that-actually-match)
  - [Recognized by the parser but never match](#recognized-by-the-parser-but-never-match)
  - [Pseudo-elements](#pseudo-elements)
  - [Test coverage](#test-coverage-1)

---

This document describes litehtml's CSS support: which CSS properties and selectors are
recognized, and how well each one is *actually* supported at render time. "Recognized" (the parser
reads and stores a value) is **not** the same as "supported" (the value changes the rendered
output). Each entry was checked by rendering a probe document and confirming the value actually
changes the output — rather than merely being accepted by the parser.

Status legend used throughout:

- ✅ **Supported** — parsed, computed and applied; confirmed to change the rendered output.
- ⚠️ **Partial** — works for some values/contexts, or deviates from the CSS spec.
- ❌ **Not supported** — parsed (recognized) but has no effect, or the value is not recognized.

> Test files listed are **active** tests (no leading `-` in the filename). Files/directories with a
> leading `-` and the `support/` directories are ignored by the test runner. Links point to the
> [`litehtml/litehtml-tests`](https://github.com/litehtml/litehtml-tests) repository,
> branch `main`.

---

## Properties

### Summary

| Property | Status | Values (supported count) | Tests |
|---|---|---|---|
| `display` | ⚠️ | 18 (no `grid`) | ~45 |
| `position` | ✅ | 4 | ~290 |
| `float` | ✅ | 3 | ~170 |
| `clear` | ✅ | 4 | ~35 |
| `width` / `height` | ✅ | `auto` + length/% | ~1100 |
| `min-width` / `min-height` | ✅ | `auto` + length/% | ~170 |
| `max-width` / `max-height` | ✅ | `none` + length/% | ~180 |
| `margin` (+ `-top/-right/-bottom/-left`) | ✅ | `auto` + length/% | ~250 |
| `padding` (+ `-top/-right/-bottom/-left`) | ✅ | length/% | ~300 |
| `box-sizing` | ✅ | 2 | 1 |
| `z-index` | ✅ | `auto` + integer | ~37 |
| `top` / `right` / `bottom` / `left` | ✅ | `auto` + length/% | — |
| `overflow` | ⚠️ | 6 | ~30 |
| `visibility` | ⚠️ | 3 | ~16 |
| `vertical-align` | ✅ | 8 | ~52 |
| `background` (+ longhands) | ⚠️ | see below | ~500 |
| `border` (+ longhands) | ✅ | see below | ~1200 |
| `border-radius` | ✅ | length/% (1–4 / 1–4) | 0 |
| `border-collapse` | ✅ | 2 | ~14 |
| `border-spacing` | ✅ | length{1,2} | ~53 |
| `color` | ✅ | any color | ~950 |
| `font` (+ `font-family/-size/-style/-weight`) | ✅ | see below | ~225 |
| `font-variant` | ❌ | 2 | — |
| `line-height` | ✅ | `normal` + number/length/% | ~89 |
| `text-align` | ✅ | 4 | ~19 |
| `text-indent` | ✅ | length/% | ~77 |
| `text-transform` | ✅ | 4 | ~1 |
| `white-space` | ✅ | 5 | ~97 |
| `text-decoration` (+ longhands) | ✅ | see below | ~119 |
| `text-emphasis` (+ longhands) | ❌ | — | 0 |
| `list-style` (+ longhands) | ✅ | see below | ~72 |
| `flex` (+ longhands) | ✅ | see below | ~350 |
| `caption-side` | ✅ | 2 | ~6 |
| `content` | ✅ | see below | ~240 |
| `counter-reset` / `counter-increment` | ✅ | — | ~240 |
| `cursor` | ⚠️ | any | ~10 |
| `appearance` | ❌ | 16 | 0 |

Properties **not recognized at all** (silently ignored by the parser): `letter-spacing`, `word-spacing`, `table-layout`, `empty-cells`, `direction`,
`unicode-bidi`, `quotes`, `text-shadow`, `box-shadow`, `outline`, `opacity`, `transform`,
`transition`, `animation`, `columns`, `column-*`, `grid`/`grid-*`, `filter`, `clip-path`,
`gap`/`row-gap`/`column-gap`, `object-fit` … (everything else not listed above).

---

### Box model & layout

#### `display`
⚠️ Partial — all 18 parsed values work, but modern layout values are not recognized.

Supported values: `none`, `block`, `inline`, `inline-block`, `inline-table`, `list-item`,
`table`, `table-caption`, `table-cell`, `table-column`, `table-column-group`,
`table-footer-group`, `table-header-group`, `table-row`, `table-row-group`, `inline-text`,
`flex`, `inline-flex`.

- `inline-text` is an internal value used for text nodes.
- **Not supported:** `grid`, `inline-grid`, `ruby`, `inline ruby`, `flow-root`, `contents`.
  An unrecognized value is silently dropped, so the element falls back to its default display
  (verified: `display:grid` renders identically to `display:block`).

Tests: [flex/display-flex-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/display-flex-001.htm),
[flex/flexbox_display.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flexbox_display.htm),
[flex/flexbox_stf-inline-block.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flexbox_stf-inline-block.htm).

#### `position`
✅ Supported — `static`, `relative`, `absolute`, `fixed`. (`sticky` is not recognized.)

Tests: [fixed-descendant-pr485.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/fixed-descendant-pr485.htm),
[fixed-scroll-pr485.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/fixed-scroll-pr485.htm),
[flex/abspos-autopos-htb-ltr.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/abspos-autopos-htb-ltr.htm).

#### `float`
✅ Supported — `none`, `left`, `right`.

Tests: [floats-min-width-01.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/floats-min-width-01.htm),
[float-break-pr485.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/float-break-pr485.htm),
[css21/floats-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/floats-001.htm).

#### `clear`
✅ Supported — `none`, `left`, `right`, `both`.

Tests: [css21/clear-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/clear-001.htm),
[flex/flexbox_item-clear.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flexbox_item-clear.htm).

#### `width`, `height`
✅ Supported — `auto` | `<length-percentage>`. `min-content`/`max-content`/`fit-content` are
**not** accepted for `width`/`height` (they are only supported on `flex-basis`).

Tests: [table-1-width.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/table-1-width.htm),
[table-2-width.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/table-2-width.htm),
[css21/width-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/width-001.htm).

#### `min-width`, `min-height`
✅ Supported — `auto` | `<length-percentage>`.

Tests: [floats-min-width-01.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/floats-min-width-01.htm),
[css21/min-width-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/min-width-001.htm),
[css21/min-height-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/min-height-001.htm).

#### `max-width`, `max-height`
✅ Supported — `none` | `<length-percentage>`.

Tests: [css21/max-width-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/max-width-001.htm),
[css21/max-height-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/max-height-001.htm).

#### `margin` (shorthand), `margin-top/right/bottom/left`
✅ Supported — `auto` | `<length-percentage>` (1–4 values).

Tests: [css21/margin-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/margin-001.htm),
[flex/flex-container-margin.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flex-container-margin.htm),
[flex/flex-margin-no-collapse.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flex-margin-no-collapse.htm).

#### `padding` (shorthand), `padding-top/right/bottom/left`
✅ Supported — `<length-percentage>` (non-negative, 1–4 values).

Tests: [padding.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/padding.htm),
[css21/padding-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/padding-001.htm).

#### `box-sizing`
✅ Supported — `content-box`, `border-box`.

Tests: [flex-box-sizing.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex-box-sizing.htm).

#### `z-index`
✅ Supported — `auto` | `<integer>`.

Tests: [css21/z-index-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/z-index-001.htm),
[css21/z-index-002.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/z-index-002.htm).

#### `top`, `right`, `bottom`, `left`
✅ Supported — `auto` | `<length-percentage>` (offsets for positioned elements). Covered by the
`position`/`abspos` tests above.

#### `overflow`
⚠️ Partial — `visible`, `hidden`, `scroll`, `auto`, `no-display`, `no-content`.

- `hidden`, `scroll`, `auto` clip the content (verified: `visible` overflows, the others clip).
- `scroll`/`auto` do **not** draw scrollbars — scrolling is delegated to the container.
- `no-display`/`no-content` are parsed but treated as a plain clip (their special semantics are
  not implemented).

Tests: [css21/overflow-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/overflow-001.htm),
[flex/flexbox-overflow-horiz-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flexbox-overflow-horiz-001.htm).

#### `visibility`
⚠️ Partial — `visible`, `hidden`, `collapse`.

- `hidden` hides the element but keeps its space (verified).
- `collapse` is accepted but behaves like `hidden`; the CSS table-row collapse (removing the row
  like `display:none`) is **not** implemented (verified: a `collapse` row keeps its space while a
  `display:none` row is removed).

Tests: [flex/flexbox_visibility-collapse.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flexbox_visibility-collapse.htm),
[flex/flexbox_visibility-collapse-line-wrapping.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flexbox_visibility-collapse-line-wrapping.htm).

#### `vertical-align`
✅ Supported — `baseline`, `sub`, `super`, `top`, `text-top`, `middle`, `bottom`, `text-bottom`.
(All verified to change baseline/alignment.)

Tests: [table-cell-valign-block.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/table-cell-valign-block.htm),
[css21/vertical-align-004.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/vertical-align-004.htm).

---

### Background

`background` shorthand expands to: `background-color`, `background-image`,
`background-position(-x/-y)`, `background-repeat`, `background-attachment`, `background-size`,
`background-origin`, `background-clip`.

#### `background-color`
✅ Supported.

#### `background-image`
✅ Supported — `none` | `<url>` | `linear-gradient()` | `radial-gradient()` | `conic-gradient()`
and their `repeating-*` variants.

#### `background-repeat`
⚠️ Partial — `repeat`, `repeat-x`, `repeat-y`, `no-repeat`. Two-value syntax
(`repeat no-repeat`) is not supported.

#### `background-attachment`
⚠️ Partial — `scroll`, `fixed`. Both are parsed and consumed, but litehtml paints a single
viewport with no scroll offset, so `fixed` renders identically to `scroll`.

#### `background-position` (and `-x`/`-y`)
⚠️ Partial — keywords `left`, `right`, `top`, `bottom`, `center` and lengths/percentages.
3/4-value side-relative syntax (e.g. `bottom 10px`) is not supported.

#### `background-size`
✅ Supported — `auto`, `cover`, `contain`, and `<length-percentage>{1,2}`.

#### `background-origin`, `background-clip`
✅ Supported — `border-box`, `padding-box`, `content-box`.

Tests: [background.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/background.htm),
[background-1.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/background-1.htm),
[body-background.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/body-background.htm),
[css21/background-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/background-001.htm).

---

### Border

Shorthands: `border` (all sides), `border-top/right/bottom/left`; longhands
`border-*-width/style/color`.

#### `border-width` (+ `border-*-width`)
✅ Supported — `thin`, `medium`, `thick` (1px/3px/5px), or `<length>`.

#### `border-style` (+ `border-*-style`)
✅ Supported — `none`, `hidden`, `dotted`, `dashed`, `solid`, `double`, `groove`, `ridge`,
`inset`, `outset` (all 10 render distinctly, verified).

#### `border-color` (+ `border-*-color`)
✅ Supported.

#### `border-radius` (shorthand), `border-*-radius`, `border-*-*-radius-{x,y}`
✅ Supported — `<length-percentage>{1,4} [/ <length-percentage>{1,4}]?`. **No dedicated test
coverage** in the suite.

#### `border-collapse`
✅ Supported — `collapse`, `separate`.

Tests: [css21/border-collapse-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/border-collapse-001.htm).

#### `border-spacing`
✅ Supported — `<length>{1,2}`.

Tests: [css21/border-spacing-002.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/border-spacing-002.htm).

Other border tests: [table-border-0.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/table-border-0.htm),
[css21/border-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/border-001.htm).

---

### Text & font

#### `color`
✅ Supported — any color (including `currentcolor`).

Tests: [css-2-invalid-color.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css-2-invalid-color.htm),
[css21/color-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/color-001.htm).

#### `font` (shorthand)
✅ Supported — expands to `font-style`, `font-variant`, `font-weight`, `font-size`,
`line-height`, `font-family`.

#### `font-family`
✅ Supported — comma-separated family list.

#### `font-size`
✅ Supported — `xx-small`, `x-small`, `small`, `medium`, `large`, `x-large`, `xx-large`,
`smaller`, `larger`, and `<length-percentage>`.

#### `font-style`
✅ Supported — `normal`, `italic` (verified).

#### `font-weight`
✅ Supported — `normal`, `bold`, `bolder`, `lighter`, and numeric `1`–`1000` (fractional allowed).

#### `font-variant`
❌ Not supported — `normal`, `small-caps` are parsed, but the value is never passed to the font
description, so `small-caps` has no visual effect (verified: identical to `normal`).

#### `line-height`
✅ Supported — `normal`, `<number>`, `<length-percentage>`.

Tests: [css-1-line-height.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css-1-line-height.htm),
[css21/line-height-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/line-height-001.htm),
[css21/font-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/font-001.htm).

#### `text-align`
✅ Supported — `left`, `right`, `center`, `justify`.

Tests: [css21/text-align-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/text-align-001.htm).

#### `text-indent`
✅ Supported — `<length-percentage>`.

Tests: [css21/text-indent-004.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/text-indent-004.htm).

#### `text-transform`
✅ Supported — `none`, `capitalize`, `uppercase`, `lowercase`.

Tests: [css21/text-transform-applies-to-013.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/text-transform-applies-to-013.htm).

#### `white-space`
✅ Supported — `normal`, `nowrap`, `pre`, `pre-line`, `pre-wrap`.

Tests: [css21/white-space-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/white-space-001.htm),
[css21/white-space-002.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/white-space-002.htm).

#### `text-decoration` (shorthand), `-line`, `-style`, `-color`, `-thickness`
✅ Supported:
- `text-decoration-line`: `none`, `underline`, `overline`, `line-through`.
- `text-decoration-style`: `solid`, `double`, `dotted`, `dashed`, `wavy`.
- `text-decoration-color`: any color.
- `text-decoration-thickness`: `auto`, `from-font`, or `<length-percentage>`.

All are rendered by the container (verified: each style produces a distinct line).

Tests: [text-decoration.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/text-decoration.htm),
[css21/text-decoration-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/text-decoration-001.htm).

#### `text-emphasis` (shorthand), `-style`, `-color`, `-position`
❌ Not supported — parsed and computed into the font description, but neither litehtml nor the
reference cairo container draws emphasis marks, so it has no visual effect (verified: identical to
no emphasis).

---

### List

#### `list-style` (shorthand)
✅ Supported — expands to `list-style-image`, `list-style-position`, `list-style-type`.

#### `list-style-type`
✅ Supported — `none`, `circle`, `disc`, `square`, `armenian`, `cjk-ideographic`, `decimal`,
`decimal-leading-zero`, `georgian`, `hebrew`, `hiragana`, `hiragana-iroha`, `katakana`,
`katakana-iroha`, `lower-alpha`, `lower-greek`, `lower-latin`, `lower-roman`, `upper-alpha`,
`upper-latin`, `upper-roman`.

#### `list-style-position`
✅ Supported — `inside`, `outside`.

#### `list-style-image`
✅ Supported — `none` | `<url>`.

Tests: [css21/list-style-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/list-style-001.htm),
[css21/list-style-position-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/list-style-position-001.htm).

---

### Flexbox

Shorthands: `flex` (→ `flex-grow`, `flex-shrink`, `flex-basis`), `flex-flow` (→ `flex-direction`,
`flex-wrap`).

#### `flex-direction`
✅ Supported — `row`, `row-reverse`, `column`, `column-reverse`.

#### `flex-wrap`
✅ Supported — `nowrap`, `wrap`, `wrap-reverse`.

#### `justify-content`
✅ Supported — `normal`, `flex-start`, `flex-end`, `center`, `space-between`, `space-around`,
`start`, `end`, `left`, `right`, `space-evenly`, `stretch`.

#### `align-items`
✅ Supported — `normal`, `stretch`, `baseline`, `center`, `start`, `end`, `self-start`,
`self-end`, `flex-start`, `flex-end`.

#### `align-self`
✅ Supported — `auto` + the `align-items` values.

#### `align-content`
✅ Supported — `flex-start`, `start`, `flex-end`, `end`, `center`, `space-between`,
`space-around`, `stretch`.

#### `flex-grow`, `flex-shrink`
✅ Supported — `<number [0,∞]>`.

#### `flex-basis`
✅ Supported — `auto`, `content`, `fit-content`, `min-content`, `max-content`, or
`<length-percentage>`. (Note: the `min/max/fit-content` keywords are supported here but **not** on
`width`/`height`.)

#### `order`
✅ Supported — `<integer>`.

Flexbox has the deepest test coverage (≈350 active files). Examples:
[flex/flex-grow-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flex-grow-001.htm),
[flex/flex-shrink-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flex-shrink-001.htm),
[flex/flex-basis-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flex-basis-001.htm),
[flex/flex-direction.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flex-direction.htm),
[flex/flexbox_flex-wrap-wrap.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/flexbox_flex-wrap-wrap.htm),
[flex/justify-content_center.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/justify-content_center.htm),
[flex/align-items-005.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/align-items-005.htm),
[flex/align-content-006.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/align-content-006.htm),
[flex/align-self-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/align-self-001.htm),
[flex/order-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/flex/order-001.htm).

---

### Table

#### `caption-side`
✅ Supported — `top`, `bottom` (verified: the caption moves).

Tests: [css21/caption-side-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/caption-side-001.htm).

`border-collapse` and `border-spacing` are documented under [Border](#border).

---

### Generated content & counters

#### `content`
✅ Supported (on `::before`/`::after`) — quoted strings, `attr()`, `counter()`, `counters()`,
`url()`, and the quote keywords `open-quote`, `close-quote`, `no-open-quote`, `no-close-quote`.

#### `counter-reset`, `counter-increment`
✅ Supported — `<counter-name> <integer>?` lists; used by `counter()`/`counters()` in `content`.

Tests: [counter.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/counter.htm),
[css21/content-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/content-001.htm),
[css21/counter-increment-006.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/counter-increment-006.htm).

---

### UI & misc

#### `cursor`
⚠️ Partial — the value is parsed and forwarded to the container. It has no visual effect in a
static render, so it cannot be verified via PNG. Support depends entirely on the container.

Tests: [css21/cursor-002.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/cursor-002.htm).

#### `appearance`
❌ Not supported — all values (`none`, `auto`, `button`, `checkbox`, `radio`, `textfield`,
`menulist`, `listbox`, `meter`, `progress-bar`, `push-button`, `searchfield`,
`slider-horizontal`, `square-button`, `textarea`, `menulist-button`) are parsed but never
consumed, so they have no effect on rendering (verified: identical output with/without
`appearance`).

---

## Selectors

Supported selector types are listed below. See the next section for pseudo-classes/pseudo-elements.

### Type & universal selectors
- ✅ Type selector (`div`, `p`, …) — ASCII case-insensitive for HTML documents.
- ✅ Universal selector `*`.
- ⚠️ Namespace prefixes (`ns|div`, `*|div`, `|div`, `[ns|attr]`) are parsed, but the prefix is
  ignored during matching (HTML elements have no namespace), so `ns|div` behaves like `div`.

### ID & class selectors
- ✅ `#id` — ASCII case-insensitive in quirks mode.
- ✅ `.class` — ASCII case-insensitive in quirks mode.

### Attribute selectors
- ✅ `[attr]` (exists).
- ✅ `[attr=value]`, `[attr~=value]` (whitespace-separated word), `[attr|=value]`
  (`value` or `value-` prefix), `[attr^=value]`, `[attr$=value]`, `[attr*=value]`.
- ✅ Case-sensitivity modifiers `i` (ASCII case-insensitive) and `s` (case-sensitive).
- Attribute values of a set of HTML attributes (`type`, `rel`, `lang`, `disabled`, …) are matched
  ASCII case-insensitively by default, per the HTML spec.

### Combinators
- ✅ Descendant `A B`.
- ✅ Child `A > B`.
- ✅ Adjacent sibling `A + B`.
- ✅ General sibling `A ~ B`.
- ❌ Column combinator `A || B` — not implemented.

### Selector lists & specificity
- ✅ Comma-separated selector lists with forgiving parsing (an invalid selector is dropped without
  invalidating the whole list).
- ✅ Specificity is computed as `(a,b,c,d)`: `b` = number of `#id`s, `c` = number of
  classes/attributes/pseudo-classes, `d` = 1 when a type selector is present; `:where()` contributes
  nothing. Later/more-specific rules win as expected.

### Test coverage

| Feature | Active tests |
|---|---|
| Type / universal | [css21/type-selector-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/type-selector-001.htm), [css21/universal-selector-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/universal-selector-001.htm) |
| ID / class | [class-selector.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/class-selector.htm), [css21/id-selector-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/id-selector-001.htm), [css21/class-selector-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/class-selector-001.htm) |
| Attribute | [attr-selector.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/attr-selector.htm), [attr-modifier-s.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/attr-modifier-s.htm), [css21/attribute-selector-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/attribute-selector-001.htm) |
| Combinators | [css21/descendant-selector-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/descendant-selector-001.htm), [css21/child-selector-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/child-selector-001.htm), [css21/adjacent-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/adjacent-001.htm), [css21/sibling-selector-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/sibling-selector-001.htm) |
| Case sensitivity | [class-id-case-sensitivity-quirks-mode.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/class-id-case-sensitivity-quirks-mode.htm) |
| General | [selectors.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/selectors.htm) |

---

## Selectors: pseudo-classes & pseudo-elements

### Pseudo-classes that actually match

**Structural**

| Pseudo-class | Notes |
|---|---|
| `:root` | matches the root element (set dynamically) |
| `:first-child`, `:last-child`, `:only-child` | |
| `:first-of-type`, `:last-of-type`, `:only-of-type` | |
| `:nth-child(An+B [of S])`, `:nth-last-child(An+B [of S])` | full `An+B` + `of` selector list |
| `:nth-of-type(An+B)`, `:nth-last-of-type(An+B)` | full `An+B` |

**Logical combinators**

| Pseudo-class | Notes |
|---|---|
| `:is(<forgiving-selector-list>)` | |
| `:where(<forgiving-selector-list>)` | matches like `:is()`, but zero specificity |
| `:not(<selector-list>)` | |
| `:lang(<language-range>)` | |

**Link / user action / form state**

| Pseudo-class | Notes |
|---|---|
| `:link` | matches `<a href>` elements |
| `:hover` | set on mouse-over |
| `:active` | set on mouse press |
| `:checked` | matches the `checked` attribute or dynamic state |
| `:disabled` | matches the `disabled` attribute or dynamic state |

### Recognized by the parser but never match

These are accepted so that a selector list containing them (e.g. `a:link, a:visited { … }`) keeps
its other selectors, but litehtml has no matching logic or state for them, so they never match on
their own:

`:any-link`, `:visited`, `:local-link`, `:target`, `:target-within`, `:scope`, `:focus`,
`:focus-visible`, `:focus-within`, `:empty`, `:host`.

- `:visited` — litehtml does not track visited links.
- `:focus` / `:focus-visible` / `:focus-within` — no focus state is maintained.
- `:empty` — no structural emptiness check is implemented.
- `:host` — shadow DOM is not implemented (never matches, as in a browser outside a shadow tree).

### Pseudo-elements

- ✅ `::before`, `::after` — generate boxes (single-colon `:before`/`:after` also accepted).
- ❌ `::first-line`, `::first-letter`, `::selection`, `::marker`, `::placeholder`, `::backdrop`,
  `::file-selector-button` — parsed but never match (no box generated).

### Test coverage

| Selector | Active tests |
|---|---|
| `:nth-child()` | [nth-child.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/nth-child.htm) |
| `:first-child` | [selectors.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/selectors.htm), [css21/first-child-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/first-child-001.htm) |
| `:nth-of-type()`, `:is()` | [selectors.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/selectors.htm) |
| `:not()` | [class-selector.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/class-selector.htm), [selectors.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/selectors.htm) |
| `:lang()` | [css21/lang-selector-003.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/lang-selector-003.htm) |
| `:link` | [css21/c21-pseud-link-000.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/c21-pseud-link-000.htm) |
| `:active` | [css21/active-selector-001.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/css21/active-selector-001.htm) |
| `:hover` | [test6.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/test6.htm), [test27.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/test27.htm) |
| `::before` / `::after` | [text-before-after.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/text-before-after.htm), [counter.htm](https://github.com/litehtml/litehtml-tests/blob/main/render/counter.htm) |

No dedicated active tests exist for `:root`, `:empty`, `:visited`, `:checked`, `:disabled`,
`:where()`, `:nth-last-child()`, `:nth-last-of-type()`, `:only-child`, `:first-of-type`,
`:last-of-type`, or `:only-of-type`.
