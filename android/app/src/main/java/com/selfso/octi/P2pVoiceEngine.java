/*
 * P2pVoiceEngine.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;
import java.security.SecureRandom;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Deque;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Strictly Push-to-Talk (PTT) peer-to-peer UDP voice engine wire-compatible
 * with Desktop {@code P2pVoiceEngine}.
 *
 * <ul>
 *   <li>336-byte UDP packet: 16-byte big-endian header ({@code "OCTV"} magic
 *       {@code 0x4F435456}, {@code uint32 seq}, {@code uint32 timestampMs},
 *       {@code uint32 authTag}) + 320 bytes of 16 kHz mono 20 ms G.711
 *       mu-law audio payload.</li>
 *   <li>Strictly gated by {@link #isPushToTalkActive()}: zero UDP packets are
 *       transmitted while PTT is inactive.</li>
 *   <li>Enforces server moderation events ({@code USER_MUTED},
 *       {@code OBSERVER_BANNED}, {@code POLICY_UPDATED (DISABLED)})
 *       immediately.</li>
 * </ul>
 */
public final class P2pVoiceEngine implements AutoCloseable {

    public static final int SAMPLE_RATE_HZ = 16000;
    public static final int FRAME_DURATION_MS = 20;
    public static final int HEADER_BYTES = 16;
    public static final int PAYLOAD_SAMPLES = 320;
    public static final int PACKET_BYTES = HEADER_BYTES + PAYLOAD_SAMPLES;
    public static final int MAGIC_OCTV = 0x4F435456;
    public static final int MAX_SIGNALING_PAYLOAD_BYTES = 4096;
    public static final int MAX_JITTER_BUFFER_FRAMES = 32;

    public static final class VoiceUdpPacket {
        public final byte[] magic = new byte[] { 'O', 'C', 'T', 'V' };
        public int seq;
        public int timestampMs;
        public int authTag;
        public final byte[] payload = new byte[PAYLOAD_SAMPLES];
    }

    public static final class PeerVoiceEndpoint {
        public String playerUuid = "";
        public String udpHost = "127.0.0.1";
        public int udpPort = 0;
        public String sessionKey = "";
        public boolean handshakeComplete = false;
        public boolean micMuted = true;
        public boolean mutedByServer = false;
        public long packetsReceived = 0L;
        public int lastSeq = 0;
        public double lastRms = 0.0;
        public final Deque<short[]> jitterBuffer = new ArrayDeque<>(MAX_JITTER_BUFFER_FRAMES);
    }

    private final OctiNetworkClient _netClient;
    private final Object _mutex = new Object();
    private final SecureRandom _rng = new SecureRandom();

    private String _roomId = "";
    private String _localPlayerUuid = "";
    private String _localSessionKey = "";
    private int _boundUdpPort = 0;
    private DatagramSocket _udpSocket = null;
    private boolean _inVoiceRoom = false;

    private final AtomicBoolean _pttActive = new AtomicBoolean(false);
    private final AtomicBoolean _voiceMutedByServer = new AtomicBoolean(false);
    private final AtomicBoolean _voicePolicyDisabled = new AtomicBoolean(false);
    private final AtomicBoolean _hardwareAudioEnabled = new AtomicBoolean(false);
    private final AtomicBoolean _stopRx = new AtomicBoolean(false);
    private final AtomicBoolean _stopHwCapture = new AtomicBoolean(false);

    private final AtomicInteger _txSeq = new AtomicInteger(1);
    private final AtomicLong _totalSent = new AtomicLong(0L);
    private final AtomicLong _totalReceived = new AtomicLong(0L);
    private final AtomicLong _totalDropped = new AtomicLong(0L);

    private final Map<String, PeerVoiceEndpoint> _peers = new HashMap<>();

    private Thread _rxThread = null;
    private Thread _hwCaptureThread = null;
    private AudioRecord _audioRecord = null;
    private AudioTrack _audioTrack = null;

    public P2pVoiceEngine(OctiNetworkClient netClient) {
        _netClient = netClient;
    }

    public static byte linear16ToUlaw(short pcm) {
        final int kBias = 0x84;
        final int kClip = 32635;

        int sample = pcm;
        int sign = 0;
        if (sample < 0) {
            sign = 0x80;
            sample = -sample;
        }
        if (sample > kClip) {
            sample = kClip;
        }
        sample += kBias;

        int exponent = 7;
        for (int expMask = 0x4000; (sample & expMask) == 0 && exponent > 0;
                --exponent, expMask >>= 1) {
            // Find top bit of biased magnitude.
        }
        int mantissa = (sample >> (exponent + 3)) & 0x0F;
        return (byte) (~(sign | (exponent << 4) | mantissa) & 0xFF);
    }

