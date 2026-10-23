/*
 * OctiSoundSynth.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public class OctiSoundSynth {

    public enum SoundCue {
        POD_PLACE,
        PRONG_SNAP,
        POD_STEP,
        POD_JUMP,
        POD_CAPTURE,
        CLOCK_LOW_TICK,
        GAME_VICTORY
    }

    public interface PlaybackCallback {
        void onPlayPcm(short[] samples);
    }

    public static final int SAMPLE_RATE = 16000;
    private static final double TWO_PI = 2.0 * Math.PI;

    private volatile boolean muted = false;
    private long triggeredCount = 0;
    private SoundCue lastCue = SoundCue.POD_PLACE;
    private PlaybackCallback playbackCallback = null;

    public OctiSoundSynth() {
    }

    private static short clampPcm16(double sample) {
        double clamped = Math.max(-1.0, Math.min(1.0, sample));
        return (short) Math.round(clamped * 30000.0);
    }

    public short[] synthesizeCuePcm(SoundCue cue) {
        if (cue == null) {
            cue = SoundCue.POD_PLACE;
        }

        int durationMs = 110;
        switch (cue) {
            case POD_PLACE:
                durationMs = 120;
                break;
            case PRONG_SNAP:
                durationMs = 75;
                break;
            case POD_STEP:
                durationMs = 95;
                break;
            case POD_JUMP:
                durationMs = 150;
                break;
            case POD_CAPTURE:
                durationMs = 210;
                break;
            case CLOCK_LOW_TICK:
                durationMs = 45;
                break;
            case GAME_VICTORY:
                durationMs = 360;
                break;
        }

        int numSamples = (SAMPLE_RATE * durationMs) / 1000;
        short[] pcm = new short[numSamples];

        for (int i = 0; i < numSamples; i++) {
            double t = (double) i / (double) SAMPLE_RATE;
            double progress = (double) i / (double) Math.max(1, numSamples);
            double attack = Math.min(1.0, (double) i / 48.0);
            double val = 0.0;

            switch (cue) {
                case POD_PLACE: {
                    double freq = 145.0 * Math.exp(-t * 14.0) + 68.0;
                    double env = attack * Math.exp(-t * 24.0);
                    val = env * (0.72 * Math.sin(TWO_PI * freq * t)
                            + 0.28 * Math.sin(TWO_PI * (freq * 1.9) * t));
                    break;
                }
                case PRONG_SNAP: {
                    double env = attack * Math.exp(-t * 48.0);
                    val = env * (0.50 * Math.sin(TWO_PI * 1420.0 * t)
                            + 0.35 * Math.sin(TWO_PI * 2680.0 * t)
                            + 0.15 * Math.sin(TWO_PI * 3920.0 * t));
                    break;
                }
                case POD_STEP: {
                    double freq = 195.0 * Math.exp(-t * 16.0) + 85.0;
                    double env = attack * Math.exp(-t * 30.0);
                    val = env * (0.75 * Math.sin(TWO_PI * freq * t)
                            + 0.25 * Math.sin(TWO_PI * (freq * 2.1) * t));
                    break;
                }
                case POD_JUMP: {
                    double freq = (progress < 0.45) ? 440.0 : 587.33;
                    double env = attack * Math.exp(-t * 14.0);
                    val = env * (0.68 * Math.sin(TWO_PI * freq * t)
                            + 0.32 * Math.sin(TWO_PI * (freq * 1.5) * t));
                    break;
                }
                case POD_CAPTURE: {
                    double freq = (progress < 0.33) ? 440.0 :
                                  (progress < 0.66) ? 554.37 : 659.25;
                    double env = attack * Math.exp(-t * 10.0);
                    val = env * (0.65 * Math.sin(TWO_PI * freq * t)
                            + 0.25 * Math.sin(TWO_PI * (freq * 2.0) * t)
                            + 0.10 * Math.sin(TWO_PI * (freq * 3.0) * t));
                    break;
                }
                case CLOCK_LOW_TICK: {
                    double env = attack * Math.exp(-t * 75.0);
                    val = env * Math.sin(TWO_PI * 1050.0 * t);
                    break;
                }
                case GAME_VICTORY: {
                    double freq = (progress < 0.22) ? 523.25 :
                                  (progress < 0.44) ? 659.25 :
                                  (progress < 0.68) ? 783.99 : 1046.50;
                    double env = attack * Math.exp(-t * 5.5);
                    val = env * (0.60 * Math.sin(TWO_PI * freq * t)
                            + 0.28 * Math.sin(TWO_PI * (freq * 1.5) * t)
                            + 0.12 * Math.sin(TWO_PI * (freq * 2.0) * t));
                    break;
                }
            }

            pcm[i] = clampPcm16(val);
        }

        return pcm;
    }

    public void setMuted(boolean muted) {
        this.muted = muted;
    }

    public boolean isMuted() {
        return muted;
    }

    public boolean isSfxMuted() {
        return muted;
    }

    public void setPlaybackCallback(PlaybackCallback cb) {
        this.playbackCallback = cb;
    }

    public void triggerCue(SoundCue cue) {
        if (muted || cue == null) {
            return;
        }

        triggeredCount++;
        lastCue = cue;

        short[] samples = synthesizeCuePcm(cue);
        PlaybackCallback cb = this.playbackCallback;
        if (cb != null) {
            cb.onPlayPcm(samples);
            return;
        }

        tryPlayAndroidAudioTrack(samples);
    }

    private void tryPlayAndroidAudioTrack(short[] samples) {
        try {
            Class<?> audioTrackClass = Class.forName("android.media.AudioTrack");
            // Only attempt playback if running on actual Android runtime (not unit-test stub)
            String vmName = System.getProperty("java.vm.name", "");
            if (!vmName.toLowerCase().contains("dalvik")
                    && !vmName.toLowerCase().contains("art")) {
                return;
            }
            android.media.AudioAttributes attrs = new android.media.AudioAttributes.Builder()
                    .setUsage(android.media.AudioAttributes.USAGE_GAME)
                    .setContentType(android.media.AudioAttributes.CONTENT_TYPE_SONIFICATION)
                    .build();
            android.media.AudioFormat format = new android.media.AudioFormat.Builder()
                    .setEncoding(android.media.AudioFormat.ENCODING_PCM_16BIT)
                    .setSampleRate(SAMPLE_RATE)
                    .setChannelMask(android.media.AudioFormat.CHANNEL_OUT_MONO)
                    .build();
            android.media.AudioTrack track = new android.media.AudioTrack(
                    attrs,
                    format,
                    samples.length * 2,
                    android.media.AudioTrack.MODE_STATIC,
                    android.media.AudioManager.AUDIO_SESSION_ID_GENERATE);
            track.write(samples, 0, samples.length);
            track.play();
        } catch (Throwable ignored) {
            // Headless JVM unit test environment or audio device unavailable
        }
    }

    public long triggeredCueCount() {
        return triggeredCount;
    }

    public SoundCue lastTriggeredCue() {
        return lastCue;
    }
}
