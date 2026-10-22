/*
 * P2pVoiceEngine.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "P2pVoiceEngine.hxx"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
#include <iomanip>
#include <thread>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mmsystem.h>
using SocketHandle = SOCKET;
static constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#if defined(__linux__)
#include <alsa/asoundlib.h>
#endif
using SocketHandle = int;
static constexpr SocketHandle kInvalidSocket = -1;
#endif

namespace octi {

namespace {

static constexpr std::size_t kMaxSignalingPayloadBytes = 4096;
static constexpr std::size_t kMaxJitterBufferFrames = 32;

void closeSocketHandle(SocketHandle s)
{
    if (s == kInvalidSocket) {
        return;
    }
#ifdef _WIN32
    ::closesocket(s);
#else
    ::close(s);
#endif
}

void writeU32Be(std::uint8_t *dst, std::uint32_t v)
{
    dst[0] = static_cast<std::uint8_t>((v >> 24) & 0xFFU);
    dst[1] = static_cast<std::uint8_t>((v >> 16) & 0xFFU);
    dst[2] = static_cast<std::uint8_t>((v >> 8) & 0xFFU);
    dst[3] = static_cast<std::uint8_t>(v & 0xFFU);
}

std::uint32_t readU32Be(const std::uint8_t *src)
{
    return (static_cast<std::uint32_t>(src[0]) << 24) |
           (static_cast<std::uint32_t>(src[1]) << 16) |
           (static_cast<std::uint32_t>(src[2]) << 8) |
           static_cast<std::uint32_t>(src[3]);
}

std::string generateSessionKeyHex()
{
    std::random_device rd;
    std::mt19937_64 gen(
        rd() ^ static_cast<std::uint64_t>(
                   std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uint64_t hi = gen();
    std::uint64_t lo = gen();
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(16) << hi
        << std::setw(16) << lo;
    return oss.str();
}

std::uint32_t currentMonotonicMs32()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    return static_cast<std::uint32_t>(ms & 0xFFFFFFFFULL);
}

struct PeerVoiceEndpoint {
    std::string playerUuid;
    std::string udpHost = "127.0.0.1";
    int udpPort = 0;
    std::string sessionKey;
    bool handshakeComplete = false;
    bool micMuted = true;
    bool mutedByServer = false;
    std::uint64_t packetsReceived = 0;
    std::uint32_t lastSeq = 0;
    double lastRms = 0.0;
    std::deque<std::vector<std::int16_t>> jitterBuffer;
};

} // namespace

struct P2pVoiceEngine::Impl {
    OctiNetworkClient *netClient = nullptr;

    mutable std::mutex mutex;
    bool inVoiceRoom = false;
    std::string roomId;
    std::string localPlayerUuid;
    std::string localSessionKey;
    int boundUdpPort = 0;
    SocketHandle udpSocket = kInvalidSocket;

    std::atomic<bool> stopRx{false};
    std::thread rxThread;

    std::atomic<bool> pttActive{false};
    std::atomic<bool> voiceMutedByServer{false};
    std::atomic<bool> voicePolicyDisabled{false};
    std::atomic<bool> hardwareAudioEnabled{false};

    std::atomic<std::uint32_t> txSeq{1};
    std::atomic<std::uint64_t> totalSent{0};
    std::atomic<std::uint64_t> totalReceived{0};
    std::atomic<std::uint64_t> totalDropped{0};

    std::map<std::string, PeerVoiceEndpoint> peers;

#if defined(__linux__)
    snd_pcm_t *alsaPlaybackHandle = nullptr;
#elif defined(_WIN32)
    HWAVEOUT winmmWaveOut = nullptr;
#endif

    explicit Impl(OctiNetworkClient *client)
        : netClient(client)
    {
#ifdef _WIN32
        WSADATA wsaData;
        ::WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }

    ~Impl()
    {
        closeHardwareAudio();
    }

    void openHardwareAudioIfEnabled()
    {
        if (!hardwareAudioEnabled.load()) {
            return;
        }
#if defined(__linux__)
        if (alsaPlaybackHandle == nullptr) {
            snd_pcm_t *handle = nullptr;
            if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK,
                             SND_PCM_NONBLOCK) == 0 &&
                handle != nullptr) {
                if (snd_pcm_set_params(handle,
                                       SND_PCM_FORMAT_S16_LE,
                                       SND_PCM_ACCESS_RW_INTERLEAVED,
                                       1,
                                       16000,
                                       1,
                                       40000) == 0) {
                    alsaPlaybackHandle = handle;
                } else {
                    snd_pcm_close(handle);
                }
            }
        }
#elif defined(_WIN32)
        if (winmmWaveOut == nullptr && ::waveOutGetNumDevs() > 0) {
            WAVEFORMATEX wfx;
            std::memset(&wfx, 0, sizeof(wfx));
            wfx.wFormatTag = WAVE_FORMAT_PCM;
            wfx.nChannels = 1;
            wfx.nSamplesPerSec = 16000;
            wfx.wBitsPerSample = 16;
            wfx.nBlockAlign = 2;
            wfx.nAvgBytesPerSec = 32000;
            HWAVEOUT hOut = nullptr;
            if (::waveOutOpen(&hOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) ==
                MMSYSERR_NOERROR) {
                winmmWaveOut = hOut;
            }
        }
#endif
    }

    void closeHardwareAudio()
    {
#if defined(__linux__)
        if (alsaPlaybackHandle != nullptr) {
            snd_pcm_close(alsaPlaybackHandle);
            alsaPlaybackHandle = nullptr;
        }
#elif defined(_WIN32)
        if (winmmWaveOut != nullptr) {
            ::waveOutReset(winmmWaveOut);
            ::waveOutClose(winmmWaveOut);
            winmmWaveOut = nullptr;
        }
#endif
    }

    void receiveLoop()
    {
        std::uint8_t buf[512];
        while (!stopRx.load()) {
            SocketHandle sock = kInvalidSocket;
            {
                std::lock_guard<std::mutex> lock(mutex);
                sock = udpSocket;
            }
            if (sock == kInvalidSocket) {
                break;
            }

            sockaddr_in srcAddr;
            std::memset(&srcAddr, 0, sizeof(srcAddr));
#ifdef _WIN32
            int addrLen = static_cast<int>(sizeof(srcAddr));
            int n = ::recvfrom(sock, reinterpret_cast<char *>(buf),
                               static_cast<int>(sizeof(buf)), 0,
                               reinterpret_cast<sockaddr *>(&srcAddr),
                               &addrLen);
#else
            socklen_t addrLen = sizeof(srcAddr);
            ssize_t n = ::recvfrom(sock, buf, sizeof(buf), 0,
                                   reinterpret_cast<sockaddr *>(&srcAddr),
                                   &addrLen);
#endif
            if (n <= 0) {
                continue;
            }
            if (static_cast<std::size_t>(n) != VoiceUdpPacket::kPacketBytes) {
                totalDropped.fetch_add(1);
                continue;
            }

            std::lock_guard<std::mutex> lock(mutex);
            if (!inVoiceRoom || voicePolicyDisabled.load()) {
                totalDropped.fetch_add(1);
                continue;
            }

            int senderPort = ntohs(srcAddr.sin_port);
            PeerVoiceEndpoint *matchedPeer = nullptr;
            VoiceUdpPacket parsedPkt;

            /*
             * Match against registered peers by authenticating with the peer's
             * negotiated session key (or local session key in symmetric tests).
             */
            for (auto &kv : peers) {
                PeerVoiceEndpoint &peer = kv.second;
                if (!peer.handshakeComplete || peer.mutedByServer) {
                    continue;
                }
                if (!peer.sessionKey.empty() &&
                    P2pVoiceEngine::parseAndVerifyPacket(
                        buf, static_cast<std::size_t>(n), peer.sessionKey,
                        parsedPkt)) {
                    matchedPeer = &peer;
                    break;
                }
                if (peer.udpPort == senderPort && !localSessionKey.empty() &&
                    P2pVoiceEngine::parseAndVerifyPacket(
                        buf, static_cast<std::size_t>(n), localSessionKey,
                        parsedPkt)) {
                    matchedPeer = &peer;
                    break;
                }
            }

            if (matchedPeer == nullptr) {
                totalDropped.fetch_add(1);
                continue;
            }

            std::vector<std::int16_t> decoded(VoiceUdpPacket::kPayloadSamples);
            double sumSquares = 0.0;
            for (std::size_t i = 0; i < VoiceUdpPacket::kPayloadSamples; ++i) {
                std::int16_t s =
                    P2pVoiceEngine::ulawToLinear16(parsedPkt.payload[i]);
                decoded[i] = s;
                double d = static_cast<double>(s);
                sumSquares += d * d;
            }
            double rms = std::sqrt(
                sumSquares /
                static_cast<double>(VoiceUdpPacket::kPayloadSamples));

            matchedPeer->packetsReceived += 1;
            matchedPeer->lastSeq = parsedPkt.seq;
            matchedPeer->lastRms = rms;
            if (matchedPeer->jitterBuffer.size() >= kMaxJitterBufferFrames) {
                matchedPeer->jitterBuffer.pop_front();
            }
            matchedPeer->jitterBuffer.push_back(std::move(decoded));

            totalReceived.fetch_add(1);
        }
    }
};

