---
summary: "Usage and API contracts for shapingkit."
related: ["index.md", "getting-started.md", "api.md", "typed-api.md"]
---

# Linux shaped-text integration

## Runtime compatibility

This package uses PangoCairo for shaped text. Font fallback and glyph coverage
depend on installed fonts. Native runtime coverage is Linux x86_64; checks for
other targets do not establish a native backend there.

## Graphics handle compatibility

`objdump -d --disassemble=<symbol> /usr/lib/buoy/runtime/x86_64-unknown-linux-gnu/buoy_ui.so`
shows the following exported behavior:

- `bui_ui_graphics_draw_string_styled`: calls `cairo_select_font_face`,
  `cairo_set_font_size`, `cairo_move_to`, `cairo_show_text`. Underline uses
  `cairo_text_extents`, line primitives and `cairo_stroke`.
- `bui_ui_graphics_measure_string_w_styled` and `_h_styled`: call
  `cairo_text_extents` and return ink width/height, respectively.
- `bui_ui_graphics_new_for_picture`: allocates a 40-byte wrapper, stores the
  `cairo_create` result at offset zero, Picture at offset 8, surface at offset 16,
  then width/height doubles. GDB at the helper entry confirms `NativeHandle`
  supplies the wrapper, not its first pointer's value. Direct use as `cairo_t*`
  crashed inside `cairo_get_target`. The private adapter is restricted to the exact supported runtime SHA, rather than auto-detecting unknown pointer layouts.

The runtime also imports GTK label functions and Pango calls used elsewhere.
Presence of those imports does not establish Pango use by Graphics. Native control
text has a separate GTK path and is not changed or diagnosed by this helper.

## Narrow upstream patch specification (not applied here)

1. Fix the compiler/runtime implementation of `Graphics.NativeHandle` to return
   its actual platform context. On Linux expose a runtime accessor returning the
   wrapper's `cairo_t*`, and lower the public property to that accessor. Preserve
   the private wrapper for all existing internal Graphics operations. Return Nil
   after owner teardown; retain the documented Paint-event/Picture lifetime rules.
   Add regression tests using `cairo_status` and `cairo_get_target` on the public
   handle for both paint and offscreen Graphics. The helper adapter should then
   be retired; its raw-context API needs no layout change.
2. In the Linux implementations of the three exported styled string functions
   listed above, replace the toy-font/text sequence with a shared Pango layout
   constructor taking the actual current Cairo context, UTF-8 text, family,
   absolute user-space size, bold and italic. Use `pango_cairo_create_layout`,
   `pango_font_description_set_absolute_size(size * PANGO_SCALE)`, weight/style,
   plain `pango_layout_set_text`, automatic bidi and standard Pango fallback.
   Avoid the implicit DPI conversion caused by Pango's point-size setter.
3. Anchor drawing with `cairo_move_to(cr, x, y - baseline/PANGO_SCALE)`, because
   Buoy promises baseline Y while `pango_cairo_show_layout` anchors the layout top.
   Preserve the existing transform/clip/operator. Preserve source/state/path
   around drawing; do not substitute a detached context or identity matrix.
   Underline should be a Pango attribute so fallback/script placement is handled
   by the layout engine, rather than a separate toy-width rule.
4. Preserve the existing built-in MeasureStringWidth/Height **ink** contract by
   returning shaped ink width/height from `pango_layout_get_extents`. Do not silently
   substitute logical advances. Add separately named logical/layout metrics for
   applications needing trailing spaces, line boxes, baseline and overhang origins.
   Keep underline out of legacy measurement if preserving its documented lack of
   size effect; include it in a separately documented decorated ink metric. Draw
   and measurement must share text/font/style/layout construction.
5. Release every layout/font/attribute object. Validate sizes/text/context before
   native calls. Add Cairo error/null checks and runtime regression coverage using
   the same Arabic, Indic, combining, bidi, fallback, baseline, transform/clip and
   ownership cases. Choose and document newline behavior explicitly rather than
   silently changing the legacy single-string contract. Do not modify GTK native
   controls as part of the Graphics fix.

The binary symbol locations above are concrete patch entry points, not source
filenames: upstream source is required to implement these fixes.

## Primary sources

[Cairo's text API reference](https://www.cairographics.org/manual/cairo-text.html)
explicitly documents its toy API's complex-script, bidi, diacritic and fallback
limitations and recommends Pango. The helper follows Pango's documented
[context-matched layout creation](https://docs.gtk.org/PangoCairo/func.create_layout.html),
[drawing origin](https://docs.gtk.org/PangoCairo/func.show_layout.html),
[baseline](https://docs.gtk.org/Pango/method.Layout.get_baseline.html),
[ink/logical extents](https://docs.gtk.org/Pango/method.Layout.get_extents.html), and
[absolute size](https://docs.gtk.org/Pango/method.FontDescription.set_absolute_size.html)
contracts. Path handling follows [Cairo's copy-path ownership contract](https://www.cairographics.org/manual/cairo-Paths.html#cairo-copy-path).

## Continued implementation: public raster integration

`Rasterize` now provides a public-API alternative: owned Cairo/Pango image,
straight RGBA conversion, and `Picture.FromBuffer`. Its result can be composited
with `Graphics.DrawPicture` without dereferencing Graphics.NativeHandle. It
does not fix the public NativeHandle discrepancy or replace built-in text.
`Graphics0720AdapterSupported` separately exposes the exact-runtime capability.
The adapter rejects unapproved runtime binaries before dereferencing Graphics
handles. Package staging rejects stale receipts, incorrect ELF architecture and
empty native slices.
See the package README for raster dimensions, alpha, scaling and temporary
memory bounds.

## Public raster measurement and extent bounds

`MeasureRaster(text, style)` uses an owned image context matching `Rasterize`,
without the private Graphics adapter. Size and metrics are raster pixels.
The scalar-count × size product is capped at 262144, in addition to the UTF-8
byte limit, to avoid Pango signed-32-bit extent/advance overflow. Split oversized
content into bounded runs; negative native rectangle sizes are rejected.
