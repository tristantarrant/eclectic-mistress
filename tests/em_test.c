/*
 * Eclectic Mistress - LV2 test harness
 *
 * Loads the plugin with the raw LV2 C API, exercises the DSP with a few
 * control/audio scenarios and checks the results against the measured
 * behaviour of the Electric Mistress V2 that the model encodes.
 *
 * Usage: ./em_test /path/to/eclecticmistress.lv2/eclecticmistress.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>

#include "lv2host.h"

typedef struct _LV2_Descriptor LV2_Descriptor;
typedef void *LV2_Handle;

struct _LV2_Descriptor {
  const char *plugin_uri;
  LV2_Handle (*instantiate)(const LV2_Descriptor *, double, const char *,
                            const void *const *);
  void (*connect_port)(LV2_Handle, unsigned int, void *);
  void (*activate)(LV2_Handle);
  void (*run)(LV2_Handle, unsigned int);
  void (*deactivate)(LV2_Handle);
  void (*cleanup)(LV2_Handle);
  void *reserved;
};

/* DPF port layout: audio in, audio out, parameters, lv2:enabled */
enum { RATE, RANGE, COLOR, MIX, MATRIX, NCTRL };
#define P_IN      0
#define P_OUT     1
#define P_CTRL0   2
#define P_ENABLED 7

/* model constants (must match src/eclecticmistress.dsp) */
#define D_LO  1.2e-3
#define D_HI  10.7e-3

#define BLOCK 1024

static int failures = 0;
static const LV2_Descriptor *desc;
static float ctrl[NCTRL];
static float enabled = 1.0f;
static float in[BLOCK], out[BLOCK];

static void
check(int ok, const char *msg)
{
  printf("%s: %s\n", ok ? "PASS" : "FAIL", msg);
  if (!ok)
    ++failures;
}

static void
defaults(void)
{
  ctrl[RATE] = 1.0f; ctrl[RANGE] = 9.2f; ctrl[COLOR] = 6.0f;
  ctrl[MIX] = 100.0f; ctrl[MATRIX] = 0.0f;
}

static LV2_Handle
open_plugin(double sr)
{
  LV2_Handle h = desc->instantiate(desc, sr, NULL,
                                   (const void *const *)host_get_features(BLOCK));
  if (!h)
    return NULL;
  for (int i = 0; i < NCTRL; ++i)
    desc->connect_port(h, P_CTRL0 + i, &ctrl[i]);
  enabled = 1.0f;
  desc->connect_port(h, P_ENABLED, &enabled);
  desc->connect_port(h, P_OUT, out);
  desc->connect_port(h, P_IN, in);
  desc->activate(h);
  return h;
}

static void
close_plugin(LV2_Handle h)
{
  desc->deactivate(h);
  desc->cleanup(h);
}

/* Run a sine for `secs` seconds (control smoothing and filters settle),
   return the output rms over the last block; *finite reports NaN/Inf. */
static double
sine_rms(LV2_Handle h, double sr, double freq, double amp, double secs,
         int *finite)
{
  static long phase = 0;
  int nblocks = (int)(secs * sr / BLOCK) + 1;
  double acc = 0;
  *finite = 1;
  for (int b = 0; b < nblocks; ++b) {
    for (int i = 0; i < BLOCK; ++i, ++phase)
      in[i] = (float)(amp * sin(2.0 * M_PI * freq * phase / sr));
    desc->run(h, BLOCK);
    for (int i = 0; i < BLOCK; ++i)
      if (!isfinite(out[i]))
        *finite = 0;
  }
  for (int i = 0; i < BLOCK; ++i)
    acc += (double)out[i] * out[i];
  return sqrt(acc / BLOCK);
}

static double
db(double x)
{
  return 20.0 * log10(x + 1e-30);
}