std::uint8_t P2pVoiceEngine::linear16ToUlaw(std::int16_t pcm)
{
    static constexpr int kBias = 0x84;
    static constexpr int kClip = 32635;

    int sample = static_cast<int>(pcm);
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
    }
    int mantissa = (sample >> (exponent + 3)) & 0x0F;
    std::uint8_t ulawByte =
        static_cast<std::uint8_t>(~(sign | (exponent << 4) | mantissa));
    return ulawByte;
}

std::int16_t P2pVoiceEngine::ulawToLinear16(std::uint8_t ulaw)
{
    ulaw = static_cast<std::uint8_t>(~ulaw);
    int sign = (ulaw & 0x80);
    int exponent = (ulaw >> 4) & 0x07;
    int mantissa = ulaw & 0x0F;
    int t = ((mantissa << 3) + 0x84) << exponent;
    int magnitude = t - 0x84;
    return static_cast<std::int16_t>(sign ? -magnitude : magnitude);
}

std::uint32_t P2pVoiceEngine::computeAuthTag(
    std::uint32_t seq,
    std::uint32_t timestampMs,
    const std::uint8_t payload[VoiceUdpPacket::kPayloadSamples],
    const std::string &sessionKey)
{
    if (sessionKey.empty() || payload == nullptr) {
        return 0;
    }

    std::uint64_t hash = 14695981039346656037ULL;
    auto mixByte = [&hash](std::uint8_t b) {
        hash ^= static_cast<std::uint64_t>(b);
        hash *= 1099511628211ULL;
    };

    mixByte(static_cast<std::uint8_t>('O'));
    mixByte(static_cast<std::uint8_t>('C'));
    mixByte(static_cast<std::uint8_t>('T'));
    mixByte(static_cast<std::uint8_t>('V'));

    std::uint8_t headerBuf[8];
    writeU32Be(headerBuf, seq);
    writeU32Be(headerBuf + 4, timestampMs);
    for (std::uint8_t b : headerBuf) {
        mixByte(b);
    }
    for (char c : sessionKey) {
        mixByte(static_cast<std::uint8_t>(c));
    }
    for (std::size_t i = 0; i < VoiceUdpPacket::kPayloadSamples; ++i) {
        mixByte(payload[i]);
    }

    std::uint32_t folded =
        static_cast<std::uint32_t>((hash >> 32) ^ (hash & 0xFFFFFFFFULL));
    return (folded == 0) ? 0xA5A55A5AU : folded;
}

