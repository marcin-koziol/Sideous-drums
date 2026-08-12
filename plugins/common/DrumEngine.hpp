/*
 * Sideous Drums - the actual 12-voice drum kit engine: params, triggering
 * (incl. the hi-hat choke group), and per-voice sample processing. Shared
 * between the stereo-mix plugin and the multi-out plugin so the two variants
 * can never drift apart on voice behavior - they differ only in how they
 * route processVoices()'s output to their audio ports (see each variant's
 * Plugin.cpp). Deliberately DPF-agnostic (no DistrhoPlugin.hpp dependency)
 * so it's plain, host-independent DSP.
 */

#pragma once

#include <cmath>
#include <cstdint>

#include "Params.hpp"
#include "DrumMap.hpp"
#include "dsp/PitchedDrumVoice.hpp"
#include "dsp/SnareVoice.hpp"
#include "dsp/CymbalVoice.hpp"
#include "dsp/ClickVoice.hpp"
#include "dsp/ClapVoice.hpp"
#include "dsp/NoiseVoice.hpp"
#include "dsp/Distortion.hpp"

namespace sideous {

// raw MIDI byte parsing, used identically by both plugin variants' run()
// loops - kept DPF-agnostic (takes raw bytes, not DISTRHO::MidiEvent) so
// this header doesn't need to depend on DPF at all.
inline bool parseNoteOnEvent(const uint8_t* data, uint32_t size, int& note, float& vel01) noexcept
{
    if (size < 2 || size > 3)
        return false;

    const uint8_t status = data[0] & 0xF0;
    const uint8_t velocity = size > 2 ? data[2] : 0;

    if (status != 0x90 || velocity == 0)
        return false; // note-off (or note-on w/ velocity 0): these voices are one-shots, ignored

    note = data[1];
    vel01 = (float)velocity / 127.0f;
    return true;
}

class DrumEngine
{
public:
    // one sample's worth of every voice, already scaled by its own Level
    // knob - the stereo plugin sums these itself, the multi-out plugin
    // additionally writes each field to its own dedicated output bus
    struct VoiceOutputs
    {
        float kick, snare, hatClosed, hatOpen;
        float tomLow, tomMid, tomHigh;
        float crash, ride, rim, clap, maracas;

        float sum() const noexcept
        {
            return kick + snare + hatClosed + hatOpen + tomLow + tomMid + tomHigh
                 + crash + ride + rim + clap + maracas;
        }
    };

    DrumEngine() noexcept
    {
        // fixed tasteful pitch-envelope character for the toms, not
        // user-exposed (only the kick exposes Punch Depth/Decay - see
        // sideous-drums-plan.md's call-out that this is the single
        // highest-value "not just basic" feature, kept front-and-center on
        // the kick while toms stay a simple 3-knob column)
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

        // Ride gets a tighter, more harmonically-related partial cluster (a
        // fundamental, a close detuned pair for shimmer, near-integer
        // overtones) instead of Crash/hi-hats' wide inharmonic spread - gives
        // the underlying FM stack itself a more "pitched" character even
        // before the dedicated Bell layer is mixed in, see CymbalVoice
        static constexpr float kRidePartials[6] = { 400.0f, 410.0f, 802.0f, 1198.0f, 1585.0f, 2003.0f };
        fRide.setPartials(kRidePartials);

        // every Noise instance defaults to the same seed (see Noise.hpp), and every
        // voice's process() runs every sample regardless of trigger state - so left
        // unseeded, every noise-based voice produces literally the same sample-for-
        // sample signal forever, just through different filters. Two voices playing
        // together (e.g. clap + hi-hat) then sound correlated/phasey instead of like
        // two independent noise sources, since they effectively are the same source.
        // Distinct seeds decorrelate them.
        fSnare.setNoiseSeed(0x2545F491u);
        fHatClosed.setNoiseSeed(0x9E3779B1u);
        fHatOpen.setNoiseSeed(0x85EBCA6Bu);
        fCrash.setNoiseSeed(0xC2B2AE35u);
        fRide.setNoiseSeed(0x27D4EB2Fu);
        fRim.setNoiseSeed(0x165667B1u);
        fClap.setNoiseSeed(0xD3A2646Cu);
        fMaracas.setNoiseSeed(0xFD7046C5u);

        for (uint32_t i = 0; i < kParamCount; ++i)
            setParameterValue(i, getParamInfo(i).def);
    }

