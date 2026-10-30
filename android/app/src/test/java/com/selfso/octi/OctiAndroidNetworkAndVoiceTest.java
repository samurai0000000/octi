/*
 * OctiAndroidNetworkAndVoiceTest.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.HttpURLConnection;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.attribute.PosixFilePermission;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

/**
 * Envelope 2 (Gate 2) qualification suite for {@link ClientConfigStore},
 * {@link OctiNetworkClient}, and {@link P2pVoiceEngine}.
 */
public class OctiAndroidNetworkAndVoiceTest {

    private File _tempDir;
    private Process _serverProc;
    private int _serverPort;
    private File _sqliteDbFile;

    @Before
    public void setUp() throws Exception {
        _tempDir = Files.createTempDirectory("octi-android-net-test-").toFile();
    }

    @After
    public void tearDown() {
        stopLiveServer();
        if (_tempDir != null && _tempDir.exists()) {
            deleteRecursively(_tempDir);
        }
    }

    @Test
    public void testHardCodedServerEndpointAndZeroOverrideSurface() {
        assertEquals("octi.selfso.com", OctiDefs.OCTI_SERVER_HOST);
        assertEquals(443, OctiDefs.OCTI_SERVER_PORT);
        assertEquals("octi.selfso.com", OctiNetworkClient.canonicalHost());
        assertEquals(443, OctiNetworkClient.canonicalPort());

        try (OctiNetworkClient prodClient = new OctiNetworkClient()) {
            assertEquals("octi.selfso.com", prodClient.host());
            assertEquals(443, prodClient.port());
            assertEquals("octi.selfso.com", prodClient.serverHost());
            assertEquals(443, prodClient.serverPort());
            assertEquals("https://octi.selfso.com", prodClient.baseUrl());
        }

        for (Method m : OctiNetworkClient.class.getDeclaredMethods()) {
            if (Modifier.isPublic(m.getModifiers())) {
                String name = m.getName().toLowerCase();
                assertFalse("Unexpected public endpoint mutator: " + m.getName(),
                        name.contains("sethost") ||
                                name.contains("setport") ||
                                name.contains("setserver") ||
                                name.contains("setbaseurl") ||
                                name.contains("setendpoint"));
            }
        }
    }

    @Test
    public void testClientConfigStorePermissionsAndForbiddenServerKeyStripping() throws Exception {
        File cfgDir = new File(_tempDir, "secure_cfg");
        File cfgFile = new File(cfgDir, "client_config.json");
        ClientConfigStore store = new ClientConfigStore(cfgFile);

        ClientConfigStore.ClientPreferences prefs = store.preferences();
        prefs.sessionToken = "tok-secret-alpha-999";
        prefs.playerUuid = "p-uuid-1234";
        prefs.handle = "ObsidianMaster";
        prefs.theme = OctiThemeId.ROSEWOOD_IVORY;
        prefs.confirmAllMoves = true;
        prefs.sfxMuted = false;
        prefs.volumeKeyPtt = true;

        assertTrue(store.save());
        assertTrue(cfgFile.exists());

        try {
            Set<PosixFilePermission> filePerms = Files.getPosixFilePermissions(cfgFile.toPath());
            assertTrue(filePerms.contains(PosixFilePermission.OWNER_READ));
            assertTrue(filePerms.contains(PosixFilePermission.OWNER_WRITE));
            assertFalse(filePerms.contains(PosixFilePermission.GROUP_READ));
            assertFalse(filePerms.contains(PosixFilePermission.GROUP_WRITE));
            assertFalse(filePerms.contains(PosixFilePermission.OTHERS_READ));
            assertFalse(filePerms.contains(PosixFilePermission.OTHERS_WRITE));

            Set<PosixFilePermission> dirPerms = Files.getPosixFilePermissions(cfgDir.toPath());
            assertTrue(dirPerms.contains(PosixFilePermission.OWNER_READ));
            assertTrue(dirPerms.contains(PosixFilePermission.OWNER_WRITE));
            assertTrue(dirPerms.contains(PosixFilePermission.OWNER_EXECUTE));
            assertFalse(dirPerms.contains(PosixFilePermission.GROUP_READ));
            assertFalse(dirPerms.contains(PosixFilePermission.OTHERS_READ));
        } catch (UnsupportedOperationException ignored) {
            // Non-POSIX filesystems fall back to File.setReadable/setWritable.
        }

        // Inject rogue server_url / server_host / server_port keys into the JSON file.
        String rogueJson = "{\n"
                + "  \"session_token\": \"tok-secret-alpha-999\",\n"
                + "  \"player_uuid\": \"p-uuid-1234\",\n"
                + "  \"handle\": \"ObsidianMaster\",\n"
                + "  \"theme\": \"ROSEWOOD_IVORY\",\n"
                + "  \"confirm_all_moves\": true,\n"
                + "  \"sfx_muted\": false,\n"
                + "  \"volume_key_ptt\": true,\n"
                + "  \"server_url\": \"https://rogue.example.invalid:9999\",\n"
                + "  \"server_host\": \"rogue.example.invalid\",\n"
                + "  \"server_port\": 9999,\n"
                + "  \"host\": \"10.99.99.99\",\n"
                + "  \"port\": 7777\n"
                + "}\n";
        Files.write(cfgFile.toPath(), rogueJson.getBytes(StandardCharsets.UTF_8));
        assertTrue(ClientConfigStore.containsForbiddenServerOverride(rogueJson));

        ClientConfigStore verifyStore = new ClientConfigStore(cfgFile);
        assertTrue(verifyStore.load());
        ClientConfigStore.ClientPreferences loaded = verifyStore.preferences();
        assertEquals("tok-secret-alpha-999", loaded.sessionToken);
        assertEquals("p-uuid-1234", loaded.playerUuid);
        assertEquals("ObsidianMaster", loaded.handle);
        assertEquals(OctiThemeId.ROSEWOOD_IVORY, loaded.theme);
        assertTrue(loaded.confirmAllMoves);
        assertFalse(loaded.sfxMuted);
        assertTrue(loaded.volumeKeyPtt);

        // Re-save and verify all forbidden keys are stripped from disk.
        assertTrue(verifyStore.save());
        String cleanedDiskJson = new String(Files.readAllBytes(cfgFile.toPath()), StandardCharsets.UTF_8);
        assertFalse(ClientConfigStore.containsForbiddenServerOverride(cleanedDiskJson));
        assertFalse(cleanedDiskJson.contains("rogue.example.invalid"));
    }