std::vector<std::uint8_t> P2pVoiceEngine::serializePacket(
    const VoiceUdpPacket &pkt)
{
    std::vector<std::uint8_t> out(VoiceUdpPacket::kPacketBytes, 0);
    std::memcpy(out.data(), pkt.magic, 4);
    writeU32Be(out.data() + 4, pkt.seq);
    writeU32Be(out.data() + 8, pkt.timestampMs);
    writeU32Be(out.data() + 12, pkt.authTag);
    std::memcpy(out.data() + 16, pkt.payload, VoiceUdpPacket::kPayloadSamples);
    return out;
}

bool P2pVoiceEngine::parseAndVerifyPacket(
    const std::uint8_t *data,
    std::size_t len,
    const std::string &expectedSessionKey,
    VoiceUdpPacket &outPkt)
{
    if (data == nullptr || len != VoiceUdpPacket::kPacketBytes ||
        expectedSessionKey.empty()) {
        return false;
    }
    if (std::memcmp(data, "OCTV", 4) != 0) {
        return false;
    }

    std::uint32_t seq = readU32Be(data + 4);
    std::uint32_t timestampMs = readU32Be(data + 8);
    std::uint32_t wireTag = readU32Be(data + 12);
    const std::uint8_t *payloadPtr = data + 16;

    std::uint32_t expectedTag =
        computeAuthTag(seq, timestampMs, payloadPtr, expectedSessionKey);
    if (wireTag == 0 || wireTag != expectedTag) {
        return false;
    }

    std::memcpy(outPkt.magic, "OCTV", 4);
    outPkt.seq = seq;
    outPkt.timestampMs = timestampMs;
    outPkt.authTag = wireTag;
    std::memcpy(outPkt.payload, payloadPtr, VoiceUdpPacket::kPayloadSamples);
    return true;
}

