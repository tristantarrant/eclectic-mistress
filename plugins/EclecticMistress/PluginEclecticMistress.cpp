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

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------
// Presets: documented Electric Mistress settings (see README).
// Program 0 equals the parameter defaults.

struct EmProgram {
    const char* name;
    float rate, range, color;
};

static const EmProgram kPrograms[] = {
    { "Gilmour - The Wall tour 1980-81 (Comfortably Numb, Run Like Hell)", 1.0f, 9.2f, 6.0f },
    { "Gilmour - Animals tour 1977 (Dogs, Pigs)",                          0.3f, 9.2f, 6.0f },
    { "Andy Summers - De Do Do Do, De Da Da Da / Driven to Tears",         1.6f, 1.0f, 7.0f },
    { "Rotating speaker (Leslie simulation)",                              9.6f, 2.0f, 4.2f },
};
static constexpr uint32_t kProgramCount = sizeof(kPrograms) / sizeof(kPrograms[0]);

// --------------------------------------------------------------------

class PluginEclecticMistress : public Plugin
{
public:
    PluginEclecticMistress()
        : Plugin(kParamCount, kProgramCount, 0),
          fDsp(eclecticmistress::plugin())
    {
        fParams[kParamRate]   = kPrograms[0].rate;
        fParams[kParamRange]  = kPrograms[0].range;
        fParams[kParamColor]  = kPrograms[0].color;
        fParams[kParamMix]    = 100.0f;
        fParams[kParamMatrix] = 0.0f;
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
    const char* getMaker() const override { return "Rustyfoot"; }
    const char* getHomePage() const override { return "https://github.com/tristantarrant/eclectic-mistress"; }
    const char* getLicense() const override { return "GPL-2.0-or-later"; }
    uint32_t getVersion() const override { return d_version(1, 1, 0); }
    int64_t getUniqueId() const override { return d_cconst('R', 'e', 'M', 's'); }

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
            p.ranges.def = kPrograms[0].rate;
            break;
        case kParamRange:
            p.name = "RANGE"; p.symbol = "range";
            p.ranges.def = kPrograms[0].range;
            break;
        case kParamColor:
            p.name = "COLOR"; p.symbol = "color";
            p.ranges.def = kPrograms[0].color;
            break;
        case kParamMix:
            p.name = "MIX"; p.symbol = "mix"; p.unit = "%";
            p.ranges.max = 100.0f;
            p.ranges.def = 100.0f;
            break;
        case kParamMatrix:
            p.hints |= kParameterIsBoolean | kParameterIsInteger;
            p.name = "MATRIX"; p.symbol = "matrix";
            p.ranges.max = 1.0f;
            p.ranges.def = 0.0f;
            break;
        case kParamBypass:
            p.initDesignation(kParameterDesignationBypass);
            break;
        }
    }

    void initProgramName(const uint32_t index, String& name) override
    {
        if (index < kProgramCount)
            name = kPrograms[index].name;
    }

    void loadProgram(const uint32_t index) override
    {
        if (index >= kProgramCount)
            return;
        fParams[kParamRate]   = kPrograms[index].rate;
        fParams[kParamRange]  = kPrograms[index].range;
        fParams[kParamColor]  = kPrograms[index].color;
        fParams[kParamMix]    = 100.0f;
        fParams[kParamMatrix] = 0.0f;
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
