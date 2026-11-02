# Pango/Cairo shaped text on Linux

Opt-in Linux Pango/Cairo text drawing and measurement. This package does not
change built-in `Graphics.DrawString` or native GTK control text. It is independent
of FontKit and TextMetrics; applications supply an explicit family and style.

## Install

From a Buoy consumer project with BPM installed:

```sh
bpm add shapingkit --git https://github.com/BuoyBay/shapingkit.git --rev refs/heads/main
```

BPM records the resolved commit in its lockfile. Keep that lockfile with the
consumer project for reproducible installs.

## API at a glance

| API | Meaning |
| --- | --- |
| `TextStyle` | Mutable Family (`sans-serif`), Size (12), Bold/Italic/Underline (False). |
| `MeasureRaster(text, style)` | Measure in raster pixels on an owned image context matching Rasterize; no private handle. |
| `Measure(context, text, style)` | Measure plain UTF-8 on a real live Cairo context. |
| `Draw(context, text, x, baselineY, style, red, green, blue, alpha=1)` | Draw and return metrics from the exact layout drawn. |
| `MeasureGraphics0720(graphics, text, style)` | Guarded adapter for the tested 0.72.0 Linux runtime. |
| `DrawGraphics0720(graphics, text, x, baselineY, style, red, green, blue, alpha=1)` | Same guarded adapter, drawing and measuring together. |

## Platform and scope

This is an opt-in Linux Pango/Cairo backend. Keep shaped measurement and rendering on the same backend and coordinate system.

## Build and API reference

Source is in `modules/`, with native/runtime inputs where required. Use the
[package guide](docs/API.md) for backend-specific build prerequisites, ownership
rules and supported targets. Rebuilding a native backend requires that target's
compiler and SDK; sealing an archive alone does not rebuild the backend.

## Local Buoy setup

On your own machine, install Buoy and run:

```sh
buoy ai init --both
```

## Contributing

[Contributing](CONTRIBUTING.md) · [License](LICENSE)