P2pVoiceEngine::P2pVoiceEngine(OctiNetworkClient *netClient)
    : _impl(std::make_unique<Impl>(netClient))
{
}

P2pVoiceEngine::~P2pVoiceEngine()
{
    leaveVoiceRoom();
}

bool P2pVoiceEngine::joinVoiceRoom(const std::string &roomId,
                                   const std::string &localPlayerUuid,
                                   std::string &errorOut)
{
    leaveVoiceRoom();

    if (roomId.empty() || localPlayerUuid.empty()) {
        errorOut = "roomId and localPlayerUuid must not be empty";
        return false;
    }

    SocketHandle sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == kInvalidSocket) {
        errorOut = "Failed to create UDP voice socket";
        return false;
    }

#ifdef _WIN32
    DWORD timeoutMs = 50;
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                 reinterpret_cast<const char *>(&timeoutMs),
                 sizeof(timeoutMs));
#else
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    sockaddr_in bindAddr;
    std::memset(&bindAddr, 0, sizeof(bindAddr));
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bindAddr.sin_port = htons(0);

    if (::bind(sock, reinterpret_cast<const sockaddr *>(&bindAddr),
               sizeof(bindAddr)) != 0) {
        closeSocketHandle(sock);
        errorOut = "Failed to bind local UDP voice socket";
        return false;
    }

    sockaddr_in actualAddr;
    std::memset(&actualAddr, 0, sizeof(actualAddr));
#ifdef _WIN32
    int actualLen = static_cast<int>(sizeof(actualAddr));
#else
    socklen_t actualLen = sizeof(actualAddr);
#endif
    if (::getsockname(sock, reinterpret_cast<sockaddr *>(&actualAddr),
                      &actualLen) != 0) {
        closeSocketHandle(sock);
        errorOut = "Failed to query bound UDP port";
        return false;
    }
    int boundPort = ntohs(actualAddr.sin_port);

    {
        std::lock_guard<std::mutex> lock(_impl->mutex);
        _impl->roomId = roomId;
        _impl->localPlayerUuid = localPlayerUuid;
        _impl->localSessionKey = generateSessionKeyHex();
        _impl->boundUdpPort = boundPort;
        _impl->udpSocket = sock;
        _impl->inVoiceRoom = true;
        _impl->pttActive.store(false);
        _impl->voiceMutedByServer.store(false);
        _impl->voicePolicyDisabled.store(false);
        _impl->peers.clear();
        _impl->openHardwareAudioIfEnabled();
    }

    _impl->stopRx.store(false);
    _impl->rxThread = std::thread([this]() { _impl->receiveLoop(); });

    if (_impl->netClient != nullptr) {
        _impl->netClient->submitVoiceMeta(roomId, "JOINED_VOICE");
        _impl->netClient->submitVoiceMeta(roomId, "MIC_MUTED");
    }

    return true;
}

