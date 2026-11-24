# Eclectic Mistress - Electro-Harmonix Electric Mistress flanger emulation
#
# Built with DPF: one codebase -> LV2 (+ modgui), VST3, CLAP and a JACK
# standalone app. The DSP is the faust-generated class in src/.
#
# Targets:
#   make               build all formats into bin/
#   make NOUI=true     DSP only (build hosts without cairo/X11)
#   make install       install the LV2 bundle into $(INSTALL_DIR)
#   make install-all   also VST3 and CLAP into $(VST3_DIR) / $(CLAP_DIR)
#   make test          LV2 C-API test harness (DSP behaviour)
#   make test-ui       drive the real UI under Xvfb (needs xvfb-run, XTest)
#   make test-modgui   check the modgui in headless Chrome (needs PIL)
#   make test-clap     CLAP load/process smoke test
#   make test-all      all of the above
#   make dsp           regenerate src/eclecticmistress.cc from the .dsp
#   make screenshot    re-render screenshot.png (README image) from the artwork
#   make modgui        re-render the modgui images from the artwork
#   make clean

PLUG    := eclecticmistress
BUNDLE  := bin/$(PLUG).lv2
DSP     := src/$(PLUG).dsp
CC_GEN  := src/$(PLUG).cc

PREFIX      ?= /usr
INSTALL_DIR ?= $(PREFIX)/lib/lv2
VST3_DIR    ?= $(PREFIX)/lib/vst3
CLAP_DIR    ?= $(PREFIX)/lib/clap
BIN_DIR     ?= $(PREFIX)/bin

# Path to the guitarix dsp2cc tool (used by 'make dsp' only; the
# checked-in generated C++ builds without it).
DSP2CC  ?= /home/tst/Fun/guitarix/trunk/tools/dsp2cc

TTL_GEN := dpf/utils/lv2_ttl_generator

.PHONY: screenshot all plugin ttl dsp modgui install install-all test test-clap test-ui test-modgui test-all clean distclean

all: ttl

dpf/Makefile.plugins.mk:
	git submodule update --init --recursive

$(TTL_GEN): dpf/Makefile.plugins.mk
	$(MAKE) -C dpf/utils/lv2-ttl-generator

plugin: dpf/Makefile.plugins.mk
	$(MAKE) -C plugins/EclecticMistress NOUI=$(NOUI)

# generate the LV2 TTL (incl. presets.ttl from the plugin programs) and
# add the static modgui
ttl: plugin $(TTL_GEN)
	cd $(BUNDLE) && $(CURDIR)/$(TTL_GEN) ./$(PLUG)_dsp.so
	# DPF prefixes audio port symbols with "lv2_"; restore the plugin's
	# original "in"/"out" (pedalboards store connections by symbol)
	sed -i -e 's/lv2:symbol "lv2_in"/lv2:symbol "in"/' \
	       -e 's/lv2:symbol "lv2_out"/lv2:symbol "out"/' $(BUNDLE)/$(PLUG)_dsp.ttl
	rm -rf $(BUNDLE)/modgui
	cp -r modgui $(BUNDLE)/modgui
	mv $(BUNDLE)/modgui/modgui.ttl $(BUNDLE)/modgui.ttl

dsp:
	python3 $(DSP2CC) --init-type=plugin-lv2 -double -o $(CC_GEN) $(DSP)

build/render_modgui: tools/render_modgui.cpp plugins/EclecticMistress/Artwork.hpp
	mkdir -p build
	$(CXX) -O2 -std=c++17 -Wall -o $@ $< $$(pkg-config --cflags --libs cairo)

modgui: build/render_modgui
	build/render_modgui modgui

# README image: the pedal at its default settings, 1.5x the design size
screenshot: build/render_modgui
	build/render_modgui --preview screenshot.png 1.5

install: ttl
	mkdir -p "$(DESTDIR)$(INSTALL_DIR)"
	rm -rf "$(DESTDIR)$(INSTALL_DIR)/$(PLUG).lv2"
	cp -r $(BUNDLE) "$(DESTDIR)$(INSTALL_DIR)/"

install-all: install
	mkdir -p "$(DESTDIR)$(VST3_DIR)" "$(DESTDIR)$(CLAP_DIR)" "$(DESTDIR)$(BIN_DIR)"
	rm -rf "$(DESTDIR)$(VST3_DIR)/$(PLUG).vst3"
	cp -r bin/$(PLUG).vst3 "$(DESTDIR)$(VST3_DIR)/"
	cp bin/$(PLUG).clap "$(DESTDIR)$(CLAP_DIR)/"
	cp bin/$(PLUG) "$(DESTDIR)$(BIN_DIR)/$(PLUG)"

test: ttl
	$(CC) -O2 -o build/em_test tests/em_test.c -ldl -lm
	build/em_test $(CURDIR)/$(BUNDLE)/$(PLUG)_dsp.so

# Drives the real UI under Xvfb with XTest: host->UI values, knob drags,
# wheel, double-click reset, switch and footswitch clicks, resizing.
# Ports: rate 2, range 3, color 4, mix 5, matrix 6, lv2_enabled 7.
UI_SHOTS ?= build/ui-shots
test-ui: ttl
	mkdir -p build $(UI_SHOTS)
	$(CC) -O2 -o build/ui_test tests/ui_test.c -ldl -lm -lX11 -lXtst
	xvfb-run -a -s "-screen 0 1400x1400x24" build/ui_test \
	  $(CURDIR)/$(BUNDLE)/$(PLUG)_ui.so http://dataforte.net/plugins/$(PLUG) \
	  $(CURDIR)/$(BUNDLE)/ 345x440 portmap:lv2_enabled=7 \
	  size:345x440 shot:$(UI_SHOTS)/default.png \
	  set:2=5 set:6=1 set:7=0 shot:$(UI_SHOTS)/host-values.png \
	  drag:62.5,80,-50 expect:2\>7 \
	  wheel:282.5,80,5 expect:4\>6.5 \
	  drag:172.5,80,60 expect:3\<7 dclick:172.5,80 expect:3=9.2 \
	  click:172.5,17.5 expect:6=0 \
	  click:172.5,356.25 expect:7=1 \
	  resize:690x880 size:690x880 shot:$(UI_SHOTS)/690x880.png \
	  drag:282.5,351.25,50 expect:5\<80 \
	  resize:500x300 shot:$(UI_SHOTS)/500x300-letterbox.png \
	  click:172.5,356 expect:7=0

# CLAP: load through the factory, activate, process, list parameters
test-clap: ttl
	mkdir -p build
	$(CC) -O2 -std=c11 -Idpf/distrho/src -o build/clap_check tests/clap_check.c -ldl -lm
	build/clap_check bin/$(PLUG).clap net.dataforte.$(PLUG) 6

test-all: test test-clap test-ui test-modgui

# Renders the modgui in headless Chrome at the default values and checks
# every control against the cairo artwork (needs google-chrome + PIL).
test-modgui: build/render_modgui
	build/render_modgui --preview build/modgui-ref.png 1.6 1.0 9.2 6.0 100 0 0 > /dev/null
	python3 tests/modgui_check.py modgui build build/modgui-ref.png \
	  rate=1.0:0:10 range=9.2:0:10 color=6.0:0:10 mix=100:0:100 matrix=0:0:1 led=on

clean:
	$(MAKE) -C plugins/EclecticMistress clean
	rm -rf bin build

distclean: clean
	$(MAKE) -C dpf clean
