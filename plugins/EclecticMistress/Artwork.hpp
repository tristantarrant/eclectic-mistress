/*
 * Eclectic Mistress - pedal artwork, drawn with cairo.
 *
 * Shared by the scalable plugin UI (UIEclecticMistress.cpp) and by the
 * modgui renderer (tools/render_modgui.cpp), so the desktop GUI and the
 * modgui look the same. Everything is drawn in a fixed design
 * space of W x H units; callers scale the cairo context.
 *
 * The layout recalls the 1976 Electric Mistress V2: warm silver box,
 * three black knurled knobs in thin rings, big split-letter logo, black
 * footswitch field with rounded bottom corners. Names and branding are
 * our own.
 *
 * Copyright (C) 2026 Dataforte
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <cairo.h>
#include <cmath>
#include <cstring>

namespace em_art {

constexpr double W = 345.0;
constexpr double H = 440.0;

// ---- palette ---------------------------------------------------------
struct RGB { double r, g, b; };
constexpr RGB kBodyTop    { 0.835, 0.824, 0.784 };
constexpr RGB kBodyBottom { 0.745, 0.733, 0.690 };
constexpr RGB kInk        { 0.07, 0.07, 0.07 };
constexpr RGB kTeal       { 0.11, 0.61, 0.55 };
constexpr RGB kPanel      { 0.075, 0.075, 0.075 };

// ---- control layout --------------------------------------------------
enum Kind { kKnob, kToggle, kFootswitch };

struct Control {
    int    param;      // DPF parameter index
    Kind   kind;
    double x, y;       // centre
    double r;          // hit radius (knobs, footswitch)
    double w, h;       // hit box (toggles), centred on x, y
    double min, max;
};

// parameter indices (mirrors DistrhoPluginInfo.h)
enum { P_RATE, P_RANGE, P_COLOR, P_MIX, P_MATRIX, P_BYPASS, P_COUNT };

// Positions are snapped so that every modgui element (centre minus half
// its film box) lands on a whole CSS pixel (1.25 design units): browsers
// place backgrounds at fractional offsets with sub-pixel blur otherwise.
// tools/render_modgui.cpp refuses to generate fractional positions.
constexpr double KNOB_Y = 80.0, RING_R = 37.0, KNOB_R = 26.0;
constexpr double KX[3] = { 62.5, 172.5, 282.5 };
constexpr double FOOT_X = 172.5, FOOT_Y = 356.25;
constexpr double MIX_X = 282.5, MIX_Y = 351.25, MIX_R = 14.0;
constexpr double LED_X = 172.5, LED_Y = 313.75;
constexpr double SW_X = 172.5, SW_Y = 17.5;

constexpr Control kControls[] = {
    { P_RATE,   kKnob,       KX[0],  KNOB_Y, RING_R, 0, 0,   0, 10 },
    { P_RANGE,  kKnob,       KX[1],  KNOB_Y, RING_R, 0, 0,   0, 10 },
    { P_COLOR,  kKnob,       KX[2],  KNOB_Y, RING_R, 0, 0,   0, 10 },
    { P_MIX,    kKnob,       MIX_X,  MIX_Y,  20.0,   0, 0,   0, 100 },
    { P_MATRIX, kToggle,     SW_X,   SW_Y,   0,    170, 18,  0, 1 },
    { P_BYPASS, kFootswitch, FOOT_X, FOOT_Y, 24.0,   0, 0,   0, 1 },
};
constexpr int kNumControls = sizeof(kControls) / sizeof(kControls[0]);

struct State {
    float value[P_COUNT] = { 1.0f, 9.2f, 6.0f, 100.0f, 0.0f, 0.0f };
};

// ---- helpers ---------------------------------------------------------
inline void set(cairo_t* cr, const RGB& c, double a = 1.0)
{
    cairo_set_source_rgba(cr, c.r, c.g, c.b, a);
}

inline void rounded_rect(cairo_t* cr, double x, double y, double w, double h,
                         double rtl, double rtr, double rbr, double rbl)
{
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - rtr, y + rtr, rtr, -M_PI / 2, 0);
    cairo_arc(cr, x + w - rbr, y + h - rbr, rbr, 0, M_PI / 2);
    cairo_arc(cr, x + rbl, y + h - rbl, rbl, M_PI / 2, M_PI);
    cairo_arc(cr, x + rtl, y + rtl, rtl, M_PI, 3 * M_PI / 2);
    cairo_close_path(cr);
}

enum Align { kLeft, kCenter, kRight };

// Draw text with its baseline at y. If maxw > 0 the text is squeezed
// horizontally to fit; if fitw > 0 it is stretched/squeezed to exactly fitw.
inline void text(cairo_t* cr, const char* s, double x, double y, double size,
                 const RGB& c, Align align = kCenter, bool bold = true,
                 const char* family = "DejaVu Sans Condensed",
                 double maxw = 0.0, double fitw = 0.0,
                 cairo_font_slant_t slant = CAIRO_FONT_SLANT_NORMAL)
{
    cairo_save(cr);
    cairo_select_font_face(cr, family, slant,
                           bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, size);
    cairo_text_extents_t e;
    cairo_text_extents(cr, s, &e);
    double sx = 1.0;
    if (fitw > 0.0 && e.x_advance > 0.0)
        sx = fitw / e.x_advance;
    else if (maxw > 0.0 && e.x_advance > maxw)
        sx = maxw / e.x_advance;
    const double w = e.x_advance * sx;
    double x0 = x;
    if (align == kCenter) x0 = x - w / 2;
    else if (align == kRight) x0 = x - w;
    cairo_translate(cr, x0, y);
    cairo_scale(cr, sx, 1.0);
    cairo_move_to(cr, 0, 0);
    set(cr, c);
    cairo_show_text(cr, s);
    cairo_restore(cr);
    cairo_new_path(cr);
}

// 0..1 -> angle (radians, cairo convention), 7 o'clock .. 5 o'clock
inline double knob_angle(double norm)
{
    const double deg = -150.0 + 300.0 * norm;      // 0 = 12 o'clock
    return (deg - 90.0) * M_PI / 180.0;
}

inline cairo_pattern_t* body_gradient()
{
    cairo_pattern_t* g = cairo_pattern_create_linear(0, 0, 0, H);
    cairo_pattern_add_color_stop_rgb(g, 0, kBodyTop.r, kBodyTop.g, kBodyTop.b);
    cairo_pattern_add_color_stop_rgb(g, 1, kBodyBottom.r, kBodyBottom.g, kBodyBottom.b);
    return g;
}

// ---- parts -----------------------------------------------------------
inline void screw(cairo_t* cr, double x, double y, double r, bool dark = false)
{
    cairo_new_path(cr);
    cairo_pattern_t* g = cairo_pattern_create_radial(x - r * 0.4, y - r * 0.4, r * 0.1, x, y, r);
    if (dark) {
        cairo_pattern_add_color_stop_rgb(g, 0, 0.55, 0.55, 0.55);
        cairo_pattern_add_color_stop_rgb(g, 1, 0.20, 0.20, 0.20);
    } else {
        cairo_pattern_add_color_stop_rgb(g, 0, 0.92, 0.92, 0.90);
        cairo_pattern_add_color_stop_rgb(g, 1, 0.45, 0.45, 0.43);
    }
    cairo_arc(cr, x, y, r, 0, 2 * M_PI);
    cairo_set_source(cr, g);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(g);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
    cairo_set_line_width(cr, 0.6);
    cairo_stroke(cr);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
    cairo_set_line_width(cr, r * 0.28);
    cairo_move_to(cr, x - r * 0.6, y - r * 0.15); cairo_line_to(cr, x + r * 0.6, y + r * 0.15);
    cairo_move_to(cr, x - r * 0.15, y + r * 0.6); cairo_line_to(cr, x + r * 0.15, y - r * 0.6);
    cairo_stroke(cr);
}

// Black knurled pointer knob (the V2's "curled" knobs), lit from top left.
inline void black_knob(cairo_t* cr, double cx, double cy, double r, double norm)
{
    const double a = knob_angle(norm);
    cairo_new_path(cr);

    // shadow
    cairo_pattern_t* sh = cairo_pattern_create_radial(cx + r * 0.12, cy + r * 0.18, r * 0.8,
                                                      cx + r * 0.12, cy + r * 0.18, r * 1.25);
    cairo_pattern_add_color_stop_rgba(sh, 0, 0, 0, 0, 0.45);
    cairo_pattern_add_color_stop_rgba(sh, 1, 0, 0, 0, 0.0);
    cairo_arc(cr, cx + r * 0.12, cy + r * 0.18, r * 1.25, 0, 2 * M_PI);
    cairo_set_source(cr, sh);
    cairo_fill(cr);
    cairo_pattern_destroy(sh);

    // knurled skirt (teeth rotate with the knob)
    const int teeth = 36;
    cairo_new_path(cr);
    for (int i = 0; i < teeth * 2; ++i) {
        const double t = a + i * M_PI / teeth;
        const double rr = (i % 2) ? r * 0.94 : r;
        if (i == 0) cairo_move_to(cr, cx + rr * cos(t), cy + rr * sin(t));
        else        cairo_line_to(cr, cx + rr * cos(t), cy + rr * sin(t));
    }
    cairo_close_path(cr);
    cairo_pattern_t* sk = cairo_pattern_create_linear(cx - r, cy - r, cx + r, cy + r);
    cairo_pattern_add_color_stop_rgb(sk, 0, 0.30, 0.30, 0.30);
    cairo_pattern_add_color_stop_rgb(sk, 0.5, 0.10, 0.10, 0.10);
    cairo_pattern_add_color_stop_rgb(sk, 1, 0.02, 0.02, 0.02);
    cairo_set_source(cr, sk);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(sk);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.9);
    cairo_set_line_width(cr, r * 0.03);
    cairo_stroke(cr);

    // pointer fin on the skirt
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_rotate(cr, a);
    cairo_move_to(cr, r * 1.06, 0);
    cairo_line_to(cr, r * 0.80, -r * 0.13);
    cairo_line_to(cr, r * 0.80,  r * 0.13);
    cairo_close_path(cr);
    cairo_set_source_rgb(cr, 0.12, 0.12, 0.12);
    cairo_fill(cr);
    cairo_restore(cr);

    // top face
    cairo_pattern_t* f = cairo_pattern_create_radial(cx - r * 0.3, cy - r * 0.35, r * 0.05,
                                                     cx, cy, r * 0.8);
    cairo_pattern_add_color_stop_rgb(f, 0, 0.36, 0.36, 0.36);
    cairo_pattern_add_color_stop_rgb(f, 0.6, 0.13, 0.13, 0.13);
    cairo_pattern_add_color_stop_rgb(f, 1, 0.06, 0.06, 0.06);
    cairo_arc(cr, cx, cy, r * 0.78, 0, 2 * M_PI);
    cairo_set_source(cr, f);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(f);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.8);
    cairo_set_line_width(cr, r * 0.04);
    cairo_stroke(cr);

    // pointer line
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width(cr, r * 0.10);
    cairo_set_source_rgb(cr, 0.86, 0.86, 0.84);
    cairo_move_to(cr, cx + r * 0.22 * cos(a), cy + r * 0.22 * sin(a));
    cairo_line_to(cr, cx + r * 0.70 * cos(a), cy + r * 0.70 * sin(a));
    cairo_stroke(cr);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
}

// Small chrome knob (MIX), for the black field.
inline void chrome_knob(cairo_t* cr, double cx, double cy, double r, double norm)
{
    const double a = knob_angle(norm);
    cairo_new_path(cr);
    cairo_arc(cr, cx + r * 0.1, cy + r * 0.15, r * 1.1, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
    cairo_fill(cr);
    cairo_pattern_t* g = cairo_pattern_create_linear(cx - r, cy - r, cx + r, cy + r);
    cairo_pattern_add_color_stop_rgb(g, 0, 0.95, 0.95, 0.93);
    cairo_pattern_add_color_stop_rgb(g, 0.45, 0.62, 0.62, 0.60);
    cairo_pattern_add_color_stop_rgb(g, 0.55, 0.78, 0.78, 0.76);
    cairo_pattern_add_color_stop_rgb(g, 1, 0.35, 0.35, 0.34);
    cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
    cairo_set_source(cr, g);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(g);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.7);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width(cr, r * 0.16);
    cairo_set_source_rgb(cr, 0.08, 0.08, 0.08);
    cairo_move_to(cr, cx + r * 0.25 * cos(a), cy + r * 0.25 * sin(a));
    cairo_line_to(cr, cx + r * 0.85 * cos(a), cy + r * 0.85 * sin(a));
    cairo_stroke(cr);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
}

// Miniature slide switch, thumb left (0) or right (1).
inline void slide_switch(cairo_t* cr, double cx, double cy, int pos)
{
    const double w = 28, h = 11;
    cairo_new_path(cr);
    rounded_rect(cr, cx - w / 2, cy - h / 2, w, h, 3, 3, 3, 3);
    cairo_set_source_rgb(cr, 0.12, 0.12, 0.12);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.35);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);
    const double tw = 12, th = 9;
    const double tx = pos ? cx + w / 2 - tw - 1 : cx - w / 2 + 1;
    cairo_pattern_t* g = cairo_pattern_create_linear(tx, cy - th / 2, tx, cy + th / 2);
    cairo_pattern_add_color_stop_rgb(g, 0, 0.95, 0.95, 0.93);
    cairo_pattern_add_color_stop_rgb(g, 1, 0.55, 0.55, 0.53);
    rounded_rect(cr, tx, cy - th / 2, tw, th, 2, 2, 2, 2);
    cairo_set_source(cr, g);
    cairo_fill(cr);
    cairo_pattern_destroy(g);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.45);
    cairo_set_line_width(cr, 0.8);
    for (int i = 0; i < 3; ++i) {
        const double lx = tx + 3.5 + i * 2.5;
        cairo_move_to(cr, lx, cy - th / 2 + 2);
        cairo_line_to(cr, lx, cy + th / 2 - 2);
    }
    cairo_stroke(cr);
}

inline void footswitch(cairo_t* cr, double cx, double cy)
{
    // hex nut
    const double rn = 19;
    cairo_new_path(cr);  // (kept)
    for (int i = 0; i < 6; ++i) {
        const double t = M_PI / 6 + i * M_PI / 3;
        if (i == 0) cairo_move_to(cr, cx + rn * cos(t), cy + rn * sin(t));
        else        cairo_line_to(cr, cx + rn * cos(t), cy + rn * sin(t));
    }
    cairo_close_path(cr);
    cairo_pattern_t* g = cairo_pattern_create_linear(cx - rn, cy - rn, cx + rn, cy + rn);
    cairo_pattern_add_color_stop_rgb(g, 0, 0.92, 0.92, 0.90);
    cairo_pattern_add_color_stop_rgb(g, 1, 0.38, 0.38, 0.37);
    cairo_set_source(cr, g);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(g);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
    cairo_set_line_width(cr, 0.8);
    cairo_stroke(cr);
    // threaded collar
    cairo_arc(cr, cx, cy, 13.5, 0, 2 * M_PI);
    cairo_set_source_rgb(cr, 0.55, 0.55, 0.54);
    cairo_fill(cr);
    // plunger
    cairo_pattern_t* p = cairo_pattern_create_radial(cx - 3, cy - 4, 1, cx, cy, 10);
    cairo_pattern_add_color_stop_rgb(p, 0, 1.0, 1.0, 1.0);
    cairo_pattern_add_color_stop_rgb(p, 0.5, 0.80, 0.80, 0.79);
    cairo_pattern_add_color_stop_rgb(p, 1, 0.45, 0.45, 0.44);
    cairo_arc(cr, cx, cy, 9.5, 0, 2 * M_PI);
    cairo_set_source(cr, p);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(p);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
    cairo_stroke(cr);
}

inline void led(cairo_t* cr, double cx, double cy, bool lit)
{
    cairo_new_path(cr);
    cairo_arc(cr, cx, cy, 6.0, 0, 2 * M_PI);
    cairo_set_source_rgb(cr, 0.25, 0.25, 0.25);
    cairo_fill(cr);
    if (lit) {
        cairo_pattern_t* glow = cairo_pattern_create_radial(cx, cy, 2, cx, cy, 14);
        cairo_pattern_add_color_stop_rgba(glow, 0, 1.0, 0.15, 0.1, 0.55);
        cairo_pattern_add_color_stop_rgba(glow, 1, 1.0, 0.15, 0.1, 0.0);
        cairo_arc(cr, cx, cy, 14, 0, 2 * M_PI);
        cairo_set_source(cr, glow);
        cairo_fill(cr);
    }
    cairo_pattern_t* g = cairo_pattern_create_radial(cx - 1.3, cy - 1.5, 0.5, cx, cy, 4.5);
    if (lit) {
        cairo_pattern_add_color_stop_rgb(g, 0, 1.0, 0.85, 0.80);
        cairo_pattern_add_color_stop_rgb(g, 0.4, 1.0, 0.18, 0.10);
        cairo_pattern_add_color_stop_rgb(g, 1, 0.65, 0.03, 0.02);
    } else {
        cairo_pattern_add_color_stop_rgb(g, 0, 0.55, 0.25, 0.22);
        cairo_pattern_add_color_stop_rgb(g, 1, 0.22, 0.03, 0.02);
    }
    cairo_arc(cr, cx, cy, 4.5, 0, 2 * M_PI);
    cairo_set_source(cr, g);
    cairo_fill(cr);
    cairo_pattern_destroy(g);
}

// Two-line logo in split, stencil-like letters (body-coloured cuts
// through the strokes recall the fragmented 1970s lettering).
inline void logo(cairo_t* cr)
{
    const char* lines[2] = { "eclectic", "mistress" };
    const double base[2] = { 200.0, 256.0 };
    const double width = 300.0, size = 72.0;
    for (int l = 0; l < 2; ++l) {
        cairo_save(cr);
        cairo_select_font_face(cr, "DejaVu Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, size);
        cairo_text_extents_t e;
        cairo_text_extents(cr, lines[l], &e);
        const double sx = width / e.width;
        const double sy = 0.86;
        cairo_translate(cr, W / 2 - (e.x_bearing + e.width / 2) * sx, base[l]);
        cairo_scale(cr, sx, sy);
        cairo_move_to(cr, 0, 0);
        cairo_text_path(cr, lines[l]);
        cairo_restore(cr);
        set(cr, kTeal);
        cairo_fill(cr);

        // horizontal cuts across the x-height, in the body gradient so
        // they only show where they cross a letter
        cairo_pattern_t* bg = body_gradient();
        cairo_set_source(cr, bg);
        const double xh = 38.0;
        const double cuts[2] = { base[l] - xh * 0.36, base[l] - xh * 0.72 };
        for (double cy : cuts) {
            cairo_rectangle(cr, 20, cy - 1.3, W - 40, 2.6);
            cairo_fill(cr);
        }
        // a few diagonal nicks for the worn, fragmented look
        cairo_set_line_width(cr, 2.2);
        for (int k = 0; k < 7; ++k) {
            const double x = 40 + k * 44 + (l ? 18 : 0);
            cairo_move_to(cr, x, base[l] - 2);
            cairo_line_to(cr, x + 9, base[l] - xh * 0.36 + 1);
        }
        cairo_stroke(cr);
        cairo_pattern_destroy(bg);
    }
}

// ---- layers ----------------------------------------------------------
enum Layer {
    kLayerBody     = 1 << 0,   // box, print, rings, panel, screws, footswitch
    kLayerKnobs    = 1 << 1,   // RATE / RANGE / COLOR
    kLayerMix      = 1 << 2,
    kLayerSwitch   = 1 << 3,   // FLANGE / FILTER MATRIX slide switch
    kLayerLed      = 1 << 4,
    kLayerAll      = 0xff
};

inline double norm(const State& s, int p)
{
    for (const Control& c : kControls)
        if (c.param == p)
            return (s.value[p] - c.min) / (c.max - c.min);
    return 0.0;
}

inline void draw(cairo_t* cr, const State& s, unsigned layers = kLayerAll)
{
    if (layers & kLayerBody) {
        // box
        cairo_new_path(cr);
        cairo_pattern_t* g = body_gradient();
        rounded_rect(cr, 1, 1, W - 2, H - 2, 9, 9, 9, 9);
        cairo_set_source(cr, g);
        cairo_fill_preserve(cr);
        cairo_pattern_destroy(g);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.55);
        cairo_set_line_width(cr, 1.5);
        cairo_stroke(cr);
        rounded_rect(cr, 3, 3, W - 6, H - 6, 7, 7, 7, 7);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.45);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        // jack / switch legends along the top edge
        text(cr, "OUT", 50, 21, 9.5, kInk);
        text(cr, "IN", 295, 21, 9.5, kInk);
        text(cr, "FLANGE", SW_X - 20, 21, 9.5, kInk, kRight);
        text(cr, "FILTER MATRIX", SW_X + 20, 21, 9.5, kInk, kLeft);

        // knob rings and legends
        const char* names[3] = { "RATE", "RANGE", "COLOR" };
        for (int i = 0; i < 3; ++i) {
            cairo_new_path(cr);
            cairo_arc(cr, KX[i], KNOB_Y, RING_R, 0, 2 * M_PI);
            set(cr, kInk);
            cairo_set_line_width(cr, 1.6);
            cairo_stroke(cr);
            text(cr, names[i], KX[i], KNOB_Y + RING_R + 14, 10.5, kInk);
        }

        screw(cr, 21, 146, 4.5);
        screw(cr, W - 21, 146, 4.5);

        logo(cr);
        text(cr, "FLANGER/FILTER MATRIX", W / 2, 288, 21, kInk, kCenter, true,
             "DejaVu Sans Condensed", 0.0, 282.0);

        // black field with rounded bottom corners
        cairo_new_path(cr);
        rounded_rect(cr, 19, 298, W - 38, 112, 2, 2, 26, 26);
        cairo_pattern_t* pg = cairo_pattern_create_linear(0, 298, 0, 410);
        cairo_pattern_add_color_stop_rgb(pg, 0, 0.10, 0.10, 0.10);
        cairo_pattern_add_color_stop_rgb(pg, 1, kPanel.r, kPanel.g, kPanel.b);
        cairo_set_source(cr, pg);
        cairo_fill(cr);
        cairo_pattern_destroy(pg);

        screw(cr, 32, 322, 4.0, true);
        screw(cr, W - 32, 322, 4.0, true);

        footswitch(cr, FOOT_X, FOOT_Y);
        text(cr, "MIX", MIX_X, MIX_Y + MIX_R + 15, 9.0, { 0.75, 0.75, 0.72 });

        // maker line
        text(cr, "dataforte", W / 2, 431, 21, kInk, kCenter, false, "DejaVu Serif");
    }

    if (layers & kLayerKnobs)
        for (int i = 0; i < 3; ++i)
            black_knob(cr, KX[i], KNOB_Y, KNOB_R, norm(s, P_RATE + i));

    if (layers & kLayerMix)
        chrome_knob(cr, MIX_X, MIX_Y, MIX_R, norm(s, P_MIX));

    if (layers & kLayerSwitch)
        slide_switch(cr, SW_X, SW_Y, s.value[P_MATRIX] > 0.5f ? 1 : 0);

    if (layers & kLayerLed)
        led(cr, LED_X, LED_Y, s.value[P_BYPASS] < 0.5f);
}

} // namespace em_art