    public static short ulawToLinear16(byte ulawByte) {
        int ulaw = (~ulawByte) & 0xFF;
        int sign = ulaw & 0x80;
        int exponent = (ulaw >> 4) & 0x07;
        int mantissa = ulaw & 0x0F;
        int t = ((mantissa << 3) + 0x84) << exponent;
        int magnitude = t - 0x84;
        return (short) (sign != 0 ? -magnitude : magnitude);
    }

    public static byte encodeMuLaw(short pcm) {
        return linear16ToUlaw(pcm);
    }

    public static short decodeMuLaw(byte ulawByte) {
        return ulawToLinear16(ulawByte);
    }

    public static byte[] encodeMuLawFrame(short[] pcmSamples) {
        if (pcmSamples == null) {
            return new byte[0];
        }
        byte[] out = new byte[pcmSamples.length];
        for (int i = 0; i < pcmSamples.length; ++i) {
            out[i] = linear16ToUlaw(pcmSamples[i]);
        }
        return out;
    }

    public static short[] decodeMuLawFrame(byte[] ulawBytes) {
        if (ulawBytes == null) {
            return new short[0];
        }
        short[] out = new short[ulawBytes.length];
        for (int i = 0; i < ulawBytes.length; ++i) {
            out[i] = ulawToLinear16(ulawBytes[i]);
        }
        return out;
    }

    public static int computeAuthTag(
            int seq,
            int timestampMs,
            byte[] payload,
            String sessionKey) {
        if (sessionKey == null || sessionKey.isEmpty() ||
                payload == null || payload.length < PAYLOAD_SAMPLES) {
            return 0;
        }

        long hash = 0xcbf29ce484222325L;
        final long prime = 0x100000001b3L;

        hash = (hash ^ ('O' & 0xFFL)) * prime;
        hash = (hash ^ ('C' & 0xFFL)) * prime;
        hash = (hash ^ ('T' & 0xFFL)) * prime;
        hash = (hash ^ ('V' & 0xFFL)) * prime;

        byte[] headerBuf = new byte[8];
        writeU32Be(headerBuf, 0, seq);
        writeU32Be(headerBuf, 4, timestampMs);
        for (int i = 0; i < 8; ++i) {
            hash = (hash ^ (headerBuf[i] & 0xFFL)) * prime;
        }

        byte[] keyBytes = sessionKey.getBytes(StandardCharsets.UTF_8);
        for (int i = 0; i < keyBytes.length; ++i) {
            hash = (hash ^ (keyBytes[i] & 0xFFL)) * prime;
        }

        for (int i = 0; i < PAYLOAD_SAMPLES; ++i) {
            hash = (hash ^ (payload[i] & 0xFFL)) * prime;
        }

        int folded = (int) ((hash >>> 32) ^ (hash & 0xFFFFFFFFL));
        return (folded == 0) ? 0xA5A55A5A : folded;
    }

    public static byte[] serializePacket(VoiceUdpPacket pkt) {
        if (pkt == null) {
            throw new IllegalArgumentException("pkt must not be null");
        }
        byte[] out = new byte[PACKET_BYTES];
        out[0] = 'O';
        out[1] = 'C';
        out[2] = 'T';
        out[3] = 'V';
        writeU32Be(out, 4, pkt.seq);
        writeU32Be(out, 8, pkt.timestampMs);
        writeU32Be(out, 12, pkt.authTag);
        System.arraycopy(pkt.payload, 0, out, HEADER_BYTES, PAYLOAD_SAMPLES);
        return out;
    }

    public static boolean parseAndVerifyPacket(
            byte[] data,
            int len,
            String expectedSessionKey,
            VoiceUdpPacket outPkt) {
        if (data == null || len != PACKET_BYTES || data.length < PACKET_BYTES ||
                expectedSessionKey == null || expectedSessionKey.isEmpty() ||
                outPkt == null) {
            return false;
        }
        if (data[0] != 'O' || data[1] != 'C' || data[2] != 'T' || data[3] != 'V') {
            return false;
        }

        int seq = readU32Be(data, 4);
        int timestampMs = readU32Be(data, 8);
        int wireTag = readU32Be(data, 12);

        byte[] payloadSlice = new byte[PAYLOAD_SAMPLES];
        System.arraycopy(data, HEADER_BYTES, payloadSlice, 0, PAYLOAD_SAMPLES);

        int expectedTag = computeAuthTag(seq, timestampMs, payloadSlice, expectedSessionKey);
        if (wireTag == 0 || wireTag != expectedTag) {
            return false;
        }

        outPkt.magic[0] = 'O';
        outPkt.magic[1] = 'C';
        outPkt.magic[2] = 'T';
        outPkt.magic[3] = 'V';
        outPkt.seq = seq;
        outPkt.timestampMs = timestampMs;
        outPkt.authTag = wireTag;
        System.arraycopy(payloadSlice, 0, outPkt.payload, 0, PAYLOAD_SAMPLES);
        return true;
    }

