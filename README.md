# Eclectic Mistress

Faust-based LV2 emulation of the **Electro-Harmonix Electric Mistress**
flanger.

The Electric Mistress is a bucket brigade delay (BBD) whose delay time is
swept by an LFO and mixed with the dry signal to form a swept comb filter.
The delayed signal is fed back through the COLOR pot to the input of the
gain stage, giving the iconic resonant "sucked-in" flanging sound.
It defined David Gilmour's tones from 1977 to 1983 (Animals tour, his 1978
solo album, The Wall tour, The Final Cut) and Andy Summers' sound with The
Police.

## Signal path

Modeled on the 1976 Electric Mistress V2 circuit and on measurements of a
real 1977 V2, both from
[Ralf Metzger's Mistress Mystery Page](http://www.metzgerralf.de/elekt/stomp/mistress/index.shtml):

```
in ──► HPF (300 Hz) ──► resonant comb ──► dry + delayed ──► LPF (6 kHz)
                                        ▲                └──► *(0.5) ──► tanh
                                        └── feedback (COLOR, 0 .. 0.5)
out = (1 - MIX)·in + MIX·wet
```

- **Resonant comb**: `v[n] = x[n] + g·v[n-d]`. As in the circuit, the
  feedback enters the gain stage in front of the BBD. COLOR is a 10k linear
  pot dividing the BBD output, which is fed back to the non-inverting input
  of the gain stage, so it is positive feedback from 0 up to g = 0.5. That
  maximum is set by the internal feedback trimpot, estimated from the
  measured comb response at full COLOR and matching a published Axe-Fx
  match of a real unit (~50%). The BBD is inherently discrete, so the
  feedback path uses an integer delay; the dry + delayed sum uses a
  fractional delay for a smooth sweep.
- **LFO**: a triangle wave, as measured. The original is a two-comparator
  flip-flop ramp. RATE is a 1M reverse-log pot, modelled as an exponential
  law from 0.067 Hz (the measured slowest sweep) to 8 Hz. The V2's top
  speed is not measured; 8 Hz is a judgement call (see Controls).
- **Delay**: the LFO ramps between a fixed floor and a ceiling set by
  RANGE, and the VCO maps voltage linearly onto delay (measured: 0.5 V →
  1.2 ms, 10 V → 10.7 ms). So the sweep always starts at **1.2 ms**, and
  RANGE stretches its long end up to **10.7 ms**. Fully counter-clockwise,
  the two ends nearly meet (EHX manual: RANGE "sets the lower limit of the
  sweep").
- **Filter Matrix**: the pedal's FLANGE / FILTER MATRIX switch stops the
  LFO, and RANGE then sets a static delay over 1.2–10.7 ms (the measured
  VCO curve).
- **Filters**: the input highpass protects the BBD from aliasing (and gives
  the vintage top-end thinning); the output lowpass is the circuit's
  response-restoration network. The `*(0.5) : tanh` stage normalizes the
  2× dry + delayed sum (the pedal mixes 50/50) and mildly saturates the gain
  stage, reproducing the classic EM "volume drop" character.

## Controls

RATE, RANGE and COLOR are **pot positions**: 0–10 is the fraction of the
pot's clockwise travel, as on the pedal. Published settings for a real
Mistress can therefore be dialled in directly. Note that the vintage 18 V
pedals have their pots mounted at different angles, so translate clock
positions into fraction of travel first (see the presets).

| Port | Symbol   | Range   | Default | Meaning |
|------|----------|---------|---------|---------|
| 0    | `rate`   | 0–10    | 1.0     | LFO speed, 0.067 Hz → 8 Hz (exponential) |
| 1    | `range`  | 0–10    | 9.2     | sweep 1.2 ms → 1.2…10.7 ms; static delay in Filter Matrix mode |
| 2    | `color`  | 0–10    | 6.0     | positive feedback 0 → 0.5 |
| 3    | `mix`    | 0–100 % | 100     | wet/dry mix (not on the pedal) |
| 4    | `matrix` | toggle  | off     | FLANGE / FILTER MATRIX switch |

The defaults are Gilmour's "10 o'clock" setting from The Wall tour.

The 8 Hz top speed is an estimate. The Deluxe Electric Mistress measures
up to ~10 Hz, and with 8 Hz the EHX manual's rotating-speaker recipe
("rate near maximum", 97% in Metzger's diagrams) lands at 6.6 Hz, the
speed of a Leslie's fast rotor.

## Presets

Documented settings, read from Metzger's setting diagrams for the V2 and
the V5. The two versions mount their pots differently, but converted to
fraction of travel they agree within 5%; the presets use the average.
Song credits come from [gilmourish.com](https://www.gilmourish.com/?page_id=80)
(Gilmour) and Metzger (Summers).

| Preset | RATE | RANGE | COLOR | Sweep |
|---|---|---|---|---|
| Gilmour, Animals tour 1977 (*Dogs*, *Pigs*): the "10 o'clock" setting | 0.3 | 9.2 | 6.0 | 0.077 Hz, 1.2–9.9 ms |
| Gilmour, The Wall tour 1980–81 (*Comfortably Numb*, *Run Like Hell*): RATE about 22° faster | 1.0 | 9.2 | 6.0 | 0.108 Hz, 1.2–9.9 ms |
| Andy Summers (*De Do Do Do, De Da Da Da*, *Driven to Tears*) | 1.6 | 1.0 | 7.0 | 0.144 Hz, 1.2–2.6 ms |
| Rotating speaker (EHX manual: "rate near maximum, range near minimum, color approximately half-way") | 9.6 | 2.0 | 4.2 | 6.6 Hz, 1.2–3.5 ms |

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
eclecticmistress.lv2/        the LV2 bundle (manifest, plugin TTL, presets, .so)
tests/em_test.c              LV2 test harness (incl. measured delay/feedback laws)
```

## License

GPL-2.0-or-later (see LICENSE).
