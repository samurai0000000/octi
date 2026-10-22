/*
 * OctiSoundSynth.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiSoundSynth.hxx"
#include <algorithm>
#include <cmath>

namespace octi {

namespace {

constexpr double TWO_PI = 6.28318530717958647692;

std::int16_t clampPcm16(double sample)
{
    double scaled = std::clamp(sample, -1.0, 1.0) * 30000.0;
    return static_cast<std::int16_t>( std::lrint(scaled) );
}

} // namespace

OctiSoundSynth::OctiSoundSynth()
    : _muted(false)
    , _triggeredCount(0)
    , _lastCue(SoundCue::POD_PLACE)
{
}

OctiSoundSynth::~OctiSoundSynth() = default;

std::vector<std::int16_t> OctiSoundSynth::synthesizeCuePcm(SoundCue cue) const
{
    int durationMs = 110;
    switch (cue) {
    case SoundCue::POD_PLACE:      durationMs = 120; break;
    case SoundCue::PRONG_SNAP:     durationMs = 75;  break;
    case SoundCue::POD_STEP:       durationMs = 95;  break;
    case SoundCue::POD_JUMP:       durationMs = 150; break;
    case SoundCue::POD_CAPTURE:    durationMs = 210; break;
    case SoundCue::CLOCK_LOW_TICK: durationMs = 45;  break;
    case SoundCue::GAME_VICTORY:   durationMs = 360; break;
    }

    std::size_t numSamples = static_cast<std::size_t>((SAMPLE_RATE * durationMs) / 1000);
    std::vector<std::int16_t> pcm(numSamples, 0);

    for (std::size_t i = 0; i < numSamples; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(SAMPLE_RATE);
        double progress = static_cast<double>(i) / static_cast<double>(std::max<std::size_t>(1, numSamples));
        double attack = std::min(1.0, static_cast<double>(i) / 48.0);
        double val = 0.0;

        switch (cue) {
        case SoundCue::POD_PLACE: {
            // Warm resonant wood/slate tabletop thud with pitch drop
            double freq = 145.0 * std::exp(-t * 14.0) + 68.0;
            double env = attack * std::exp(-t * 24.0);
            val = env * (0.72 * std::sin(TWO_PI * freq * t) +
                         0.28 * std::sin(TWO_PI * (freq * 1.9) * t));
            break;
        }
        case SoundCue::PRONG_SNAP: {
            // Crisp metallic brass click
            double env = attack * std::exp(-t * 48.0);
            val = env * (0.50 * std::sin(TWO_PI * 1420.0 * t) +
                         0.35 * std::sin(TWO_PI * 2680.0 * t) +
                         0.15 * std::sin(TWO_PI * 3920.0 * t));
            break;
        }
        case SoundCue::POD_STEP: {
            // Soft felt/wood glide tap
            double freq = 195.0 * std::exp(-t * 16.0) + 85.0;
            double env = attack * std::exp(-t * 30.0);
            val = env * (0.75 * std::sin(TWO_PI * freq * t) +
                         0.25 * std::sin(TWO_PI * (freq * 2.1) * t));
            break;
        }
        case SoundCue::POD_JUMP: {
            // Rising two-note harmonic arc
            double freq = (progress < 0.45) ? 440.0 : 587.33;
            double env = attack * std::exp(-t * 14.0);
            val = env * (0.68 * std::sin(TWO_PI * freq * t) +
                         0.32 * std::sin(TWO_PI * (freq * 1.5) * t));
            break;
        }
        case SoundCue::POD_CAPTURE: {
            // Crisp capture chime triad (A4 -> C#5 -> E5)
            double freq = (progress < 0.33) ? 440.0 :
                          (progress < 0.66) ? 554.37 : 659.25;
            double env = attack * std::exp(-t * 10.0);
            val = env * (0.65 * std::sin(TWO_PI * freq * t) +
                         0.25 * std::sin(TWO_PI * (freq * 2.0) * t) +
                         0.10 * std::sin(TWO_PI * (freq * 3.0) * t));
            break;
        }
        case SoundCue::CLOCK_LOW_TICK: {
            // Short precision escapement tick
            double env = attack * std::exp(-t * 75.0);
            val = env * std::sin(TWO_PI * 1050.0 * t);
            break;
        }
        case SoundCue::GAME_VICTORY: {
            // Grandmaster 4-note brass/chime fanfare (C5 - E5 - G5 - C6)
            double freq = (progress < 0.22) ? 523.25 :
                          (progress < 0.44) ? 659.25 :
                          (progress < 0.68) ? 783.99 : 1046.50;
            double env = attack * std::exp(-t * 5.5);
            val = env * (0.60 * std::sin(TWO_PI * freq * t) +
                         0.28 * std::sin(TWO_PI * (freq * 1.5) * t) +
                         0.12 * std::sin(TWO_PI * (freq * 2.0) * t));
            break;
        }
        }

        pcm[i] = clampPcm16(val);
    }

    return pcm;
}

void OctiSoundSynth::setMuted(bool muted)
{
    _muted = muted;
}

bool OctiSoundSynth::isMuted() const
{
    return _muted;
}

void OctiSoundSynth::setPlaybackCallback(
    std::function<void(const std::vector<std::int16_t> &)> cb)
{
    _playbackCallback = std::move(cb);
}

void OctiSoundSynth::triggerCue(SoundCue cue)
{
    if (_muted) {
        return;
    }

    ++_triggeredCount;
    _lastCue = cue;

    if (_playbackCallback) {
        std::vector<std::int16_t> samples = synthesizeCuePcm(cue);
        _playbackCallback(samples);
    }
}

std::uint64_t OctiSoundSynth::triggeredCueCount() const
{
    return _triggeredCount;
}

SoundCue OctiSoundSynth::lastTriggeredCue() const
{
    return _lastCue;
}

} // namespace octi

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
