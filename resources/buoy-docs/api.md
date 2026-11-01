---
title: "shapingkit API contracts"
summary: "Usage and behavioral contracts for shapingkit."
category: "Reference"
symbols: ["TextMetrics", "TextStyle", "ShapingException"]
related: ["index.md", "getting-started.md", "typed-api.md"]
---

Opt-in Linux Pango/Cairo shaped text drawing and matching measurement for Graphics.NativeHandle

[Typed API](typed-api.md) · [Getting started](getting-started.md)

`Rasterize` renders into an owned Cairo surface and transfers through public
`Picture.FromBuffer`; prefer it when raster compositing is acceptable.
`Graphics0720AdapterSupported` checks the separate private compatibility path.
See [full contracts](shapingkit.md) and the package README for units/lifetime.