    public static VoiceUdpPacket parseAndVerifyPacket(byte[] data, String expectedSessionKey) {
        if (data == null) {
            return null;
        }
        VoiceUdpPacket out = new VoiceUdpPacket();
        if (!parseAndVerifyPacket(data, data.length, expectedSessionKey, out)) {
            return null;
        }
        return out;
    }

    public boolean joinVoiceRoom(String roomId, String localPlayerUuid) {
        return joinVoiceRoom(roomId, localPlayerUuid, null);
    }

    public boolean joinVoiceRoom(String roomId, String localPlayerUuid, StringBuilder errorOut) {
        leaveVoiceRoom();

        if (roomId == null || roomId.isEmpty() ||
                localPlayerUuid == null || localPlayerUuid.isEmpty()) {
            if (errorOut != null) {
                errorOut.append("roomId and localPlayerUuid must not be empty");
            }
            return false;
        }

        DatagramSocket sock;
        int boundPort;
        try {
            sock = new DatagramSocket(null);
            sock.setReuseAddress(true);
            sock.setSoTimeout(40);
            sock.bind(new InetSocketAddress(InetAddress.getByName("127.0.0.1"), 0));
            boundPort = sock.getLocalPort();
        } catch (Exception e) {
            if (errorOut != null) {
                errorOut.append("Failed to bind local UDP voice socket: ").append(e.getMessage());
            }
            return false;
        }

        synchronized (_mutex) {
            _roomId = roomId;
            _localPlayerUuid = localPlayerUuid;
            _localSessionKey = generateSessionKeyHex();
            _boundUdpPort = boundPort;
            _udpSocket = sock;
            _inVoiceRoom = true;
            _pttActive.set(false);
            _voiceMutedByServer.set(false);
            _voicePolicyDisabled.set(false);
            _peers.clear();
            openHardwareAudioIfEnabledLocked();
        }

        _stopRx.set(false);
        Thread rx = new Thread(this::receiveLoop, "octi-p2p-voice-rx");
        rx.setDaemon(true);
        _rxThread = rx;
        rx.start();

        if (_netClient != null) {
            _netClient.submitVoiceMeta(roomId, "JOINED_VOICE");
            _netClient.submitVoiceMeta(roomId, "MIC_MUTED");
        }

        return true;
    }

    public void leaveVoiceRoom() {
        String activeRoom;
        boolean wasInRoom;
        DatagramSocket sockToClose;
        Thread rxToJoin;
        Thread hwToJoin;

        synchronized (_mutex) {
            wasInRoom = _inVoiceRoom;
            activeRoom = _roomId;
            _inVoiceRoom = false;
            _pttActive.set(false);
            _boundUdpPort = 0;
            sockToClose = _udpSocket;
            _udpSocket = null;
            _peers.clear();
            closeHardwareAudioLocked();
            rxToJoin = _rxThread;
            _rxThread = null;
            hwToJoin = _hwCaptureThread;
            _hwCaptureThread = null;
        }

        _stopRx.set(true);
        _stopHwCapture.set(true);
        if (sockToClose != null) {
            sockToClose.close();
        }
        if (rxToJoin != null && rxToJoin != Thread.currentThread()) {
            try {
                rxToJoin.join(300L);
            } catch (InterruptedException ignored) {
                Thread.currentThread().interrupt();
            }
        }
        if (hwToJoin != null && hwToJoin != Thread.currentThread()) {
            try {
                hwToJoin.join(300L);
            } catch (InterruptedException ignored) {
                Thread.currentThread().interrupt();
            }
        }

        if (wasInRoom && _netClient != null && activeRoom != null && !activeRoom.isEmpty()) {
            _netClient.submitVoiceMeta(activeRoom, "LEFT_VOICE");
        }
    }

    @Override
    public void close() {
        leaveVoiceRoom();
    }

    public boolean isInVoiceRoom() {
        synchronized (_mutex) {
            return _inVoiceRoom;
        }
    }

    public String roomId() {
        synchronized (_mutex) {
            return _roomId;
        }
    }

    public String localPlayerUuid() {
        synchronized (_mutex) {
            return _localPlayerUuid;
        }
    }

    public int localUdpPort() {
        synchronized (_mutex) {
            return _boundUdpPort;
        }
    }

