/*
 * OctiSoundSynth.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_OCTI_SOUND_SYNTH_HXX
#define OCTI_DESKTOP_OCTI_SOUND_SYNTH_HXX

#include <cstdint>
#include <functional>
#include <vector>

namespace octi {

enum class SoundCue {
    POD_PLACE      = 0,
    PRONG_SNAP     = 1,
    POD_STEP       = 2,
    POD_JUMP       = 3,
    POD_CAPTURE    = 4,
    CLOCK_LOW_TICK = 5,
    GAME_VICTORY   = 6,
};

class OctiSoundSynth {
public:
    static constexpr int SAMPLE_RATE = 16000;

    OctiSoundSynth();
    ~OctiSoundSynth();

    std::vector<std::int16_t> synthesizeCuePcm(SoundCue cue) const;

    void setMuted(bool muted);
    bool isMuted() const;
    bool isSfxMuted() const { return isMuted(); }

    void setPlaybackCallback(
        std::function<void(const std::vector<std::int16_t> &)> cb);

    void triggerCue(SoundCue cue);

    std::uint64_t triggeredCueCount() const;
    SoundCue lastTriggeredCue() const;

private:
    bool _muted;
    std::uint64_t _triggeredCount;
    SoundCue _lastCue;
    std::function<void(const std::vector<std::int16_t> &)> _playbackCallback;
};

} // namespace octi

#endif // OCTI_DESKTOP_OCTI_SOUND_SYNTH_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
