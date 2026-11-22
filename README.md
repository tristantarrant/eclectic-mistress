# Eclectic Mistress

Faust-based LV2 emulation of the **Electro-Harmonix Electric Mistress**
flanger.

The Electric Mistress is a bucket brigade delay (BBD) whose delay time is
swept by an LFO and mixed with the dry signal to form a swept comb filter.
The delayed signal is fed back through the COLOR pot to the input of the
gain stage, giving the iconic resonant "sucked-in" flanging sound.
(Gilmour on *Wish You Were Here*; Summers on *Do Do Do*; the Leslie
simulation setting and friends.)

## Signal path

Modeled on the 1976 Electric Mistress V2 circuit (see
[Ralf Metzger's Mistress Mystery Page](http://www.metzgerralf.de/elekt/stomp/mistress/index.shtml)):

```
in ──► HPF (300 Hz) ──► resonant comb ──► dry + delayed ──► LPF (6 kHz)
                                        ▲                └──► *(0.5) ──► tanh
                                        └── feedback (COLOR, ±polarity)
out = (1 - MIX)·in + MIX·wet
```

- **Resonant comb** — `v[n] = x[n] + fbg·v[n-d]`, the feedback entering the
  input of the delay exactly as in the circuit (COLOR pot wired across the
  rails, so its wiper can swing the feedback from fully positive through
  zero to fully inverted). The BBD delay is inherently discrete, so the
  feedback path uses an integer delay; the dry+delayed sum uses a fractional
  delay for a smooth sweep.
- **LFO** — triangle wave (the original is a two-comparator flip-flop ramp
  that charges and discharges a capacitor), rate 0–3 Hz (slowest measured
  sweep: 0.067 Hz). Phase is accumulated so rate changes glide; **rate = 0
  freezes the delay**, which is the pedal's "Filter Matrix" mode.
- **Delay range** — 0.5 ms to 5 ms, the measured BBD sweep range of the
  original (SAD1024 run as 2×512 stages). RANGE scales the LFO excursion
  around the centre of that range.
- **Filters** — the input highpass protects the BBD from aliasing (and gives
  the vintage top-end thinning); the output lowpass is the circuit's
  response-restoration network. The `*(0.5) : tanh` stage normalizes the
  2× dry+delayed sum and mildly saturates the gain stage, reproducing the
  classic EM "volume drop" character.

## Controls

| Port | Symbol  | Range     | Default | Meaning                                    |
|------|---------|-----------|---------|--------------------------------------------|
| 0    | `rate`  | 0–3 Hz    | 0.9     | LFO sweep speed (0 = Filter Matrix mode)   |
| 1    | `range` | 0–100 %   | 30      | Sweep width over the full BBD range        |
| 2    | `color` | −100–100 %| 30      | Feedback amount and polarity               |
| 3    | `mix`   | 0–100 %   | 100     | Wet/dry mix (added convenience)            |

The defaults approximate the famous "10 o'clock" setting.

## Building

Requires a C++ compiler; no other dependencies (the LV2 API is header-only).

```
make            # build the .so into eclecticmistress.lv2/
make test       # build and run the LV2 test harness
make install    # copy the bundle to /usr/lib/lv2 (override INSTALL_DIR)
make dsp        # regenerate the faust-generated C++ from the .dsp source
                # (needs the faust compiler + the guitarix dsp2cc tool)
```

The DSP is written in [Faust](https://faust.grame.fr) (`src/eclecticmistress.dsp`)
in the same spirit as Guitarix's plugins; the checked-in `src/eclecticmistress.cc`
was generated from it with the guitarix `dsp2cc` tool
(`--init-type=plugin-lv2`), and the LV2 wrapper (`src/eclecticmistress.cpp`)
follows the Guitarix per-plugin wrapper pattern.

## Layout

```
src/eclecticmistress.dsp     Faust DSP source (authoritative)
src/eclecticmistress.cc      generated DSP class (regenerate with 'make dsp')
src/eclecticmistress.h       PortIndex enum + plugin URI
src/eclecticmistress.cpp     LV2 wrapper (instantiate/activate/run/...)
src/em_glue.h                minimal glue header for the generated class
eclecticmistress.lv2/        the LV2 bundle (manifest.ttl, plugin TTL, .so)
tests/em_test.c              LV2 test harness
```

## License

GPL-2.0-or-later (see LICENSE).
