/*
 * Sideous Drums - a fixed 12-voice retro drum kit (kick, snare, closed/open
 * hi-hat, 3 toms, crash, ride, rimshot, handclap, maracas), each voice independently
 * tunable, fixed GM-ish MIDI note map (see DrumMap.hpp). No polyphony/
 * voice-stealing logic is needed: every voice is a single always-processed
 * singleton that hard-retriggers on note-on, one-shot (decay determines
 * length, note-off is ignored), exactly like a hardware drum machine. The
 * hi-hat pair forms a choke group: a closed-hat hit cuts a still-ringing
 * open hat, just like a real hi-hat's mechanics.
 */

#include "DistrhoPlugin.hpp"
#include "Params.hpp"
#include "DrumMap.hpp"
#include "dsp/PitchedDrumVoice.hpp"
#include "dsp/SnareVoice.hpp"
#include "dsp/CymbalVoice.hpp"
#include "dsp/ClickVoice.hpp"
#include "dsp/ClapVoice.hpp"
#include "dsp/NoiseVoice.hpp"
#include "dsp/Distortion.hpp"

#include <cmath>

START_NAMESPACE_DISTRHO

using namespace sideous;

// -----------------------------------------------------------------------------------------------------------

// fixed tasteful pitch-envelope character for the toms, not user-exposed
// (only the kick exposes Punch Depth/Decay - see sideous-drums-plan.md's
// call-out that this is the single highest-value "not just basic" feature,
// kept front-and-center on the kick while toms stay a simple 3-knob column)
static constexpr float kTomPitchEnvDepth = 10.0f;
static constexpr float kTomPitchEnvDecay = 0.03f;

class SideousDrumsPlugin : public Plugin
{
public:
    SideousDrumsPlugin()
        : Plugin(kParamCount, 0, 0)
    {
        sampleRateChanged(getSampleRate());

        fTomLow.setPitchEnvDepth(kTomPitchEnvDepth);
        fTomLow.setPitchEnvDecay(kTomPitchEnvDecay);
        fTomMid.setPitchEnvDepth(kTomPitchEnvDepth);
        fTomMid.setPitchEnvDecay(kTomPitchEnvDecay);
        fTomHigh.setPitchEnvDepth(kTomPitchEnvDepth);
        fTomHigh.setPitchEnvDecay(kTomPitchEnvDecay);

        fCrash.setToneRange(2000.0f, 8000.0f);
        fRide.setToneRange(1200.0f, 6000.0f);
        fHatClosed.setToneRange(3000.0f, 10000.0f);
        fHatOpen.setToneRange(3000.0f, 10000.0f);

        for (uint32_t i = 0; i < kParamCount; ++i)
            setParameterValue(i, getParamInfo(i).def);
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

    // ---------------------------------------------------------------------
    // Internal data

    float getParameterValue(uint32_t index) const override
    {
        return fValues[index];
    }

    void setParameterValue(uint32_t index, float value) override
    {
        fValues[index] = value;

        switch (index)
        {
        case kParamMasterVolume:    fMasterVolume = value; break;
        case kParamMasterDrive:     fDistortion.setDrive(value); break;
        case kParamMasterDriveMix:  fDistortion.setMix(value); break;

        case kParamKickTune:        fKick.setTune(value); break;
        case kParamKickPunchDepth:  fKick.setPitchEnvDepth(value); break;
        case kParamKickPunchDecay:  fKick.setPitchEnvDecay(value); break;
        case kParamKickDecay:       fKick.setDecay(value); break;
        case kParamKickDrive:       fKick.setDrive(value); break;
        case kParamKickLevel:       fKickLevel = value; break;

        case kParamSnareTune:       fSnare.setTune(value); break;
        case kParamSnareToneMix:    fSnare.setToneMix(value); break;
        case kParamSnareToneDecay:  fSnare.setToneDecay(value); break;
        case kParamSnareSnap:       fSnare.setSnap(value); break;
        case kParamSnareBright:     fSnare.setBright(value); break;
        case kParamSnareLevel:      fSnareLevel = value; break;

        case kParamHatClosedDecay:  fHatClosed.setDecay(value); break;
        case kParamHatClosedTone:   fHatClosed.setTone(value); break;
        case kParamHatClosedLevel:  fHatClosedLevel = value; break;

        case kParamHatOpenDecay:    fHatOpen.setDecay(value); break;
        case kParamHatOpenTone:     fHatOpen.setTone(value); break;
        case kParamHatOpenLevel:    fHatOpenLevel = value; break;

        case kParamTomLowTune:      fTomLow.setTune(value); break;
        case kParamTomLowDecay:     fTomLow.setDecay(value); break;
        case kParamTomLowLevel:     fTomLowLevel = value; break;

        case kParamTomMidTune:      fTomMid.setTune(value); break;
        case kParamTomMidDecay:     fTomMid.setDecay(value); break;
        case kParamTomMidLevel:     fTomMidLevel = value; break;

        case kParamTomHighTune:     fTomHigh.setTune(value); break;
        case kParamTomHighDecay:    fTomHigh.setDecay(value); break;
        case kParamTomHighLevel:    fTomHighLevel = value; break;

        case kParamCrashDecay:      fCrash.setDecay(value); break;
        case kParamCrashTone:       fCrash.setTone(value); break;
        case kParamCrashLevel:      fCrashLevel = value; break;

        case kParamRideDecay:       fRide.setDecay(value); break;
        case kParamRideTone:        fRide.setTone(value); break;
        case kParamRideLevel:       fRideLevel = value; break;

        case kParamRimTune:         fRim.setTune(value); break;
        case kParamRimLevel:        fRimLevel = value; break;

        case kParamClapDecay:       fClap.setDecay(value); break;
        case kParamClapLevel:       fClapLevel = value; break;

        case kParamMaracasDecay:    fMaracas.setDecay(value); break;
        case kParamMaracasLevel:    fMaracasLevel = value; break;

        default: break;
        }
    }

    // ---------------------------------------------------------------------
    // Audio/MIDI Processing

    void activate() override
    {
        sampleRateChanged(getSampleRate());
    }

    void sampleRateChanged(double newSampleRate) override
    {
        fKick.setSampleRate(newSampleRate);
        fSnare.setSampleRate(newSampleRate);
        fHatClosed.setSampleRate(newSampleRate);
        fHatOpen.setSampleRate(newSampleRate);
        fTomLow.setSampleRate(newSampleRate);
        fTomMid.setSampleRate(newSampleRate);
        fTomHigh.setSampleRate(newSampleRate);
        fCrash.setSampleRate(newSampleRate);
        fRide.setSampleRate(newSampleRate);
        fRim.setSampleRate(newSampleRate);
        fClap.setSampleRate(newSampleRate);
        fMaracas.setSampleRate(newSampleRate);
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
                handleMidiEvent(midiEvents[nextEvent]);
                ++nextEvent;
            }

            float mix = fKickLevel      * fKick.process()
                      + fSnareLevel     * fSnare.process()
                      + fHatClosedLevel * fHatClosed.process()
                      + fHatOpenLevel   * fHatOpen.process()
                      + fTomLowLevel  * fTomLow.process()
                      + fTomMidLevel  * fTomMid.process()
                      + fTomHighLevel * fTomHigh.process()
                      + fCrashLevel   * fCrash.process()
                      + fRideLevel    * fRide.process()
                      + fRimLevel     * fRim.process()
                      + fClapLevel    * fClap.process()
                      + fMaracasLevel * fMaracas.process();

            mix = fDistortion.process(mix);
            mix *= fMasterVolume;

            // safety saturation: several drums hitting at once can otherwise
            // sum past 0dBFS, same trick as sideous's plugin-level safety clip
            mix = std::tanh(mix);

            outL[frame] = mix;
            outR[frame] = mix;
        }

