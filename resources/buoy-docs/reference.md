---
title: "shapingkit reference"
summary: "Capabilities, ownership, backends and verification."
category: Reference
related: ["index.md", "getting-started.md", "typed-api.md"]
---

# ShapingKit 0.1.0

Opt-in Linux Pango/Cairo text drawing and measurement. This package does not
change built-in `Graphics.DrawString` or native GTK control text. It is independent
of FontKit and TextMetrics; applications supply an explicit family and style.

## Public Picture integration

`Rasterize(text, style, width, height, x, baselineY, red, green, blue, alpha=1)`
returns a transparent `Picture` rendered by an independently owned Cairo/Pango
surface and copied through the public `Picture.FromBuffer` API. Draw that image
with `Graphics.DrawPicture` or assign it to `ImageView.Image`. This route never
reads a private Graphics wrapper and does not depend on a Buoy runtime fingerprint.
It is the preferred integration when raster compositing is suitable. Size and
placement are pixels at scale 1; render at the desired device resolution, then
set the Picture scale or choose an explicit destination rectangle when drawing.
It does not inherit a destination Graphics transform, clip, font options or
operator during layout; those apply later when compositing the image.

Dimensions must be 1..16384 and the RGBA buffer at most 64 MiB (16,777,216
pixels). Temporary native/managed buffers can consume several times that amount.
Empty text returns a transparent image. RGB uses straight alpha at the public
boundary; the native bridge converts Cairo premultiplied native-endian ARGB.
Partial-alpha conversion rounds at 8-bit precision. No temporary files are used.

## Installed 0.72.0 NativeHandle discrepancy

The installed docs call `Graphics.NativeHandle` a `cairo_t*`, but the tested Linux
x86_64 compiler/runtime exposes its internal Graphics wrapper instead. Passing
that value directly to Cairo crashes. The separate `MeasureGraphics0720` and
`DrawGraphics0720` functions adapt that private wrapper **only** after checking
the loaded `buoy_ui.so` SHA-256 against the exact tested binary:

`44ccfebe300450f789e006d3bf7e90c5f95abe9e252842d1c327957f7db64c50`

An unrecognized runtime raises `ShapingException` code 4 before dereferencing the
wrapper. This is a deliberately narrow compatibility adapter for the tested
0.72.0 compiler/runtime, not a general Buoy ABI. Do not use it with future compiler
versions that expose a real Cairo handle. The compiler tested here has SHA-256
`1d27cbd6ddbeaa816aba0ce145f2a6408d94b98bce736f85f46b3604ca870f63`.
The adapter checks the runtime binary, not the compiler that produced an application.

`Measure` and `Draw` accept **real** borrowed `cairo_t*` values. Use these when a
correct upstream getter is available or with an independently created Cairo
context. They must not receive this installed build's `Graphics.NativeHandle`.

## Usage and API

Install `packages/shapingkit-0.1.0.buipkg` in the application's `packages/`
directory. Import `shapingkit.ShapingKit` (an alias such as `Shaping` qualifies
module functions; `TextStyle` and `TextMetrics` are unqualified types).

The complete compiler-checked example is
`tests/shapingkit_smoke.bui` (workspace file `tests/shapingkit_smoke.bui`). The build test
also checks/runs that example with its import changed to the sealed package.

| API | Meaning |
| --- | --- |
| `TextStyle` | Mutable Family (`sans-serif`), Size (12), Bold/Italic/Underline (False). |
| `MeasureRaster(text, style)` | Measure in raster pixels on an owned image context matching Rasterize; no private handle. |
| `Measure(context, text, style)` | Measure plain UTF-8 on a real live Cairo context. |
| `Draw(context, text, x, baselineY, style, red, green, blue, alpha=1)` | Draw and return metrics from the exact layout drawn. |
| `MeasureGraphics0720(graphics, text, style)` | Guarded adapter for the tested 0.72.0 Linux runtime. |
| `DrawGraphics0720(graphics, text, x, baselineY, style, red, green, blue, alpha=1)` | Same guarded adapter, drawing and measuring together. |
| `Rasterize(text, style, width, height, x, baselineY, red, green, blue, alpha=1)` | Public Picture integration, no private runtime access. |
| `Graphics0720AdapterSupported()` | Checks the loaded runtime fingerprint without accessing a Graphics pointer. |
| `IsSupported()` | Platform predicate only: True on Linux. Does not promise installed native libraries or adapter compatibility. |

`Size` is an em size in Cairo user units, with no implicit 96/72 points-to-pixels
conversion. It is finite and in `(0, 16384]`. Colors are finite unpremultiplied
RGBA components in `[0,1]`; coordinates are finite. A style must be non-Nil and
the family must be nonempty UTF-8, at most 4096 bytes. Text is plain UTF-8 (never
markup), at most 1 MiB. Additionally, Unicode scalar count multiplied by size
must not exceed 262144; this conservative aggregate budget prevents Pango
signed-32-bit advance/extent overflow. Split large text into bounded runs.
Native negative rectangle sizes are rejected rather than returned as metrics. Pango resolves family fallback, script shaping, combining
marks and automatic paragraph bidi direction. Coverage still depends on installed
fonts; `UnknownGlyphs` reports unresolved glyphs. No registration or enumeration
is performed. This version has no forced direction, language, features, wrapping,
alignment or ellipsis API. Newlines create additional unwrapped lines.