    public String localSessionKey() {
        synchronized (_mutex) {
            return _localSessionKey;
        }
    }

    public boolean setPushToTalkActive(boolean talking) {
        String activeRoom;
        synchronized (_mutex) {
            if (!_inVoiceRoom) {
                _pttActive.set(false);
                return false;
            }
            if (talking && (_voiceMutedByServer.get() || _voicePolicyDisabled.get())) {
                _pttActive.set(false);
                return false;
            }
            activeRoom = _roomId;
        }

        boolean prev = _pttActive.getAndSet(talking);
        if (prev != talking && _netClient != null && activeRoom != null && !activeRoom.isEmpty()) {
            _netClient.submitVoiceMeta(activeRoom, talking ? "MIC_UNMUTED" : "MIC_MUTED");
        }
        return true;
    }

    public boolean isPushToTalkActive() {
        return _pttActive.get();
    }

    public boolean isVoiceMutedByServer() {
        return _voiceMutedByServer.get();
    }

    public boolean isVoicePolicyDisabled() {
        return _voicePolicyDisabled.get();
    }

    public boolean initiatePeerOffer(String peerPlayerUuid) {
        String activeRoom;
        String sessionKey;
        int localPort;

        synchronized (_mutex) {
            if (!_inVoiceRoom || _voicePolicyDisabled.get() ||
                    peerPlayerUuid == null || peerPlayerUuid.isEmpty() ||
                    peerPlayerUuid.equals(_localPlayerUuid)) {
                return false;
            }
            activeRoom = _roomId;
            sessionKey = _localSessionKey;
            localPort = _boundUdpPort;

            PeerVoiceEndpoint peer = _peers.get(peerPlayerUuid);
            if (peer == null) {
                peer = new PeerVoiceEndpoint();
                peer.playerUuid = peerPlayerUuid;
                _peers.put(peerPlayerUuid, peer);
            }
        }

        if (_netClient == null) {
            return false;
        }

        Map<String, Object> offerPayload = new LinkedHashMap<>();
        offerPayload.put("udp_host", "127.0.0.1");
        offerPayload.put("udp_port", localPort);
        offerPayload.put("codec", "pcmu-16k-20ms");
        offerPayload.put("session_key", sessionKey);
        OctiNetworkClient.ApiResult<Long> r1 = _netClient.submitVoiceSignal(
                activeRoom, peerPlayerUuid, "SDP_OFFER", OctiMiniJson.stringify(offerPayload));
        if (!r1.ok) {
            return false;
        }

        Map<String, Object> icePayload = new LinkedHashMap<>();
        icePayload.put("udp_host", "127.0.0.1");
        icePayload.put("udp_port", localPort);
        OctiNetworkClient.ApiResult<Long> r2 = _netClient.submitVoiceSignal(
                activeRoom, peerPlayerUuid, "ICE_CANDIDATE", OctiMiniJson.stringify(icePayload));
        return r2.ok;
    }

    public void handleGameSseEvent(OctiNetworkClient.SseEvent ev) {
        if (ev == null) {
            return;
        }
        handleGameSseEvent(ev.event, ev.dataJson);
    }

