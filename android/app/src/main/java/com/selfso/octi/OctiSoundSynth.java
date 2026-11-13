/*
 * OctiSoundSynth.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.EnumMap;
import java.util.List;
import java.util.Map;

public class OctiSoundSynth {

    public enum SoundCue {
        POD_PLACE,
        PRONG_SNAP,
        POD_STEP,
        POD_JUMP,
        POD_CAPTURE,
        CLOCK_LOW_TICK,
        GAME_VICTORY;

        public static final SoundCue POD_DROP = POD_PLACE;
    }

    public static final class Cue {
        public static final SoundCue POD_PLACE = SoundCue.POD_PLACE;
        public static final SoundCue POD_DROP = SoundCue.POD_PLACE;
        public static final SoundCue PRONG_SNAP = SoundCue.PRONG_SNAP;
        public static final SoundCue POD_STEP = SoundCue.POD_STEP;
        public static final SoundCue POD_JUMP = SoundCue.POD_JUMP;
        public static final SoundCue POD_CAPTURE = SoundCue.POD_CAPTURE;
        public static final SoundCue CLOCK_LOW_TICK = SoundCue.CLOCK_LOW_TICK;
        public static final SoundCue GAME_VICTORY = SoundCue.GAME_VICTORY;

        public static SoundCue[] values() {
            return SoundCue.values();
        }

        public static SoundCue valueOf(String name) {
            return SoundCue.valueOf(name);
        }
    }

    private static final OctiSoundSynth sDefaultInstance = new OctiSoundSynth();

    public static OctiSoundSynth getDefaultInstance() {
        return sDefaultInstance;
    }

    public static void playCue(SoundCue cue, boolean soundEnabled) {
        if (!soundEnabled || cue == null) {
            return;
        }
        sDefaultInstance.triggerCue(cue);
    }

    public interface PlaybackCallback {
        void onPlayPcm(short[] samples);
    }

    public static final int SAMPLE_RATE = 16000;

    private volatile boolean muted = false;
    private long triggeredCount = 0;
    private SoundCue lastCue = SoundCue.POD_PLACE;
    private PlaybackCallback playbackCallback = null;
    private final Object[] trackPool = new Object[4];
    private int trackPoolCursor = 0;
    private final Map<SoundCue, byte[]> cachedCueWav = new EnumMap<>(SoundCue.class);
    private final Map<SoundCue, short[]> cachedCuePcm = new EnumMap<>(SoundCue.class);

    public OctiSoundSynth() {
        for (SoundCue cue : SoundCue.values()) {
            byte[] wav = loadCueWavBytes(cue);
            int[] sampleRate = new int[1];
            short[] pcm = decodeWavPcm16Mono(wav, sampleRate);
            if (sampleRate[0] != SAMPLE_RATE) {
                throw new IllegalStateException(
                        "Unexpected WAV sample rate for " + cue + ": " + sampleRate[0]);
            }
            cachedCueWav.put(cue, wav);
            cachedCuePcm.put(cue, pcm);
        }
    }

    public static String assetFileNameForCue(SoundCue cue) {
        if (cue == null) {
            return "";
        }
        switch (cue) {
            case POD_PLACE:
                return "pod_place.wav";
            case PRONG_SNAP:
                return "prong_snap.wav";
            case POD_STEP:
                return "pod_step.wav";
            case POD_JUMP:
                return "pod_jump.wav";
            case POD_CAPTURE:
                return "pod_capture.wav";
            case CLOCK_LOW_TICK:
                return "clock_low_tick.wav";
            case GAME_VICTORY:
                return "game_victory.wav";
            default:
                return "";
        }
    }

    public byte[] wavBytesForCue(SoundCue cue) {
        if (cue == null) {
            cue = SoundCue.POD_PLACE;
        }
        byte[] cached = cachedCueWav.get(cue);
        if (cached != null) {
            return cached.clone();
        }
        return loadCueWavBytes(cue);
    }

    public short[] synthesizeCuePcm(SoundCue cue) {
        if (cue == null) {
            cue = SoundCue.POD_PLACE;
        }
        short[] cached = cachedCuePcm.get(cue);
        if (cached != null) {
            return cached.clone();
        }
        return decodeWavPcm16Mono(wavBytesForCue(cue));
    }

    private static byte[] readAllStreamBytes(InputStream in) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buf = new byte[2048];
        int n;
        while ((n = in.read(buf)) != -1) {
            out.write(buf, 0, n);
        }
        return out.toByteArray();
    }

    private static byte[] loadCueWavBytes(SoundCue cue) {
        String fileName = assetFileNameForCue(cue);
        if (fileName.isEmpty()) {
            throw new IllegalArgumentException("Unknown SoundCue: " + cue);
        }

        String resourcePath = "/sounds/" + fileName;
        try (InputStream in = OctiSoundSynth.class.getResourceAsStream(resourcePath)) {
            if (in != null) {
                byte[] data = readAllStreamBytes(in);
                if (data.length > 0) {
                    return data;
                }
            }
        } catch (IOException e) {
            throw new IllegalStateException("Failed reading classpath sound resource " + resourcePath, e);
        }

        ClassLoader cl = OctiSoundSynth.class.getClassLoader();
        if (cl != null) {
            try (InputStream in = cl.getResourceAsStream("sounds/" + fileName)) {
                if (in != null) {
                    byte[] data = readAllStreamBytes(in);
                    if (data.length > 0) {
                        return data;
                    }
                }
            } catch (IOException e) {
                throw new IllegalStateException("Failed reading classloader sound resource sounds/" + fileName, e);
            }
        }

        String[] relativeCandidates = new String[]{
            "assets/sounds/" + fileName,
            "../assets/sounds/" + fileName,
            "../../assets/sounds/" + fileName
        };
        for (String rel : relativeCandidates) {
            File f = new File(rel);
            if (f.isFile()) {
                try {
                    return Files.readAllBytes(f.toPath());
                } catch (IOException e) {
                    throw new IllegalStateException("Failed reading fallback sound file " + rel, e);
                }
            }
        }

        File cur = new File(System.getProperty("user.dir", ".")).getAbsoluteFile();
        while (cur != null) {
            File candidate = new File(cur, "assets/sounds/" + fileName);
            if (candidate.isFile()) {
                try {
                    return Files.readAllBytes(candidate.toPath());
                } catch (IOException e) {
                    throw new IllegalStateException("Failed reading fallback sound file " + fileName, e);
                }
            }
            cur = cur.getParentFile();
        }

        throw new IllegalStateException("Shared sound asset not found for " + cue + " (" + fileName + ")");
    }

    private static int readLe16Unsigned(byte[] buf, int offset) {
        return (buf[offset] & 0xFF) | ((buf[offset + 1] & 0xFF) << 8);
    }

    private static long readLe32Unsigned(byte[] buf, int offset) {
        return ((long) (buf[offset] & 0xFF))
                | (((long) (buf[offset + 1] & 0xFF)) << 8)
                | (((long) (buf[offset + 2] & 0xFF)) << 16)
                | (((long) (buf[offset + 3] & 0xFF)) << 24);
    }

    private static boolean matchesAscii4(byte[] buf, int offset, String fourCc) {
        byte[] expected = fourCc.getBytes(StandardCharsets.US_ASCII);
        return expected.length == 4
                && offset + 4 <= buf.length
                && buf[offset] == expected[0]
                && buf[offset + 1] == expected[1]
                && buf[offset + 2] == expected[2]
                && buf[offset + 3] == expected[3];
    }

    public static short[] decodeWavPcm16Mono(byte[] wavData) {
        return decodeWavPcm16Mono(wavData, null);
    }

    public static short[] decodeWavPcm16Mono(byte[] wavData, int[] outSampleRate) {
        if (outSampleRate != null && outSampleRate.length > 0) {
            outSampleRate[0] = 0;
        }
        if (wavData == null || wavData.length < 44) {
            throw new IllegalArgumentException("WAV buffer is null or shorter than 44 bytes");
        }
        if (!matchesAscii4(wavData, 0, "RIFF") || !matchesAscii4(wavData, 8, "WAVE")) {
            throw new IllegalArgumentException("Invalid RIFF/WAVE header magic");
        }

        long riffSize = readLe32Unsigned(wavData, 4);
        if (riffSize < 36L || riffSize > (long) (wavData.length - 8)) {
            throw new IllegalArgumentException("Invalid RIFF chunk size: " + riffSize);
        }

        int riffEnd = 8 + (int) riffSize;
        int offset = 12;
        boolean seenFmt = false;
        boolean seenData = false;
        int parsedSampleRate = 0;
        short[] parsedPcm = null;

        while (offset < riffEnd) {
            if (riffEnd - offset < 8) {
                throw new IllegalArgumentException("Truncated RIFF subchunk header");
            }

            long chunkSize = readLe32Unsigned(wavData, offset + 4);
            int payloadOffset = offset + 8;
            if (chunkSize > (long) (riffEnd - payloadOffset)) {
                throw new IllegalArgumentException("RIFF subchunk size overflows container bounds");
            }

            if (matchesAscii4(wavData, offset, "fmt ")) {
                if (seenFmt || chunkSize < 16L) {
                    throw new IllegalArgumentException("Malformed or duplicate fmt chunk");
                }
                int audioFormat = readLe16Unsigned(wavData, payloadOffset);
                int numChannels = readLe16Unsigned(wavData, payloadOffset + 2);
                long sampleRate = readLe32Unsigned(wavData, payloadOffset + 4);
                long byteRate = readLe32Unsigned(wavData, payloadOffset + 8);
                int blockAlign = readLe16Unsigned(wavData, payloadOffset + 12);
                int bitsPerSample = readLe16Unsigned(wavData, payloadOffset + 14);

                if (audioFormat != 1
                        || numChannels != 1
                        || bitsPerSample != 16
                        || blockAlign != 2
                        || sampleRate == 0L
                        || sampleRate > 0x3FFFFFFFL
                        || byteRate != sampleRate * 2L) {
                    throw new IllegalArgumentException("Unsupported WAV fmt parameters (expected PCM16 mono)");
                }
                parsedSampleRate = (int) sampleRate;
                seenFmt = true;
            } else if (matchesAscii4(wavData, offset, "data")) {
                if (!seenFmt || seenData || (chunkSize & 1L) != 0L) {
                    throw new IllegalArgumentException("Invalid or misaligned WAV data chunk");
                }
                int sampleCount = (int) (chunkSize / 2L);
                parsedPcm = new short[sampleCount];
                for (int i = 0; i < sampleCount; i++) {
                    parsedPcm[i] = (short) readLe16Unsigned(wavData, payloadOffset + i * 2);
                }
                seenData = true;
            }

            int step = (int) chunkSize;
            offset = payloadOffset + step;
            if ((step & 1) != 0 && offset < riffEnd) {
                offset++;
            }
        }

        if (!seenFmt || !seenData || parsedPcm == null) {
            throw new IllegalArgumentException("WAV missing fmt or data chunk");
        }
        if (outSampleRate != null && outSampleRate.length > 0) {
            outSampleRate[0] = parsedSampleRate;
        }
        return parsedPcm;
    }

    private static long wrappedAbsDelta(int from, int to, boolean edgeless, int span) {
        long d = (long) to - (long) from;
        if (edgeless && span > 2) {
            long s = (long) span;
            d %= s;
            if (d > s / 2L) {
                d -= s;
            } else if (d < -(s / 2L)) {
                d += s;
            }
        }
        return d < 0L ? -d : d;
    }

    private static SoundCue classifyOps(
            List<OctiMoveOp> ops,
            boolean edgeless,
            int boardWidth,
            int boardHeight) {
        if (ops == null || ops.isEmpty()) {
            return SoundCue.POD_PLACE;
        }

        for (OctiMoveOp op : ops) {
            for (OctiHop hop : op.hops) {
                if (hop.capture) {
                    return SoundCue.POD_CAPTURE;
                }
            }
        }

        for (OctiMoveOp op : ops) {
            int curX = op.x0;
            int curY = op.y0;
            for (OctiHop hop : op.hops) {
                long dx = wrappedAbsDelta(curX, hop.x, edgeless, boardWidth);
                long dy = wrappedAbsDelta(curY, hop.y, edgeless, boardHeight);
                long cheb = Math.max(dx, dy);
                if (cheb == 2L) {
                    return SoundCue.POD_JUMP;
                }
                curX = hop.x;
                curY = hop.y;
            }
        }

        boolean hasRegStep = false;
        boolean hasRegProng = false;
        boolean hasRegPod = false;
        boolean hasBonusStep = false;

        for (OctiMoveOp op : ops) {
            if (op.isSuperProngBonusMove) {
                if (!op.hops.isEmpty()) {
                    hasBonusStep = true;
                }
            } else {
                if (!op.hops.isEmpty()) {
                    hasRegStep = true;
                }
                if (op.addProngDir != OctiDir.DIR_UNDEF) {
                    hasRegProng = true;
                }
                if (op.addPod) {
                    hasRegPod = true;
                }
            }
        }

        if (hasRegStep) {
            return SoundCue.POD_STEP;
        }
        if (hasRegProng) {
            return SoundCue.PRONG_SNAP;
        }
        if (hasRegPod) {
            return SoundCue.POD_PLACE;
        }
        if (hasBonusStep) {
            return SoundCue.POD_STEP;
        }
        return SoundCue.POD_PLACE;
    }

    public static SoundCue classifyMoveSoundCue(
            OctiMove move,
            boolean edgeless,
            int boardW,
            int boardH) {
        if (move == null) {
            return SoundCue.POD_PLACE;
        }
        return classifyOps(move.getOps(), edgeless, boardW, boardH);
    }

    public static SoundCue classifyMoveNotationSoundCue(String notation) {
        return classifyMoveNotationSoundCue(notation, false, 9, 9);
    }

    public static SoundCue classifyMoveNotationSoundCue(
            String notation,
            boolean edgeless,
            int boardW,
            int boardH) {
        if (notation == null || notation.isEmpty() || notation.indexOf('\0') >= 0) {
            return SoundCue.POD_PLACE;
        }
        List<OctiMoveOp> ops = new ArrayList<>();
        try {
            OctiMove.parseMoveString(notation, ops);
        } catch (Throwable ignored) {
            return SoundCue.POD_PLACE;
        }
        return classifyOps(ops, edgeless, boardW, boardH);
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

    private synchronized void tryPlayAndroidAudioTrack(short[] samples) {
        if (samples == null || samples.length == 0) {
            return;
        }
        try {
            Class<?> audioTrackClass = Class.forName("android.media.AudioTrack");
            // Only attempt playback if running on actual Android runtime (not unit-test stub)
            String vmName = System.getProperty("java.vm.name", "");
            if (!vmName.toLowerCase().contains("dalvik")
                    && !vmName.toLowerCase().contains("art")) {
                return;
            }

            int pcmBytes = samples.length * 2;
            int minBufBytes = android.media.AudioTrack.getMinBufferSize(
                    SAMPLE_RATE,
                    android.media.AudioFormat.CHANNEL_OUT_MONO,
                    android.media.AudioFormat.ENCODING_PCM_16BIT);

            int slot = trackPoolCursor;
            trackPoolCursor = (trackPoolCursor + 1) % trackPool.length;
            if (trackPool[slot] instanceof android.media.AudioTrack) {
                try {
                    ((android.media.AudioTrack) trackPool[slot]).release();
                } catch (Throwable ignored) {
                }
                trackPool[slot] = null;
            }

            android.media.AudioAttributes attrs = new android.media.AudioAttributes.Builder()
                    .setUsage(android.media.AudioAttributes.USAGE_GAME)
                    .setContentType(android.media.AudioAttributes.CONTENT_TYPE_MUSIC)
                    .build();
            android.media.AudioFormat format = new android.media.AudioFormat.Builder()
                    .setEncoding(android.media.AudioFormat.ENCODING_PCM_16BIT)
                    .setSampleRate(SAMPLE_RATE)
                    .setChannelMask(android.media.AudioFormat.CHANNEL_OUT_MONO)
                    .build();

            // 1. Primary: MODE_STATIC with exact static buffer size (samples.length * 2)
            android.media.AudioTrack track = new android.media.AudioTrack(
                    attrs,
                    format,
                    pcmBytes,
                    android.media.AudioTrack.MODE_STATIC,
                    android.media.AudioManager.AUDIO_SESSION_ID_GENERATE);

            boolean played = false;
            // MODE_STATIC track starts in STATE_NO_STATIC_DATA (2). It only transitions
            // to STATE_INITIALIZED (1) after track.write() loads the full buffer.
            if (track.getState() != android.media.AudioTrack.STATE_UNINITIALIZED) {
                int written = track.write(samples, 0, samples.length);
                if (written > 0 && track.getState() == android.media.AudioTrack.STATE_INITIALIZED) {
                    track.play();
                    trackPool[slot] = track;
                    played = true;
                }
            }

            // 2. Fallback: MODE_STREAM if MODE_STATIC fails to reach STATE_INITIALIZED
            if (!played) {
                try {
                    track.release();
                } catch (Throwable ignored) {
                }
                int streamBufSize = Math.max(pcmBytes, minBufBytes > 0 ? minBufBytes : pcmBytes);
                android.media.AudioTrack streamTrack = new android.media.AudioTrack(
                        attrs,
                        format,
                        streamBufSize,
                        android.media.AudioTrack.MODE_STREAM,
                        android.media.AudioManager.AUDIO_SESSION_ID_GENERATE);
                if (streamTrack.getState() == android.media.AudioTrack.STATE_INITIALIZED) {
                    streamTrack.play();
                    streamTrack.write(samples, 0, samples.length);
                    trackPool[slot] = streamTrack;
                } else {
                    try {
                        streamTrack.release();
                    } catch (Throwable ignored) {
                    }
                }
            }
        } catch (Throwable ignored) {
            // Headless JVM unit test environment or audio device unavailable
        }
    }

    public synchronized void shutdown() {
        for (int i = 0; i < trackPool.length; i++) {
            if (trackPool[i] instanceof android.media.AudioTrack) {
                try {
                    ((android.media.AudioTrack) trackPool[i]).release();
                } catch (Throwable ignored) {
                }
                trackPool[i] = null;
            }
        }
    }

    public long triggeredCueCount() {
        return triggeredCount;
    }

    public SoundCue lastTriggeredCue() {
        return lastCue;
    }
}

/*
 * Local variables:
 * mode: Java
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