`TextMetrics` exposes `InkX/Y/Width/Height`, `LogicalX/Y/Width/Height`, `Baseline`
and `UnknownGlyphs`. Both rectangles are in **user space relative to `(x,
baselineY)`**, not device pixel bounding boxes. `Baseline` is the first baseline's
distance from the Pango layout top. Negative Y is normal. Logical width includes
spaces; ink bounds describe painted glyphs and decorations. Underline can change
ink bounds but not logical layout size. For collision/placement use rectangle
origin and size, not width alone. Multi-line drawing anchors the first baseline.

## State, lifetime, ownership and errors

Every call creates its layout from the current Cairo transformation and surface,
and releases its Pango layout, font description, attributes and copied Cairo path
before returning. No Graphics/context/surface is retained or destroyed. Measure
does not draw. Draw inherits CTM, clip, font options and compositing operator,
sets its explicit RGBA source temporarily, and restores source/state/current path
and current point. Cairo `save/restore` alone does not preserve the path, so the
bridge copies and restores that separately. Measure again after changing the
transform, target, font environment, text or style; clipping does not reduce metrics.

For paint Graphics, call only within that Paint event. For offscreen Graphics,
keep the owning Picture alive throughout the call. The adapters reacquire the
native handle each time. Do not concurrently mutate/draw the same Graphics or
style. A raw context must be a valid live writable Cairo context. Nil and Cairo
error objects are rejected; arbitrary non-Nil addresses, dangling pointers and
wrong object types cannot be safely validated by Cairo. A finished surface can
still report success before drawing; callers must not pass finished surfaces.

Invalid arguments raise `InvalidArgumentException`. Cairo context/render failures
raise `ShapingException` (code 2 or 3). Non-Linux calls raise code 4 and never call
the native bridge. `IsSupported` is only a platform check. The sealed archive
contains **x86-64 and ARM64 Linux** native slices. ARM64 libraries and public
Buoy Picture rendering passed under userspace QEMU on x86-64, not ARM hardware.
The private 0.72.0 adapter still rejects that different ARM runtime.
macOS/Windows stubs are compiler-checked; they deliberately provide no renderer.

The String ABI is NUL-terminated. Do not pass embedded NUL. The wrapper rejects
NUL bytes if visible to `Bytes.FromString`, but the installed runtime already
turns `String.Chr(0)` into zero UTF-8 bytes: it cannot reliably diagnose NUL data
discarded before the helper receives it. The native ABI likewise consumes C
strings. Supporting arbitrary binary UTF-8 buffers requires a separate length-aware
API, outside this focused helper.

## Build and verification

On Linux install a C compiler, `pkg-config`, and Pango/Cairo development headers
(for example `libpango1.0-dev` and `libcairo2-dev`). Build with
`python3 tools/build_shapingkit.py`; run tests with
`python3 tools/build_shapingkit.py --test`. `--check-only` runs just the compiler
checks. All diagnostics JSON `errors` arrays are read even on exit 0.

Sealing stages only Linux slices with matching native-source receipts and a verified
64-bit ELF architecture. Stale slices are omitted; an architecture mismatch or
no current slice fails sealing. Packaging tests exercise these failure paths.
The adapter regression exports the expected symbol from a different binary and
checks rejection before touching an intentionally invalid pointer. The Buoy
consumer tests the public raster route, pixel channels/partial alpha, and normal
Picture compositing as well as the exact-runtime adapter.

An isolated Docker toolchain now supplies ARM64 GCC/Pango/Fontconfig and
userspace QEMU without host package or binfmt changes. The native tests, actual
package DSO, and Buoy public MeasureRaster/Rasterize/Picture composition passed
under emulation. A real cross-built JPEG8 library supports the image runtime;
no ABI symlink is used. See `tests/font_arm_evidence/README.md`. Hardware-native
ARM execution and ARM private-adapter compatibility remain unverified.

The native runtime depends on the system Pango/Cairo/GLib libraries; these are not
vendored. Buoy's installed Picture runtime additionally needs its own JPEG ABI.
This Debian host lacks Ubuntu's `libjpeg.so.8`; the integration test was run with
`LD_LIBRARY_PATH=/tmp/buoy-chatui-jpeg8/build`, an existing real JPEG8 build. No
system files or toolchain were modified and no incompatible JPEG symlink was used.

See [evidence and upstream specification](../../resources/buoy-docs/shapingkit.md).

## Primary references

- [Cairo text API limitations](https://www.cairographics.org/manual/cairo-text.html)
- [PangoCairo create_layout: context transform/surface and ownership](https://docs.gtk.org/PangoCairo/func.create_layout.html)
- [PangoCairo show_layout: top-left positioning](https://docs.gtk.org/PangoCairo/func.show_layout.html)
- [Pango layout baseline](https://docs.gtk.org/Pango/method.Layout.get_baseline.html)
- [Pango logical/ink extents and origins](https://docs.gtk.org/Pango/method.Layout.get_extents.html)
- [Pango absolute font size](https://docs.gtk.org/Pango/method.FontDescription.set_absolute_size.html)
- [Cairo path copy ownership](https://www.cairographics.org/manual/cairo-Paths.html#cairo-copy-path)

- [Cairo image-surface pixel format and ownership](https://www.cairographics.org/manual/cairo-Image-Surfaces.html)