    public void handleGameSseEvent(String eventType, String dataJson) {
        if (eventType == null || dataJson == null || dataJson.isEmpty() ||
                dataJson.length() > MAX_SIGNALING_PAYLOAD_BYTES) {
            return;
        }

        Map<String, Object> j = OctiMiniJson.parseObject(dataJson);
        if (j.isEmpty()) {
            return;
        }

        if ("VOICE_SIGNAL".equals(eventType)) {
            String fromUuid = OctiMiniJson.getString(
                    j, "player_uuid", OctiMiniJson.getString(j, "from_player_uuid", ""));
            String targetUuid = OctiMiniJson.getString(
                    j, "target_uuid", OctiMiniJson.getString(j, "to_player_uuid", ""));
            String signalType = OctiMiniJson.getString(
                    j, "signal_type", OctiMiniJson.getString(j, "type", ""));

            if (fromUuid.isEmpty() || signalType.isEmpty()) {
                return;
            }

            Map<String, Object> payloadObj = new LinkedHashMap<>();
            Object rawPayload = j.get("payload");
            if (rawPayload instanceof Map<?, ?>) {
                payloadObj = OctiMiniJson.getObject(j, "payload");
            } else if (rawPayload instanceof String) {
                String rawStr = (String) rawPayload;
                if (rawStr.length() <= MAX_SIGNALING_PAYLOAD_BYTES) {
                    payloadObj = OctiMiniJson.parseObject(rawStr);
                }
            }

            String activeRoom = "";
            String localKey = "";
            int localPort = 0;
            boolean sendAnswer = false;

            synchronized (_mutex) {
                if (!_inVoiceRoom || _voicePolicyDisabled.get()) {
                    return;
                }
                if (fromUuid.equals(_localPlayerUuid)) {
                    return;
                }
                if (!targetUuid.isEmpty() && !targetUuid.equals(_localPlayerUuid)) {
                    return;
                }

                PeerVoiceEndpoint peer = _peers.get(fromUuid);
                if (peer == null) {
                    peer = new PeerVoiceEndpoint();
                    peer.playerUuid = fromUuid;
                    _peers.put(fromUuid, peer);
                }

                String host = OctiMiniJson.getString(payloadObj, "udp_host", "127.0.0.1");
                int port = OctiMiniJson.getInt(payloadObj, "udp_port", 0);
                String sKey = OctiMiniJson.getString(payloadObj, "session_key", "");

                if (!host.isEmpty()) {
                    peer.udpHost = host;
                }
                if (port > 0 && port <= 65535) {
                    peer.udpPort = port;
                }
                if (!sKey.isEmpty()) {
                    peer.sessionKey = sKey;
                }

                if ("SDP_OFFER".equals(signalType)) {
                    if (peer.udpPort > 0 && !peer.sessionKey.isEmpty()) {
                        peer.handshakeComplete = true;
                    }
                    activeRoom = _roomId;
                    localKey = _localSessionKey;
                    localPort = _boundUdpPort;
                    sendAnswer = true;
                } else if ("SDP_ANSWER".equals(signalType) ||
                        "ICE_CANDIDATE".equals(signalType)) {
                    if (peer.udpPort > 0 && !peer.sessionKey.isEmpty()) {
                        peer.handshakeComplete = true;
                    }
                }
            }

            if (sendAnswer && _netClient != null && !activeRoom.isEmpty()) {
                Map<String, Object> answerPayload = new LinkedHashMap<>();
                answerPayload.put("udp_host", "127.0.0.1");
                answerPayload.put("udp_port", localPort);
                answerPayload.put("codec", "pcmu-16k-20ms");
                answerPayload.put("session_key", localKey);
                _netClient.submitVoiceSignal(
                        activeRoom, fromUuid, "SDP_ANSWER", OctiMiniJson.stringify(answerPayload));

                Map<String, Object> icePayload = new LinkedHashMap<>();
                icePayload.put("udp_host", "127.0.0.1");
                icePayload.put("udp_port", localPort);
                _netClient.submitVoiceSignal(
                        activeRoom, fromUuid, "ICE_CANDIDATE", OctiMiniJson.stringify(icePayload));
            }
            return;
        }

        if ("VOICE_STATE".equals(eventType) || "VOICE_META".equals(eventType)) {
            String peerUuid = OctiMiniJson.getString(j, "player_uuid", "");
            String action = OctiMiniJson.getString(j, "action", "");
            synchronized (_mutex) {
                if ("LEFT_VOICE".equals(action)) {
                    _peers.remove(peerUuid);
                } else if ("MIC_MUTED".equals(action) || "MIC_UNMUTED".equals(action)) {
                    PeerVoiceEndpoint peer = _peers.get(peerUuid);
                    if (peer != null) {
                        peer.micMuted = "MIC_MUTED".equals(action);
                    }
                }
            }
            return;
        }

        if ("USER_MUTED".equals(eventType)) {
            String targetUuid = OctiMiniJson.getString(
                    j, "target", OctiMiniJson.getString(j, "player_uuid", ""));
            boolean muteVoice = OctiMiniJson.getBoolean(j, "mute_voice", false);
            String activeRoom = "";
            boolean needNotifyMuted = false;

            synchronized (_mutex) {
                if (targetUuid.equals(_localPlayerUuid)) {
                    _voiceMutedByServer.set(muteVoice);
                    if (muteVoice && _pttActive.getAndSet(false)) {
                        activeRoom = _roomId;
                        needNotifyMuted = true;
                    }
                } else {
                    PeerVoiceEndpoint peer = _peers.get(targetUuid);
                    if (peer != null) {
                        peer.mutedByServer = muteVoice;
                        if (muteVoice) {
                            peer.lastRms = 0.0;
                            peer.jitterBuffer.clear();
                        }
                    }
                }
            }

            if (needNotifyMuted && _netClient != null && !activeRoom.isEmpty()) {
                _netClient.submitVoiceMeta(activeRoom, "MIC_MUTED");
            }
            return;
        }

        if ("OBSERVER_BANNED".equals(eventType)) {
            String targetUuid = OctiMiniJson.getString(
                    j, "target", OctiMiniJson.getString(j, "player_uuid", ""));
            boolean isSelf = false;
            synchronized (_mutex) {
                if (targetUuid.equals(_localPlayerUuid)) {
                    isSelf = true;
                } else {
                    _peers.remove(targetUuid);
                }
            }
            if (isSelf) {
                leaveVoiceRoom();
            }
            return;
        }

        if ("POLICY_UPDATED".equals(eventType)) {
            String voicePolicy = OctiMiniJson.getString(j, "voice_policy", "");
            String activeRoom = "";
            boolean notifyMuted = false;
            synchronized (_mutex) {
                if ("DISABLED".equals(voicePolicy)) {
                    _voicePolicyDisabled.set(true);
                    if (_pttActive.getAndSet(false)) {
                        activeRoom = _roomId;
                        notifyMuted = true;
                    }
                    _peers.clear();
                } else if (!voicePolicy.isEmpty()) {
                    _voicePolicyDisabled.set(false);
                }
            }
            if (notifyMuted && _netClient != null && !activeRoom.isEmpty()) {
                _netClient.submitVoiceMeta(activeRoom, "MIC_MUTED");
            }
        }
    }

