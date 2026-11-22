/*
 * Eclectic Mistress - LV2 test harness
 *
 * Loads the plugin with the raw LV2 C API, exercises the DSP with a few
 * control/audio scenarios and checks the results.
 *
 * Usage: ./em_test /path/to/eclecticmistress.lv2/eclecticmistress.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>
#include <stdint.h>

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

const LV2_Descriptor *
lv2_descriptor(unsigned int);

#define SR      48000
#define BLOCK   1024
#define NPASS   0

static int failures = 0;

static void
check(int ok, const char *msg)
{
  if (ok)
    printf("PASS: %s\n", msg);
  else {
    printf("FAIL: %s\n", msg);
    ++failures;
  }
}

static int
finite_buffer(const float *buf, int n)
{
  for (int i = 0; i < n; ++i)
    if (!isfinite(buf[i]))
      return 0;
  return 1;
}

static double
rms(const float *buf, int n)
{
  double acc = 0;
  for (int i = 0; i < n; ++i)
    acc += (double)buf[i] * buf[i];
  return sqrt(acc / n);
}

int
main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s plugin.so\n", argv[0]);
    return 1;
  }

  void *handle = dlopen(argv[1], RTLD_NOW);
  if (!handle) {
    fprintf(stderr, "cannot dlopen %s: %s\n", argv[1], dlerror());
    return 1;
  }

  const LV2_Descriptor *(*desc_fn)(unsigned int) =
    (const LV2_Descriptor * (*)(unsigned int)) dlsym(handle, "lv2_descriptor");
  if (!desc_fn) {
    fprintf(stderr, "no lv2_descriptor symbol\n");
    return 1;
  }

  const LV2_Descriptor *desc = desc_fn(0);
  check(desc != NULL && desc->plugin_uri != NULL, "lv2_descriptor(0)");
  if (!desc)
    return 1;

  LV2_Handle h = desc->instantiate(desc, SR, NULL, NULL);
  check(h != NULL, "instantiate");
  if (!h)
    return 1;

  float ctrl[4];       /* rate, range, color, mix */
  float in[BLOCK], out[BLOCK];

  desc->connect_port(h, 0, &ctrl[0]);
  desc->connect_port(h, 1, &ctrl[1]);
  desc->connect_port(h, 2, &ctrl[2]);
  desc->connect_port(h, 3, &ctrl[3]);
  desc->connect_port(h, 4, out);   /* audio output */
  desc->connect_port(h, 5, in);    /* audio input */
  desc->activate(h);
  check(1, "connect_port + activate");

  /* --- test 1: mix = 0 must bypass (output == input) --------------- */
  ctrl[0] = 0.9; ctrl[1] = 30; ctrl[2] = 30; ctrl[3] = 0;
  for (int i = 0; i < BLOCK; ++i)
    in[i] = sinf(2.0f * (float)M_PI * 1000.0f * i / SR);
  desc->run(h, BLOCK);
  double maxdiff = 0;
  for (int i = 0; i < BLOCK; ++i)
    maxdiff = fmax(maxdiff, fabs((double)out[i] - in[i]));
  check(maxdiff < 1e-6, "mix=0 bypass (max |out-in| < 1e-6)");

  /* --- test 2: default settings, 1 kHz sine ------------------------ */
  ctrl[0] = 0.9; ctrl[1] = 30; ctrl[2] = 30; ctrl[3] = 100;
  desc->run(h, BLOCK);
  check(finite_buffer(out, BLOCK), "default settings: output finite");
  double r = rms(out, BLOCK);
  check(r > 0.01 && r < 5.0, "default settings: sensible output level");
  printf("      rms(default) = %.4f\n", r);

  /* --- test 3: filter matrix mode (rate=0), no feedback ------------- */
  ctrl[0] = 0.0; ctrl[1] = 0; ctrl[2] = 0; ctrl[3] = 100;
  for (int b = 0; b < 16; ++b) {
    for (int i = 0; i < BLOCK; ++i)
      in[i] = sinf(2.0f * (float)M_PI * 1000.0f * (i + b * BLOCK) / SR);
    desc->run(h, BLOCK);
  }
  check(finite_buffer(out, BLOCK), "matrix mode (rate=0, fb=0): output finite");
  r = rms(out, BLOCK);
  check(r > 0.01 && r < 5.0, "matrix mode: sensible output level");
  printf("      rms(matrix) = %.4f\n", r);

  /* --- test 4: extreme feedback both polarities --------------------- */
  for (int pol = 0; pol < 2; ++pol) {
    ctrl[0] = 0.5; ctrl[1] = 100; ctrl[2] = pol ? 100 : -100; ctrl[3] = 100;
    for (int b = 0; b < 32; ++b) {
      for (int i = 0; i < BLOCK; ++i)
        in[i] = sinf(2.0f * (float)M_PI * 1000.0f * (i + b * BLOCK) / SR)
          + 0.5 * sinf(2.0f * (float)M_PI * 437.0f * (i + b * BLOCK) / SR);
      desc->run(h, BLOCK);
    }
    check(finite_buffer(out, BLOCK),
          pol ? "color=+100 (max positive fb): output finite"
              : "color=-100 (max negative fb): output finite");
  }

  /* --- test 5: long-run stability ------------------------------------ */
  ctrl[0] = 1.5; ctrl[1] = 100; ctrl[2] = 30; ctrl[3] = 100;
  int stable = 1;
  for (int b = 0; b < 4096; ++b) {
    for (int i = 0; i < BLOCK; ++i)
      in[i] = (rand() % 1000 / 500.0f - 1.0f) * 0.7f;
    desc->run(h, BLOCK);
    if (!finite_buffer(out, BLOCK)) {
      stable = 0;
      break;
    }
  }
  check(stable, "long run (4096 blocks): no NaN/Inf");

  desc->deactivate(h);
  desc->cleanup(h);
  check(1, "deactivate + cleanup");

  dlclose(handle);

  printf("\n%s (%d failure%s)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED",
         failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