    @Test
    public void testOAuthLoopbackListenerCapturesBrowserCallback() throws Exception {
        try (OctiNetworkClient.OAuthLoopbackListener listener =
                     new OctiNetworkClient.OAuthLoopbackListener()) {
            assertTrue(listener.start());
            assertTrue(listener.boundPort() > 0);
            assertTrue(listener.callbackUrl().startsWith("http://127.0.0.1:" + listener.boundPort() + "/callback"));

            URL callbackUrl = new URL(listener.callbackUrl()
                    + "?id_token=eyJhbGciOiJSUzI1NiJ9.google.sig&provider=google");
            HttpURLConnection conn = (HttpURLConnection) callbackUrl.openConnection();
            conn.setRequestMethod("GET");
            assertEquals(200, conn.getResponseCode());
            String html;
            try (BufferedReader br = new BufferedReader(
                    new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8))) {
                StringBuilder sb = new StringBuilder();
                String line;
                while ((line = br.readLine()) != null) {
                    sb.append(line).append('\n');
                }
                html = sb.toString();
            }
            assertTrue(html.contains("OCTI"));

            String[] provAndToken = new String[2];
            assertTrue(listener.waitForCallback(2000L, provAndToken));
            assertEquals("google", provAndToken[0]);
            assertEquals("eyJhbGciOiJSUzI1NiJ9.google.sig", provAndToken[1]);
            assertEquals("eyJhbGciOiJSUzI1NiJ9.google.sig", listener.getReceivedIdToken());
        }
    }

    @Test
    public void testFiveProviderSsoOnboardingAndAccountLinkConflictViaProtocolHarness() throws Exception {
        try (EmbeddedSsoProtocolServer ssoServer = new EmbeddedSsoProtocolServer();
             OctiNetworkClient client = OctiNetworkClient.forHermeticTest(ssoServer.port())) {

            String[] providers = new String[] { "google", "apple", "microsoft", "facebook", "line" };
            for (int i = 0; i < providers.length; ++i) {
                String prov = providers[i];
                // First login -> 201 Created onboarding
                OctiNetworkClient.ApiResult<OctiNetworkClient.SsoLoginData> first =
                        client.loginWithSso(prov, "token-new-" + prov);
                assertTrue("Failed first SSO login for " + prov, first.ok);
                assertEquals(201, first.httpStatus);
                assertTrue(first.data.isNewAccount);
                assertTrue(first.data.onboardingRequired);
                assertEquals("p-" + prov, first.data.profile.uuid);
                assertFalse(first.data.sessionToken.isEmpty());

                // Repeat login -> 200 OK existing account
                OctiNetworkClient.ApiResult<OctiNetworkClient.SsoLoginData> repeat =
                        client.loginWithSso(prov, "token-existing-" + prov);
                assertTrue(repeat.ok);
                assertEquals(200, repeat.httpStatus);
                assertFalse(repeat.data.isNewAccount);
                assertFalse(repeat.data.onboardingRequired);
            }

            // Nominal multi-provider link (200 OK)
            OctiNetworkClient.ApiResult<OctiNetworkClient.LinkIdentityData> linked =
                    client.linkIdentity("apple", "token-link-apple-ok");
            assertTrue(linked.ok);
            assertEquals(200, linked.httpStatus);
            assertFalse(linked.data.conflictAlreadyLinked);
            assertEquals(2, linked.data.identities.size());

            // 409 Conflict when linking an identity bound to another OCTI profile
            OctiNetworkClient.ApiResult<OctiNetworkClient.LinkIdentityData> conflict =
                    client.linkIdentity("microsoft", "token-conflict-ms");
            assertFalse(conflict.ok);
            assertEquals(409, conflict.httpStatus);
            assertTrue(conflict.data.conflictAlreadyLinked);
            assertEquals("p-existing-ms-owner", conflict.data.existingProfile.uuid);
            assertEquals("GrandmasterVega", conflict.data.existingProfile.handle);
            assertEquals(1680, conflict.data.existingProfile.elo2p);
        }
    }

    @Test
    public void testLiveOctiServerEndToEndRestSseMatchmakingVoiceAndModeration() throws Exception {
        startLiveServer();

        String aliceUuid = "11111111-1111-4111-8111-111111111111";
        String bobUuid = "22222222-2222-4222-8222-222222222222";
        String charlieUuid = "33333333-3333-4333-8333-333333333333";

        String aliceToken = "tok-live-alice-session-0001";
        String bobToken = "tok-live-bob-session-0002";
        String charlieToken = "tok-live-charlie-session-0003";

        seedLivePlayer(1, aliceUuid, "Alice", aliceToken, "google", "sub-alice-g", "apple", "sub-alice-a");
        seedLivePlayer(2, bobUuid, "Bob", bobToken, "microsoft", "sub-bob-m", null, null);
        seedLivePlayer(3, charlieUuid, "Charlie", charlieToken, "line", "sub-charlie-l", null, null);

        try (OctiNetworkClient clientA = OctiNetworkClient.forHermeticTest(_serverPort);
             OctiNetworkClient clientB = OctiNetworkClient.forHermeticTest(_serverPort);
             OctiNetworkClient clientC = OctiNetworkClient.forHermeticTest(_serverPort)) {

            clientA.setSessionToken(aliceToken);
            clientB.setSessionToken(bobToken);
            clientC.setSessionToken(charlieToken);

            // 1. Health & Providers
            OctiNetworkClient.ApiResult<OctiNetworkClient.HealthInfo> health = clientA.fetchHealth();
            assertTrue(health.ok);
            assertEquals("ok", health.data.status);

            OctiNetworkClient.ApiResult<List<OctiNetworkClient.AuthProviderInfo>> provs =
                    clientA.fetchProviders();
            assertTrue(provs.ok);
            assertEquals(5, provs.data.size());

            // 2. Profile & Identity Unlink (2->1 succeeds; 1->0 rejected with 400)
            OctiNetworkClient.ApiResult<OctiNetworkClient.PlayerProfileInfo> meA = clientA.fetchMyProfile();
            assertTrue(meA.ok);
            assertEquals(aliceUuid, meA.data.uuid);
            assertEquals(2, meA.data.identities.size());

            OctiNetworkClient.ApiResult<OctiNetworkClient.PlayerProfileInfo> updatedA =
                    clientA.updateMyProfile("LadyAlice");
            assertTrue(updatedA.ok);
            assertEquals("LadyAlice", updatedA.data.handle);

            OctiNetworkClient.ApiResult<OctiNetworkClient.PlayerProfileInfo> pubA =
                    clientB.fetchPlayerProfile(aliceUuid);
            assertTrue(pubA.ok);
            assertEquals("LadyAlice", pubA.data.handle);

            OctiNetworkClient.ApiResult<OctiNetworkClient.UnlinkIdentityData> unlinkSecond =
                    clientA.unlinkIdentity("apple");
            assertTrue(unlinkSecond.ok);
            assertEquals(1, unlinkSecond.data.identities.size());

            OctiNetworkClient.ApiResult<OctiNetworkClient.UnlinkIdentityData> unlinkLast =
                    clientA.unlinkIdentity("google");
            assertFalse("Unlinking sole remaining identity must fail with 400", unlinkLast.ok);
            assertEquals(400, unlinkLast.httpStatus);

            // 3. Leaderboard
            OctiNetworkClient.ApiResult<List<OctiNetworkClient.LeaderboardRow>> lb =
                    clientA.fetchLeaderboard("2P", 10);
            assertTrue(lb.ok);
            assertTrue(lb.data.size() >= 3);

            // 4. Lobby SSE stream & Lobby Chat
            CountDownLatch lobbyChatLatch = new CountDownLatch(1);
            List<OctiNetworkClient.SseEvent> lobbyEvents = new CopyOnWriteArrayList<>();
            clientA.startLobbyEventStream(ev -> {
                lobbyEvents.add(ev);
                if ("LOBBY_CHAT".equals(ev.event)) {
                    lobbyChatLatch.countDown();
                }
            });

            OctiNetworkClient.ApiResult<Long> chatPost =
                    clientB.postLobbyChat("Hello OCTI Lobby from Bob!");
            assertTrue(chatPost.ok);
            assertTrue("Expected LOBBY_CHAT SSE event", lobbyChatLatch.await(3, TimeUnit.SECONDS));

            OctiNetworkClient.ApiResult<OctiNetworkClient.LobbyStateData> lobbyState =
                    clientA.fetchLobbyState();
            assertTrue(lobbyState.ok);
            assertFalse(lobbyState.data.recentChat.isEmpty());

            // 5. Matchmaking enqueue / status / dequeue
            OctiNetworkClient.MatchmakingRequest mmReq = new OctiNetworkClient.MatchmakingRequest();
            mmReq.mode = "2P";
            mmReq.bases = 3;
            mmReq.timeControlEnabled = true;
            mmReq.initialMs = 600000L;
            mmReq.incrementMs = 5000L;
            OctiNetworkClient.ApiResult<Boolean> mmEnq = clientC.enqueueMatchmaking(mmReq);
            assertTrue(mmEnq.ok);

            OctiNetworkClient.ApiResult<OctiNetworkClient.MatchmakingStatusData> mmStat =
                    clientC.fetchMatchmakingStatus();
            assertTrue(mmStat.ok);
            assertTrue(mmStat.data.queued);

            OctiNetworkClient.ApiResult<Boolean> mmDeq = clientC.dequeueMatchmaking();
            assertTrue(mmDeq.ok);

            // 6. Pre-game 4P Table creation to test kickSeat & leaveTable before game start
            OctiNetworkClient.CreateTableRequest req4p = new OctiNetworkClient.CreateTableRequest();
            req4p.mode = "4P";
            req4p.bases = 3;
            req4p.timeControlEnabled = true;
            req4p.initialMs = 600000L;
            req4p.incrementMs = 5000L;
            OctiNetworkClient.ApiResult<OctiNetworkClient.CreateTableResponse> preTable =
                    clientA.createTable(req4p);
            assertTrue("4P createTable failed: " + preTable.errorMessage, preTable.ok);
            String preRoomId = preTable.data.roomId;

            assertTrue(clientB.joinTable(preRoomId, 2, "").ok);
            assertTrue(clientA.kickSeat(preRoomId, bobUuid).ok);
            assertTrue(clientA.leaveTable(preRoomId).ok);

            // 7. Create Passcode-Protected 2P Table, test wrong passcode (403), then start game
            OctiNetworkClient.CreateTableRequest req2p = new OctiNetworkClient.CreateTableRequest();
            req2p.mode = "2P";
            req2p.bases = 1;
            req2p.superProngs = true;
            req2p.edgeless = false;
            req2p.rated = false;
            req2p.allowTakeback = true;
            req2p.timeControlEnabled = true;
            req2p.initialMs = 600000L;
            req2p.incrementMs = 5000L;
            req2p.passcode = "brass42";
            req2p.observerPolicy = "OPEN";
            req2p.voicePolicy = "PLAYERS_AND_OBSERVERS";

            OctiNetworkClient.ApiResult<OctiNetworkClient.CreateTableResponse> created2p =
                    clientA.createTable(req2p);
            assertTrue("2P createTable failed: " + created2p.errorMessage, created2p.ok);
            String roomId = created2p.data.roomId;

            OctiNetworkClient.ApiResult<List<OctiNetworkClient.TableSummaryInfo>> tables =
                    clientB.listTables();
            assertTrue(tables.ok);
            assertFalse(tables.data.isEmpty());

            OctiNetworkClient.ApiResult<OctiNetworkClient.JoinTableResponse> wrongPass =
                    clientB.joinTable(roomId, 2, "wrong-pass");
            assertFalse(wrongPass.ok);
            assertEquals(403, wrongPass.httpStatus);

            OctiNetworkClient.ApiResult<OctiNetworkClient.JoinTableResponse> joined2p =
                    clientB.joinTable(roomId, 2, "brass42");
            assertTrue("joinTable failed: " + joined2p.errorMessage, joined2p.ok);
            assertTrue(joined2p.data.gameStarted);

            // Charlie joins as observer
            assertTrue(clientC.observeTable(roomId, "brass42").ok);

            // 8. Attach Game SSE streams for Alice and Bob + P2pVoiceEngine instances
            try (P2pVoiceEngine voiceA = new P2pVoiceEngine(clientA);
                 P2pVoiceEngine voiceB = new P2pVoiceEngine(clientB)) {

                CountDownLatch moveLatchB = new CountDownLatch(1);
                CountDownLatch gameChatLatchB = new CountDownLatch(1);

                clientA.startGameEventStream(roomId, voiceA::handleGameSseEvent);
                clientB.startGameEventStream(roomId, ev -> {
                    voiceB.handleGameSseEvent(ev);
                    if ("MOVE".equals(ev.event) || "MOVE_APPLIED".equals(ev.event)) {
                        moveLatchB.countDown();
                    } else if ("CHAT".equals(ev.event) || "CHAT_MESSAGE".equals(ev.event)) {
                        gameChatLatchB.countDown();
                    }
                });

                // 9. P2P Voice Join, Signaling Handshake, PTT Gating & 440 Hz Audio Streaming
                assertTrue(voiceA.joinVoiceRoom(roomId, aliceUuid));
                assertTrue(voiceB.joinVoiceRoom(roomId, bobUuid));
                assertTrue(voiceA.isInVoiceRoom());
                assertTrue(voiceB.isInVoiceRoom());
                assertTrue(voiceA.localUdpPort() > 0);
                assertTrue(voiceB.localUdpPort() > 0);
                assertFalse(voiceA.isPushToTalkActive());
                assertFalse(voiceB.isPushToTalkActive());

                assertTrue(voiceA.initiatePeerOffer(bobUuid));

                long deadline = System.currentTimeMillis() + 3000L;
                while (System.currentTimeMillis() < deadline &&
                        (!voiceA.isPeerConnected(bobUuid) || !voiceB.isPeerConnected(aliceUuid))) {
                    Thread.sleep(25L);
                }
                assertTrue("Alice must complete voice handshake with Bob",
                        voiceA.isPeerConnected(bobUuid));
                assertTrue("Bob must complete voice handshake with Alice",
                        voiceB.isPeerConnected(aliceUuid));

                // Synthesize 440 Hz sine-wave 20 ms frame (320 samples @ 16 kHz)
                short[] sine440 = new short[P2pVoiceEngine.PAYLOAD_SAMPLES];
                for (int i = 0; i < sine440.length; ++i) {
                    sine440[i] = (short) Math.round(12000.0 * Math.sin(
                            2.0 * Math.PI * 440.0 * i / P2pVoiceEngine.SAMPLE_RATE_HZ));
                }

                // PTT inactive -> ZERO UDP packets transmitted
                for (int i = 0; i < 10; ++i) {
                    assertEquals(0, voiceA.injectTestPcmFrame(sine440));
                }
                assertEquals(0L, voiceA.totalPacketsSent());
                assertEquals(0L, voiceB.totalPacketsReceived());

                // Press PTT -> MIC_UNMUTED + stream UDP audio frames
                assertTrue(voiceA.setPushToTalkActive(true));
                assertTrue(voiceA.isPushToTalkActive());

                for (int i = 0; i < 15; ++i) {
                    int sent = voiceA.injectTestPcmFrame(sine440);
                    assertEquals(1, sent);
                    Thread.sleep(5L);
                }

                long rxDeadline = System.currentTimeMillis() + 2000L;
                while (System.currentTimeMillis() < rxDeadline &&
                        voiceB.packetsReceivedFrom(aliceUuid) < 5L) {
                    Thread.sleep(20L);
                }
                assertTrue("Bob should receive UDP voice frames from Alice",
                        voiceB.packetsReceivedFrom(aliceUuid) >= 5L);
                assertTrue("Decoded 440 Hz RMS energy must be non-zero",
                        voiceB.receivedAudioRms(aliceUuid) > 4000.0);

                short[] decodedFrame = voiceB.popDecodedFrameFrom(aliceUuid);
                assertEquals(P2pVoiceEngine.PAYLOAD_SAMPLES, decodedFrame.length);

                // Release PTT -> MIC_MUTED + zero packets sent afterward
                assertTrue(voiceA.setPushToTalkActive(false));
                assertFalse(voiceA.isPushToTalkActive());
                long sentBefore = voiceA.totalPacketsSent();
                assertEquals(0, voiceA.injectTestPcmFrame(sine440));
                assertEquals(sentBefore, voiceA.totalPacketsSent());

                // Forged UDP packet to Bob's voice port must be dropped
                try (DatagramSocket attackerSock = new DatagramSocket()) {
                    byte[] forged = new byte[P2pVoiceEngine.PACKET_BYTES];
                    forged[0] = 'O';
                    forged[1] = 'C';
                    forged[2] = 'T';
                    forged[3] = 'V';
                    forged[12] = 0x12;
                    forged[13] = 0x34;
                    forged[14] = 0x56;
                    forged[15] = 0x78;
                    DatagramPacket badDgram = new DatagramPacket(
                            forged, forged.length,
                            InetAddress.getByName("127.0.0.1"), voiceB.localUdpPort());
                    attackerSock.send(badDgram);
                }
                long dropDeadline = System.currentTimeMillis() + 1000L;
                while (System.currentTimeMillis() < dropDeadline &&
                        voiceB.totalInvalidPacketsDropped() < 1L) {
                    Thread.sleep(20L);
                }
                assertTrue(voiceB.totalInvalidPacketsDropped() >= 1L);

                // 10. Moderation Cutoff: Server USER_MUTED, OBSERVER_BANNED, POLICY_UPDATED(DISABLED)
                assertTrue(voiceA.setPushToTalkActive(true));
                assertTrue(voiceA.isPushToTalkActive());

                // Mute Charlie (observer) via live server REST API + simulate local mute enforcement
                assertTrue(clientA.muteParticipant(roomId, charlieUuid, true, true).ok);
                voiceA.handleGameSseEvent("USER_MUTED",
                        "{\"target\":\"" + aliceUuid + "\",\"mute_voice\":true}");
                assertTrue(voiceA.isVoiceMutedByServer());
                assertFalse("PTT must immediately deactivate when muted by server",
                        voiceA.isPushToTalkActive());
                assertEquals(0, voiceA.injectTestPcmFrame(sine440));

                // Unmute Alice so we can test POLICY_UPDATED(DISABLED) cutoff
                voiceA.handleGameSseEvent("USER_MUTED",
                        "{\"target\":\"" + aliceUuid + "\",\"mute_voice\":false}");
                assertFalse(voiceA.isVoiceMutedByServer());
                assertTrue(voiceA.setPushToTalkActive(true));

                // Ban Observer Charlie via live server
                assertTrue(clientA.banObserver(roomId, charlieUuid).ok);

                // Update room settings to disable voice -> tears down P2P voice session
                assertTrue(clientA.updateRoomSettings(roomId, "OPEN", "DISABLED").ok);
                long polDeadline = System.currentTimeMillis() + 2000L;
                while (System.currentTimeMillis() < polDeadline &&
                        !voiceA.isVoicePolicyDisabled()) {
                    Thread.sleep(20L);
                }
                assertTrue(voiceA.isVoicePolicyDisabled());
                assertFalse(voiceA.isPushToTalkActive());
                assertEquals(0, voiceA.activePeerCount());

                // 11. Live Game View, Moves, Chat, Proposals, Resign & Archive
                OctiNetworkClient.ApiResult<OctiNetworkClient.GameViewData> view0 =
                        clientA.fetchGameView(roomId);
                assertTrue(view0.ok);
                assertEquals(roomId, view0.data.roomId);

                OctiNetworkClient.ApiResult<OctiNetworkClient.MoveSubmitData> move1 =
                        clientA.submitMove(roomId, "33+a");
                assertTrue("Move 1 failed: " + move1.errorMessage, move1.ok);
                assertTrue("Bob should receive MOVE_APPLIED over SSE",
                        moveLatchB.await(3, TimeUnit.SECONDS));

                OctiNetworkClient.ApiResult<OctiNetworkClient.MoveSubmitData> move2 =
                        clientB.submitMove(roomId, "37+e");
                assertTrue("Move 2 failed: " + move2.errorMessage, move2.ok);

                assertTrue(clientA.postGameChat(roomId, "ALL", "Well played!").ok);
                assertTrue("Bob should receive CHAT_MESSAGE over SSE",
                        gameChatLatchB.await(3, TimeUnit.SECONDS));

                // Proposals: draw offer/decline, pause request/accept/resume, undo request/decline
                assertTrue(clientA.submitAction(roomId, "offer_draw").ok);
                assertTrue(clientB.submitAction(roomId, "decline_draw").ok);
                assertTrue(clientA.submitAction(roomId, "request_pause").ok);
                assertTrue(clientB.submitAction(roomId, "accept_pause").ok);
                assertTrue(clientA.submitAction(roomId, "resume").ok);
                assertTrue(clientB.submitAction(roomId, "request_undo").ok);
                assertTrue(clientA.submitAction(roomId, "decline_undo").ok);

                // Resign game & rematch proposal
                OctiNetworkClient.ApiResult<Boolean> resigned =
                        clientB.resignGame(roomId);
                assertTrue(resigned.ok);
                assertTrue(resigned.data);

                assertTrue(clientB.submitAction(roomId, "request_rematch").ok);
                assertTrue(clientA.submitAction(roomId, "decline_rematch").ok);

                // Archive list, detail, and .octi transcript download
                OctiNetworkClient.ApiResult<OctiNetworkClient.ArchivedGamesPage> archiveList =
                        clientA.listArchivedGames(20, 0);
                assertTrue(archiveList.ok);
                assertFalse(archiveList.data.games.isEmpty());

                OctiNetworkClient.ApiResult<OctiNetworkClient.ArchivedGameSummary> archiveDetail =
                        clientA.fetchArchivedGame(roomId);
                assertTrue(archiveDetail.ok);
                assertEquals(roomId, archiveDetail.data.gameId);
                assertEquals(2, archiveDetail.data.plyCount);

                OctiNetworkClient.ApiResult<String> transcript =
                        clientA.downloadOctiTranscript(roomId);
                assertTrue(transcript.ok);
                assertTrue(transcript.data.contains("33+a"));
                assertTrue(transcript.data.contains("37+e"));
            }

            // 12. Logout revokes session token
            assertTrue(clientC.logout().ok);
            OctiNetworkClient.ApiResult<OctiNetworkClient.PlayerProfileInfo> afterLogout =
                    clientC.fetchMyProfile();
            assertFalse(afterLogout.ok);
            assertEquals(401, afterLogout.httpStatus);

            // 13. Rate limit (429 Retry-After) on /api/v1/auth/sso burst (>30 per minute)
            OctiNetworkClient.ApiResult<OctiNetworkClient.SsoLoginData> rateLimited = null;
            for (int i = 0; i < 35; ++i) {
                OctiNetworkClient.ApiResult<OctiNetworkClient.SsoLoginData> r =
                        clientC.loginWithSso("google", "invalid.jwt.token");
                if (r.httpStatus == 429) {
                    rateLimited = r;
                    break;
                }
            }
            assertNotNull("Expected 429 Too Many Requests after burst on /api/v1/auth/sso", rateLimited);
            assertEquals(429, rateLimited.httpStatus);
            assertTrue(rateLimited.retryAfterSec > 0);
        }
    }

    @Test
    public void testPropertyMuLawCodecMonotonicityAndBoundedErrorAcrossAll65536PcmValues() {
        int prevAbsDecoded = 0;
        for (int mag = 0; mag <= 32767; ++mag) {
            short pos = (short) mag;
            byte uPos = P2pVoiceEngine.encodeMuLaw(pos);
            short decPos = P2pVoiceEngine.decodeMuLaw(uPos);
            assertTrue("Decoded positive sample must be >= 0 for input " + mag, decPos >= 0);
            assertTrue("G.711 mu-law decoding must be monotonic non-decreasing",
                    decPos >= prevAbsDecoded);
            prevAbsDecoded = decPos;

            int err = Math.abs(mag - decPos);
            int maxAllowedErr = Math.max(16, (int) (mag * 0.08) + 132);
            assertTrue("Quantization error too large at " + mag + ": decoded=" + decPos,
                    err <= maxAllowedErr);

            if (mag > 0) {
                short neg = (short) -mag;
                byte uNeg = P2pVoiceEngine.encodeMuLaw(neg);
                short decNeg = P2pVoiceEngine.decodeMuLaw(uNeg);
                assertEquals("Sign symmetry violated at " + mag, -decPos, decNeg);
            }
        }
        // Check Short.MIN_VALUE (-32768)
        byte uMin = P2pVoiceEngine.encodeMuLaw(Short.MIN_VALUE);
        short decMin = P2pVoiceEngine.decodeMuLaw(uMin);
        assertTrue(decMin < -31000);
    }

    @Test
    public void testPropertyVoiceUdpPacketParserFuzzing500Trials() {
        Random rng = new Random(0x0C712026L);
        String key = "0123456789abcdef0123456789abcdef";

        for (int trial = 0; trial < 500; ++trial) {
            P2pVoiceEngine.VoiceUdpPacket pkt = new P2pVoiceEngine.VoiceUdpPacket();
            pkt.seq = trial + 1;
            pkt.timestampMs = 100000 + trial * 20;
            rng.nextBytes(pkt.payload);
            pkt.authTag = P2pVoiceEngine.computeAuthTag(
                    pkt.seq, pkt.timestampMs, pkt.payload, key);

            byte[] wire = P2pVoiceEngine.serializePacket(pkt);
            assertEquals(P2pVoiceEngine.PACKET_BYTES, wire.length);

            P2pVoiceEngine.VoiceUdpPacket parsed = new P2pVoiceEngine.VoiceUdpPacket();
            assertTrue(P2pVoiceEngine.parseAndVerifyPacket(wire, wire.length, key, parsed));
            assertEquals(pkt.seq, parsed.seq);
            assertEquals(pkt.timestampMs, parsed.timestampMs);
            assertEquals(pkt.authTag, parsed.authTag);
            assertTrue(Arrays.equals(pkt.payload, parsed.payload));

            // Wrong session key must fail
            assertFalse(P2pVoiceEngine.parseAndVerifyPacket(
                    wire, wire.length, key + "x", parsed));

            // Single random bit flip anywhere in packet must fail verification
            byte[] flipped = Arrays.copyOf(wire, wire.length);
            int byteIdx = rng.nextInt(flipped.length);
            int bitMask = 1 << rng.nextInt(8);
            flipped[byteIdx] ^= (byte) bitMask;
            assertFalse(P2pVoiceEngine.parseAndVerifyPacket(
                    flipped, flipped.length, key, parsed));

            // Random truncated or oversized garbage buffer must never throw and must return false
            int randLen = rng.nextInt(512);
            if (randLen == P2pVoiceEngine.PACKET_BYTES) {
                randLen += 1;
            }
            byte[] garbage = new byte[randLen];
            rng.nextBytes(garbage);
            assertFalse(P2pVoiceEngine.parseAndVerifyPacket(
                    garbage, garbage.length, key, parsed));
            assertNull(P2pVoiceEngine.parseAndVerifyPacket(garbage, key));
        }
    }

    @Test
    public void testProtocolVersionConstantsAndHealthInfoParsing() {
        assertEquals(1, OctiDefs.OCTI_PROTOCOL_VERSION);
        assertEquals(1, OctiDefs.OCTI_MIN_PROTOCOL_VERSION);

        OctiNetworkClient.HealthInfo info = new OctiNetworkClient.HealthInfo();
        assertEquals(OctiDefs.OCTI_PROTOCOL_VERSION, info.protocolVersion);
        assertEquals(OctiDefs.OCTI_MIN_PROTOCOL_VERSION, info.minProtocolVersion);

        Map<String, Object> fakeJson = new HashMap<>();
        fakeJson.put("status", "ok");
        fakeJson.put("version", "2.0.7");
        fakeJson.put("protocol_version", 1);
        fakeJson.put("min_protocol_version", 1);

        info.protocolVersion = OctiMiniJson.getInt(fakeJson, "protocol_version", OctiDefs.OCTI_PROTOCOL_VERSION);
        info.minProtocolVersion = OctiMiniJson.getInt(fakeJson, "min_protocol_version", OctiDefs.OCTI_MIN_PROTOCOL_VERSION);
        assertEquals(1, info.protocolVersion);
        assertEquals(1, info.minProtocolVersion);
    }

    @Test
    public void testLiveServerRestartPreservesGameSseAndAllowsMoveContinuation() throws Exception {
        startLiveServer();
        int fixedPort = _serverPort;

        String aliceUuid = "11111111-1111-4111-8111-111111111111";
        String bobUuid = "22222222-2222-4222-8222-222222222222";
        String aliceToken = "tok-live-alice-restart-0001";
        String bobToken = "tok-live-bob-restart-0002";

        seedLivePlayer(1, aliceUuid, "AliceRestart", aliceToken, "google", "sub-al-restart", null, null);
        seedLivePlayer(2, bobUuid, "BobRestart", bobToken, "apple", "sub-bob-restart", null, null);

        try (OctiNetworkClient clientA = OctiNetworkClient.forHermeticTest(fixedPort);
             OctiNetworkClient clientB = OctiNetworkClient.forHermeticTest(fixedPort)) {

            clientA.setSessionToken(aliceToken);
            clientB.setSessionToken(bobToken);

            OctiNetworkClient.CreateTableRequest req = new OctiNetworkClient.CreateTableRequest();
            req.mode = "2P";
            req.bases = 3;
            req.timeControlEnabled = true;
            req.initialMs = 600000L;
            req.incrementMs = 0L;

            OctiNetworkClient.ApiResult<OctiNetworkClient.CreateTableResponse> created = clientA.createTable(req);
            assertTrue("createTable failed: " + created.errorMessage, created.ok);
            String roomId = created.data.roomId;

            OctiNetworkClient.ApiResult<OctiNetworkClient.JoinTableResponse> joined = clientB.joinTable(roomId, 2, "");
            assertTrue("joinTable failed: " + joined.errorMessage, joined.ok);
            assertTrue(joined.data.gameStarted);

            List<OctiNetworkClient.SseEvent> eventsA = new CopyOnWriteArrayList<>();
            List<OctiNetworkClient.SseEvent> eventsB = new CopyOnWriteArrayList<>();

            clientA.startGameEventStream(roomId, eventsA::add);
            clientB.startGameEventStream(roomId, eventsB::add);

            long waitDeadline = System.currentTimeMillis() + 3000L;
            while (System.currentTimeMillis() < waitDeadline &&
                    (!clientA.isGameEventStreamActive() || !clientB.isGameEventStreamActive())) {
                Thread.sleep(20L);
            }
            assertTrue(clientA.isGameEventStreamActive());
            assertTrue(clientB.isGameEventStreamActive());

            // Move 1: Alice moves "73+a"
            OctiNetworkClient.ApiResult<OctiNetworkClient.MoveSubmitData> m1 = clientA.submitMove(roomId, "73+a");
            assertTrue("Move 1 failed: " + m1.errorMessage, m1.ok);

            boolean seenM1 = false;
            long m1Deadline = System.currentTimeMillis() + 3000L;
            while (System.currentTimeMillis() < m1Deadline) {
                for (OctiNetworkClient.SseEvent ev : eventsB) {
                    if (ev.dataJson.contains("73+a")) {
                        seenM1 = true;
                        break;
                    }
                }
                if (seenM1) break;
                Thread.sleep(20L);
            }
            assertTrue("Bob must receive Move 1 over SSE", seenM1);

            // Move 2: Bob moves "77+e"
            OctiNetworkClient.ApiResult<OctiNetworkClient.MoveSubmitData> m2 = clientB.submitMove(roomId, "77+e");
            assertTrue("Move 2 failed: " + m2.errorMessage, m2.ok);

            boolean seenM2 = false;
            long m2Deadline = System.currentTimeMillis() + 3000L;
            while (System.currentTimeMillis() < m2Deadline) {
                for (OctiNetworkClient.SseEvent ev : eventsA) {
                    if (ev.dataJson.contains("77+e")) {
                        seenM2 = true;
                        break;
                    }
                }
                if (seenM2) break;
                Thread.sleep(20L);
            }
            assertTrue("Alice must receive Move 2 over SSE", seenM2);

            OctiNetworkClient.ApiResult<OctiNetworkClient.GameViewData> viewBefore = clientA.fetchGameView(roomId);
            assertTrue(viewBefore.ok);
            assertEquals(1, viewBefore.data.turnSeat);
            assertEquals(2, viewBefore.data.plyCount);

            // Stop server subprocess!
            stopLiveServer();

            // Relaunch server subprocess on the exact same port and SQLite DB!
            startLiveServer(fixedPort);

            // SSE streams stay active and auto-reconnect
            long reconnectDeadline = System.currentTimeMillis() + 3000L;
            while (System.currentTimeMillis() < reconnectDeadline &&
                    (!clientA.isGameEventStreamActive() || !clientB.isGameEventStreamActive())) {
                Thread.sleep(20L);
            }
            assertTrue(clientA.isGameEventStreamActive());
            assertTrue(clientB.isGameEventStreamActive());

            // Move 3: Alice submits next move "73a-74" after server restart
            OctiNetworkClient.ApiResult<OctiNetworkClient.MoveSubmitData> m3 = clientA.submitMove(roomId, "73a-74");
            assertTrue("Move 3 after restart failed: " + m3.errorMessage, m3.ok);

            boolean seenM3 = false;
            long m3Deadline = System.currentTimeMillis() + 3000L;
            while (System.currentTimeMillis() < m3Deadline) {
                for (OctiNetworkClient.SseEvent ev : eventsB) {
                    if (ev.dataJson.contains("73a-74")) {
                        seenM3 = true;
                        break;
                    }
                }
                if (seenM3) break;
                Thread.sleep(20L);
            }
            assertTrue("Bob's auto-reconnected SSE stream must receive Move 3 after server restart", seenM3);

            OctiNetworkClient.ApiResult<OctiNetworkClient.GameViewData> viewAfter = clientA.fetchGameView(roomId);
            assertTrue(viewAfter.ok);
            assertEquals(3, viewAfter.data.plyCount);
            assertEquals(2, viewAfter.data.turnSeat);
            assertTrue(viewAfter.data.seats.get(0).clockMs > 0);

            clientA.stopEventStreams();
            clientB.stopEventStreams();
        }
    }

    // -----------------------------------------------------------------------
    // Live octi_server process management & SQLite seeding helpers
    // -----------------------------------------------------------------------

    private void startLiveServer() throws Exception {
        startLiveServer(0);
    }

    private void startLiveServer(int fixedPort) throws Exception {
        File serverBin = findServerBinary();
        assertNotNull("octi_server binary must exist under ../server/build/octi_server", serverBin);
        assertTrue("octi_server binary must be executable", serverBin.canExecute());

        if (_sqliteDbFile == null) {
            _sqliteDbFile = new File(_tempDir, "live_octi.db");
        }
        File oidcConfigFile = new File(_tempDir, "oidc_config.json");
        if (!oidcConfigFile.exists()) {
            String oidcJson = "{\n"
                    + "  \"providers\": {\n"
                    + "    \"google\": { \"client_ids\": [\"octi-google-client\"] },\n"
                    + "    \"apple\": { \"client_ids\": [\"octi-apple-client\"] },\n"
                    + "    \"microsoft\": { \"client_ids\": [\"octi-ms-client\"] },\n"
                    + "    \"facebook\": { \"client_ids\": [\"octi-fb-client\"], \"app_secret\": \"fb-test-secret\" },\n"
                    + "    \"line\": { \"client_ids\": [\"octi-line-client\"] }\n"
                    + "  }\n"
                    + "}\n";
            Files.write(oidcConfigFile.toPath(), oidcJson.getBytes(StandardCharsets.UTF_8));
            oidcConfigFile.setReadable(false, false);
            oidcConfigFile.setReadable(true, true);
            oidcConfigFile.setWritable(false, false);
            oidcConfigFile.setWritable(true, true);
        }

        ProcessBuilder pb = new ProcessBuilder(
                serverBin.getAbsolutePath(),
                "--bind", "127.0.0.1",
                "--port", String.valueOf(fixedPort),
                "--db", _sqliteDbFile.getAbsolutePath(),
                "--oidc-config", oidcConfigFile.getAbsolutePath());
        pb.redirectErrorStream(true);
        _serverProc = pb.start();

        BufferedReader reader = new BufferedReader(
                new InputStreamReader(_serverProc.getInputStream(), StandardCharsets.UTF_8));
        Pattern portPattern = Pattern.compile("listening on 127\\.0\\.0\\.1:(\\d+)");
        long deadline = System.currentTimeMillis() + 5000L;
        _serverPort = 0;
        while (System.currentTimeMillis() < deadline) {
            String line = reader.readLine();
            if (line == null) {
                break;
            }
            Matcher m = portPattern.matcher(line);
            if (m.find()) {
                _serverPort = Integer.parseInt(m.group(1));
                break;
            }
        }
        assertTrue("Failed to capture live octi_server listening port", _serverPort > 0);

        // Drain stdout in background so pipe buffer never blocks
        Thread drain = new Thread(() -> {
            try {
                while (reader.readLine() != null) {
                    // discard
                }
            } catch (Exception ignored) {
            }
        }, "octi-server-stdout-drain");
        drain.setDaemon(true);
        drain.start();
    }

    private void stopLiveServer() {
        if (_serverProc != null) {
            _serverProc.destroy();
            try {
                if (!_serverProc.waitFor(2, TimeUnit.SECONDS)) {
                    _serverProc.destroyForcibly();
                }
            } catch (InterruptedException ignored) {
                Thread.currentThread().interrupt();
            }
            _serverProc = null;
        }
    }

    private static File findServerBinary() {
        String[] candidates = new String[] {
                "../server/build/octi_server",
                "../../server/build/octi_server",
                "server/build/octi_server"
        };
        for (String rel : candidates) {
            File f = new File(rel);
            if (f.isFile() && f.canExecute()) {
                return f;
            }
        }
        return null;
    }

    private void seedLivePlayer(
            int playerId,
            String playerUuid,
            String handle,
            String rawToken,
            String provider1,
            String sub1,
            String provider2,
            String sub2) throws Exception {
        long now = System.currentTimeMillis() / 1000L;
        long exp = now + 86400L;
        String tokenHash = sha256Hex(rawToken);

        StringBuilder sql = new StringBuilder();
        sql.append("PRAGMA busy_timeout=5000;\n");
        sql.append(String.format(
                "INSERT INTO players(player_id, uuid, handle, display_name, avatar_url, "
                        + "elo_2p, elo_4p, elo_ofk, games_played, wins, losses, draws, created_at, last_seen_at) "
                        + "VALUES(%d, '%s', '%s', '%s', '', 1500, 1500, 1500, 1, 1, 0, 0, %d, %d);\n",
                playerId, playerUuid, handle, handle, now, now));
        sql.append(String.format(
                "INSERT INTO linked_identities(player_id, provider, provider_sub, provider_email, display_hint, linked_at, last_login_at) "
                        + "VALUES(%d, '%s', '%s', '%s@selfso.com', '%s', %d, %d);\n",
                playerId, provider1, sub1, handle.toLowerCase(), handle, now, now));
        if (provider2 != null && sub2 != null) {
            sql.append(String.format(
                    "INSERT INTO linked_identities(player_id, provider, provider_sub, provider_email, display_hint, linked_at, last_login_at) "
                            + "VALUES(%d, '%s', '%s', '%s@selfso.com', '%s', %d, %d);\n",
                    playerId, provider2, sub2, handle.toLowerCase(), handle, now, now));
        }
        sql.append(String.format(
                "INSERT INTO sessions(token_hash, player_id, client_agent, created_at, expires_at, revoked) "
                        + "VALUES('%s', %d, 'octi-android-test', %d, %d, 0);\n",
                tokenHash, playerId, now, exp));

        Process p = new ProcessBuilder("sqlite3", _sqliteDbFile.getAbsolutePath())
                .redirectErrorStream(true)
                .start();
        try (OutputStream os = p.getOutputStream()) {
            os.write(sql.toString().getBytes(StandardCharsets.UTF_8));
        }
        String out;
        try (BufferedReader br = new BufferedReader(
                new InputStreamReader(p.getInputStream(), StandardCharsets.UTF_8))) {
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = br.readLine()) != null) {
                sb.append(line).append('\n');
            }
            out = sb.toString();
        }
        int code = p.waitFor();
        assertEquals("sqlite3 seeding failed: " + out, 0, code);
    }

    private static String sha256Hex(String input) throws Exception {
        MessageDigest md = MessageDigest.getInstance("SHA-256");
        byte[] digest = md.digest(input.getBytes(StandardCharsets.UTF_8));
        StringBuilder sb = new StringBuilder(digest.length * 2);
        for (byte b : digest) {
            sb.append(String.format("%02x", b & 0xFF));
        }
        return sb.toString();
    }

    private static void deleteRecursively(File f) {
        if (f.isDirectory()) {
            File[] children = f.listFiles();
            if (children != null) {
                for (File c : children) {
                    deleteRecursively(c);
                }
            }
        }
        f.delete();
    }

    /**
     * Minimal loopback HTTP harness exercising 5-provider {@code loginWithSso}
     * ({@code 201 Created} & {@code 200 OK}) and {@code linkIdentity}
     * ({@code 200 OK} & {@code 409 Conflict} with {@code existing_profile}).
     */
    private static final class EmbeddedSsoProtocolServer implements AutoCloseable {
        private final ServerSocket _serverSocket;
        private final AtomicBoolean _running = new AtomicBoolean(true);
        private final Thread _acceptThread;

        EmbeddedSsoProtocolServer() throws Exception {
            _serverSocket = new ServerSocket();
            _serverSocket.setReuseAddress(true);
            _serverSocket.bind(new InetSocketAddress(InetAddress.getByName("127.0.0.1"), 0));
            _acceptThread = new Thread(this::acceptLoop, "octi-sso-test-harness");
            _acceptThread.setDaemon(true);
            _acceptThread.start();
        }

        int port() {
            return _serverSocket.getLocalPort();
        }

        private void acceptLoop() {
            while (_running.get()) {
                try (Socket s = _serverSocket.accept()) {
                    s.setSoTimeout(2000);
                    BufferedReader r = new BufferedReader(
                            new InputStreamReader(s.getInputStream(), StandardCharsets.UTF_8));
                    String reqLine = r.readLine();
                    if (reqLine == null) {
                        continue;
                    }
                    int contentLength = 0;
                    String h;
                    while ((h = r.readLine()) != null && !h.isEmpty()) {
                        int colon = h.indexOf(':');
                        if (colon > 0 && h.substring(0, colon).trim().equalsIgnoreCase("Content-Length")) {
                            contentLength = Integer.parseInt(h.substring(colon + 1).trim());
                        }
                    }
                    char[] bodyChars = new char[Math.max(0, contentLength)];
                    int readTotal = 0;
                    while (readTotal < contentLength) {
                        int n = r.read(bodyChars, readTotal, contentLength - readTotal);
                        if (n < 0) {
                            break;
                        }
                        readTotal += n;
                    }
                    String body = new String(bodyChars, 0, readTotal);
                    Map<String, Object> reqJson = OctiMiniJson.parseObject(body);
                    String provider = OctiMiniJson.getString(reqJson, "provider", "google");
                    String token = OctiMiniJson.getString(reqJson, "id_token",
                            OctiMiniJson.getString(reqJson, "access_token", ""));

                    int status = 200;
                    String respBody;

                    if (reqLine.contains("/api/v1/auth/sso")) {
                        boolean isNew = token.startsWith("token-new-");
                        status = isNew ? 201 : 200;
                        respBody = "{\n"
                                + "  \"session_token\": \"sess-" + provider + "\",\n"
                                + "  \"expires_at\": 1999999999,\n"
                                + "  \"created\": " + isNew + ",\n"
                                + "  \"onboarding_required\": " + isNew + ",\n"
                                + "  \"profile\": {\n"
                                + "    \"uuid\": \"p-" + provider + "\",\n"
                                + "    \"handle\": \"User_" + provider + "\",\n"
                                + "    \"elo_2p\": 1500\n"
                                + "  },\n"
                                + "  \"linked_identities\": [\n"
                                + "    {\"provider\": \"" + provider + "\", \"provider_email\": \"u@selfso.com\"}\n"
                                + "  ]\n"
                                + "}";
                    } else if (reqLine.contains("/api/v1/auth/link")) {
                        if (token.contains("conflict")) {
                            status = 409;
                            respBody = "{\n"
                                    + "  \"error\": \"identity_conflict\",\n"
                                    + "  \"message\": \"Identity is already linked to another OCTI profile\",\n"
                                    + "  \"existing_profile\": {\n"
                                    + "    \"uuid\": \"p-existing-ms-owner\",\n"
                                    + "    \"handle\": \"GrandmasterVega\",\n"
                                    + "    \"elo_2p\": 1680\n"
                                    + "  }\n"
                                    + "}";
                        } else {
                            status = 200;
                            respBody = "{\n"
                                    + "  \"status\": \"linked\",\n"
                                    + "  \"linked_identities\": [\n"
                                    + "    {\"provider\": \"google\", \"provider_email\": \"u@selfso.com\"},\n"
                                    + "    {\"provider\": \"" + provider + "\", \"provider_email\": \"u@selfso.com\"}\n"
                                    + "  ]\n"
                                    + "}";
                        }
                    } else {
                        status = 404;
                        respBody = "{\"error\":\"not_found\"}";
                    }

                    byte[] payload = respBody.getBytes(StandardCharsets.UTF_8);
                    String statusText = (status == 201) ? "Created"
                            : (status == 409) ? "Conflict"
                            : (status == 200) ? "OK" : "Error";
                    String headers = "HTTP/1.1 " + status + " " + statusText + "\r\n"
                            + "Content-Type: application/json\r\n"
                            + "Content-Length: " + payload.length + "\r\n"
                            + "Connection: close\r\n\r\n";
                    OutputStream os = s.getOutputStream();
                    os.write(headers.getBytes(StandardCharsets.UTF_8));
                    os.write(payload);
                    os.flush();
                } catch (Exception ignored) {
                }
            }
        }

        @Override
        public void close() {
            _running.set(false);
            try {
                _serverSocket.close();
            } catch (Exception ignored) {
            }
            try {
                _acceptThread.join(300L);
            } catch (InterruptedException ignored) {
                Thread.currentThread().interrupt();
            }
        }
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