    public int transmitPcmFrame20Ms(short[] pcm320) {
        if (pcm320 == null || pcm320.length < PAYLOAD_SAMPLES ||
                !_pttActive.get() || _voiceMutedByServer.get() || _voicePolicyDisabled.get()) {
            return 0;
        }

        DatagramSocket sock;
        String sessionKey;
        List<InetSocketAddress> targets = new ArrayList<>();

        synchronized (_mutex) {
            if (!_inVoiceRoom || _udpSocket == null) {
                return 0;
            }
            sock = _udpSocket;
            sessionKey = _localSessionKey;
            for (PeerVoiceEndpoint p : _peers.values()) {
                if (p.handshakeComplete && !p.mutedByServer && p.udpPort > 0) {
                    String host = (p.udpHost == null || p.udpHost.isEmpty()) ? "127.0.0.1" : p.udpHost;
                    targets.add(new InetSocketAddress(host, p.udpPort));
                }
            }
        }

        if (targets.isEmpty()) {
            return 0;
        }

        VoiceUdpPacket pkt = new VoiceUdpPacket();
        pkt.seq = _txSeq.getAndIncrement();
        pkt.timestampMs = (int) (System.nanoTime() / 1_000_000L);
        for (int i = 0; i < PAYLOAD_SAMPLES; ++i) {
            pkt.payload[i] = linear16ToUlaw(pcm320[i]);
        }
        pkt.authTag = computeAuthTag(pkt.seq, pkt.timestampMs, pkt.payload, sessionKey);
        byte[] wire = serializePacket(pkt);

        int sentCount = 0;
        for (InetSocketAddress dst : targets) {
            try {
                DatagramPacket dgram = new DatagramPacket(wire, wire.length, dst);
                sock.send(dgram);
                ++sentCount;
                _totalSent.incrementAndGet();
            } catch (Exception ignored) {
                // Ignore transient send error on closed/unreachable peer socket.
            }
        }
        return sentCount;
    }

    public int injectTestPcmFrame(short[] samples) {
        if (samples == null) {
            return 0;
        }
        return injectTestPcmFrame(samples, samples.length);
    }

    public int injectTestPcmFrame(short[] samples, int count) {
        if (samples == null || count < PAYLOAD_SAMPLES || samples.length < PAYLOAD_SAMPLES) {
            return 0;
        }
        if (samples.length == PAYLOAD_SAMPLES) {
            return transmitPcmFrame20Ms(samples);
        }
        short[] slice = Arrays.copyOf(samples, PAYLOAD_SAMPLES);
        return transmitPcmFrame20Ms(slice);
    }

    public int activePeerCount() {
        synchronized (_mutex) {
            int count = 0;
            for (PeerVoiceEndpoint p : _peers.values()) {
                if (p.handshakeComplete && !p.mutedByServer) {
                    ++count;
                }
            }
            return count;
        }
    }

    public boolean isPeerConnected(String peerUuid) {
        synchronized (_mutex) {
            PeerVoiceEndpoint p = _peers.get(peerUuid);
            return p != null && p.handshakeComplete && !p.mutedByServer;
        }
    }

    public boolean isPeerMicMuted(String peerUuid) {
        synchronized (_mutex) {
            PeerVoiceEndpoint p = _peers.get(peerUuid);
            return p == null || p.micMuted;
        }
    }

    public boolean isPeerMutedByServer(String peerUuid) {
        synchronized (_mutex) {
            PeerVoiceEndpoint p = _peers.get(peerUuid);
            return p != null && p.mutedByServer;
        }
    }

    public long totalPacketsSent() {
        return _totalSent.get();
    }

