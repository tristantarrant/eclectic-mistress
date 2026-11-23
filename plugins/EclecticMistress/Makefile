#!/usr/bin/make -f
# DPF build for Eclectic Mistress (driven by the top-level Makefile)

NAME = eclecticmistress

FILES_DSP = PluginEclecticMistress.cpp

ifneq ($(NOUI),true)
FILES_UI  = UIEclecticMistress.cpp
UI_TYPE   = cairo
endif

include ../../dpf/Makefile.plugins.mk

# the faust-generated code is derived from dsp2cc templates
BUILD_CXX_FLAGS += -Wno-unused-parameter -Wno-unused-variable

# lv2_sep: separate DSP and UI binaries, so the DSP (all mod-host loads
# on Stom-Pi) does not depend on X11/cairo.
TARGETS ?= lv2_sep vst3 clap jack

all: $(TARGETS)