int
main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s plugin.so\n", argv[0]);
    return 1;
  }
  void *lib = dlopen(argv[1], RTLD_NOW);
  if (!lib) {
    fprintf(stderr, "cannot dlopen %s: %s\n", argv[1], dlerror());
    return 1;
  }
  const LV2_Descriptor *(*desc_fn)(unsigned int) =
    (const LV2_Descriptor * (*)(unsigned int)) dlsym(lib, "lv2_descriptor");
  desc = desc_fn ? desc_fn(0) : NULL;
  check(desc != NULL && desc->plugin_uri != NULL, "lv2_descriptor(0)");
  if (!desc)
    return 1;

  const double SR = 48000;
  int fin;
  double r, r1, r2;
  char msg[160];

  defaults();
  LV2_Handle h = open_plugin(SR);
  check(h != NULL, "instantiate @48k");
  if (!h)
    return 1;

  /* --- mix = 0 must bypass (output == input) ----------------------- */
  ctrl[MIX] = 0;
  sine_rms(h, SR, 1000, 0.5, 0.5, &fin);
  double maxdiff = 0;
  for (int i = 0; i < BLOCK; ++i)
    maxdiff = fmax(maxdiff, fabs((double)out[i] - in[i]));
  check(maxdiff < 1e-6, "mix=0 bypass (max |out-in| < 1e-6)");
  defaults();

  /* --- defaults: sane level ---------------------------------------- */
  r = sine_rms(h, SR, 1000, 0.5, 1.0, &fin);
  check(fin && r > 0.01 && r < 2.0, "defaults: finite, sensible level");
  printf("      rms(defaults) = %.4f\n", r);

  /* --- Filter Matrix: delay law D_LO + RANGE/10*(D_HI-D_LO) --------- */
  /* With COLOR 0 the comb is dry + delayed: notches at (k+1/2)/d, peaks
     at k/d. Check notch depth relative to the neighbouring peak for a
     short, a medium and the longest static delay. */
  ctrl[MATRIX] = 1; ctrl[COLOR] = 0;
  const float ranges[] = { 0.0f, 1.6316f, 10.0f };
  for (int k = 0; k < 3; ++k) {
    ctrl[RANGE] = ranges[k];
    double d = D_LO + ranges[k] / 10.0 * (D_HI - D_LO);
    /* pick a notch/peak pair inside the passband (300 Hz .. 6 kHz) */
    int n = (int)ceil(400.0 * d - 0.5);
    double f_notch = (n + 0.5) / d, f_peak = (n + 1) / d;
    double rn = sine_rms(h, SR, f_notch, 0.05, 1.0, &fin);
    double rp = sine_rms(h, SR, f_peak, 0.05, 1.0, &fin);
    snprintf(msg, sizeof msg,
             "matrix RANGE %.2f (d = %.2f ms): notch %.0f Hz %.1f dB below peak %.0f Hz",
             ranges[k], d * 1e3, f_notch, db(rp / rn), f_peak);
    check(fin && db(rp / rn) > 30, msg);
  }

  /* --- COLOR: positive feedback, max g = 0.5 -> comb peaks +6 dB ----- */
  ctrl[RANGE] = 1.6316f;   /* d = 2.75 ms, peak at 1/d = 363.6 Hz x 3 */
  double f_pk = 3.0 / 2.75e-3;
  ctrl[COLOR] = 0;
  r1 = sine_rms(h, SR, f_pk, 0.02, 1.0, &fin);
  ctrl[COLOR] = 10;
  r2 = sine_rms(h, SR, f_pk, 0.02, 1.0, &fin);
  snprintf(msg, sizeof msg,
           "COLOR 10 lifts comb peak by %+.2f dB (g = 0.5 -> +6.02 dB)",
           db(r2 / r1));
  check(fin && fabs(db(r2 / r1) - 6.02) < 0.5, msg);
  defaults();

  /* --- extremes: max everything, both modes -------------------------- */
  for (int m = 0; m < 2; ++m) {
    ctrl[RATE] = 10; ctrl[RANGE] = 10; ctrl[COLOR] = 10; ctrl[MATRIX] = m;
    r = sine_rms(h, SR, 437, 0.9, 2.0, &fin);
    check(fin && r < 1.0, m ? "all max, matrix: finite, bounded"
                            : "all max, flange: finite, bounded");
  }
  defaults();

  /* --- bypass (lv2:enabled): click-free crossfade to exact dry -------- */
  /* Warm up engaged, then bypass. During the fade no sample step may
     exceed the larger of the wet and dry signals' own steady steps. */
  {
    long ph = 0;
    double wetstep = 0, drystep = 0, fadestep = 0, maxdiff = 0, prev = 0;
    enabled = 1.0f;
    for (int b = 0; b < 48; ++b) {
      for (int i = 0; i < BLOCK; ++i, ++ph)
        in[i] = (float)(0.5 * sin(2.0 * M_PI * 220.0 * ph / SR));
      desc->run(h, BLOCK);
      for (int i = 0; i < BLOCK; ++i) {
        if (b == 47 && i > 0)
          wetstep = fmax(wetstep, fabs(out[i] - out[i - 1]));
        if (i > 0 || b > 0)
          drystep = fmax(drystep, fabs(in[i] - (i ? in[i - 1] : in[i])));
      }
      prev = out[BLOCK - 1];
    }
    enabled = 0.0f;
    for (int b = 0; b < 4; ++b) {
      for (int i = 0; i < BLOCK; ++i, ++ph)
        in[i] = (float)(0.5 * sin(2.0 * M_PI * 220.0 * ph / SR));
      desc->run(h, BLOCK);
      for (int i = 0; i < BLOCK; ++i) {
        if (b < 2)
          fadestep = fmax(fadestep, fabs(out[i] - prev));
        prev = out[i];
        if (b >= 2)
          maxdiff = fmax(maxdiff, fabs((double)out[i] - in[i]));
      }
    }
    snprintf(msg, sizeof msg,
             "bypass fade: max step %.4f (wet %.4f, dry %.4f): no click",
             fadestep, wetstep, drystep);
    check(fadestep <= 1.2 * fmax(wetstep, drystep), msg);
    check(maxdiff < 1e-6, "bypassed: output == input after the 10 ms fade");
    enabled = 1.0f;
    r = sine_rms(h, SR, 1000, 0.5, 0.5, &fin);
    check(fin && r > 0.01, "re-enabled: effect back");
  }
  defaults();

  /* --- long run with random controls and noise ----------------------- */
  int stable = 1;
  srand(1);
  for (int b = 0; b < 4096 && stable; ++b) {
    if (b % 16 == 0) {
      ctrl[RATE] = rand() % 101 / 10.0f;
      ctrl[RANGE] = rand() % 101 / 10.0f;
      ctrl[COLOR] = rand() % 101 / 10.0f;
      ctrl[MIX] = rand() % 101;
      ctrl[MATRIX] = rand() % 2;
    }
    for (int i = 0; i < BLOCK; ++i)
      in[i] = (rand() % 2001 / 1000.0f - 1.0f) * 0.8f;
    desc->run(h, BLOCK);
    for (int i = 0; i < BLOCK; ++i)
      if (!isfinite(out[i]) || fabsf(out[i]) > 2.0f)
        stable = 0;
  }
  check(stable, "4096 blocks noise + random controls: finite, bounded");
  close_plugin(h);

  /* --- other sample rates -------------------------------------------- */
  const double rates[] = { 44100, 88200, 96000, 192000 };
  for (int i = 0; i < 4; ++i) {
    defaults();
    LV2_Handle h2 = open_plugin(rates[i]);
    r = h2 ? sine_rms(h2, rates[i], 1000, 0.5, 1.0, &fin) : -1;
    snprintf(msg, sizeof msg, "@%.0f Hz: finite, sensible level (rms %.3f)",
             rates[i], r);
    check(h2 && fin && r > 0.01 && r < 2.0, msg);
    if (h2)
      close_plugin(h2);
  }

  dlclose(lib);
  printf("\n%s (%d failure%s)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED",
         failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
