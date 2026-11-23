declare id "eclecticmistress";
declare name "Eclectic Mistress";
declare category "Modulation";
declare author "Dataforte";
declare license "GPL-2.0-or-later";
declare description "Emulation of the Electro-Harmonix Electric Mistress flanger: a bucket brigade delay (BBD) whose delay time is swept by an LFO, mixed with the dry signal to form a swept comb filter with resonant feedback (the COLOR pot). Reference: the 1976 Electric Mistress V2 circuit (buffer, highpass, gain stage, BBD, feedback trim + COLOR pot, summing and response-restoration filtering). Controls are pot positions (0..10) as on the pedal; FLANGE/FILTER MATRIX switch.";

import("stdfaust.lib");

// ====================================================================
//  Constants
// ====================================================================

// Measurements of a 1977 Electric Mistress V2 (R. Metzger, "The
// Mistress Mystery Page", measure.shtml / how.shtml):
//  - the LFO is a triangle between a fixed 0.4-0.5 V floor (comparator
//    threshold) and a ceiling set by RANGE, 0.5 V .. 10.5 V at full RANGE;
//  - the VCO maps the LFO voltage linearly onto the BBD delay,
//    0.5 V -> 1.2 ms, ~10 V -> 10.7 ms.
// So the sweep starts at a fixed shortest delay and RANGE stretches it
// towards longer delays (EHX manual: RANGE "sets the lower limit of
// the sweep"; fully counter-clockwise the limits nearly coincide).
D_LO   = 0.0012;     // s, shortest delay (LFO floor)
D_HI   = 0.0107;     // s, longest delay (full RANGE / Filter Matrix)
W_MIN  = 0.05;       // sweep width fraction with RANGE fully CCW

// LFO speed: slowest measured sweep 0.067 Hz (V2). The fastest is not
// measured on the V2 (Deluxe: ~10 Hz); 8 Hz puts the EHX manual's
// rotating-speaker recipe (RATE near max, ~97%) at ~6.9 Hz, the speed
// of a Leslie's fast rotor.
F_MIN  = 0.067;      // Hz
F_MAX  = 8.0;        // Hz

// Maximum feedback with COLOR fully clockwise, as limited by the
// feedback trimpot: ~0.5 (comb measurements at max COLOR; Axe-Fx
// matching of a real unit gave ~50%).
G_MAX  = 0.5;

// The input highpass removes low frequencies to protect the BBD from
// aliasing (per the circuit description; the lows are partly restored
// by the output network). Corner chosen from the measured V2 response.
F_HP = 300.0;        // Hz

// The output network "regains a linear frequency response in the
// audible range"; the measured response rolls off gently above ~5 kHz.
F_LP = 6000.0;       // Hz

// ====================================================================
//  Controls
// ====================================================================

// RATE, RANGE and COLOR are pot positions, 0..10 = fraction of the
// pot's travel clockwise, as on the pedal. Defaults: the Gilmour
// "10 o'clock" setting (Wall tour), see bundle presets.

// RATE: 1M reverse-log pot in the LFO timing -> exponential speed law.
rate  = hslider("rate[name:RATE]", 1.0, 0.0, 10.0, 0.1) : si.smooth(0.999);

// RANGE: 100k linear pot on the LFO's upper threshold. In Filter
// Matrix mode it sets the static delay instead.
range = hslider("range[name:RANGE]", 9.2, 0.0, 10.0, 0.1) : si.smooth(0.999);

// COLOR: 10k linear pot dividing the BBD output fed back to the
// non-inverting input of the gain stage: positive feedback, 0..G_MAX.
color = hslider("color[name:COLOR]", 6.0, 0.0, 10.0, 0.1) : si.smooth(0.999);

// Wet/dry mix (added convenience; the pedal has no mix control).
mix   = hslider("mix[name:MIX]", 100.0, 0.0, 100.0, 1.0) : si.smooth(0.999);

// FLANGE / FILTER MATRIX switch: 1 stops the LFO; RANGE then tunes a
// fixed comb ("chimes" in the EHX manual).
matrix = checkbox("matrix[name:MATRIX]");

f_lfo = F_MIN * pow(F_MAX/F_MIN, rate/10);
w     = W_MIN + (1 - W_MIN)*range/10;
fbg   = G_MAX * color/10;

// ====================================================================
//  LFO
// ====================================================================
// The Electric Mistress LFO is a two-comparator flip-flop ramp that
// charges and discharges a capacitor, i.e. a triangle wave (measured).
// Phase is accumulated (and wrapped) so that rate changes glide.
ph = phnode ~ _ with {
    phnode(p) = ma.decimal(p + f_lfo/ma.SR);
};

tri = 1 - abs(2*ph - 1);

// ====================================================================
//  Delay time
// ====================================================================
// Delay is proportional to the LFO voltage (linear VCO). Flange mode:
// the triangle sweeps from D_LO up to D_LO + w*(D_HI - D_LO). Filter
// Matrix: RANGE sets the delay directly over D_LO .. D_HI.
pos = select2(matrix, tri*w, range/10);
d_s = (D_LO + pos*(D_HI - D_LO))*ma.SR;

// ====================================================================
//  Resonant comb filter
// ====================================================================
// In the circuit the delayed signal is fed back through the COLOR pot
// to the input of the gain stage, so the comb is formed at the input
// of the delay:
//
//     v[n] = x[n] + fbg * v[n-d]
//
// The BBD delay is inherently discrete (whole stages), so the feedback
// path uses an integer delay; the dry-plus-delayed sum below uses a
// fractional delay for a smooth sweep.
comb(x) = fbnode ~ _ with {
    fbnode(p) = x + (p : (de.delay(dmax, d_int - 1) : *(fbg)));
    dmax  = int(D_HI*ma.SR) + 2;
    d_int = int(round(d_s));
};

// ====================================================================
//  Effect body
// ====================================================================
// Input highpass -> comb -> dry + delayed sum (fractional delay) ->
// response-restoration lowpass -> normalize the 2x dry+delayed sum
// and gently saturate the gain stage (the vintage circuit clips the
// comb at high feedback; the mild tanh knee also gives the classic
// EM "volume drop" character).
fx(x) = x : fi.highpass(2,F_HP) : comb :
         (_ <: _, (_ : de.fdelay(fdlen, d_s)) : +) :
         fi.lowpass(2,F_LP) : *(0.5) : ma.tanh with {
    fdlen = int(D_HI*ma.SR) + 1;
};

// ====================================================================
//  Process
// ====================================================================
process = _ : pdm with {
    pdm(x) = (x : *(1-m)) + (x : fx : *(m));
    m = mix/100;
};