void P2pVoiceEngine::leaveVoiceRoom()
{
    std::string activeRoom;
    bool wasInRoom = false;
    SocketHandle sockToClose = kInvalidSocket;

    {
        std::lock_guard<std::mutex> lock(_impl->mutex);
        wasInRoom = _impl->inVoiceRoom;
        activeRoom = _impl->roomId;
        _impl->inVoiceRoom = false;
        _impl->pttActive.store(false);
        _impl->boundUdpPort = 0;
        sockToClose = _impl->udpSocket;
        _impl->udpSocket = kInvalidSocket;
        _impl->peers.clear();
        _impl->closeHardwareAudio();
    }

    _impl->stopRx.store(true);
    if (sockToClose != kInvalidSocket) {
        closeSocketHandle(sockToClose);
    }
    if (_impl->rxThread.joinable()) {
        _impl->rxThread.join();
    }

    if (wasInRoom && _impl->netClient != nullptr && !activeRoom.empty()) {
        _impl->netClient->submitVoiceMeta(activeRoom, "LEFT_VOICE");
    }
}

bool P2pVoiceEngine::isInVoiceRoom() const
{
    std::lock_guard<std::mutex> lock(_impl->mutex);
    return _impl->inVoiceRoom;
}

int P2pVoiceEngine::localUdpPort() const
{
    std::lock_guard<std::mutex> lock(_impl->mutex);
    return _impl->boundUdpPort;
}

std::string P2pVoiceEngine::localSessionKey() const
{
    std::lock_guard<std::mutex> lock(_impl->mutex);
    return _impl->localSessionKey;
}

bool P2pVoiceEngine::setPushToTalkActive(bool talking)
{
    std::string activeRoom;
    {
        std::lock_guard<std::mutex> lock(_impl->mutex);
        if (!_impl->inVoiceRoom) {
            _impl->pttActive.store(false);
            return false;
        }
        if (talking &&
            (_impl->voiceMutedByServer.load() ||
             _impl->voicePolicyDisabled.load())) {
            _impl->pttActive.store(false);
            return false;
        }
        activeRoom = _impl->roomId;
    }

    bool prev = _impl->pttActive.exchange(talking);
    if (prev != talking && _impl->netClient != nullptr &&
        !activeRoom.empty()) {
        _impl->netClient->submitVoiceMeta(
            activeRoom, talking ? "MIC_UNMUTED" : "MIC_MUTED");
    }
    return true;
}

bool P2pVoiceEngine::isPushToTalkActive() const
{
    return _impl->pttActive.load();
}

bool P2pVoiceEngine::isVoiceMutedByServer() const
{
    return _impl->voiceMutedByServer.load();
}

bool P2pVoiceEngine::isVoicePolicyDisabled() const
{
    return _impl->voicePolicyDisabled.load();
}

bool P2pVoiceEngine::initiatePeerOffer(const std::string &peerPlayerUuid)
{
    std::string activeRoom;
    std::string sessionKey;
    int localPort = 0;

    {
        std::lock_guard<std::mutex> lock(_impl->mutex);
        if (!_impl->inVoiceRoom || _impl->voicePolicyDisabled.load() ||
            peerPlayerUuid.empty() ||
            peerPlayerUuid == _impl->localPlayerUuid) {
            return false;
        }
        activeRoom = _impl->roomId;
        sessionKey = _impl->localSessionKey;
        localPort = _impl->boundUdpPort;

        PeerVoiceEndpoint &peer = _impl->peers[peerPlayerUuid];
        peer.playerUuid = peerPlayerUuid;
    }

    if (_impl->netClient == nullptr) {
        return false;
    }

    nlohmann::json offerPayload = {
        {"udp_host", "127.0.0.1"},
        {"udp_port", localPort},
        {"codec", "pcmu-16k-20ms"},
        {"session_key", sessionKey}
    };
    auto r1 = _impl->netClient->submitVoiceSignal(
        activeRoom, peerPlayerUuid, "SDP_OFFER", offerPayload.dump());
    if (!r1.ok) {
        return false;
    }

    nlohmann::json icePayload = {
        {"udp_host", "127.0.0.1"},
        {"udp_port", localPort}
    };
    auto r2 = _impl->netClient->submitVoiceSignal(
        activeRoom, peerPlayerUuid, "ICE_CANDIDATE", icePayload.dump());
    return r2.ok;
}