        while (nextEvent < midiEventCount)
            handleMidiEvent(midiEvents[nextEvent++]);
    }

private:
    void handleMidiEvent(const MidiEvent& event) noexcept
    {
        if (event.size < 2 || event.size > 3)
            return;

        const uint8_t status = event.data[0] & 0xF0;
        const uint8_t note = event.data[1];
        const uint8_t velocity = event.size > 2 ? event.data[2] : 0;

        if (status != 0x90 || velocity == 0)
            return; // note-off (or note-on w/ velocity 0) ignored: these are one-shots

        const float vel01 = (float)velocity / 127.0f;

        if (note == kDrumMidiNotes[kVoiceKick])         fKick.trigger(vel01);
        else if (note == kDrumMidiNotes[kVoiceSnare])   fSnare.trigger(vel01);
        else if (note == kDrumMidiNotes[kVoiceHatClosed])
        {
            // choke group: a closed-hat hit always cuts a still-ringing open hat
            fHatOpen.choke();
            fHatClosed.trigger(vel01);
        }
        else if (note == kDrumMidiNotes[kVoiceHatOpen]) fHatOpen.trigger(vel01);
        else if (note == kDrumMidiNotes[kVoiceTomLow])  fTomLow.trigger(vel01);
        else if (note == kDrumMidiNotes[kVoiceTomMid])  fTomMid.trigger(vel01);
        else if (note == kDrumMidiNotes[kVoiceTomHigh]) fTomHigh.trigger(vel01);
        else if (note == kDrumMidiNotes[kVoiceCrash])   fCrash.trigger(vel01);
        else if (note == kDrumMidiNotes[kVoiceRide])    fRide.trigger(vel01);
        else if (note == kDrumMidiNotes[kVoiceRim])     fRim.trigger(vel01);
        else if (note == kDrumMidiNotes[kVoiceClap])    fClap.trigger(vel01);
        else if (note == kDrumMidiNotes[kVoiceMaracas]) fMaracas.trigger(vel01);
    }

    float fValues[kParamCount] {};

    float fMasterVolume = 0.8f;
    Distortion fDistortion;

    PitchedDrumVoice fKick;
    float fKickLevel = 0.95f;

    SnareVoice fSnare;
    float fSnareLevel = 0.85f;

    CymbalVoice fHatClosed, fHatOpen;
    float fHatClosedLevel = 0.75f, fHatOpenLevel = 0.75f;

    PitchedDrumVoice fTomLow, fTomMid, fTomHigh;
    float fTomLowLevel = 0.85f, fTomMidLevel = 0.85f, fTomHighLevel = 0.85f;

    CymbalVoice fCrash, fRide;
    float fCrashLevel = 0.8f, fRideLevel = 0.75f;

    ClickVoice fRim;
    float fRimLevel = 0.8f;

    ClapVoice fClap;
    float fClapLevel = 0.8f;

    NoiseVoice fMaracas;
    float fMaracasLevel = 0.7f;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SideousDrumsPlugin)
};

// -----------------------------------------------------------------------------------------------------------

Plugin* createPlugin()
{
    return new SideousDrumsPlugin();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
