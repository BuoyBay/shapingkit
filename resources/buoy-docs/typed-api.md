---
title: "shapingkit typed API"
summary: "Source-derived public signatures, properties, parameters, defaults, and return values."
category: "Reference"
order: 21
types: ["ShapingException", "TextMetrics", "TextStyle"]
dependencies: []
symbols: ["ShapingException", "TextMetrics", "TextStyle"]
related: ["api.md", "getting-started.md", "index.md"]
---

# shapingkit typed API

Generated from the package source declarations. Platform guards are shown where present; inherited members are documented on the base type. See the [API contracts](api.md) for behavior and limitations.

## TextMetrics

`Sequential Structure TextMetrics`

User-space rectangles relative to the layout origin at the first baseline.

### Fields

| Name | Type | Value / Default | Description |
| --- | --- | --- | --- |
| `InkX` | `Double` | — | — |
| `InkY` | `Double` | — | — |
| `InkWidth` | `Double` | — | — |
| `InkHeight` | `Double` | — | — |
| `LogicalX` | `Double` | — | — |
| `LogicalY` | `Double` | — | — |
| `LogicalWidth` | `Double` | — | — |
| `LogicalHeight` | `Double` | — | — |
| `Baseline` | `Double` | — | — |
| `UnknownGlyphs` | `Integer` | — | — |

## TextStyle

`Public Class TextStyle`

Explicit styles, independent of Graphics' mutable font properties.

### Properties

| Name | Type | Read-Only | Shared | Default | Description |
| --- | --- | --- | --- | --- | --- |
| `Family` | `String` | No | No | `"sans-serif"` | — |
| `Size` | `Double` | No | No | `12.0` | — |
| `Bold` | `Boolean` | No | No | `False` | — |
| `Italic` | `Boolean` | No | No | `False` | — |
| `Underline` | `Boolean` | No | No | `False` | — |

## ShapingException

`Public Class ShapingException Inherits Exception`

### Constructor

`Public Sub Constructor(message As String, code As Integer)`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `message` | `String` | `ByVal` | Required |
| `code` | `Integer` | `ByVal` | Required |

Returns: no value.

## ShapingKit functions

### IsSupported

`Public Function IsSupported() As Boolean`

No parameters.

#### Returns

| Type |
| --- |
| `Boolean` |

### Graphics0720AdapterSupported

`Public Function Graphics0720AdapterSupported() As Boolean`

True only when the loaded runtime matches the tested private adapter ABI. Does not validate a particular Graphics or its owner lifetime.

No parameters.

#### Returns

| Type |
| --- |
| `Boolean` |

### MeasureRaster

`Public Function MeasureRaster(text As String, style As TextStyle) As TextMetrics`

Measure in raster pixels using the same image-surface configuration as Rasterize. No private Graphics ABI or native context is required.

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `text` | `String` | `ByVal` | Required |
| `style` | `TextStyle` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `TextMetrics` |

### Rasterize

`Public Function Rasterize(text As String, style As TextStyle, width As Integer, height As Integer, x As Double, baselineY As Double, red As Double, green As Double, blue As Double, alpha As Double = 1.0) As Picture`

Rasterize through public Picture.FromBuffer, without private runtime access. Size and placement are raster pixels. The returned image has scale 1.

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `text` | `String` | `ByVal` | Required |
| `style` | `TextStyle` | `ByVal` | Required |
| `width` | `Integer` | `ByVal` | Required |
| `height` | `Integer` | `ByVal` | Required |
| `x` | `Double` | `ByVal` | Required |
| `baselineY` | `Double` | `ByVal` | Required |
| `red` | `Double` | `ByVal` | Required |
| `green` | `Double` | `ByVal` | Required |
| `blue` | `Double` | `ByVal` | Required |
| `alpha` | `Double` | `ByVal` | `1.0` |

#### Returns

| Type |
| --- |
| `Picture` |

### Measure

`Public Function Measure(context As Ptr, text As String, style As TextStyle) As TextMetrics`

Measures with the current Cairo transform/surface. Does not retain context.

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `context` | `Ptr` | `ByVal` | Required |
| `text` | `String` | `ByVal` | Required |
| `style` | `TextStyle` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `TextMetrics` |

### Draw

`Public Function Draw(context As Ptr, text As String, x As Double, baselineY As Double, style As TextStyle, red As Double, green As Double, blue As Double, alpha As Double = 1.0) As TextMetrics`

Draw at the first baseline, inheriting transform/clip/operator. RGBA is 0..1. Returns metrics from the very layout drawn; does not retain context.

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `context` | `Ptr` | `ByVal` | Required |
| `text` | `String` | `ByVal` | Required |
| `x` | `Double` | `ByVal` | Required |
| `baselineY` | `Double` | `ByVal` | Required |
| `style` | `TextStyle` | `ByVal` | Required |
| `red` | `Double` | `ByVal` | Required |
| `green` | `Double` | `ByVal` | Required |
| `blue` | `Double` | `ByVal` | Required |
| `alpha` | `Double` | `ByVal` | `1.0` |

#### Returns

| Type |
| --- |
| `TextMetrics` |

### MeasureGraphics0720

`Public Function MeasureGraphics0720(graphics As Graphics, text As String, style As TextStyle) As TextMetrics`

Compatibility adapter for the verified Linux x86_64 0.72.0 runtime only. Reacquires Graphics.NativeHandle each call; never caches its private wrapper.

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `graphics` | `Graphics` | `ByVal` | Required |
| `text` | `String` | `ByVal` | Required |
| `style` | `TextStyle` | `ByVal` | Required |

#### Returns

| Type |
| --- |
| `TextMetrics` |

### DrawGraphics0720

`Public Function DrawGraphics0720(graphics As Graphics, text As String, x As Double, baselineY As Double, style As TextStyle, red As Double, green As Double, blue As Double, alpha As Double = 1.0) As TextMetrics`

#### Parameters

| Name | Type | Passing | Default |
| --- | --- | --- | --- |
| `graphics` | `Graphics` | `ByVal` | Required |
| `text` | `String` | `ByVal` | Required |
| `x` | `Double` | `ByVal` | Required |
| `baselineY` | `Double` | `ByVal` | Required |
| `style` | `TextStyle` | `ByVal` | Required |
| `red` | `Double` | `ByVal` | Required |
| `green` | `Double` | `ByVal` | Required |
| `blue` | `Double` | `ByVal` | Required |
| `alpha` | `Double` | `ByVal` | `1.0` |

#### Returns

| Type |
| --- |
| `TextMetrics` |