void P2pVoiceEngine::handleGameSseEvent(const SseEvent &ev)
{
    if (ev.dataJson.empty() ||
        ev.dataJson.size() > kMaxSignalingPayloadBytes) {
        return;
    }

    nlohmann::json j = nlohmann::json::parse(ev.dataJson, nullptr, false);
    if (j.is_discarded() || !j.is_object()) {
        return;
    }

    if (ev.event == "VOICE_SIGNAL") {
        std::string fromUuid =
            j.value("player_uuid", j.value("from_player_uuid", ""));
        std::string targetUuid =
            j.value("target_uuid", j.value("to_player_uuid", ""));
        std::string signalType =
            j.value("signal_type", j.value("type", ""));

        if (fromUuid.empty() || signalType.empty()) {
            return;
        }

        nlohmann::json payloadObj = nlohmann::json::object();
        if (j.contains("payload")) {
            if (j["payload"].is_object()) {
                payloadObj = j["payload"];
            } else if (j["payload"].is_string()) {
                std::string raw = j["payload"].get<std::string>();
                if (raw.size() <= kMaxSignalingPayloadBytes) {
                    auto parsed = nlohmann::json::parse(raw, nullptr, false);
                    if (parsed.is_object()) {
                        payloadObj = parsed;
                    }
                }
            }
        }

        std::string activeRoom;
        std::string localKey;
        int localPort = 0;
        bool sendAnswer = false;

        {
            std::lock_guard<std::mutex> lock(_impl->mutex);
            if (!_impl->inVoiceRoom || _impl->voicePolicyDisabled.load()) {
                return;
            }
            if (fromUuid == _impl->localPlayerUuid) {
                return;
            }
            if (!targetUuid.empty() && targetUuid != _impl->localPlayerUuid) {
                return;
            }

            PeerVoiceEndpoint &peer = _impl->peers[fromUuid];
            peer.playerUuid = fromUuid;

            std::string host = payloadObj.value("udp_host", "127.0.0.1");
            int port = payloadObj.value("udp_port", 0);
            std::string sKey = payloadObj.value("session_key", "");

            if (!host.empty()) {
                peer.udpHost = host;
            }
            if (port > 0 && port <= 65535) {
                peer.udpPort = port;
            }
            if (!sKey.empty()) {
                peer.sessionKey = sKey;
            }

            if (signalType == "SDP_OFFER") {
                if (peer.udpPort > 0 && !peer.sessionKey.empty()) {
                    peer.handshakeComplete = true;
                }
                activeRoom = _impl->roomId;
                localKey = _impl->localSessionKey;
                localPort = _impl->boundUdpPort;
                sendAnswer = true;
            } else if (signalType == "SDP_ANSWER") {
                if (peer.udpPort > 0 && !peer.sessionKey.empty()) {
                    peer.handshakeComplete = true;
                }
            } else if (signalType == "ICE_CANDIDATE") {
                if (peer.udpPort > 0 && !peer.sessionKey.empty()) {
                    peer.handshakeComplete = true;
                }
            }
        }

        if (sendAnswer && _impl->netClient != nullptr && !activeRoom.empty()) {
            nlohmann::json answerPayload = {
                {"udp_host", "127.0.0.1"},
                {"udp_port", localPort},
                {"codec", "pcmu-16k-20ms"},
                {"session_key", localKey}
            };
            _impl->netClient->submitVoiceSignal(
                activeRoom, fromUuid, "SDP_ANSWER", answerPayload.dump());

            nlohmann::json icePayload = {
                {"udp_host", "127.0.0.1"},
                {"udp_port", localPort}
            };
            _impl->netClient->submitVoiceSignal(
                activeRoom, fromUuid, "ICE_CANDIDATE", icePayload.dump());
        }
        return;
    }

    if (ev.event == "VOICE_STATE" || ev.event == "VOICE_META") {
        std::string peerUuid = j.value("player_uuid", "");
        std::string action = j.value("action", "");
        std::lock_guard<std::mutex> lock(_impl->mutex);
        if (action == "LEFT_VOICE") {
            _impl->peers.erase(peerUuid);
        } else if (action == "MIC_MUTED" || action == "MIC_UNMUTED") {
            auto it = _impl->peers.find(peerUuid);
            if (it != _impl->peers.end()) {
                it->second.micMuted = (action == "MIC_MUTED");
            }
        }
        return;
    }

    if (ev.event == "USER_MUTED") {
        std::string targetUuid =
            j.value("target", j.value("player_uuid", ""));
        bool muteVoice = j.value("mute_voice", false);
        std::string activeRoom;
        bool needNotifyMuted = false;

        {
            std::lock_guard<std::mutex> lock(_impl->mutex);
            if (targetUuid == _impl->localPlayerUuid) {
                _impl->voiceMutedByServer.store(muteVoice);
                if (muteVoice && _impl->pttActive.exchange(false)) {
                    activeRoom = _impl->roomId;
                    needNotifyMuted = true;
                }
            } else {
                auto it = _impl->peers.find(targetUuid);
                if (it != _impl->peers.end()) {
                    it->second.mutedByServer = muteVoice;
                }
            }
        }

        if (needNotifyMuted && _impl->netClient != nullptr &&
            !activeRoom.empty()) {
            _impl->netClient->submitVoiceMeta(activeRoom, "MIC_MUTED");
        }
        return;
    }

    if (ev.event == "OBSERVER_BANNED") {
        std::string targetUuid =
            j.value("target", j.value("player_uuid", ""));
        bool isSelf = false;
        {
            std::lock_guard<std::mutex> lock(_impl->mutex);
            if (targetUuid == _impl->localPlayerUuid) {
                isSelf = true;
            } else {
                _impl->peers.erase(targetUuid);
            }
        }
        if (isSelf) {
            leaveVoiceRoom();
        }
        return;
    }

    if (ev.event == "POLICY_UPDATED") {
        std::string voicePolicy = j.value("voice_policy", "");
        std::string activeRoom;
        bool notifyMuted = false;
        {
            std::lock_guard<std::mutex> lock(_impl->mutex);
            if (voicePolicy == "DISABLED") {
                _impl->voicePolicyDisabled.store(true);
                if (_impl->pttActive.exchange(false)) {
                    activeRoom = _impl->roomId;
                    notifyMuted = true;
                }
                _impl->peers.clear();
            } else if (!voicePolicy.empty()) {
                _impl->voicePolicyDisabled.store(false);
            }
        }
        if (notifyMuted && _impl->netClient != nullptr && !activeRoom.empty()) {
            _impl->netClient->submitVoiceMeta(activeRoom, "MIC_MUTED");
        }
        return;
    }
}

