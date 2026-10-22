/*
 * P2pVoiceEngine.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef OCTI_DESKTOP_P2P_VOICE_ENGINE_HXX
#define OCTI_DESKTOP_P2P_VOICE_ENGINE_HXX

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "OctiNetworkClient.hxx"

namespace octi {

/*
 * 336-byte P2P UDP voice packet format:
 *   - 4 bytes magic: "OCTV"
 *   - 4 bytes sequence number (network byte order on wire)
 *   - 4 bytes timestamp in milliseconds (network byte order on wire)
 *   - 4 bytes keyed authentication tag (FNV-1a / Sip-like MAC over header +
 *     payload + sessionKey, network byte order on wire)
 *   - 320 bytes G.711 u-law payload (20 ms @ 16 kHz mono)
 */
struct VoiceUdpPacket {
    static constexpr std::size_t kPacketBytes = 336;
    static constexpr std::size_t kPayloadSamples = 320;

    char magic[4];
    std::uint32_t seq = 0;
    std::uint32_t timestampMs = 0;
    std::uint32_t authTag = 0;
    std::uint8_t payload[kPayloadSamples];
};

/*
 * Push-to-Talk (PTT) Peer-to-Peer UDP Voice Engine.
 *
 * Coordinates session setup via OctiNetworkClient (/voice-signal and
 * /voice-meta) and streams 16 kHz mono 20 ms G.711 u-law audio frames directly
 * peer-to-peer over UDP strictly while Push-to-Talk is active. Zero audio bytes
 * are ever routed through octi_server.
 */
class P2pVoiceEngine {
public:
    explicit P2pVoiceEngine(OctiNetworkClient *netClient);
    ~P2pVoiceEngine();

    P2pVoiceEngine(const P2pVoiceEngine &) = delete;
    P2pVoiceEngine &operator=(const P2pVoiceEngine &) = delete;

    /* ---- G.711 u-law Codec ---- */
    static std::uint8_t linear16ToUlaw(std::int16_t pcm);
    static std::int16_t ulawToLinear16(std::uint8_t ulaw);

    /* ---- Wire Framing & Authentication ---- */
    static std::uint32_t computeAuthTag(
        std::uint32_t seq,
        std::uint32_t timestampMs,
        const std::uint8_t payload[VoiceUdpPacket::kPayloadSamples],
        const std::string &sessionKey);

    static std::vector<std::uint8_t> serializePacket(
        const VoiceUdpPacket &pkt);

    static bool parseAndVerifyPacket(
        const std::uint8_t *data,
        std::size_t len,
        const std::string &expectedSessionKey,
        VoiceUdpPacket &outPkt);

    /* ---- Room Lifecycle ---- */
    bool joinVoiceRoom(const std::string &roomId,
                       const std::string &localPlayerUuid,
                       std::string &errorOut);
    void leaveVoiceRoom();
    bool isInVoiceRoom() const;

    int localUdpPort() const;
    std::string localSessionKey() const;

    /* ---- Push-to-Talk (PTT) Control ---- */
    bool setPushToTalkActive(bool talking);
    bool isPushToTalkActive() const;
    bool isVoiceMutedByServer() const;
    bool isVoicePolicyDisabled() const;

    /* ---- Signaling & Server Event Handling ---- */
    bool initiatePeerOffer(const std::string &peerPlayerUuid);
    void handleGameSseEvent(const SseEvent &ev);

    /* ---- Audio Frame Transmission & Reception ---- */
    std::size_t transmitPcmFrame20Ms(const std::int16_t *pcm320);
    std::size_t injectTestPcmFrame(const std::int16_t *samples,
                                   std::size_t count);

    std::size_t activePeerCount() const;
    bool isPeerConnected(const std::string &peerUuid) const;

    std::uint64_t totalPacketsSent() const;
    std::uint64_t totalPacketsReceived() const;
    std::uint64_t totalInvalidPacketsDropped() const;
    std::uint64_t packetsReceivedFrom(const std::string &peerUuid) const;
    double lastReceivedRmsFrom(const std::string &peerUuid) const;
    double receivedAudioRms(const std::string &peerUuid) const
    {
        return lastReceivedRmsFrom(peerUuid);
    }

    std::vector<std::int16_t> popDecodedFrameFrom(
        const std::string &peerUuid);

    void setHardwareAudioEnabled(bool enabled);
    bool isHardwareAudioEnabled() const;

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

} // namespace octi

#endif // OCTI_DESKTOP_P2P_VOICE_ENGINE_HXX

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
