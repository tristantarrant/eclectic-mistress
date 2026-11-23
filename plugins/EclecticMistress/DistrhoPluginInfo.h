/*
 * Eclectic Mistress - Electro-Harmonix Electric Mistress flanger emulation
 * Copyright (C) 2026 Dataforte
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND   "Dataforte"
#define DISTRHO_PLUGIN_NAME    "Eclectic Mistress"
#define DISTRHO_PLUGIN_URI     "http://dataforte.stom-pi/plugins/eclecticmistress"
#define DISTRHO_PLUGIN_CLAP_ID "pi.stom.dataforte.eclecticmistress"

#define DISTRHO_PLUGIN_BRAND_ID  Dfte
#define DISTRHO_PLUGIN_UNIQUE_ID eMis

#define DISTRHO_PLUGIN_HAS_UI          1
#define DISTRHO_PLUGIN_IS_RT_SAFE      1
#define DISTRHO_PLUGIN_NUM_INPUTS      1
#define DISTRHO_PLUGIN_NUM_OUTPUTS     1
#define DISTRHO_PLUGIN_WANT_PROGRAMS   1
// static modgui shipped in modgui/ (rendered from Artwork.hpp), not DPF's
// generated one; this only adds the manifest reference to modgui.ttl
#define DISTRHO_PLUGIN_USES_MODGUI        1
#define DISTRHO_PLUGIN_USES_CUSTOM_MODGUI 1

#define DISTRHO_PLUGIN_LV2_CATEGORY    "lv2:FlangerPlugin"
#define DISTRHO_PLUGIN_VST3_CATEGORIES "Fx|Modulation|Mono"
#define DISTRHO_PLUGIN_CLAP_FEATURES   "audio-effect", "flanger", "mono"

#define DISTRHO_UI_USE_CAIRO       1
#define DISTRHO_UI_USER_RESIZABLE  1
#define DISTRHO_UI_DEFAULT_WIDTH   345
#define DISTRHO_UI_DEFAULT_HEIGHT  440

// Parameter order matches PortIndex in src/eclecticmistress.h (the
// faust-generated DSP binds its controls by that enum); bypass is ours.
enum Parameters {
    kParamRate,
    kParamRange,
    kParamColor,
    kParamMix,
    kParamMatrix,
    kParamBypass,
    kParamCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