std::size_t P2pVoiceEngine::transmitPcmFrame20Ms(const std::int16_t *pcm320)
{
    if (pcm320 == nullptr || !_impl->pttActive.load() ||
        _impl->voiceMutedByServer.load() ||
        _impl->voicePolicyDisabled.load()) {
        return 0;
    }

    SocketHandle sock = kInvalidSocket;
    std::string sessionKey;
    std::vector< std::pair<std::string, int> > targets;

    {
        std::lock_guard<std::mutex> lock(_impl->mutex);
        if (!_impl->inVoiceRoom || _impl->udpSocket == kInvalidSocket) {
            return 0;
        }
        sock = _impl->udpSocket;
        sessionKey = _impl->localSessionKey;
        for (const auto &kv : _impl->peers) {
            const PeerVoiceEndpoint &p = kv.second;
            if (p.handshakeComplete && !p.mutedByServer && p.udpPort > 0) {
                targets.emplace_back(p.udpHost, p.udpPort);
            }
        }
    }

    if (targets.empty()) {
        return 0;
    }

    VoiceUdpPacket pkt;
    std::memcpy(pkt.magic, "OCTV", 4);
    pkt.seq = _impl->txSeq.fetch_add(1);
    pkt.timestampMs = currentMonotonicMs32();
    for (std::size_t i = 0; i < VoiceUdpPacket::kPayloadSamples; ++i) {
        pkt.payload[i] = linear16ToUlaw(pcm320[i]);
    }
    pkt.authTag = computeAuthTag(pkt.seq, pkt.timestampMs, pkt.payload,
                                 sessionKey);
    std::vector<std::uint8_t> wire = serializePacket(pkt);

    std::size_t sentCount = 0;
    for (const auto &target : targets) {
        sockaddr_in dstAddr;
        std::memset(&dstAddr, 0, sizeof(dstAddr));
        dstAddr.sin_family = AF_INET;
        dstAddr.sin_port = htons(static_cast<std::uint16_t>(target.second));
        if (::inet_pton(AF_INET, target.first.c_str(), &dstAddr.sin_addr) != 1) {
            dstAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        }
#ifdef _WIN32
        int res = ::sendto(sock,
                           reinterpret_cast<const char *>(wire.data()),
                           static_cast<int>(wire.size()),
                           0,
                           reinterpret_cast<const sockaddr *>(&dstAddr),
                           static_cast<int>(sizeof(dstAddr)));
#else
        ssize_t res = ::sendto(sock,
                               wire.data(),
                               wire.size(),
                               0,
                               reinterpret_cast<const sockaddr *>(&dstAddr),
                               sizeof(dstAddr));
#endif
        if (res == static_cast<int>(wire.size())) {
            ++sentCount;
            _impl->totalSent.fetch_add(1);
        }
    }

    return sentCount;
}

