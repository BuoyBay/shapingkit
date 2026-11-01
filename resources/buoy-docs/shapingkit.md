---
summary: "Usage and API contracts for shapingkit."
related: ["index.md", "getting-started.md", "api.md", "typed-api.md"]
---

# Linux Graphics text evidence and upstream patch specification

## Verified environment and artifacts

Buoy 0.72.0, Linux x86_64, PangoCairo 1.56.3 and Cairo 1.18.4. Exact runtime
fingerprints and host details are recorded in `tests/shapingkit_evidence/environment.json`.
The build script regenerates the environment, native diagnostics, Buoy comparison
output, disassembly and `comparison.png` under `tests/shapingkit_evidence/`.
The PNG's left column is built-in `Graphics.DrawString`; right is the opt-in helper.
Rows: Arabic, Indic, combining acute, stacked combining marks, mixed bidi, CJK
fallback and trailing space. Both columns use DejaVu Sans at size 32 and the same
baseline. These are actual offscreen Picture.Graphics calls, not simulated built-ins.

The native tests inspect glyph count, missing glyphs, Pango run direction levels
and font changes. They also assert Arabic lam-alef shaping, combining composed/
decomposed metric equivalence, all eight style combinations, unknown-glyph reporting,
empty/multiline text, size effects, argument rejection, Cairo error-object rejection,
exact Draw/Measure metric equality, actual pixel bounds around the baseline,
translation/scale with partial clipping, rotation/scale state preservation,
source/path/current point preservation and fully clipped draws. Private-adapter
rejection is tested without the recognized runtime loaded; the Buoy test exercises
the adapter success path. Pixel assertions validate geometry/state, not linguistic
correctness. The PNG was visually inspected: joined Arabic, Devanagari output,
reordered Hebrew runs and CJK fallback are visible on the helper side. This is
not a full script conformance, accessibility or line-breaking suite.

Observed diagnostics with the installed fonts:

- Arabic `سلام`: Cairo toy API emits four glyphs; Pango emits three and an odd bidi
  level run. Built-in ink width 87; helper logical width 66; no helper missing glyphs.
- Indic `नमस्ते`: toy API emits six missing glyphs; Pango emits five glyphs with
  zero unknowns. Built-in width 112; helper logical width 68.
- Combining `é`: toy API emits two glyphs; Pango emits one. Composed/decomposed
  Pango ink width and logical width agree. This particular acute sample looks
  similar in both columns; glyph-count differences alone do not prove a visual defect.
- Mixed `abc אבג 123`: Pango reports an odd bidi run and reorders output. Built-in
  draws in its input glyph order. Width alone cannot diagnose bidi correctness.
- `A漢B`: toy API emits one missing glyph; Pango switches fonts across runs and
  reports zero unknowns. Built-in width 61; helper logical width 76.
- `A `: built-in width 22 is ink width, excluding the trailing space; Cairo advance
  and Pango logical width are 32. This is a metric-definition difference, not proof
  that built-in ink measurement disagrees with its own toy glyph drawing.

Ink and logical dimensions are deliberately labelled separately. Comparing them
does not imply a one-to-one measurement contract. All six desktop target compiler
checks pass; **native runtime verification covers Linux x86_64 only**. Unsupported
macOS/Windows runtime exceptions were compiler-checked, not executed natively.

## Runtime-source availability and direct binary evidence

No upstream Buoy compiler or Graphics runtime sources were found in this repository
or the installed `/usr/share/buoy` and `/usr/lib/buoy` resources. The checkout has
third-party package bridge sources. The installation provides compiled runtime
libraries, stubs, docs and CRT objects. This finding is scoped to the authorized
checkout and toolchain install; no claim is made about other machines or repositories.

`objdump -d --disassemble=<symbol> /usr/lib/buoy/runtime/x86_64-unknown-linux-gnu/buoy_ui.so`
confirms the following exported functions (full output is saved in evidence):

- `bui_ui_graphics_draw_string_styled`: calls `cairo_select_font_face`,
  `cairo_set_font_size`, `cairo_move_to`, `cairo_show_text`. Underline uses
  `cairo_text_extents`, line primitives and `cairo_stroke`.
- `bui_ui_graphics_measure_string_w_styled` and `_h_styled`: call
  `cairo_text_extents` and return ink width/height, respectively.
- `bui_ui_graphics_new_for_picture`: allocates a 40-byte wrapper, stores the
  `cairo_create` result at offset zero, Picture at offset 8, surface at offset 16,
  then width/height doubles. GDB at the helper entry confirms `NativeHandle`
  supplies the wrapper, not its first pointer's value. Direct use as `cairo_t*`
  crashed inside `cairo_get_target`. The private adapter is tied to this evidence
  and exact runtime SHA, rather than auto-detecting unknown pointer layouts.

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
filenames: upstream source is required to implement these fixes. No toolchain files
or files outside this task's owned paths were changed.

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
Mismatch tests export the same symbol from an unapproved executable and pass
an invalid pointer; the guard rejects before dereference. Package staging now
rejects stale receipts, incorrect ELF architecture and empty native slices.
See the package README for raster dimensions, alpha, scaling and temporary
memory bounds.

## Public raster measurement and extent bounds

`MeasureRaster(text, style)` uses an owned image context matching `Rasterize`,
without the private Graphics adapter. Size and metrics are raster pixels.
The scalar-count × size product is capped at 262144, in addition to the UTF-8
byte limit, to avoid Pango signed-32-bit extent/advance overflow. Split oversized
content into bounded runs; negative native rectangle sizes are rejected.
