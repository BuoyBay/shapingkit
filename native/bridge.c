/* Linux-only opt-in Pango/Cairo layout. Never retains a caller's cairo_t. */
#define _GNU_SOURCE
#include <pango/pangocairo.h>
#include <dlfcn.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

typedef struct {
    double ink_x, ink_y, ink_width, ink_height;
    double logical_x, logical_y, logical_width, logical_height;
    double baseline;
    int64_t unknown_glyphs;
} BskMetrics;

/* The installed compiler exposes Buoy's wrapper, contrary to its docs. Keep
 * this compatibility path separate from the cairo_t API. This exact runtime
 * was disassembled and exercised; never guess another build's private ABI.
 * The caller must supply a live Graphics.NativeHandle from Buoy 0.72.0.
 * Checking a binary hash does not make arbitrary caller addresses safe. */
int64_t bsk_buoy0720_adapter_supported(void) {
    static gsize checked = 0;
    if (g_once_init_enter(&checked)) {
        int matched = 0;
        Dl_info info;
        void *symbol = dlsym(RTLD_DEFAULT, "bui_ui_graphics_new_for_picture");
        if (symbol && dladdr(symbol, &info) && info.dli_fname) {
            char *data = NULL;
            gsize length = 0;
            if (g_file_get_contents(info.dli_fname, &data, &length, NULL)) {
                char *hash = g_compute_checksum_for_data(G_CHECKSUM_SHA256, (const guchar *)data, length);
                matched = !strcmp(hash, "44ccfebe300450f789e006d3bf7e90c5f95abe9e252842d1c327957f7db64c50");
                g_free(hash);
                g_free(data);
            }
        }
        g_once_init_leave(&checked, matched ? 1 : 2);
    }
    return checked == 1;
}

void *bsk_buoy0720_context(void *graphics) {
    if (!graphics || !bsk_buoy0720_adapter_supported()) return NULL;
    cairo_t *cr = *(cairo_t **)graphics;
    if (!cr || cairo_status(cr) != CAIRO_STATUS_SUCCESS) return NULL;
    return cr;
}

/* Status: 0 success, 1 invalid argument, 2 invalid Cairo context/surface,
 * 3 allocation/render failure. Invalid non-NULL/dangling pointers are outside
 * the C ABI contract: no portable API can validate an arbitrary address. */
int64_t bsk_layout(cairo_t *cr, const char *text, const char *family,
                   double size, int64_t flags, int64_t draw,
                   double x, double baseline_y, double r, double g,
                   double b, double a, BskMetrics *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!out || !text || !family || !*family ||
        !g_utf8_validate(text, -1, NULL) || !g_utf8_validate(family, -1, NULL) ||
        strlen(text) > 1048576 || strlen(family) > 4096 ||
        !isfinite(size) || size <= 0 || size > 16384 ||
        flags < 0 || flags > 7 || (draw != 0 && draw != 1) ||
        !isfinite(x) || !isfinite(baseline_y) ||
        !isfinite(r) || !isfinite(g) || !isfinite(b) || !isfinite(a) ||
        r < 0 || r > 1 || g < 0 || g > 1 || b < 0 || b > 1 || a < 0 || a > 1)
        return 1;
    if (!cr || cairo_status(cr) != CAIRO_STATUS_SUCCESS ||
        cairo_surface_status(cairo_get_target(cr)) != CAIRO_STATUS_SUCCESS)
        return 2;
    /* Pango extents/advances use signed 32-bit Pango units. Bound the
     * aggregate scalar-size product before shaping, conservatively below its
     * ~2-million-user-unit range. This also bounds multiline total height. */
    if ((double)g_utf8_strlen(text, -1) * size > 262144.0) return 1;
    PangoLayout *layout = pango_cairo_create_layout(cr);
    if (!layout) return 3;
    PangoFontDescription *font = pango_font_description_new();
    pango_font_description_set_family(font, family);
    /* Absolute size avoids implicit 96/72 DPI conversion; size is in Cairo
     * user units. The layout context incorporates the caller's CTM. */
    pango_font_description_set_absolute_size(font, size * PANGO_SCALE);
    pango_font_description_set_weight(font, (flags & 1) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
    pango_font_description_set_style(font, (flags & 2) ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
    pango_layout_set_font_description(layout, font);
    pango_font_description_free(font);
    pango_layout_set_text(layout, text, -1); /* plain UTF-8, never markup */
    pango_layout_set_auto_dir(layout, TRUE);
    if (flags & 4) {
        PangoAttrList *attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs, pango_attr_underline_new(PANGO_UNDERLINE_SINGLE));
        pango_layout_set_attributes(layout, attrs);
        pango_attr_list_unref(attrs);
    }
    PangoRectangle ink, logical;
    pango_layout_get_extents(layout, &ink, &logical);
    if (ink.width < 0 || ink.height < 0 || logical.width < 0 || logical.height < 0) {
        g_object_unref(layout);
        return 1;
    }
    double baseline = pango_layout_get_baseline(layout) / (double)PANGO_SCALE;
    *out = (BskMetrics){ink.x / (double)PANGO_SCALE, ink.y / (double)PANGO_SCALE - baseline,
        ink.width / (double)PANGO_SCALE, ink.height / (double)PANGO_SCALE,
        logical.x / (double)PANGO_SCALE, logical.y / (double)PANGO_SCALE - baseline,
        logical.width / (double)PANGO_SCALE, logical.height / (double)PANGO_SCALE,
        baseline, pango_layout_get_unknown_glyphs_count(layout)};
    int64_t status = 0;
    if (draw) {
        /* Cairo save/restore does not preserve the current path. Preserve it
         * explicitly, including its current point, for Graphics path callers. */
        cairo_path_t *path = cairo_copy_path(cr);
        if (path->status != CAIRO_STATUS_SUCCESS) {
            cairo_path_destroy(path);
            g_object_unref(layout);
            return 3;
        }
        cairo_save(cr);
        cairo_new_path(cr);
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_move_to(cr, x, baseline_y - baseline);
        pango_cairo_show_layout(cr, layout);
        cairo_restore(cr);
        cairo_new_path(cr);
        cairo_append_path(cr, path);
        cairo_path_destroy(path);
        if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) status = 3;
    }
    g_object_unref(layout);
    return status;
}

