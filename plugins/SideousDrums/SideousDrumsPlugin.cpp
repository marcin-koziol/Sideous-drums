/*
 * Sideous Drums - stereo-mix build: single stereo output bus, the summed
 * kit through the master drive/grit stage. See DrumEngine.hpp for the
 * actual 12-voice kit (shared with the multi-out build, SideousDrumsMulti/).
 */

#include "DistrhoPlugin.hpp"
#include "../common/Params.hpp"
#include "../common/DrumEngine.hpp"

START_NAMESPACE_DISTRHO

using namespace sideous;

// -----------------------------------------------------------------------------------------------------------

class SideousDrumsPlugin : public Plugin
{
public:
    SideousDrumsPlugin()
        : Plugin(kParamCount, 0, 0)
    {
        fEngine.setSampleRate(getSampleRate());
    }

protected:
    // ---------------------------------------------------------------------
    // Information

    const char* getLabel() const override { return "SideousDrums"; }
    const char* getDescription() const override
    {
        return "Retro 8-bit-vibe drum synth: kick, snare, closed/open hi-hat, 3 toms, crash, "
               "ride, rimshot, handclap and maracas, each independently tunable RD-9-style, "
               "with a master drive/grit stage.";
    }
    const char* getMaker() const override { return "Sideous"; }
    const char* getHomePage() const override { return DISTRHO_PLUGIN_URI; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(0, 2, 0); }

    // ---------------------------------------------------------------------
    // Init

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        const ParamInfo& info = getParamInfo(index);

        parameter.hints = kParameterIsAutomatable;
        parameter.name = info.name;
        parameter.symbol = info.symbol;
        parameter.unit = info.unit;
        parameter.ranges.min = info.min;
        parameter.ranges.max = info.max;
        parameter.ranges.def = info.def;

        if (info.shape == ParamShape::Logarithmic)
            parameter.hints |= kParameterIsLogarithmic;
    }

    // ---------------------------------------------------------------------
    // Internal data

    float getParameterValue(uint32_t index) const override
    {
        return fEngine.getParameterValue(index);
    }

    void setParameterValue(uint32_t index, float value) override
    {
        fEngine.setParameterValue(index, value);
    }

    // ---------------------------------------------------------------------
    // Audio/MIDI Processing

    void activate() override
    {
        fEngine.setSampleRate(getSampleRate());
    }

    void sampleRateChanged(double newSampleRate) override
    {
        fEngine.setSampleRate(newSampleRate);
    }

    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        float* outL = outputs[0];
        float* outR = outputs[1];

        uint32_t nextEvent = 0;

        for (uint32_t frame = 0; frame < frames; ++frame)
        {
            while (nextEvent < midiEventCount && midiEvents[nextEvent].frame == frame)
            {
                int note; float vel01;
                if (parseNoteOnEvent(midiEvents[nextEvent].data, midiEvents[nextEvent].size, note, vel01))
                    fEngine.trigger(note, vel01);
                ++nextEvent;
            }

            const float mix = fEngine.applyMasterBus(fEngine.processVoices().sum());
            outL[frame] = mix;
            outR[frame] = mix;
        }

        while (nextEvent < midiEventCount)
        {
            int note; float vel01;
            if (parseNoteOnEvent(midiEvents[nextEvent].data, midiEvents[nextEvent].size, note, vel01))
                fEngine.trigger(note, vel01);
            ++nextEvent;
        }
    }

private:
    DrumEngine fEngine;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SideousDrumsPlugin)
};

// -----------------------------------------------------------------------------------------------------------

Plugin* createPlugin()
{
    return new SideousDrumsPlugin();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