    public long totalPacketsReceived() {
        return _totalReceived.get();
    }

    public long totalInvalidPacketsDropped() {
        return _totalDropped.get();
    }

    public long packetsReceivedFrom(String peerUuid) {
        synchronized (_mutex) {
            PeerVoiceEndpoint p = _peers.get(peerUuid);
            return p == null ? 0L : p.packetsReceived;
        }
    }

    public double receivedAudioRms(String peerUuid) {
        return lastReceivedRmsFrom(peerUuid);
    }

    public double lastReceivedRmsFrom(String peerUuid) {
        synchronized (_mutex) {
            PeerVoiceEndpoint p = _peers.get(peerUuid);
            if (p == null || p.mutedByServer) {
                return 0.0;
            }
            return p.lastRms;
        }
    }

    public short[] popDecodedFrameFrom(String peerUuid) {
        synchronized (_mutex) {
            PeerVoiceEndpoint p = _peers.get(peerUuid);
            if (p == null || p.jitterBuffer.isEmpty()) {
                return new short[0];
            }
            return p.jitterBuffer.removeFirst();
        }
    }

    public void setHardwareAudioEnabled(boolean enabled) {
        synchronized (_mutex) {
            _hardwareAudioEnabled.set(enabled);
            if (!enabled) {
                closeHardwareAudioLocked();
            } else if (_inVoiceRoom) {
                openHardwareAudioIfEnabledLocked();
            }
        }
    }

    public boolean isHardwareAudioEnabled() {
        return _hardwareAudioEnabled.get();
    }

    private void receiveLoop() {
        byte[] buf = new byte[512];
        while (!_stopRx.get()) {
            DatagramSocket sock;
            synchronized (_mutex) {
                sock = _udpSocket;
            }
            if (sock == null || sock.isClosed()) {
                break;
            }

            DatagramPacket dgram = new DatagramPacket(buf, buf.length);
            try {
                sock.receive(dgram);
            } catch (SocketTimeoutException timeout) {
                continue;
            } catch (Exception e) {
                if (_stopRx.get() || sock.isClosed()) {
                    break;
                }
                continue;
            }

            int n = dgram.getLength();
            if (n != PACKET_BYTES) {
                _totalDropped.incrementAndGet();
                continue;
            }

            int senderPort = dgram.getPort();
            byte[] wire = Arrays.copyOfRange(dgram.getData(), dgram.getOffset(), dgram.getOffset() + n);

            short[] decodedToPlay = null;
            AudioTrack trackToPlay = null;

            synchronized (_mutex) {
                if (!_inVoiceRoom || _voicePolicyDisabled.get()) {
                    _totalDropped.incrementAndGet();
                    continue;
                }

                PeerVoiceEndpoint matchedPeer = null;
                VoiceUdpPacket parsedPkt = new VoiceUdpPacket();

                for (PeerVoiceEndpoint peer : _peers.values()) {
                    if (!peer.handshakeComplete || peer.mutedByServer) {
                        continue;
                    }
                    if (!peer.sessionKey.isEmpty() &&
                            parseAndVerifyPacket(wire, n, peer.sessionKey, parsedPkt)) {
                        matchedPeer = peer;
                        break;
                    }
                    if (peer.udpPort == senderPort && !_localSessionKey.isEmpty() &&
                            parseAndVerifyPacket(wire, n, _localSessionKey, parsedPkt)) {
                        matchedPeer = peer;
                        break;
                    }
                }

                if (matchedPeer == null) {
                    _totalDropped.incrementAndGet();
                    continue;
                }

                short[] decoded = new short[PAYLOAD_SAMPLES];
                double sumSquares = 0.0;
                for (int i = 0; i < PAYLOAD_SAMPLES; ++i) {
                    short s = ulawToLinear16(parsedPkt.payload[i]);
                    decoded[i] = s;
                    double d = s;
                    sumSquares += d * d;
                }
                double rms = Math.sqrt(sumSquares / (double) PAYLOAD_SAMPLES);

                matchedPeer.packetsReceived += 1L;
                matchedPeer.lastSeq = parsedPkt.seq;
                matchedPeer.lastRms = rms;
                if (matchedPeer.jitterBuffer.size() >= MAX_JITTER_BUFFER_FRAMES) {
                    matchedPeer.jitterBuffer.removeFirst();
                }
                matchedPeer.jitterBuffer.addLast(decoded);

                _totalReceived.incrementAndGet();
                if (_audioTrack != null && _hardwareAudioEnabled.get()) {
                    decodedToPlay = decoded;
                    trackToPlay = _audioTrack;
                }
            }

            if (trackToPlay != null && decodedToPlay != null) {
                try {
                    trackToPlay.write(decodedToPlay, 0, decodedToPlay.length);
                } catch (Throwable ignored) {
                }
            }
        }
    }