std::size_t P2pVoiceEngine::injectTestPcmFrame(const std::int16_t *samples,
                                               std::size_t count)
{
    if (samples == nullptr || count < VoiceUdpPacket::kPayloadSamples) {
        return 0;
    }
    return transmitPcmFrame20Ms(samples);
}

std::size_t P2pVoiceEngine::activePeerCount() const
{
    std::lock_guard<std::mutex> lock(_impl->mutex);
    std::size_t count = 0;
    for (const auto &kv : _impl->peers) {
        if (kv.second.handshakeComplete && !kv.second.mutedByServer) {
            ++count;
        }
    }
    return count;
}

bool P2pVoiceEngine::isPeerConnected(const std::string &peerUuid) const
{
    std::lock_guard<std::mutex> lock(_impl->mutex);
    auto it = _impl->peers.find(peerUuid);
    if (it == _impl->peers.end()) {
        return false;
    }
    return it->second.handshakeComplete && !it->second.mutedByServer;
}

std::uint64_t P2pVoiceEngine::totalPacketsSent() const
{
    return _impl->totalSent.load();
}

std::uint64_t P2pVoiceEngine::totalPacketsReceived() const
{
    return _impl->totalReceived.load();
}

std::uint64_t P2pVoiceEngine::totalInvalidPacketsDropped() const
{
    return _impl->totalDropped.load();
}

std::uint64_t P2pVoiceEngine::packetsReceivedFrom(
    const std::string &peerUuid) const
{
    std::lock_guard<std::mutex> lock(_impl->mutex);
    auto it = _impl->peers.find(peerUuid);
    if (it == _impl->peers.end()) {
        return 0;
    }
    return it->second.packetsReceived;
}

double P2pVoiceEngine::lastReceivedRmsFrom(const std::string &peerUuid) const
{
    std::lock_guard<std::mutex> lock(_impl->mutex);
    auto it = _impl->peers.find(peerUuid);
    if (it == _impl->peers.end()) {
        return 0.0;
    }
    return it->second.lastRms;
}

std::vector<std::int16_t> P2pVoiceEngine::popDecodedFrameFrom(
    const std::string &peerUuid)
{
    std::lock_guard<std::mutex> lock(_impl->mutex);
    auto it = _impl->peers.find(peerUuid);
    if (it == _impl->peers.end() || it->second.jitterBuffer.empty()) {
        return {};
    }
    std::vector<std::int16_t> frame =
        std::move(it->second.jitterBuffer.front());
    it->second.jitterBuffer.pop_front();
    return frame;
}

void P2pVoiceEngine::setHardwareAudioEnabled(bool enabled)
{
    std::lock_guard<std::mutex> lock(_impl->mutex);
    _impl->hardwareAudioEnabled.store(enabled);
    if (!enabled) {
        _impl->closeHardwareAudio();
    } else if (_impl->inVoiceRoom) {
        _impl->openHardwareAudioIfEnabled();
    }
}

bool P2pVoiceEngine::isHardwareAudioEnabled() const
{
    return _impl->hardwareAudioEnabled.load();
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
