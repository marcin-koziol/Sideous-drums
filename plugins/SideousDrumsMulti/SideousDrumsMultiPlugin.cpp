/*
 * Sideous Drums Multi - multi-out build: 13 stereo busses (Main mix + one
 * per voice), so any single drum (e.g. the handclap) can be routed to its
 * own send/insert in the host mixer instead of only ever reaching the
 * summed Main bus. Same 12-voice kit as the stereo build - see
 * DrumEngine.hpp, shared by both so they can't drift apart on voice
 * behavior. Per-voice busses carry the dry, undistorted voice signal (only
 * its own Level knob and the master Volume applied); the master Drive/grit
 * stage only makes sense applied to the summed Main bus, so it's skipped
 * here for the individual busses - see DrumEngine::applyMasterBus().
 */

#include "DistrhoPlugin.hpp"
#include "../common/Params.hpp"
#include "../common/DrumEngine.hpp"

START_NAMESPACE_DISTRHO

using namespace sideous;

// -----------------------------------------------------------------------------------------------------------

// bus 0 = Main mix, busses 1..12 = one per voice, index-aligned with
// DrumEngine::VoiceOutputs and the writeBus() calls in run() below
static constexpr uint32_t kBusCount = 13;
static const char* const kBusNames[kBusCount] = {
    "Main", "Kick", "Snare", "Hat Closed", "Hat Open",
    "Tom Low", "Tom Mid", "Tom High", "Crash", "Ride", "Rimshot", "Clap", "Maracas"
};
static const char* const kBusSymbols[kBusCount] = {
    "main", "kick", "snare", "hat_closed", "hat_open",
    "tom_low", "tom_mid", "tom_high", "crash", "ride", "rimshot", "clap", "maracas"
};

class SideousDrumsMultiPlugin : public Plugin
{
public:
    SideousDrumsMultiPlugin()
        : Plugin(kParamCount, 0, 0)
    {
        fEngine.setSampleRate(getSampleRate());
    }

protected:
    // ---------------------------------------------------------------------
    // Information

    const char* getLabel() const override { return "SideousDrumsMulti"; }
    const char* getDescription() const override
    {
        return "Multi-out build of Sideous Drums: the same 12-voice retro kit (kick, snare, "
               "closed/open hi-hat, 3 toms, crash, ride, rimshot, handclap, maracas), but with "
               "13 stereo busses - a Main mix plus one dedicated bus per voice - so any drum "
               "can get its own send/insert in the host mixer.";
    }
    const char* getMaker() const override { return "Sideous"; }
    const char* getHomePage() const override { return DISTRHO_PLUGIN_URI; }
    const char* getLicense() const override { return "ISC"; }
    uint32_t getVersion() const override { return d_version(0, 1, 0); }

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

    void initAudioPort(bool input, uint32_t index, AudioPort& port) override
    {
        if (!input)
        {
            const uint32_t bus = index / 2;
            const bool isLeft = (index % 2) == 0;
            port.groupId = bus;
            port.name = kBusNames[bus];
            port.name += isLeft ? " L" : " R";
            port.symbol = kBusSymbols[bus];
            port.symbol += isLeft ? "_l" : "_r";
            return;
        }
        Plugin::initAudioPort(input, index, port);
    }

    void initPortGroup(uint32_t groupId, PortGroup& portGroup) override
    {
        if (groupId < kBusCount)
        {
            portGroup.name = kBusNames[groupId];
            portGroup.symbol = kBusSymbols[groupId];
            return;
        }
        Plugin::initPortGroup(groupId, portGroup);
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

            const DrumEngine::VoiceOutputs vo = fEngine.processVoices();
            const float masterVol = fEngine.masterVolume();

            writeBus(outputs, 0, frame, fEngine.applyMasterBus(vo.sum()));
            writeBus(outputs, 1,  frame, vo.kick      * masterVol);
            writeBus(outputs, 2,  frame, vo.snare     * masterVol);
            writeBus(outputs, 3,  frame, vo.hatClosed * masterVol);
            writeBus(outputs, 4,  frame, vo.hatOpen   * masterVol);
            writeBus(outputs, 5,  frame, vo.tomLow    * masterVol);
            writeBus(outputs, 6,  frame, vo.tomMid    * masterVol);
            writeBus(outputs, 7,  frame, vo.tomHigh   * masterVol);
            writeBus(outputs, 8,  frame, vo.crash     * masterVol);
            writeBus(outputs, 9,  frame, vo.ride      * masterVol);
            writeBus(outputs, 10, frame, vo.rim       * masterVol);
            writeBus(outputs, 11, frame, vo.clap      * masterVol);
            writeBus(outputs, 12, frame, vo.maracas   * masterVol);
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
    static void writeBus(float** outputs, uint32_t bus, uint32_t frame, float value) noexcept
    {
        outputs[bus * 2][frame] = value;
        outputs[bus * 2 + 1][frame] = value;
    }

    DrumEngine fEngine;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SideousDrumsMultiPlugin)
};

// -----------------------------------------------------------------------------------------------------------

Plugin* createPlugin()
{
    return new SideousDrumsMultiPlugin();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
