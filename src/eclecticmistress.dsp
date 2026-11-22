declare id "eclecticmistress";
declare name "Eclectic Mistress";
declare category "Modulation";
declare author "Dataforte";
declare license "GPL-2.0-or-later";
declare description "Emulation of the Electro-Harmonix Electric Mistress flanger: a bucket brigade delay (BBD) whose delay time is swept by an LFO, mixed with the dry signal to form a swept comb filter with resonant feedback (the COLOR pot). Reference: the 1976 Electric Mistress V2 circuit (buffer, highpass, gain stage, BBD, feedback trim + COLOR pot, summing and response-restoration filtering).";

import("stdfaust.lib");

// ====================================================================
//  Constants
// ====================================================================

// Bucket brigade delay limits of the Electric Mistress (SAD1024 run as
// 2x512 stages; delay = stages / (2*fclock)). Measured sweep range
// of the LFO-driven delay: ~0.5 ms .. ~5 ms.
D_MIN = 0.0005;      // s
D_MAX = 0.005;       // s

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

// LFO sweep speed. 0 Hz freezes the delay ("Filter Matrix" mode of
// the pedal). Slowest measured sweep: 0.067 Hz; fastest ~3 Hz.
rate  = hslider("rate[name:RATE][unit:Hz]", 0.9, 0.0, 3.0, 0.01) : si.smooth(0.999);

// LFO sweep width as percentage of the full BBD delay range
// (0.5 ms .. 5 ms). 0 = fixed delay at the centre of the range.
range = hslider("range[name:RANGE]", 30.0, 0.0, 100.0, 1.0) : si.smooth(0.999);

// Feedback amount and polarity, in percentage. The pedal's COLOR pot
// is wired across the rails so its wiper can swing the feedback signal
// from fully positive (reinforces comb peaks) through zero (centre
// detent) to fully inverted (emphasizes comb notches).
color = hslider("color[name:COLOR]", 30.0, -100.0, 100.0, 1.0) : si.smooth(0.999);

// Wet/dry mix (added convenience; the pedal has no mix control).
mix   = hslider("mix[name:MIX]", 100.0, 0.0, 100.0, 1.0) : si.smooth(0.999);

// ====================================================================
//  LFO
// ====================================================================
// The Electric Mistress LFO is a two-comparator flip-flop ramp that
// charges and discharges a capacitor, i.e. a triangle wave. Phase is
// accumulated so that rate changes glide instead of jumping, and a
// rate of 0 Hz stops the sweep (Filter Matrix mode).
ph = phnode ~ _ with {
    phnode(p) = p + rate/ma.SR;
};

tri = 1 - abs(2*ma.decimal(ph) - 1);

// ====================================================================
//  Delay time
// ====================================================================
// The VCO converts the LFO voltage to a BBD clock and the delay time
// is proportional to the LFO voltage (delay = 256/fclock[kHz] ms with
// fclock inversely proportional to the LFO voltage). We therefore map
// the LFO linearly onto the delay range: RANGE scales the excursion
// around the centre of the BBD range.
d_s = (D_MIN + D_MAX)/2*ma.SR
      + range/100 * (D_MAX - D_MIN)/2*ma.SR * (2*tri - 1);

// ====================================================================
//  Feedback (COLOR pot)
// ====================================================================
// |fbg| < 1 keeps the recursive comb stable; the pedal's feedback trim
// limits the excursion well below the rails.
fbg = color/100 : min(0.99) : max(-0.99);

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
    dmax  = int(D_MAX*ma.SR) + 2;
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
    fdlen = int(D_MAX*ma.SR) + 1;
};

// ====================================================================
//  Process
// ====================================================================
process = _ : pdm with {
    pdm(x) = (x : *(1-m)) + (x : fx : *(m));
    m = mix/100;
};