/* Match Rasterize's owned image-surface layout without a Buoy private handle.
 * Surface dimensions do not constrain the unwrapped Pango layout. */
int64_t bsk_measure_raster(const char *text, const char *family, double size,
                           int64_t flags, BskMetrics *out) {
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t *cr = cairo_create(surface);
    int64_t status = bsk_layout(cr, text, family, size, flags, 0,
                                0, 0, 0, 0, 0, 1, out);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return status;
}

/* Public Picture.FromBuffer integration: an owned surface, never Buoy internals.
 * Output is tightly packed straight-alpha RGBA, copied by Picture.FromBuffer.
 * Bounded to 64 MiB so callers cannot accidentally request unbounded raster work. */
int64_t bsk_raster(const char *text, const char *family, double size, int64_t flags,
                   int64_t width, int64_t height, double x, double baseline_y,
                   double r, double g, double b, double a, unsigned char *rgba,
                   int64_t capacity) {
    if (!rgba || width <= 0 || height <= 0 || width > 16384 || height > 16384 ||
        width * height > 16777216 || capacity < width * height * 4) return 1;
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)width, (int)height);
    cairo_t *cr = cairo_create(surface);
    BskMetrics metrics;
    int64_t status = bsk_layout(cr, text, family, size, flags, 1, x, baseline_y, r, g, b, a, &metrics);
    if (!status) {
        cairo_surface_flush(surface);
        const unsigned char *data = cairo_image_surface_get_data(surface);
        int stride = cairo_image_surface_get_stride(surface);
        for (int64_t y = 0; y < height; ++y) {
            for (int64_t col = 0; col < width; ++col) {
                uint32_t pixel;
                memcpy(&pixel, data + y * stride + col * 4, 4);
                unsigned int alpha = pixel >> 24;
                unsigned char *out = rgba + (y * width + col) * 4;
                out[0] = alpha ? (unsigned char)((((pixel >> 16) & 255) * 255 + alpha / 2) / alpha) : 0;
                out[1] = alpha ? (unsigned char)((((pixel >> 8) & 255) * 255 + alpha / 2) / alpha) : 0;
                out[2] = alpha ? (unsigned char)(((pixel & 255) * 255 + alpha / 2) / alpha) : 0;
                out[3] = (unsigned char)alpha;
            }
        }
    }
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return status;
}
