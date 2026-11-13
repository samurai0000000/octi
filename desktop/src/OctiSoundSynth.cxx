/*
 * OctiSoundSynth.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "OctiSoundSynth.hxx"
#include <array>
#include <QGuiApplication>
#include <QtGlobal>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#elif defined(__linux__)
#include <alsa/asoundlib.h>
#endif

namespace octi {

namespace {

const std::vector<std::int16_t> &cachedCuePcm(SoundCue cue)
{
    static const std::array<std::vector<std::int16_t>, OCTI_SOUND_CUE_COUNT> cache = []() {
        std::array<std::vector<std::int16_t>, OCTI_SOUND_CUE_COUNT> arr;
        for (std::size_t i = 0; i < OCTI_SOUND_CUE_COUNT; ++i) {
            SoundCue c = static_cast<SoundCue>(i);
            std::size_t wavSize = 0;
            const std::uint8_t *wavPtr = soundCueWavBytes(c, wavSize);
            int sampleRate = 0;
            if (wavPtr && wavSize > 0) {
                decodeWavPcm16Mono(wavPtr, wavSize, arr[i], sampleRate);
            }
        }
        return arr;
    }();
    std::size_t idx = static_cast<std::size_t>(cue);
    if (idx >= OCTI_SOUND_CUE_COUNT) {
        idx = 0;
    }
    return cache[idx];
}

bool isHardwareAudioAllowed()
{
#ifdef OCTI_DESKTOP_HERMETIC_TEST
    return false;
#else
    const QString envPlatform =
        qEnvironmentVariable("QT_QPA_PLATFORM").trimmed().toLower();
    if (envPlatform.contains(QStringLiteral("offscreen")) ||
        envPlatform.contains(QStringLiteral("minimal"))) {
        return false;
    }
    if (QGuiApplication::instance() != nullptr) {
        const QString plat = QGuiApplication::platformName().trimmed().toLower();
        if (plat.contains(QStringLiteral("offscreen")) ||
            plat.contains(QStringLiteral("minimal"))) {
            return false;
        }
    }
    return true;
#endif
}

} // namespace

OctiSoundSynth::OctiSoundSynth()
    : _muted(false)
    , _triggeredCount(0)
    , _lastCue(SoundCue::POD_PLACE)
{
}

OctiSoundSynth::~OctiSoundSynth()
{
#if defined(__linux__)
    {
        std::lock_guard<std::mutex> lock(_alsaMutex);
        _alsaStop = true;
        _alsaQueue.clear();
    }
    _alsaCv.notify_all();
    if (_alsaThread.joinable()) {
        _alsaThread.join();
    }
#endif
}

std::vector<std::uint8_t> OctiSoundSynth::wavBytesForCue(SoundCue cue) const
{
    std::size_t wavSize = 0;
    const std::uint8_t *wavPtr = soundCueWavBytes(cue, wavSize);
    if (!wavPtr || wavSize == 0) {
        return {};
    }
    return std::vector<std::uint8_t>(wavPtr, wavPtr + wavSize);
}

std::vector<std::int16_t> OctiSoundSynth::synthesizeCuePcm(SoundCue cue) const
{
    return cachedCuePcm(cue);
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
        _playbackCallback(synthesizeCuePcm(cue));
        return;
    }

    if (!isHardwareAudioAllowed()) {
        return;
    }

#ifdef _WIN32
    std::size_t wavSize = 0;
    const std::uint8_t *wavPtr = soundCueWavBytes(cue, wavSize);
    if (wavPtr && wavSize > 0) {
        PlaySoundA(reinterpret_cast<LPCSTR>(wavPtr),
                   nullptr,
                   SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
    }
#elif defined(__linux__)
    enqueueAlsaCue(cue);
#endif
}

#if defined(__linux__)
void OctiSoundSynth::enqueueAlsaCue(SoundCue cue)
{
    std::lock_guard<std::mutex> lock(_alsaMutex);
    if (_alsaStop) {
        return;
    }
    if (!_alsaThreadStarted) {
        _alsaThreadStarted = true;
        _alsaThread = std::thread(&OctiSoundSynth::alsaWorkerLoop, this);
    }
    constexpr std::size_t MAX_QUEUED_CUES = 4;
    if (_alsaQueue.size() >= MAX_QUEUED_CUES) {
        _alsaQueue.pop_front();
    }
    _alsaQueue.push_back(cue);
    _alsaCv.notify_one();
}

void OctiSoundSynth::alsaWorkerLoop()
{
    while (true) {
        SoundCue cue = SoundCue::POD_PLACE;
        {
            std::unique_lock<std::mutex> lock(_alsaMutex);
            _alsaCv.wait(lock, [this]() {
                return _alsaStop || !_alsaQueue.empty();
            });
            if (_alsaStop && _alsaQueue.empty()) {
                break;
            }
            cue = _alsaQueue.front();
            _alsaQueue.pop_front();
        }

        const std::vector<std::int16_t> &samples = cachedCuePcm(cue);
        if (samples.empty()) {
            continue;
        }

        snd_pcm_t *pcm = nullptr;
        if (snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0 || !pcm) {
            continue;
        }
        if (snd_pcm_set_params(pcm,
                               SND_PCM_FORMAT_S16_LE,
                               SND_PCM_ACCESS_RW_INTERLEAVED,
                               1,
                               SAMPLE_RATE,
                               1,
                               50000) >= 0) {
            snd_pcm_uframes_t totalFrames =
                static_cast<snd_pcm_uframes_t>(samples.size());
            snd_pcm_uframes_t offset = 0;
            while (offset < totalFrames) {
                snd_pcm_sframes_t written = snd_pcm_writei(
                    pcm,
                    samples.data() + offset,
                    totalFrames - offset);
                if (written < 0) {
                    written = snd_pcm_recover(pcm, static_cast<int>(written), 1);
                    if (written < 0) {
                        break;
                    }
                } else if (written == 0) {
                    break;
                } else {
                    offset += static_cast<snd_pcm_uframes_t>(written);
                }
            }
            snd_pcm_drain(pcm);
        }
        snd_pcm_close(pcm);
    }
}
#endif

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