    void setSampleRate(double sampleRate) noexcept
    {
        fKick.setSampleRate(sampleRate);
        fSnare.setSampleRate(sampleRate);
        fHatClosed.setSampleRate(sampleRate);
        fHatOpen.setSampleRate(sampleRate);
        fTomLow.setSampleRate(sampleRate);
        fTomMid.setSampleRate(sampleRate);
        fTomHigh.setSampleRate(sampleRate);
        fCrash.setSampleRate(sampleRate);
        fRide.setSampleRate(sampleRate);
        fRim.setSampleRate(sampleRate);
        fClap.setSampleRate(sampleRate);
        fMaracas.setSampleRate(sampleRate);
    }

    float getParameterValue(uint32_t index) const noexcept
    {
        switch (index)
        {
        case kParamKickActive:      return fKick.getLevel();
        case kParamSnareActive:     return fSnare.getLevel();
        case kParamHatClosedActive: return fHatClosed.getLevel();
        case kParamHatOpenActive:   return fHatOpen.getLevel();
        case kParamTomLowActive:    return fTomLow.getLevel();
        case kParamTomMidActive:    return fTomMid.getLevel();
        case kParamTomHighActive:   return fTomHigh.getLevel();
        case kParamCrashActive:     return fCrash.getLevel();
        case kParamRideActive:      return fRide.getLevel();
        case kParamRimActive:       return fRim.getLevel();
        case kParamClapActive:      return fClap.getLevel();
        case kParamMaracasActive:   return fMaracas.getLevel();
        default:                    return fValues[index];
        }
    }

    void setParameterValue(uint32_t index, float value) noexcept
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
        case kParamHatClosedMetal:  fHatClosed.setMetal(value); break;
        case kParamHatClosedTune:   fHatClosed.setTune(value); break;
        case kParamHatClosedLevel:  fHatClosedLevel = value; break;

        case kParamHatOpenDecay:    fHatOpen.setDecay(value); break;
        case kParamHatOpenTone:     fHatOpen.setTone(value); break;
        case kParamHatOpenMetal:    fHatOpen.setMetal(value); break;
        case kParamHatOpenTune:     fHatOpen.setTune(value); break;
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

        case kParamCrashTune:       fCrash.setTune(value); break;
        case kParamCrashMetal:      fCrash.setMetal(value); break;
        case kParamRideTune:        fRide.setTune(value); break;
        case kParamRideMetal:       fRide.setMetal(value); break;
        case kParamRideBellMix:     fRide.setBellMix(value); break;

        case kParamRimTune:         fRim.setTune(value); break;
        case kParamRimLevel:        fRimLevel = value; break;

        case kParamClapDecay:       fClap.setDecay(value); break;
        case kParamClapLevel:       fClapLevel = value; break;
        case kParamClapTone:        fClap.setTone(value); break;
        case kParamClapHands:       fClap.setHands(value); break;

        case kParamMaracasDecay:    fMaracas.setDecay(value); break;
        case kParamMaracasLevel:    fMaracasLevel = value; break;
        case kParamMaracasRattle:   fMaracas.setRattle(value); break;

        default: break;
        }
    }

    void trigger(int note, float vel01) noexcept
    {
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

    VoiceOutputs processVoices() noexcept
    {
        VoiceOutputs vo;
        vo.kick      = fKickLevel      * fKick.process();
        vo.snare     = fSnareLevel     * fSnare.process();
        vo.hatClosed = fHatClosedLevel * fHatClosed.process();
        vo.hatOpen   = fHatOpenLevel   * fHatOpen.process();
        vo.tomLow    = fTomLowLevel    * fTomLow.process();
        vo.tomMid    = fTomMidLevel    * fTomMid.process();
        vo.tomHigh   = fTomHighLevel   * fTomHigh.process();
        vo.crash     = fCrashLevel     * fCrash.process();
        vo.ride      = fRideLevel      * fRide.process();
        vo.rim       = fRimLevel       * fRim.process();
        vo.clap      = fClapLevel      * fClap.process();
        vo.maracas   = fMaracasLevel   * fMaracas.process();
        return vo;
    }

    // master bus processing (drive/grit + volume + safety clamp), applied
    // only to the *summed* mix - the multi-out plugin's individual voice
    // busses skip this (see SideousDrumsMultiPlugin.cpp): a lone voice at
    // its own Level knob can't exceed sane peaks the way several drums
    // summed together can, and re-applying the "glue" Drive stage per voice
    // would just be a different, unwanted kind of distortion on each one.
    float applyMasterBus(float mix) noexcept
    {
        mix = fDistortion.process(mix);
        mix *= fMasterVolume;
        // several drums hitting at once can otherwise sum past 0dBFS
        return std::tanh(mix);
    }

    float masterVolume() const noexcept { return fMasterVolume; }

private:
    static constexpr float kTomPitchEnvDepth = 10.0f;
    static constexpr float kTomPitchEnvDecay = 0.03f;

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
};

} // namespace sideous