    private void openHardwareAudioIfEnabledLocked() {
        if (!_hardwareAudioEnabled.get()) {
            return;
        }
        try {
            int minTrackBuf = AudioTrack.getMinBufferSize(
                    SAMPLE_RATE_HZ,
                    AudioFormat.CHANNEL_OUT_MONO,
                    AudioFormat.ENCODING_PCM_16BIT);
            if (minTrackBuf > 0) {
                AudioAttributes attrs = new AudioAttributes.Builder()
                        .setUsage(AudioAttributes.USAGE_VOICE_COMMUNICATION)
                        .setContentType(AudioAttributes.CONTENT_TYPE_SPEECH)
                        .build();
                AudioFormat fmt = new AudioFormat.Builder()
                        .setSampleRate(SAMPLE_RATE_HZ)
                        .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                        .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
                        .build();
                _audioTrack = new AudioTrack(
                        attrs,
                        fmt,
                        Math.max(minTrackBuf, PAYLOAD_SAMPLES * 4),
                        AudioTrack.MODE_STREAM,
                        AudioManager.AUDIO_SESSION_ID_GENERATE);
                _audioTrack.play();
            }

            int minRecBuf = AudioRecord.getMinBufferSize(
                    SAMPLE_RATE_HZ,
                    AudioFormat.CHANNEL_IN_MONO,
                    AudioFormat.ENCODING_PCM_16BIT);
            if (minRecBuf > 0) {
                _audioRecord = new AudioRecord(
                        MediaRecorder.AudioSource.VOICE_COMMUNICATION,
                        SAMPLE_RATE_HZ,
                        AudioFormat.CHANNEL_IN_MONO,
                        AudioFormat.ENCODING_PCM_16BIT,
                        Math.max(minRecBuf, PAYLOAD_SAMPLES * 4));
                _audioRecord.startRecording();
                _stopHwCapture.set(false);
                Thread cap = new Thread(this::hardwareCaptureLoop, "octi-p2p-voice-tx");
                cap.setDaemon(true);
                _hwCaptureThread = cap;
                cap.start();
            }
        } catch (Throwable ignored) {
            // Host JVM unit tests or devices without microphone permission fall back cleanly.
        }
    }

    private void closeHardwareAudioLocked() {
        _stopHwCapture.set(true);
        if (_audioRecord != null) {
            try {
                _audioRecord.stop();
            } catch (Throwable ignored) {
            }
            try {
                _audioRecord.release();
            } catch (Throwable ignored) {
            }
            _audioRecord = null;
        }
        if (_audioTrack != null) {
            try {
                _audioTrack.stop();
            } catch (Throwable ignored) {
            }
            try {
                _audioTrack.release();
            } catch (Throwable ignored) {
            }
            _audioTrack = null;
        }
    }

    private void hardwareCaptureLoop() {
        short[] frame = new short[PAYLOAD_SAMPLES];
        while (!_stopHwCapture.get()) {
            AudioRecord rec;
            synchronized (_mutex) {
                rec = _audioRecord;
            }
            if (rec == null) {
                break;
            }
            if (!_pttActive.get() || _voiceMutedByServer.get() || _voicePolicyDisabled.get()) {
                try {
                    Thread.sleep(10L);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    break;
                }
                continue;
            }
            try {
                int read = rec.read(frame, 0, PAYLOAD_SAMPLES);
                if (read == PAYLOAD_SAMPLES) {
                    transmitPcmFrame20Ms(frame);
                }
            } catch (Throwable e) {
                break;
            }
        }
    }

    private String generateSessionKeyHex() {
        byte[] bytes = new byte[16];
        _rng.nextBytes(bytes);
        StringBuilder sb = new StringBuilder(32);
        for (byte b : bytes) {
            sb.append(String.format("%02x", b & 0xFF));
        }
        return sb.toString();
    }

    private static void writeU32Be(byte[] dst, int offset, int v) {
        dst[offset] = (byte) ((v >>> 24) & 0xFF);
        dst[offset + 1] = (byte) ((v >>> 16) & 0xFF);
        dst[offset + 2] = (byte) ((v >>> 8) & 0xFF);
        dst[offset + 3] = (byte) (v & 0xFF);
    }

    private static int readU32Be(byte[] src, int offset) {
        return ((src[offset] & 0xFF) << 24) |
                ((src[offset + 1] & 0xFF) << 16) |
                ((src[offset + 2] & 0xFF) << 8) |
                (src[offset + 3] & 0xFF);
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
