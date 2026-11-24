/*
 * Eclectic Mistress - Electro-Harmonix Electric Mistress flanger emulation
 * DPF plugin wrapper around the faust-generated DSP class.
 *
 * Copyright (C) 2026 Dataforte
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DistrhoPlugin.hpp"

#include <cstring>
#include <vector>

// Faust/dsp2cc generated DSP (global namespace, binds controls by PortIndex)
#include "../../src/em_glue.h"
#include "../../src/eclecticmistress.h"
#include "../../src/eclecticmistress.cc"

// Preset table and parameter defaults, generated from presets/eclecticmistress.json
#include "Presets.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------
// Presets come from presets/eclecticmistress.json via Presets.hpp
// (make presets). DPF exports them to LV2 presets.ttl and to the
// VST3/CLAP program lists.

// --------------------------------------------------------------------

class PluginEclecticMistress : public Plugin
{
public:
    PluginEclecticMistress()
        : Plugin(kParamCount, em_presets::kCount, 0),
          fDsp(eclecticmistress::plugin())
    {
        for (uint32_t i = 0; i <= kParamMatrix; ++i)
            fParams[i] = em_presets::kDefaults.value[i];
        fParams[kParamBypass] = 0.0f;

        for (uint32_t i = 0; i < kParamBypass; ++i)
            fDsp->connect_ports(i, &fParams[i], fDsp);

        fDsp->set_samplerate(static_cast<uint32_t>(getSampleRate()), fDsp);
        fDry.resize(getBufferSize());
        fFadeStep = 1.0f / (0.010f * static_cast<float>(getSampleRate()));
    }

    ~PluginEclecticMistress() override
    {
        if (fDsp->activate_plugin != nullptr)
            fDsp->activate_plugin(false, fDsp);
        fDsp->delete_instance(fDsp);
    }

protected:
    const char* getLabel() const override { return "eclecticmistress"; }
    const char* getDescription() const override
    {
        return "Emulation of the 1976 Electro-Harmonix Electric Mistress (V2) flanger: "
               "a BBD comb filter swept 1.2-10.7 ms by a triangle LFO, with COLOR feedback "
               "and the Filter Matrix mode. Controls are pot positions as on the pedal.";
    }
    const char* getMaker() const override { return "Dataforte"; }
    const char* getHomePage() const override { return "https://github.com/tristantarrant/eclectic-mistress"; }
    const char* getLicense() const override { return "GPL-2.0-or-later"; }
    uint32_t getVersion() const override { return d_version(1, 1, 0); }
    int64_t getUniqueId() const override { return d_cconst('D', 'e', 'M', 's'); }

    void initAudioPort(const bool input, const uint32_t index, AudioPort& port) override
    {
        port.groupId = kPortGroupMono;
        Plugin::initAudioPort(input, index, port);
        // keep the symbols of the pre-DPF plugin: pedalboards store them
        port.symbol = input ? "in" : "out";
        port.name   = input ? "In" : "Out";
    }

    void initParameter(const uint32_t index, Parameter& p) override
    {
        p.hints = kParameterIsAutomatable;
        p.ranges.min = 0.0f;
        p.ranges.max = 10.0f;

        switch (index)
        {
        case kParamRate:
            p.name = "RATE"; p.symbol = "rate";
            p.ranges.def = em_presets::kDefaults.value[kParamRate];
            break;
        case kParamRange:
            p.name = "RANGE"; p.symbol = "range";
            p.ranges.def = em_presets::kDefaults.value[kParamRange];
            break;
        case kParamColor:
            p.name = "COLOR"; p.symbol = "color";
            p.ranges.def = em_presets::kDefaults.value[kParamColor];
            break;
        case kParamMix:
            p.name = "MIX"; p.symbol = "mix"; p.unit = "%";
            p.ranges.max = 100.0f;
            p.ranges.def = em_presets::kDefaults.value[kParamMix];
            break;
        case kParamMatrix:
            p.hints |= kParameterIsBoolean | kParameterIsInteger;
            p.name = "MATRIX"; p.symbol = "matrix";
            p.ranges.max = 1.0f;
            p.ranges.def = em_presets::kDefaults.value[kParamMatrix];
            break;
        case kParamBypass:
            p.initDesignation(kParameterDesignationBypass);
            break;
        }
    }

    void initProgramName(const uint32_t index, String& name) override
    {
        if (index < em_presets::kCount)
            name = em_presets::kEntries[index].name;
    }

    void loadProgram(const uint32_t index) override
    {
        if (index >= em_presets::kCount)
            return;
        for (uint32_t i = 0; i <= kParamMatrix; ++i)
            fParams[i] = em_presets::kEntries[index].value[i];
    }

    float getParameterValue(const uint32_t index) const override
    {
        return index < kParamCount ? fParams[index] : 0.0f;
    }

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index < kParamCount)
            fParams[index] = value;
    }

    void activate() override
    {
        if (fDsp->activate_plugin != nullptr)
            fDsp->activate_plugin(true, fDsp);
        fWet = fParams[kParamBypass] > 0.5f ? 0.0f : 1.0f;
    }

    void deactivate() override
    {
        if (fDsp->activate_plugin != nullptr)
            fDsp->activate_plugin(false, fDsp);
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        fDry.resize(newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        fDsp->set_samplerate(static_cast<uint32_t>(newSampleRate), fDsp);
        fFadeStep = 1.0f / (0.010f * static_cast<float>(newSampleRate));
    }

    void run(const float** inputs, float** outputs, const uint32_t frames) override
    {
        AVOIDDENORMALS();

        const float* const in = inputs[0];
        float* const out = outputs[0];
        if (frames == 0)
            return;

        const float target = fParams[kParamBypass] > 0.5f ? 0.0f : 1.0f;
        const bool fading = fWet != target;

        // keep the DSP running while bypassed so that re-engaging is seamless
        if (fading || target == 0.0f)
            std::memcpy(fDry.data(), in, sizeof(float) * frames);

        fDsp->mono_audio(static_cast<int>(frames), const_cast<float*>(in), out, fDsp);

        if (!fading && target == 1.0f)
            return;
        if (!fading)
        {
            std::memcpy(out, fDry.data(), sizeof(float) * frames);
            return;
        }
        for (uint32_t i = 0; i < frames; ++i)
        {
            if (fWet < target)
                fWet = std::min(target, fWet + fFadeStep);
            else if (fWet > target)
                fWet = std::max(target, fWet - fFadeStep);
            out[i] = fDry[i] + fWet * (out[i] - fDry[i]);
        }
    }

private:
    PluginLV2* const fDsp;
    float fParams[kParamCount];
    std::vector<float> fDry;
    float fWet = 1.0f;
    float fFadeStep = 0.002f;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEclecticMistress)
};

Plugin* createPlugin()
{
    return new PluginEclecticMistress();
}

END_NAMESPACE_DISTRHO
