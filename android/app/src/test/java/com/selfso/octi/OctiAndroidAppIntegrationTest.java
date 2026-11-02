/*
 * OctiAndroidAppIntegrationTest.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import android.graphics.Bitmap;
import android.view.KeyEvent;

import com.selfso.octi.ui.OctiAppViewModel;
import com.selfso.octi.ui.OctiDialogs;
import com.selfso.selfupdate.UpdateManifest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.lang.reflect.Constructor;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

/**
 * Envelope 3 (Gate 3) end-to-end qualification suite for {@link OctiAppViewModel},
 * local Pass-and-Play &amp; AI bot play, 4-quadrant board rotation, VCR move
 * scrubbing, live {@code octi_server} online room &amp; PTT voice lifecycle,
 * {@code :selfupdate} OTA manifest &amp; SHA-256 APK verification, and high-DPI
 * PNG artifact generation.
 */
public class OctiAndroidAppIntegrationTest {

    private File _tempDir;
    private Process _serverProc;
    private int _serverPort;
    private File _sqliteDbFile;

    @Before
    public void setUp() throws Exception {
        _tempDir = Files.createTempDirectory("octi-android-app-test-").toFile();
    }

    @After
    public void tearDown() {
        stopLiveServer();
        if (_tempDir != null && _tempDir.exists()) {
            deleteRecursively(_tempDir);
        }
    }

    @Test
    public void testHardCodedEndpointAndZeroOverrideInViewModelAndConfig() throws Exception {
        assertEquals("octi.selfso.com:443", OctiAppViewModel.canonicalServerEndpoint());
        assertEquals("octi.selfso.com", OctiAppViewModel.canonicalServerHost());
        assertEquals(443, OctiAppViewModel.canonicalServerPort());
        assertFalse(OctiAppViewModel.hasEditableServerOrPortField());

        File cfgFile = new File(_tempDir, "client_config.json");
        String rogueJson = "{\n"
                + "  \"session_token\": \"tok-local-123\",\n"
                + "  \"player_uuid\": \"u-local-123\",\n"
                + "  \"handle\": \"BrassKnight\",\n"
                + "  \"theme\": \"ROSEWOOD_IVORY\",\n"
                + "  \"server_url\": \"http://evil.example.com:9999\",\n"
                + "  \"server_host\": \"evil.example.com\",\n"
                + "  \"server_port\": 9999,\n"
                + "  \"host\": \"10.0.0.99\",\n"
                + "  \"port\": 12345\n"
                + "}\n";
        Files.write(cfgFile.toPath(), rogueJson.getBytes(StandardCharsets.UTF_8));

        try (OctiAppViewModel vm = new OctiAppViewModel(new ClientConfigStore(cfgFile))) {
            assertEquals("octi.selfso.com", vm.getNetClient().serverHost());
            assertEquals(443, vm.getNetClient().serverPort());
            assertEquals("octi.selfso.com:443", OctiAppViewModel.canonicalServerEndpoint());
            assertEquals(OctiThemeId.ROSEWOOD_IVORY, vm.getTheme().getValue());

            Map<String, String> rogueExtras = new HashMap<>();
            rogueExtras.put("server_url", "http://127.0.0.1:1111");
            rogueExtras.put("server_host", "127.0.0.1");
            rogueExtras.put("server_port", "1111");
            rogueExtras.put("tab", "LOBBY");
            vm.applyExternalArguments(rogueExtras);

            assertEquals("octi.selfso.com", vm.getNetClient().serverHost());
            assertEquals(443, vm.getNetClient().serverPort());
            assertEquals(OctiAppViewModel.AppTab.ONLINE_LOBBY, vm.getCurrentTab().getValue());

            vm.setTheme(OctiThemeId.OBSIDIAN_BRASS);
            String savedJson = new String(
                    Files.readAllBytes(cfgFile.toPath()), StandardCharsets.UTF_8);
            assertFalse(savedJson.contains("server_url"));
            assertFalse(savedJson.contains("server_host"));
            assertFalse(savedJson.contains("server_port"));
            assertFalse(savedJson.contains("evil.example.com"));
        }

        File userDir = new File(System.getProperty("user.dir", "."));
        File lobbyFile = new File(userDir, "src/main/java/com/selfso/octi/ui/LobbyScreen.kt");
        if (!lobbyFile.exists()) {
            lobbyFile = new File(userDir, "app/src/main/java/com/selfso/octi/ui/LobbyScreen.kt");
        }
        File dialogsFile = new File(userDir, "src/main/java/com/selfso/octi/ui/OctiDialogs.kt");
        if (!dialogsFile.exists()) {
            dialogsFile = new File(userDir, "app/src/main/java/com/selfso/octi/ui/OctiDialogs.kt");
        }
        assertTrue("LobbyScreen.kt exists at " + lobbyFile.getAbsolutePath(), lobbyFile.exists());
        assertTrue("OctiDialogs.kt exists at " + dialogsFile.getAbsolutePath(), dialogsFile.exists());

        String lobbyCode = new String(Files.readAllBytes(lobbyFile.toPath()), StandardCharsets.UTF_8);
        String dialogsCode = new String(Files.readAllBytes(dialogsFile.toPath()), StandardCharsets.UTF_8);
        String[] forbidden = new String[] {
            "Endpoint:",
            "Server:",
            "canonicalServerEndpoint()",
            "trumpet.selfso.com",
            "6874"
        };
        for (String target : forbidden) {
            assertFalse("LobbyScreen.kt must not contain " + target, lobbyCode.contains(target));
            assertFalse("OctiDialogs.kt must not contain " + target, dialogsCode.contains(target));
        }
    }

    @Test
    public void testLocalPassAndPlayBotMatchVcrScrubbingAndBoardRotation() {
        File cfgFile = new File(_tempDir, "local_bot_config.json");
        try (OctiAppViewModel vm = new OctiAppViewModel(new ClientConfigStore(cfgFile))) {
            // 1. Verify 4-quadrant seat rotations (0, 90, 180, 270) and manual 90-degree toggle
            assertEquals(
                    OctiBoardGeometry.BoardRotation.ROT_0,
                    OctiBoardGeometry.rotationForSeat(OctiGameMode.GAME_MODE_4, OctiPlayer.PLAYER_A));
            assertEquals(
                    OctiBoardGeometry.BoardRotation.ROT_90,
                    OctiBoardGeometry.rotationForSeat(OctiGameMode.GAME_MODE_4, OctiPlayer.PLAYER_B));
            assertEquals(
                    OctiBoardGeometry.BoardRotation.ROT_180,
                    OctiBoardGeometry.rotationForSeat(OctiGameMode.GAME_MODE_4, OctiPlayer.PLAYER_C));
            assertEquals(
                    OctiBoardGeometry.BoardRotation.ROT_270,
                    OctiBoardGeometry.rotationForSeat(OctiGameMode.GAME_MODE_4, OctiPlayer.PLAYER_D));

            assertEquals(OctiBoardGeometry.BoardRotation.ROT_0, vm.getBoardRotation().getValue());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_90, vm.rotateBoard90Clockwise());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_180, vm.rotateBoard90Clockwise());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_270, vm.rotateBoard90Clockwise());
            assertEquals(OctiBoardGeometry.BoardRotation.ROT_0, vm.rotateBoard90Clockwise());

            // 2. Start a local vs. Bot match and play 6 plies
            vm.startNewLocalGame(
                    OctiGameMode.GAME_MODE_2,
                    false,
                    false,
                    OctiGameBases.GAME_BASES_3,
                    true,
                    OctiBotEngine.BotDifficulty.CASUAL,
                    OctiPlayer.PLAYER_A);
            vm.getBotEngine().setTimeBudgetMs(25);
            assertEquals(OctiAppViewModel.MatchType.LOCAL_VS_BOT, vm.getMatchType().getValue());
            assertEquals(0, vm.livePlyCount());

            for (int i = 0; i < 6; ++i) {
                String mv = vm.playBotTurnSynchronously();
                assertFalse("Expected non-empty bot move at ply " + i, mv.isEmpty());
            }
            assertEquals(6, vm.livePlyCount());
            assertEquals(6, vm.getViewingPly().getValue().intValue());
            assertEquals(6, vm.moveHistory().size());
            assertEquals(7, vm.fenHistory().size());

            // 3. Scrub VCR move history: |<<, >, <, >>|
            String initialFen = vm.fenHistory().get(0);
            String ply3Fen = vm.fenHistory().get(3);
            String liveFen = vm.fenHistory().get(6);

            assertEquals(0, vm.vcrFirst());
            assertTrue(vm.isViewingHistory());
            assertEquals(initialFen, vm.getDisplayBoard().toPositionString());

            assertEquals(1, vm.vcrNext());
            assertEquals(2, vm.vcrNext());
            assertEquals(3, vm.vcrNext());
            assertEquals(ply3Fen, vm.getDisplayBoard().toPositionString());

            assertEquals(2, vm.vcrPrev());
            assertEquals(vm.fenHistory().get(2), vm.getDisplayBoard().toPositionString());

            assertEquals(6, vm.vcrLast());
            assertFalse(vm.isViewingHistory());
            assertEquals(liveFen, vm.getDisplayBoard().toPositionString());

            // 4. Export JSON v3.0 transcript and reload into VCR replay mode
            String transcript = vm.exportCurrentTranscript();
            assertNotNull(transcript);
            assertTrue(transcript.contains("\"format\": \"octi.transcript.v3\"") || transcript.contains("\"format\":\"octi.transcript.v3\""));

            vm.startNewLocalGame(OctiGameMode.GAME_MODE_OFK);
            assertEquals(0, vm.livePlyCount());

            assertTrue(vm.loadTranscriptForReplay(transcript));
            assertEquals(OctiAppViewModel.MatchType.ARCHIVE_REPLAY, vm.getMatchType().getValue());
            assertEquals(6, vm.livePlyCount());
            assertEquals(0, vm.vcrFirst());
            assertEquals(initialFen, vm.getDisplayBoard().toPositionString());
            assertEquals(6, vm.vcrLast());
            assertEquals(liveFen, vm.getDisplayBoard().toPositionString());
        }
    }

    @Test
    public void testCanonicalSsoProvidersOnlyGoogleAndFacebook() throws Exception {
        assertEquals(1, OctiDialogs.CANONICAL_SSO_PROVIDERS.size());
        assertEquals("google", OctiDialogs.CANONICAL_SSO_PROVIDERS.get(0).getFirst());
        assertEquals("Google", OctiDialogs.CANONICAL_SSO_PROVIDERS.get(0).getSecond());
        assertEquals(2, OctiDialogs.ALL_SSO_PROVIDERS.size());
        assertEquals("google", OctiDialogs.ALL_SSO_PROVIDERS.get(0).getFirst());
        assertEquals("facebook", OctiDialogs.ALL_SSO_PROVIDERS.get(1).getFirst());

        try (MockSsoHarnessServer ssoServer = new MockSsoHarnessServer();
             OctiNetworkClient netClient = OctiNetworkClient.forHermeticTest(ssoServer.port());
             OctiAppViewModel vm = new OctiAppViewModel(
                     new ClientConfigStore(new File(_tempDir, "browser_sso_vm.json")),
                     netClient)) {
            final String[] openedUrl = new String[]{""};
            String url = vm.startBrowserSignIn("google", u -> {
                openedUrl[0] = u;
                return kotlin.Unit.INSTANCE;
            });
            assertTrue(url.contains("/api/v1/auth/authorize?provider=google"));
            assertEquals(url, openedUrl[0]);
            assertTrue(vm.isBrowserAuthInProgress().getValue());
            assertTrue(vm.loopbackAuthPort() > 0);
            vm.cancelBrowserAuth();
            assertFalse(vm.isBrowserAuthInProgress().getValue());
        }
    }

    @Test
    public void testViewModelSsoLoginMultiProviderLinkingAnd409ConflictAccountSwitch() throws Exception {
        try (MockSsoHarnessServer ssoServer = new MockSsoHarnessServer();
             OctiNetworkClient netClient = OctiNetworkClient.forHermeticTest(ssoServer.port());
             OctiAppViewModel vm = new OctiAppViewModel(
                     new ClientConfigStore(new File(_tempDir, "sso_vm.json")),
                     netClient)) {

            // 1. First SSO login (201 Created -> onboardingRequired)
            OctiNetworkClient.ApiResult<OctiNetworkClient.SsoLoginData> login1 =
                    vm.loginWithSso("google", "token-new-alice");
            assertTrue(login1.ok);
            assertTrue(vm.isAuthenticated().getValue());
            assertTrue(vm.getOnboardingRequired().getValue());
            assertEquals("p-google", vm.getCurrentProfile().getValue().uuid);

            // 2. Link second SSO identity (200 OK)
            OctiNetworkClient.ApiResult<OctiNetworkClient.LinkIdentityData> linkOk =
                    vm.linkIdentity("github", "token-link-github-ok");
            assertTrue(linkOk.ok);
            assertEquals(2, vm.getLinkedIdentities().getValue().size());
            assertNull(vm.getLinkConflictProfile().getValue());

            // 3. Link an identity already bound to another OCTI account -> 409 Conflict prompt
            OctiNetworkClient.ApiResult<OctiNetworkClient.LinkIdentityData> linkConflict =
                    vm.linkIdentity("microsoft", "token-conflict-vega");
            assertFalse(linkConflict.ok);
            assertEquals(409, linkConflict.httpStatus);
            assertNotNull(vm.getLinkConflictProfile().getValue());
            assertNotNull(vm.getLinkConflict().getValue());
            assertEquals("GrandmasterVega", vm.getLinkConflictProfile().getValue().handle);

            // 4. Merge conflicting identity into current account (force_relink=true) on background executor
            java.util.concurrent.Future<?> mergeFuture = vm.mergeConflictingIdentityIntoCurrentAccount();
            assertNotNull(mergeFuture);
            mergeFuture.get(5, TimeUnit.SECONDS);
            assertNull(vm.getLinkConflictProfile().getValue());
            assertNull(vm.getLinkConflict().getValue());
            assertTrue(vm.getStatusBanner().getValue().contains("Merged microsoft"));

            // 5. Trigger conflict again and confirm switching to the conflict account
            OctiNetworkClient.ApiResult<OctiNetworkClient.LinkIdentityData> linkConflict2 =
                    vm.linkIdentity("microsoft", "token-conflict-vega");
            assertFalse(linkConflict2.ok);
            assertEquals(409, linkConflict2.httpStatus);
            assertNotNull(vm.getLinkConflictProfile().getValue());

            OctiNetworkClient.ApiResult<OctiNetworkClient.SsoLoginData> switched =
                    vm.confirmSwitchToConflictAccount();
            assertTrue(switched.ok);
            assertNull(vm.getLinkConflictProfile().getValue());
            assertEquals("p-microsoft", vm.getCurrentProfile().getValue().uuid);
        }
    }

    @Test
    public void testForceRelinkAndGuestObservationVersusChatGating() throws Exception {
        try (MockSsoHarnessServer ssoServer = new MockSsoHarnessServer();
             OctiNetworkClient netClient = OctiNetworkClient.forHermeticTest(ssoServer.port());
             OctiAppViewModel vm = new OctiAppViewModel(
                     new ClientConfigStore(new File(_tempDir, "guest_vm.json")),
                     netClient)) {

            // 1. Guest observation vs. chat gating when unauthenticated
            assertTrue(netClient.canGuestObserve());
            assertFalse(netClient.canGuestChat());
            assertFalse(netClient.isAuthenticated());
            assertFalse(vm.isAuthenticated().getValue());

            // Chatting while unauthenticated must fail immediately with 401 UNAUTHENTICATED
            OctiNetworkClient.ApiResult<Long> lobbyChatRes = netClient.postLobbyChat("Hello from guest");
            assertFalse(lobbyChatRes.ok);
            assertEquals(401, lobbyChatRes.httpStatus);
            assertEquals("UNAUTHENTICATED", lobbyChatRes.errorCode);

            OctiNetworkClient.ApiResult<Long> vmLobbyChatRes = vm.sendLobbyChat("Hello from vm guest");
            assertFalse(vmLobbyChatRes.ok);
            assertEquals(401, vmLobbyChatRes.httpStatus);
            assertEquals("UNAUTHENTICATED", vmLobbyChatRes.errorCode);

            OctiNetworkClient.ApiResult<Long> vmGameChatRes = vm.sendGameChat("all", "Hello game from guest");
            assertFalse(vmGameChatRes.ok);
            assertEquals(401, vmGameChatRes.httpStatus);
            assertEquals("UNAUTHENTICATED", vmGameChatRes.errorCode);

            // Guest can refresh lobby and observe tables
            OctiNetworkClient.ApiResult<OctiNetworkClient.LobbyStateData> lobbyRes = vm.refreshLobby();
            assertTrue(lobbyRes.ok);

            OctiNetworkClient.ApiResult<Boolean> observeRes = vm.observeTable("room-test-123", "");
            assertTrue(observeRes.ok);
            assertTrue(vm.getStatusBanner().getValue().contains("Observing as anonymous"));

            // 2. Direct AuthResult linkIdentity with forceRelink=true
            netClient.setSessionToken("sess-test-token");
            assertTrue(netClient.isAuthenticated());
            assertTrue(netClient.canGuestChat());

            // Linking conflict without forceRelink fails with 409
            OctiNetworkClient.AuthResult conflictRes =
                    netClient.linkIdentity("facebook", "token-conflict-user", "", false);
            assertFalse(conflictRes.ok);
            assertEquals(409, conflictRes.httpStatus);
            assertTrue(conflictRes.data.conflictAlreadyLinked);

            // Linking conflict with forceRelink=true succeeds with 200 OK
            OctiNetworkClient.AuthResult forceRes =
                    netClient.linkIdentity("facebook", "token-conflict-user", "", true);
            assertTrue(forceRes.ok);
            assertEquals(200, forceRes.httpStatus);
            assertEquals("linked", forceRes.data.status);
            assertFalse(forceRes.data.identities.isEmpty());

            // Programmatic linkIdentity works with any arbitrary provider string
            OctiNetworkClient.AuthResult customProvRes =
                    netClient.linkIdentity("custom_provider", "token-custom-ok", "", false);
            assertTrue(customProvRes.ok);
            assertEquals(200, customProvRes.httpStatus);
        }
    }

    @Test
    public void testFullOnlineLifecycleViaViewModelAgainstLiveOctiServer() throws Exception {
        startLiveServer();

        String aliceUuid = "aaaaaaaa-1111-4111-8111-111111111111";
        String bobUuid = "bbbbbbbb-2222-4222-8222-222222222222";
        String aliceToken = "tok-vm-alice-0001";
        String bobToken = "tok-vm-bob-0002";

        seedLivePlayer(1, aliceUuid, "AliceVM", aliceToken, "google", "sub-alice-g", "apple", "sub-alice-a");
        seedLivePlayer(2, bobUuid, "BobVM", bobToken, "microsoft", "sub-bob-m", null, null);

        try (OctiNetworkClient netA = OctiNetworkClient.forHermeticTest(_serverPort);
             OctiNetworkClient netB = OctiNetworkClient.forHermeticTest(_serverPort);
             OctiAppViewModel vmA = new OctiAppViewModel(
                     new ClientConfigStore(new File(_tempDir, "alice_cfg.json")), netA);
             OctiAppViewModel vmB = new OctiAppViewModel(
                     new ClientConfigStore(new File(_tempDir, "bob_cfg.json")), netB)) {

            assertTrue(vmA.attachAuthenticatedSession(aliceToken, aliceUuid, "AliceVM").ok);
            assertTrue(vmB.attachAuthenticatedSession(bobToken, bobUuid, "BobVM").ok);
            assertEquals("AliceVM", vmA.getCurrentProfile().getValue().handle);
            assertEquals("BobVM", vmB.getCurrentProfile().getValue().handle);

            // 1. Lobby Chat & State
            assertTrue(vmA.sendLobbyChat("Ready for a 2P match!").ok);
            assertTrue(vmB.refreshLobbyState().ok);
            assertFalse(vmB.getLobbyState().getValue().recentChat.isEmpty());

            // 2. Alice creates a 2P table; Bob joins Seat 2
            OctiNetworkClient.CreateTableRequest req = new OctiNetworkClient.CreateTableRequest();
            req.mode = "2P";
            req.bases = 3;
            req.superProngs = false;
            req.edgeless = false;
            req.rated = true;
            req.allowTakeback = true;
            req.voicePolicy = "PLAYERS_ONLY";

            OctiNetworkClient.ApiResult<OctiNetworkClient.CreateTableResponse> created =
                    vmA.createOnlineTable(req);
            assertTrue("createOnlineTable failed: " + created.errorMessage, created.ok);
            String roomId = created.data.roomId;
            assertEquals(roomId, vmA.getActiveRoomId().getValue());

            OctiNetworkClient.ApiResult<OctiNetworkClient.JoinTableResponse> joined =
                    vmB.joinOnlineTable(roomId, 2, "");
            assertTrue("joinOnlineTable failed: " + joined.errorMessage, joined.ok);
            assertTrue(joined.data.gameStarted);
            assertEquals(roomId, vmB.getActiveRoomId().getValue());
            assertEquals(
                    OctiBoardGeometry.BoardRotation.ROT_180,
                    vmB.getBoardRotation().getValue());

            // 3. Exchange live moves via ViewModel
            assertTrue(vmA.applyMoveNotation("53+a"));
            assertTrue(vmB.refreshActiveGameView().ok);
            assertEquals(1, vmB.livePlyCount());

            assertTrue(vmB.applyMoveNotation("57+e"));
            assertTrue(vmA.refreshActiveGameView().ok);
            assertEquals(2, vmA.livePlyCount());

            // 4. Live game chat (ALL scope in 2P; verify TEAM scope rejected in 2P)
            assertTrue(vmA.sendGameChat("all", "Good luck Bob!").ok);
            assertTrue(vmB.sendGameChat("all", "Thanks Alice!").ok);
            assertFalse("2P games reject team-scoped chat",
                    vmB.sendGameChat("team", "Team note").ok);

            long chatDeadline = System.currentTimeMillis() + 3000L;
            while (System.currentTimeMillis() < chatDeadline &&
                    vmB.getGameChatLog().getValue().isEmpty()) {
                Thread.sleep(25L);
            }
            assertFalse("Bob must receive Alice's game chat via SSE",
                    vmB.getGameChatLog().getValue().isEmpty());

            // 5. Push-to-Talk Voice Room & Hardware Volume Key PTT
            assertTrue(vmA.joinVoiceChannel());
            assertTrue(vmB.joinVoiceChannel());
            assertTrue(vmA.isInVoiceRoom().getValue());
            assertTrue(vmB.isInVoiceRoom().getValue());

            assertTrue(vmA.getVoiceEngine().initiatePeerOffer(bobUuid));
            long voiceDeadline = System.currentTimeMillis() + 3000L;
            while (System.currentTimeMillis() < voiceDeadline &&
                    (!vmA.getVoiceEngine().isPeerConnected(bobUuid) ||
                            !vmB.getVoiceEngine().isPeerConnected(aliceUuid))) {
                Thread.sleep(25L);
            }
            assertTrue(vmA.getVoiceEngine().isPeerConnected(bobUuid));
            assertTrue(vmB.getVoiceEngine().isPeerConnected(aliceUuid));

            short[] toneFrame = new short[P2pVoiceEngine.PAYLOAD_SAMPLES];
            for (int i = 0; i < toneFrame.length; ++i) {
                double angle = 2.0 * Math.PI * 440.0 * ((double) i / P2pVoiceEngine.SAMPLE_RATE_HZ);
                toneFrame[i] = (short) (Math.sin(angle) * 14000.0);
            }

            // Hardware Volume Down key engages PTT
            assertTrue(vmA.onHardwareKeyEvent(KeyEvent.KEYCODE_VOLUME_DOWN, true));
            assertTrue(vmA.isPttActive().getValue());
            for (int f = 0; f < 4; ++f) {
                assertTrue(vmA.getVoiceEngine().injectTestPcmFrame(toneFrame) > 0);
                Thread.sleep(15L);
            }
            assertTrue(vmA.onHardwareKeyEvent(KeyEvent.KEYCODE_VOLUME_DOWN, false));
            assertFalse(vmA.isPttActive().getValue());

            long rxDeadline = System.currentTimeMillis() + 2000L;
            while (System.currentTimeMillis() < rxDeadline &&
                    vmB.getVoiceEngine().packetsReceivedFrom(aliceUuid) < 2L) {
                Thread.sleep(20L);
            }
            assertTrue(vmB.getVoiceEngine().packetsReceivedFrom(aliceUuid) >= 2L);
            assertTrue(vmB.getVoiceEngine().receivedAudioRms(aliceUuid) > 0.05);

            // 6. Complete game by resignation and load archived match into VCR replay
            assertTrue(vmB.resignOnlineGame().ok);
            assertTrue(vmA.refreshArchivedGames("2P", "", 20, 0).ok);
            assertFalse(vmA.getArchivedGamesPage().getValue().games.isEmpty());
            String archivedGameId = vmA.getArchivedGamesPage().getValue().games.get(0).gameId;

            assertTrue(vmA.openArchivedGameForReplay(archivedGameId));
            assertEquals(OctiAppViewModel.MatchType.ARCHIVE_REPLAY, vmA.getMatchType().getValue());
            assertEquals(2, vmA.livePlyCount());
            assertEquals(0, vmA.vcrFirst());
            assertEquals(1, vmA.vcrNext());
            assertEquals(2, vmA.vcrLast());
        }
    }

    @Test
    public void testOtaSelfUpdateManifestAndApkDownloaderSha256Verification() throws Exception {
        byte[] fakeApkBytes = new byte[32768];
        new Random(0x0C712026L).nextBytes(fakeApkBytes);
        String validSha256 = sha256HexBytes(fakeApkBytes);

        try (LoopbackOtaServer otaServer = new LoopbackOtaServer(fakeApkBytes)) {
            String baseUrl = "http://127.0.0.1:" + otaServer.port() + "/software/android/";
            String manifestUrl = baseUrl + "version.json";
            String manifestJson = "{\n"
                    + "  \"apps\": {\n"
                    + "    \"com.selfso.octi\": {\n"
                    + "      \"applicationId\": \"com.selfso.octi\",\n"
                    + "      \"versionCode\": 2,\n"
                    + "      \"versionName\": \"1.1.0\",\n"
                    + "      \"apkUrl\": \"octi/octi-1.1.0.apk\",\n"
                    + "      \"sha256\": \"" + validSha256 + "\",\n"
                    + "      \"sizeBytes\": " + fakeApkBytes.length + ",\n"
                    + "      \"releaseNotes\": \"OCTI Android OTA verification test\"\n"
                    + "    }\n"
                    + "  }\n"
                    + "}\n";

            UpdateManifest manifest = UpdateManifest.Companion.fromJson(
                    manifestJson, manifestUrl, "com.selfso.octi");
            assertEquals("com.selfso.octi", manifest.getApplicationId());
            assertEquals(2, manifest.getVersionCode());
            assertEquals("1.1.0", manifest.getVersionName());
            assertEquals(baseUrl + "octi/octi-1.1.0.apk", manifest.getApkUrl());
            assertEquals(validSha256, manifest.getSha256());

            File downloadedApk = OctiAppViewModel.downloadAndVerifyUpdateBlocking(_tempDir, manifest);
            assertNotNull(downloadedApk);
            assertTrue(downloadedApk.isFile());
            assertEquals(fakeApkBytes.length, downloadedApk.length());
            assertEquals(validSha256, sha256HexBytes(Files.readAllBytes(downloadedApk.toPath())));

            // Verify corrupted SHA-256 is strictly rejected
            String badSha = "0000000000000000000000000000000000000000000000000000000000000000";
            String badManifestJson = manifestJson.replace(validSha256, badSha);
            UpdateManifest badManifest = UpdateManifest.Companion.fromJson(
                    badManifestJson, manifestUrl, "com.selfso.octi");
            File badCacheDir = new File(_tempDir, "bad_ota_cache");
            badCacheDir.mkdirs();
            try {
                OctiAppViewModel.downloadAndVerifyUpdateBlocking(badCacheDir, badManifest);
                fail("Expected IOException on SHA-256 mismatch");
            } catch (Exception expected) {
                assertTrue(expected.getMessage() != null &&
                        expected.getMessage().contains("SHA-256 mismatch"));
            }
        }
    }

    @Test
    public void testExportRequiredVisualPngArtifacts() throws Exception {
        File artifactsDir = resolvePlanArtifactsDir();
        if (!artifactsDir.exists()) {
            assertTrue(artifactsDir.mkdirs());
        }

        // 1. octi_app_icon_512.png
        File iconFile = new File(artifactsDir, "octi_app_icon_512.png");
        OctiAppIconRenderer.writePng(512, iconFile);
        assertValidArtifactPng(iconFile);

        // 2. android_board_2p_obsidian.png (2-Player board with multi-prong pods & legal guides)
        OctiBoard board2p = new OctiBoard(
                OctiGameMode.GAME_MODE_2, true, false, OctiGameBases.GAME_BASES_3);
        board2p.getGame().start(OctiPlayer.PLAYER_A);
        board2p.getGame().move("33+a");
        board2p.getGame().move("37+e");
        board2p.getGame().move("53+h");
        board2p.getGame().move("57+d");
        board2p.getGame().move("73+b!");
        board2p.getGame().move("77+f");
        board2p.getGame().move("33a-34");
        board2p.getGame().move("37e-36");
        board2p.getGame().move("34a+c");
        board2p.getGame().move("36e-35");

        OctiMoveComposer comp2p = new OctiMoveComposer(board2p);
        comp2p.beginMoveOrJumpPod(3, 4, 0, false);
        OctiBoardGeometry geo2p = new OctiBoardGeometry(
                960, 960, 9, 9, OctiBoardGeometry.BoardRotation.ROT_0);

        File board2pObsidian = new File(artifactsDir, "android_board_2p_obsidian.png");
        int[] px2pObsidian = OctiBoardRenderer.renderBoardPixels(
                board2p, comp2p, geo2p, OctiThemeId.OBSIDIAN_BRASS);
        OctiAppIconRenderer.writeArgbPng(960, 960, px2pObsidian, board2pObsidian);
        assertValidArtifactPng(board2pObsidian);

        // 3. android_board_2p_rosewood.png (Grandmaster Rosewood & Ivory theme)
        File board2pRosewood = new File(artifactsDir, "android_board_2p_rosewood.png");
        int[] px2pRosewood = OctiBoardRenderer.renderBoardPixels(
                board2p, comp2p, geo2p, OctiThemeId.ROSEWOOD_IVORY);
        OctiAppIconRenderer.writeArgbPng(960, 960, px2pRosewood, board2pRosewood);
        assertValidArtifactPng(board2pRosewood);

        // 4. android_board_4p_obsidian.png (4-Player 4-color board in Obsidian & Brass)
        OctiBoard board4p = new OctiBoard(
                OctiGameMode.GAME_MODE_4, true, true, OctiGameBases.GAME_BASES_1);
        board4p.getGame().start(OctiPlayer.PLAYER_A);
        board4p.getGame().move("33+a");
        board4p.getGame().move("37+e");
        board4p.getGame().move("77+g");
        board4p.getGame().move("73+c");
        OctiMoveComposer comp4p = new OctiMoveComposer(board4p);
        comp4p.beginMoveOrJumpPod(3, 3, 0, false);
        OctiBoardGeometry geo4p = new OctiBoardGeometry(
                960, 960, 9, 9, OctiBoardGeometry.BoardRotation.ROT_0);

        File board4pObsidian = new File(artifactsDir, "android_board_4p_obsidian.png");
        int[] px4pObsidian = OctiBoardRenderer.renderBoardPixels(
                board4p, comp4p, geo4p, OctiThemeId.OBSIDIAN_BRASS);
        OctiAppIconRenderer.writeArgbPng(960, 960, px4pObsidian, board4pObsidian);
        assertValidArtifactPng(board4pObsidian);

        // 5. android_board_ofk_obsidian.png (6x7 OCTI for Kids board in Obsidian & Brass)
        OctiBoard boardOfk = new OctiBoard(
                OctiGameMode.GAME_MODE_OFK, false, false, OctiGameBases.GAME_BASES_1);
        boardOfk.getGame().start(OctiPlayer.PLAYER_A);
        boardOfk.getGame().move("22+a");
        boardOfk.getGame().move("26+e");
        boardOfk.getGame().move("32+b");
        boardOfk.getGame().move("36+f");
        OctiMoveComposer compOfk = new OctiMoveComposer(boardOfk);
        compOfk.beginMoveOrJumpPod(2, 2, 0, false);
        OctiBoardGeometry geoOfk = new OctiBoardGeometry(
                960, 960, 6, 7, OctiBoardGeometry.BoardRotation.ROT_0);

        File boardOfkObsidian = new File(artifactsDir, "android_board_ofk_obsidian.png");
        int[] pxOfkObsidian = OctiBoardRenderer.renderBoardPixels(
                boardOfk, compOfk, geoOfk, OctiThemeId.OBSIDIAN_BRASS);
        OctiAppIconRenderer.writeArgbPng(960, 960, pxOfkObsidian, boardOfkObsidian);
        assertValidArtifactPng(boardOfkObsidian);
    }

    private static void assertValidArtifactPng(File file) throws IOException {
        assertTrue("Artifact must exist: " + file.getAbsolutePath(), file.isFile());
        assertTrue(
                "Artifact must be > 10 KB (" + file.length() + " bytes): " + file.getName(),
                file.length() > 10240L);
        byte[] head = Files.readAllBytes(file.toPath());
        assertEquals((byte) 0x89, head[0]);
        assertEquals((byte) 'P', head[1]);
        assertEquals((byte) 'N', head[2]);
        assertEquals((byte) 'G', head[3]);
    }

    private static File resolvePlanArtifactsDir() {
        File cwd = new File(System.getProperty("user.dir", ".")).getAbsoluteFile();
        File cur = cwd;
        while (cur != null) {
            if (new File(cur, "plan/plan_octi_android.md").exists()
                    || new File(cur, "plan/done/plan_octi_android.md").exists()
                    || (new File(cur, "android").isDirectory() && new File(cur, "core").isDirectory())) {
                if (new File(cur, "plan/done/plan_octi_android.md").exists()) {
                    return new File(cur, "plan/done/plan_octi_android.artifacts");
                }
                return new File(cur, "plan/plan_octi_android.artifacts");
            }
            cur = cur.getParentFile();
        }
        return new File(cwd, "plan/plan_octi_android.artifacts");
    }

    // -----------------------------------------------------------------------
    // Live octi_server & Mock HTTP Helpers
    // -----------------------------------------------------------------------

    private void startLiveServer() throws Exception {
        File serverBin = findServerBinary();
        assertNotNull("octi_server binary must exist under ../server/build/octi_server", serverBin);
        assertTrue("octi_server binary must be executable", serverBin.canExecute());

        _sqliteDbFile = new File(_tempDir, "live_octi_app.db");
        File oidcConfigFile = new File(_tempDir, "oidc_config.json");
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

        ProcessBuilder pb = new ProcessBuilder(
                serverBin.getAbsolutePath(),
                "--bind", "127.0.0.1",
                "--port", "0",
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

        Thread drain = new Thread(() -> {
            try {
                while (reader.readLine() != null) {
                    // discard stdout
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
        String tokenHash = sha256HexBytes(rawToken.getBytes(StandardCharsets.UTF_8));

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

    private static String sha256HexBytes(byte[] data) throws Exception {
        MessageDigest md = MessageDigest.getInstance("SHA-256");
        byte[] digest = md.digest(data);
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

    private static final class LoopbackOtaServer implements AutoCloseable {
        private final ServerSocket _serverSocket;
        private final byte[] _apkPayload;
        private final AtomicBoolean _running = new AtomicBoolean(true);
        private final Thread _acceptThread;

        LoopbackOtaServer(byte[] apkPayload) throws Exception {
            _apkPayload = apkPayload;
            _serverSocket = new ServerSocket();
            _serverSocket.setReuseAddress(true);
            _serverSocket.bind(new InetSocketAddress(InetAddress.getByName("127.0.0.1"), 0));
            _acceptThread = new Thread(this::acceptLoop, "octi-ota-test-server");
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
                    String h;
                    while ((h = r.readLine()) != null && !h.isEmpty()) {
                        // consume headers
                    }
                    OutputStream os = s.getOutputStream();
                    String headers = "HTTP/1.1 200 OK\r\n"
                            + "Content-Type: application/vnd.android.package-archive\r\n"
                            + "Content-Length: " + _apkPayload.length + "\r\n"
                            + "Connection: close\r\n\r\n";
                    os.write(headers.getBytes(StandardCharsets.US_ASCII));
                    os.write(_apkPayload);
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
        }
    }

    private static final class MockSsoHarnessServer implements AutoCloseable {
        private final ServerSocket _serverSocket;
        private final AtomicBoolean _running = new AtomicBoolean(true);
        private final Thread _acceptThread;
        final List<String> receivedInstallIds = new ArrayList<>();
        final List<Map<String, Object>> receivedPatchBodies = new ArrayList<>();
        volatile String currentHandle = "samurai";
        volatile String currentDisplayName = "User";
        volatile String currentAvatarUrl = "/api/v1/avatars/1.png";
        volatile String currentBio = "Octi Master";
        volatile String currentLocation = "Kyoto, Japan";
        volatile int avatarFetchCount = 0;

        MockSsoHarnessServer() throws Exception {
            _serverSocket = new ServerSocket();
            _serverSocket.setReuseAddress(true);
            _serverSocket.bind(new InetSocketAddress(InetAddress.getByName("127.0.0.1"), 0));
            _acceptThread = new Thread(this::acceptLoop, "octi-vm-sso-harness");
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
                        if (colon > 0) {
                            String name = h.substring(0, colon).trim();
                            String val = h.substring(colon + 1).trim();
                            if (name.equalsIgnoreCase("Content-Length")) {
                                contentLength = Integer.parseInt(val);
                            } else if (name.equalsIgnoreCase("X-Octi-Install-Id")) {
                                synchronized (receivedInstallIds) {
                                    receivedInstallIds.add(val);
                                }
                            }
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
                    Map<String, Object> reqJson = body.trim().isEmpty()
                            ? java.util.Collections.emptyMap()
                            : OctiMiniJson.parseObject(body);
                    String provider = OctiMiniJson.getString(reqJson, "provider", "google");
                    String token = OctiMiniJson.getString(reqJson, "id_token",
                            OctiMiniJson.getString(reqJson, "access_token", ""));

                    int status = 200;
                    String respBody;

                    if (reqLine.contains("/api/v1/auth/sso")) {
                        boolean isNew = token.startsWith("token-new-");
                        status = isNew ? 201 : 200;
                        String ssoHandle = token.contains("samurai") ? "samurai" : ("User_" + provider);
                        respBody = "{\n"
                                + "  \"session_token\": \"sess-" + provider + "\",\n"
                                + "  \"expires_at\": 1999999999,\n"
                                + "  \"created\": " + isNew + ",\n"
                                + "  \"onboarding_required\": " + isNew + ",\n"
                                + "  \"profile\": {\n"
                                + "    \"uuid\": \"p-" + provider + "\",\n"
                                + "    \"handle\": \"" + ssoHandle + "\",\n"
                                + "    \"elo_2p\": 1500\n"
                                + "  },\n"
                                + "  \"linked_identities\": [\n"
                                + "    {\"provider\": \"" + provider + "\", \"provider_email\": \"u@selfso.com\"}\n"
                                + "  ]\n"
                                + "}";
                    } else if (reqLine.contains("/api/v1/auth/link")) {
                        boolean forceRelink = Boolean.TRUE.equals(reqJson.get("force_relink"));
                        if (token.contains("conflict") && !forceRelink) {
                            status = 409;
                            respBody = "{\n"
                                    + "  \"error\": \"identity_conflict\",\n"
                                    + "  \"message\": \"Identity is already linked to another OCTI profile\",\n"
                                    + "  \"existing_profile\": {\n"
                                    + "    \"uuid\": \"p-microsoft\",\n"
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
                    } else if (reqLine.contains("/api/v1/lobby/state")) {
                        status = 200;
                        respBody = "{\n"
                                + "  \"players\": [],\n"
                                + "  \"tables\": [],\n"
                                + "  \"recent_chat\": [],\n"
                                + "  \"latest_seq\": 0\n"
                                + "}";
                    } else if (reqLine.contains("/api/v1/profile/me")) {
                        if (reqLine.startsWith("PATCH")) {
                            synchronized (receivedPatchBodies) {
                                receivedPatchBodies.add(new HashMap<>(reqJson));
                            }
                            String hVal = OctiMiniJson.getString(reqJson, "handle", "samurai");
                            if (hVal.contains("taken")) {
                                status = 409;
                                respBody = "{\"error\":\"handle_taken\",\"message\":\"Handle is already taken\"}";
                            } else if (hVal.contains("invalid") || (hVal.length() > 0 && hVal.length() < 3)) {
                                status = 400;
                                respBody = "{\"error\":\"invalid_handle\",\"message\":\"Invalid handle format\"}";
                            } else {
                                status = 200;
                                if (reqJson.containsKey("handle")) {
                                    currentHandle = hVal;
                                }
                                if (reqJson.containsKey("display_name")) {
                                    currentDisplayName = OctiMiniJson.getString(reqJson, "display_name", currentDisplayName);
                                }
                                if (reqJson.containsKey("bio")) {
                                    currentBio = OctiMiniJson.getString(reqJson, "bio", currentBio);
                                }
                                if (reqJson.containsKey("location")) {
                                    currentLocation = OctiMiniJson.getString(reqJson, "location", currentLocation);
                                }
                                if (reqJson.containsKey("avatar_url")) {
                                    currentAvatarUrl = OctiMiniJson.getString(reqJson, "avatar_url", currentAvatarUrl);
                                }
                                respBody = "{\n"
                                        + "  \"uuid\": \"p-me\",\n"
                                        + "  \"handle\": \"" + currentHandle + "\",\n"
                                        + "  \"display_name\": \"" + currentDisplayName + "\",\n"
                                        + "  \"avatar_url\": \"" + currentAvatarUrl + "\",\n"
                                        + "  \"bio\": \"" + currentBio + "\",\n"
                                        + "  \"location\": \"" + currentLocation + "\",\n"
                                        + "  \"elo_2p\": 1500,\n"
                                        + "  \"linked_identities\": [\n"
                                        + "    {\"provider\": \"google\", \"provider_email\": \"u@selfso.com\"}\n"
                                        + "  ]\n"
                                        + "}";
                            }
                        } else {
                            status = 200;
                            respBody = "{\n"
                                    + "  \"uuid\": \"p-me\",\n"
                                    + "  \"handle\": \"" + currentHandle + "\",\n"
                                    + "  \"display_name\": \"" + currentDisplayName + "\",\n"
                                    + "  \"avatar_url\": \"" + currentAvatarUrl + "\",\n"
                                    + "  \"bio\": \"" + currentBio + "\",\n"
                                    + "  \"location\": \"" + currentLocation + "\",\n"
                                    + "  \"elo_2p\": 1500,\n"
                                    + "  \"linked_identities\": [\n"
                                    + "    {\"provider\": \"google\", \"provider_email\": \"u@selfso.com\"}\n"
                                    + "  ]\n"
                                    + "}";
                        }
                    } else if (reqLine.contains("/api/v1/avatars/")) {
                        avatarFetchCount++;
                        byte[] avatarPayload = new byte[]{0x01, 0x02, 0x03, 0x04};
                        String headers = "HTTP/1.1 200 OK\r\n"
                                + "Content-Type: image/jpeg\r\n"
                                + "Content-Length: " + avatarPayload.length + "\r\n"
                                + "Connection: close\r\n\r\n";
                        OutputStream os = s.getOutputStream();
                        os.write(headers.getBytes(StandardCharsets.UTF_8));
                        os.write(avatarPayload);
                        os.flush();
                        continue;
                    } else if (reqLine.contains("/api/v1/tables") && reqLine.startsWith("POST") && !reqLine.contains("/leave") && !reqLine.contains("/observe")) {
                        status = 201;
                        respBody = "{\n"
                                + "  \"room_id\": \"room-leave-test-456\",\n"
                                + "  \"seat\": 1,\n"
                                + "  \"phase\": \"WAITING_FOR_PLAYERS\"\n"
                                + "}";
                    } else if (reqLine.contains("/leave")) {
                        status = 200;
                        respBody = "{\"ok\":true}";
                    } else if (reqLine.contains("/observe")) {
                        status = 200;
                        respBody = "{\"ok\":true}";
                    } else if (reqLine.contains("/api/v1/games/room-leave-test-456")) {
                        status = 200;
                        respBody = "{\n"
                                + "  \"room_id\": \"room-leave-test-456\",\n"
                                + "  \"mode\": \"2P\",\n"
                                + "  \"phase\": \"WAITING_FOR_PLAYERS\",\n"
                                + "  \"fen\": \"\",\n"
                                + "  \"seats\": [],\n"
                                + "  \"observers\": []\n"
                                + "}";
                    } else if (reqLine.contains("/view")) {
                        status = 200;
                        respBody = "{\n"
                                + "  \"room_id\": \"room-test-123\",\n"
                                + "  \"mode\": \"2P\",\n"
                                + "  \"fen\": \"\",\n"
                                + "  \"seats\": [],\n"
                                + "  \"observers\": []\n"
                                + "}";
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
        }
    }

    private static final class MockTelemetryServer implements AutoCloseable {
        private final ServerSocket _serverSocket;
        private final AtomicBoolean _running = new AtomicBoolean(true);
        private final Thread _acceptThread;
        private volatile boolean _crashEndpointOnline = true;
        private volatile boolean _uploadEndpointOnline = true;
        private final List<String> _receivedCrashBodies = new ArrayList<>();
        private final List<String> _receivedUploadBodies = new ArrayList<>();
        final List<String> receivedInstallIds = new ArrayList<>();

        MockTelemetryServer() throws Exception {
            _serverSocket = new ServerSocket();
            _serverSocket.setReuseAddress(true);
            _serverSocket.bind(new InetSocketAddress(InetAddress.getByName("127.0.0.1"), 0));
            _acceptThread = new Thread(this::acceptLoop, "mock-telemetry-server");
            _acceptThread.setDaemon(true);
            _acceptThread.start();
        }

        int port() {
            return _serverSocket.getLocalPort();
        }

        void setCrashEndpointOnline(boolean online) {
            _crashEndpointOnline = online;
        }

        void setUploadEndpointOnline(boolean online) {
            _uploadEndpointOnline = online;
        }

        List<String> receivedCrashBodies() {
            synchronized (_receivedCrashBodies) {
                return new ArrayList<>(_receivedCrashBodies);
            }
        }

        List<String> receivedUploadBodies() {
            synchronized (_receivedUploadBodies) {
                return new ArrayList<>(_receivedUploadBodies);
            }
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
                        if (colon > 0) {
                            String name = h.substring(0, colon).trim();
                            String val = h.substring(colon + 1).trim();
                            if (name.equalsIgnoreCase("Content-Length")) {
                                contentLength = Integer.parseInt(val);
                            } else if (name.equalsIgnoreCase("X-Octi-Install-Id")) {
                                synchronized (receivedInstallIds) {
                                    receivedInstallIds.add(val);
                                }
                            }
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

                    int status = 200;
                    String respBody;

                    if (reqLine.contains("/api/v1/telemetry/crash")) {
                        if (!_crashEndpointOnline) {
                            status = 503;
                            respBody = "{\"error\":\"service_unavailable\"}";
                        } else {
                            synchronized (_receivedCrashBodies) {
                                _receivedCrashBodies.add(body);
                            }
                            status = 201;
                            respBody = "{\"crash_id\":\"c-" + System.currentTimeMillis() + "\",\"deduplicated\":false}";
                        }
                    } else if (reqLine.contains("/api/v1/archive/upload")) {
                        if (!_uploadEndpointOnline) {
                            status = 503;
                            respBody = "{\"error\":\"service_unavailable\"}";
                        } else {
                            synchronized (_receivedUploadBodies) {
                                _receivedUploadBodies.add(body);
                            }
                            status = 200;
                            respBody = "{\"game_id\":\"mock-game-id\",\"ply_count\":42}";
                        }
                    } else {
                        status = 404;
                        respBody = "{\"error\":\"not_found\"}";
                    }

                    byte[] payload = respBody.getBytes(StandardCharsets.UTF_8);
                    String statusText = (status == 201) ? "Created"
                            : (status == 200) ? "OK"
                            : (status == 503) ? "Service Unavailable" : "Not Found";
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
        }
    }

    @Test
    public void testOctiCrashReporterHandledAndUncaughtSpoolCapAndFlushing() throws Exception {
        OctiCrashReporter.setStorageRootDir(_tempDir);
        OctiCrashReporter.updateGameContext("LOCAL_VS_BOT", 5, "fake-fen-context");
        String ctx = OctiCrashReporter.getGameContextJson();
        assertTrue(ctx.contains("LOCAL_VS_BOT"));
        assertTrue(ctx.contains("fake-fen-context"));

        File handled = OctiCrashReporter.recordHandledException(
                _tempDir, "TestWorkerThread", new RuntimeException("TestHandledCrash"), "2.1.2", "install-001");
        assertNotNull(handled);
        assertTrue(handled.exists());
        String content = new String(Files.readAllBytes(handled.toPath()), StandardCharsets.UTF_8);
        assertTrue(content.contains("TestHandledCrash"));
        assertTrue(content.contains("TestWorkerThread"));
        assertTrue(content.contains("install-001"));
        assertTrue(content.contains("fake-fen-context"));

        // Enforce 32-file cap
        for (int i = 0; i < 35; i++) {
            OctiCrashReporter.recordHandledException(
                    _tempDir, "Worker_" + i, new RuntimeException("Crash_" + i), "2.1.2", "inst-" + i);
        }
        List<File> pendingCrashes = OctiCrashReporter.listPendingCrashFiles(_tempDir);
        assertTrue("Pending crashes count must not exceed 32", pendingCrashes.size() <= 32);

        // Global uncaught exception handler
        OctiCrashReporter.installGlobalHandler(_tempDir, "2.1.2", "install-uncaught-999");
        Thread.UncaughtExceptionHandler h = Thread.getDefaultUncaughtExceptionHandler();
        assertNotNull(h);
        h.uncaughtException(Thread.currentThread(), new IllegalStateException("SimulatedFatalJvmCrash"));
        OctiCrashReporter.uninstallGlobalHandler();

        boolean foundFatal = false;
        for (File f : OctiCrashReporter.listPendingCrashFiles(_tempDir)) {
            String c = new String(Files.readAllBytes(f.toPath()), StandardCharsets.UTF_8);
            if (c.contains("SimulatedFatalJvmCrash")) {
                foundFatal = true;
                break;
            }
        }
        assertTrue("SimulatedFatalJvmCrash must be spooled to disk", foundFatal);

        // Test flushing with mock server: 503 retains on disk, 201 deletes
        try (MockTelemetryServer server = new MockTelemetryServer()) {
            OctiNetworkClient client = OctiNetworkClient.forHermeticTest(server.port());

            server.setCrashEndpointOnline(false);
            int beforeCount = OctiCrashReporter.listPendingCrashFiles(_tempDir).size();
            assertTrue(beforeCount > 0);

            int flushed503 = OctiCrashReporter.flushPendingCrashReports(_tempDir, client, "fallback-id");
            assertEquals(0, flushed503);
            assertEquals(beforeCount, OctiCrashReporter.listPendingCrashFiles(_tempDir).size());

            server.setCrashEndpointOnline(true);
            int flushed201 = OctiCrashReporter.flushPendingCrashReports(_tempDir, client, "fallback-id");
            assertEquals(beforeCount, flushed201);
            assertEquals(0, OctiCrashReporter.listPendingCrashFiles(_tempDir).size());
            assertTrue(server.receivedCrashBodies().size() >= beforeCount);
        }
    }

    @Test
    public void testCompletedAiGameSpoolsTranscriptWhenOfflineAndFlushesWhenOnline() throws Exception {
        File cfgFile = new File(_tempDir, "client_config.json");
        String json = "{\n"
                + "  \"telemetry_consent\": \"share\",\n"
                + "  \"telemetry_consent_version\": 1,\n"
                + "  \"install_id\": \"inst-durable-test-888\"\n"
                + "}\n";
        Files.write(cfgFile.toPath(), json.getBytes(StandardCharsets.UTF_8));

        try (MockTelemetryServer server = new MockTelemetryServer()) {
            server.setUploadEndpointOnline(false);
            OctiNetworkClient client = OctiNetworkClient.forHermeticTest(server.port());
            ClientConfigStore configStore = new ClientConfigStore(cfgFile);

            try (OctiAppViewModel vm = new OctiAppViewModel(configStore, client)) {
                vm.setStorageRootDir(_tempDir);
                vm.setAsyncBotThinkingEnabled(false);
                vm.startNewLocalGame(
                        OctiGameMode.GAME_MODE_2,
                        false,
                        false,
                        OctiGameBases.GAME_BASES_3,
                        true,
                        OctiBotEngine.BotDifficulty.CASUAL,
                        OctiPlayer.PLAYER_A);

                String legal = vm.getBoard().getGame().generateLegalMoves().get(0).asString();
                assertTrue(vm.applyMoveNotation(legal));

                vm.getBoard().getGame().resign(OctiPlayer.PLAYER_B);
                assertTrue(vm.getBoard().getGame().isGameOver());

                // Attempt upload while server is offline
                boolean uploadedSync = vm.uploadCompletedBotGameIfEligible(false, true);
                assertFalse("Upload must return false when server returns 503", uploadedSync);

                // Spooled file must exist in pending_uploads
                List<File> pending = OctiCrashReporter.listPendingTranscriptFiles(_tempDir);
                assertEquals(1, pending.size());
                String spooledContent = new String(Files.readAllBytes(pending.get(0).toPath()), StandardCharsets.UTF_8);
                assertTrue(spooledContent.contains("octi.transcript.v3") || spooledContent.contains("transcript"));

                // Bring server online and flush telemetry
                server.setUploadEndpointOnline(true);
                vm.flushPendingTelemetry(false);

                // Spooled file must now be deleted
                assertEquals(0, OctiCrashReporter.listPendingTranscriptFiles(_tempDir).size());
                assertEquals(1, server.receivedUploadBodies().size());
                assertTrue(server.receivedUploadBodies().get(0).contains("transcript"));
            }

            // Verify bounded 32-file cap on pending_uploads spooling
            for (int i = 0; i < 35; i++) {
                OctiCrashReporter.spoolTrainingTranscript(
                        _tempDir, "game_" + i, "{\"game_id\":\"g" + i + "\"}", "client-test");
            }
            List<File> spooled = OctiCrashReporter.listPendingTranscriptFiles(_tempDir);
            assertTrue("Pending transcripts count must not exceed 32", spooled.size() <= 32);
        }
    }

    @Test
    public void testHandleOnboardingAndEditorAsyncNominal() throws Exception {
        File cfgFile = new File(_tempDir, "client_config_async.json");
        Files.write(cfgFile.toPath(), "{}".getBytes(StandardCharsets.UTF_8));

        try (MockSsoHarnessServer server = new MockSsoHarnessServer()) {
            OctiNetworkClient client = OctiNetworkClient.forHermeticTest(server.port());
            ClientConfigStore configStore = new ClientConfigStore(cfgFile);

            try (OctiAppViewModel vm = new OctiAppViewModel(configStore, client)) {
                vm.setStorageRootDir(_tempDir);

                // Sign in with new account triggering onboarding
                vm.loginWithSso("google", "token-new-handle-test");
                assertTrue(vm.getOnboardingRequired().getValue());

                CountDownLatch latch1 = new CountDownLatch(1);
                vm.completeHandleOnboardingAsync("samurai", res -> {
                    assertTrue(res.ok);
                    latch1.countDown();
                });
                assertTrue("completeHandleOnboardingAsync timed out", latch1.await(5, TimeUnit.SECONDS));

                assertNotNull(vm.getOnlineProfile().getValue());
                assertEquals("samurai", vm.getOnlineProfile().getValue().handle);
                assertFalse(vm.getOnboardingRequired().getValue());
                assertEquals("", vm.getProfileActionMessage().getValue());

                // Post-signup handle editing
                CountDownLatch latch2 = new CountDownLatch(1);
                vm.updateMyHandleAsync("ninja", res -> {
                    assertTrue(res.ok);
                    latch2.countDown();
                });
                assertTrue("updateMyHandleAsync timed out", latch2.await(5, TimeUnit.SECONDS));

                assertEquals("ninja", vm.getOnlineProfile().getValue().handle);
                assertTrue(vm.getProfileActionMessage().getValue().contains("Handle updated to @ninja"));
            }
        }
    }

    @Test
    public void testHandleConflictDuringOnboardingAndEditing() throws Exception {
        File cfgFile = new File(_tempDir, "client_config_conflict.json");
        Files.write(cfgFile.toPath(), "{}".getBytes(StandardCharsets.UTF_8));

        try (MockSsoHarnessServer server = new MockSsoHarnessServer()) {
            OctiNetworkClient client = OctiNetworkClient.forHermeticTest(server.port());
            ClientConfigStore configStore = new ClientConfigStore(cfgFile);

            try (OctiAppViewModel vm = new OctiAppViewModel(configStore, client)) {
                vm.setStorageRootDir(_tempDir);

                vm.loginWithSso("google", "token-new-conflict-test");
                assertTrue(vm.getOnboardingRequired().getValue());

                CountDownLatch latch1 = new CountDownLatch(1);
                vm.completeHandleOnboardingAsync("taken_user", res -> {
                    assertFalse(res.ok);
                    latch1.countDown();
                });
                assertTrue("completeHandleOnboardingAsync timed out", latch1.await(5, TimeUnit.SECONDS));

                // Onboarding must remain true on conflict failure
                assertTrue(vm.getOnboardingRequired().getValue());
                assertTrue(vm.getProfileActionMessage().getValue().contains("already taken"));

                // Conflict during profile editor
                CountDownLatch latch2 = new CountDownLatch(1);
                vm.updateMyHandleAsync("taken_user2", res -> {
                    assertFalse(res.ok);
                    latch2.countDown();
                });
                assertTrue("updateMyHandleAsync timed out", latch2.await(5, TimeUnit.SECONDS));

                assertTrue(vm.getProfileActionMessage().getValue().contains("already taken"));
            }
        }
    }

    @Test
    public void testNetworkClientRuntimeExceptionRecordsCrashAndDoesNotRetry() throws Exception {
        OctiCrashReporter.setStorageRootDir(_tempDir);
        try (MockTelemetryServer server = new MockTelemetryServer()) {
            OctiNetworkClient client = OctiNetworkClient.forHermeticTest(server.port());
            OctiCrashReporter reporter = OctiCrashReporter.getInstance();
            client.setCrashReporter(reporter);

            int initPending = reporter.getPendingCount();

            // Inject RuntimeException into request execution
            client.injectRuntimeExceptionForTest(new RuntimeException("SimulatedNetworkOnMainThreadException"));

            long start = System.currentTimeMillis();
            OctiNetworkClient.ApiResult<OctiNetworkClient.PlayerProfileInfo> res = client.updateMyProfile("any");
            long duration = System.currentTimeMillis() - start;

            assertFalse("Request must fail when RuntimeException occurs", res.ok);
            assertTrue("Client must abort retry loop immediately (< 1000ms), took " + duration + "ms", duration < 1000);
            assertTrue("Pending crash count must increase, was " + initPending + " now " + reporter.getPendingCount(),
                    reporter.getPendingCount() >= initPending + 1);

            client.injectRuntimeExceptionForTest(null);
        }
    }

    @Test
    public void testTranscriptAttributionAndFullProfileEditingParity() throws Exception {
        File cfgFile = new File(_tempDir, "client_config_profile.json");
        Files.write(cfgFile.toPath(), "{}".getBytes(StandardCharsets.UTF_8));

        try (MockSsoHarnessServer server = new MockSsoHarnessServer()) {
            OctiNetworkClient client = OctiNetworkClient.forHermeticTest(server.port());
            ClientConfigStore configStore = new ClientConfigStore(cfgFile);

            try (OctiAppViewModel vm = new OctiAppViewModel(configStore, client)) {
                vm.setStorageRootDir(_tempDir);
                vm.setAsyncBotThinkingEnabled(false);

                // 1. Check anonymous / unauthenticated transcript attribution
                vm.startNewLocalGame(
                        OctiGameMode.GAME_MODE_2,
                        false,
                        false,
                        OctiGameBases.GAME_BASES_3,
                        true,
                        OctiBotEngine.BotDifficulty.CASUAL,
                        OctiPlayer.PLAYER_A);

                String anonTranscript = vm.buildCurrentGameTranscriptJson(false);
                Map<String, Object> anonDoc = OctiMiniJson.parseObject(anonTranscript);
                List<Object> anonPlayers = OctiMiniJson.getArray(anonDoc, "players");
                assertNotNull(anonPlayers);
                assertEquals(2, anonPlayers.size());
                @SuppressWarnings("unchecked")
                Map<String, Object> p1Anon = (Map<String, Object>) anonPlayers.get(0);
                @SuppressWarnings("unchecked")
                Map<String, Object> p2Anon = (Map<String, Object>) anonPlayers.get(1);
                assertEquals(1, OctiMiniJson.getInt(p1Anon, "seat", 0));
                assertEquals("HUMAN", OctiMiniJson.getString(p1Anon, "type", ""));
                assertEquals("Player", OctiMiniJson.getString(p1Anon, "handle", ""));
                assertEquals(2, OctiMiniJson.getInt(p2Anon, "seat", 0));
                assertEquals("AI", OctiMiniJson.getString(p2Anon, "type", ""));

                // 2. Sign in with SSO
                vm.loginWithSso("google", "token-samurai");
                assertEquals("samurai", vm.getOnlineProfile().getValue().handle);
                assertTrue(vm.isAuthenticated().getValue());

                // 3. Test PLAYER_A as Human: seat 1 = HUMAN (samurai), seat 2 = AI
                vm.startNewLocalGame(
                        OctiGameMode.GAME_MODE_2,
                        false,
                        false,
                        OctiGameBases.GAME_BASES_3,
                        true,
                        OctiBotEngine.BotDifficulty.CASUAL,
                        OctiPlayer.PLAYER_A);

                String pATranscript = vm.buildCurrentGameTranscriptJson(false);
                Map<String, Object> pADoc = OctiMiniJson.parseObject(pATranscript);
                List<Object> pAPlayers = OctiMiniJson.getArray(pADoc, "players");
                assertNotNull(pAPlayers);
                assertEquals(2, pAPlayers.size());
                @SuppressWarnings("unchecked")
                Map<String, Object> p1A = (Map<String, Object>) pAPlayers.get(0);
                @SuppressWarnings("unchecked")
                Map<String, Object> p2A = (Map<String, Object>) pAPlayers.get(1);
                assertEquals(1, OctiMiniJson.getInt(p1A, "seat", 0));
                assertEquals("A", OctiMiniJson.getString(p1A, "player", ""));
                assertEquals("HUMAN", OctiMiniJson.getString(p1A, "type", ""));
                assertEquals("samurai", OctiMiniJson.getString(p1A, "handle", ""));
                assertEquals(2, OctiMiniJson.getInt(p2A, "seat", 0));
                assertEquals("B", OctiMiniJson.getString(p2A, "player", ""));
                assertEquals("AI", OctiMiniJson.getString(p2A, "type", ""));
                assertTrue(OctiMiniJson.getString(p2A, "handle", "").startsWith("AI_L"));

                // 4. Test PLAYER_B as Human: seat 1 = AI, seat 2 = HUMAN (samurai)
                vm.startNewLocalGame(
                        OctiGameMode.GAME_MODE_2,
                        false,
                        false,
                        OctiGameBases.GAME_BASES_3,
                        true,
                        OctiBotEngine.BotDifficulty.CASUAL,
                        OctiPlayer.PLAYER_B);

                String pBTranscript = vm.buildCurrentGameTranscriptJson(false);
                Map<String, Object> pBDoc = OctiMiniJson.parseObject(pBTranscript);
                List<Object> pBPlayers = OctiMiniJson.getArray(pBDoc, "players");
                assertNotNull(pBPlayers);
                assertEquals(2, pBPlayers.size());
                @SuppressWarnings("unchecked")
                Map<String, Object> p1B = (Map<String, Object>) pBPlayers.get(0);
                @SuppressWarnings("unchecked")
                Map<String, Object> p2B = (Map<String, Object>) pBPlayers.get(1);
                assertEquals(1, OctiMiniJson.getInt(p1B, "seat", 0));
                assertEquals("A", OctiMiniJson.getString(p1B, "player", ""));
                assertEquals("AI", OctiMiniJson.getString(p1B, "type", ""));
                assertEquals(2, OctiMiniJson.getInt(p2B, "seat", 0));
                assertEquals("B", OctiMiniJson.getString(p2B, "player", ""));
                assertEquals("HUMAN", OctiMiniJson.getString(p2B, "type", ""));
                assertEquals("samurai", OctiMiniJson.getString(p2B, "handle", ""));

                // 5. Test full profile update: handle, display_name, location, bio, avatar_url
                String initialInstallId = configStore.preferences().getInstallId();
                assertFalse(initialInstallId.isEmpty());

                CountDownLatch latch = new CountDownLatch(1);
                vm.updateMyProfileAsync(
                        "ronin",
                        "Master Ronin",
                        "Kyoto, Japan",
                        "Octi enthusiast and strategist",
                        "data:image/jpeg;base64,ZmFrZWF2YXRhcg==",
                        true,
                        res -> {
                            assertTrue(res.ok);
                            latch.countDown();
                        });
                assertTrue("updateMyProfileAsync timed out", latch.await(5, TimeUnit.SECONDS));

                // Verify ClientConfigStore persistence
                assertEquals("ronin", configStore.preferences().getHandle());
                assertEquals("Master Ronin", configStore.preferences().getDisplayName());
                assertEquals("Kyoto, Japan", configStore.preferences().getLocation());
                assertEquals("Octi enthusiast and strategist", configStore.preferences().getBio());
                assertEquals("data:image/jpeg;base64,ZmFrZWF2YXRhcg==", configStore.preferences().getAvatarUrl());

                // Verify server received headers and payload
                synchronized (server.receivedInstallIds) {
                    assertFalse(server.receivedInstallIds.isEmpty());
                    assertTrue(server.receivedInstallIds.contains(initialInstallId));
                }

                synchronized (server.receivedPatchBodies) {
                    assertFalse(server.receivedPatchBodies.isEmpty());
                    Map<String, Object> lastPatch = server.receivedPatchBodies.get(server.receivedPatchBodies.size() - 1);
                    assertEquals("ronin", lastPatch.get("handle"));
                    assertEquals("Master Ronin", lastPatch.get("display_name"));
                    assertEquals("Kyoto, Japan", lastPatch.get("location"));
                    assertEquals("Octi enthusiast and strategist", lastPatch.get("bio"));
                    assertEquals("data:image/jpeg;base64,ZmFrZWF2YXRhcg==", lastPatch.get("avatar_url"));
                }

                // 6. Test clearing avatar sends avatar_url = ""
                CountDownLatch latchClear = new CountDownLatch(1);
                vm.updateMyProfileAsync(
                        "ronin",
                        "Master Ronin",
                        "Kyoto, Japan",
                        "Octi enthusiast and strategist",
                        "",
                        true,
                        res -> {
                            assertTrue(res.ok);
                            latchClear.countDown();
                        });
                assertTrue("updateMyProfileAsync clear timed out", latchClear.await(5, TimeUnit.SECONDS));

                synchronized (server.receivedPatchBodies) {
                    Map<String, Object> lastPatch = server.receivedPatchBodies.get(server.receivedPatchBodies.size() - 1);
                    assertTrue(lastPatch.containsKey("avatar_url"));
                    assertEquals("", lastPatch.get("avatar_url"));
                }

                // 7. Test fetchAvatarBytes
                byte[] avatarBytes = client.fetchAvatarBytes("/api/v1/avatars/ronin.jpg");
                assertNotNull(avatarBytes);
                assertEquals(4, avatarBytes.length);
                assertEquals(0x01, avatarBytes[0]);
                assertEquals(0x02, avatarBytes[1]);
                assertEquals(0x03, avatarBytes[2]);
                assertEquals(0x04, avatarBytes[3]);
            }
        }
    }

    @Test
    public void testProfilePartialPatchAndAvatarPreservationParity() throws Exception {
        try (MockSsoHarnessServer server = new MockSsoHarnessServer()) {
            server.currentBio = "Server Bio Baseline";
            server.currentLocation = "Tokyo, Japan";
            server.currentAvatarUrl = "/api/v1/avatars/existing.png";
            server.currentDisplayName = "Initial Display";
            server.currentHandle = "baselineuser";

            try (OctiNetworkClient client = OctiNetworkClient.forHermeticTest(server.port())) {
                client.setSessionToken("sess-test");

                // 1. 3-arg updateMyProfile(handle, avatarUrl, displayName) must NOT overwrite bio or location
                OctiNetworkClient.ApiResult<OctiNetworkClient.PlayerProfileInfo> res3 =
                        client.updateMyProfile("updatedhandle", null, "New Name");
                assertTrue(res3.ok);
                assertEquals("updatedhandle", res3.data.handle);
                assertEquals("New Name", res3.data.displayName);
                assertEquals("Server Bio Baseline", res3.data.bio);
                assertEquals("Tokyo, Japan", res3.data.location);
                assertEquals("/api/v1/avatars/existing.png", res3.data.avatarUrl);

                synchronized (server.receivedPatchBodies) {
                    Map<String, Object> lastPatch = server.receivedPatchBodies.get(server.receivedPatchBodies.size() - 1);
                    assertEquals("updatedhandle", lastPatch.get("handle"));
                    assertEquals("New Name", lastPatch.get("display_name"));
                    assertFalse("3-arg updateMyProfile must not send bio", lastPatch.containsKey("bio"));
                    assertFalse("3-arg updateMyProfile must not send location", lastPatch.containsKey("location"));
                    assertFalse("3-arg updateMyProfile must not send avatar_url when null/empty", lastPatch.containsKey("avatar_url"));
                }

                // 2. 6-arg updateMyProfile with setAvatarUrl = false must preserve existing avatar_url
                OctiNetworkClient.ApiResult<OctiNetworkClient.PlayerProfileInfo> res6 =
                        client.updateMyProfile(
                                "updatedhandle2",
                                "data:image/jpeg;base64,ignored",
                                "Display 2",
                                "Updated Bio 2",
                                "Osaka, Japan",
                                false);
                assertTrue(res6.ok);
                assertEquals("updatedhandle2", res6.data.handle);
                assertEquals("Display 2", res6.data.displayName);
                assertEquals("Updated Bio 2", res6.data.bio);
                assertEquals("Osaka, Japan", res6.data.location);
                assertEquals("/api/v1/avatars/existing.png", res6.data.avatarUrl);

                synchronized (server.receivedPatchBodies) {
                    Map<String, Object> lastPatch = server.receivedPatchBodies.get(server.receivedPatchBodies.size() - 1);
                    assertEquals("updatedhandle2", lastPatch.get("handle"));
                    assertEquals("Display 2", lastPatch.get("display_name"));
                    assertEquals("Updated Bio 2", lastPatch.get("bio"));
                    assertEquals("Osaka, Japan", lastPatch.get("location"));
                    assertFalse("setAvatarUrl=false must omit avatar_url from PATCH", lastPatch.containsKey("avatar_url"));
                }
            }
        }
    }

    @Test
    public void testAvatarEncodingAndDecodingCacheParity() throws Exception {
        try (MockSsoHarnessServer server = new MockSsoHarnessServer()) {
            try (OctiNetworkClient client = OctiNetworkClient.forHermeticTest(server.port())) {
                // Test 1: Hook avatarEncoderForTest and verify non-webp encoding & decode round-trip
                OctiAppViewModel.avatarEncoderForTest = bmp -> "data:image/jpeg;base64,ZmFrZWF2YXRhcg==";
                try {
                    Constructor<Bitmap> ctor = Bitmap.class.getDeclaredConstructor();
                    ctor.setAccessible(true);
                    Bitmap b = ctor.newInstance();

                    String dataUri = OctiAppViewModel.encodeAvatarBitmapToDataUri(b);
                    assertTrue(dataUri.startsWith("data:image/jpeg;base64,") || dataUri.startsWith("data:image/png;base64,"));
                    assertFalse("Avatar data URI must never use webp", dataUri.contains("webp"));

                    byte[] decoded = OctiAppViewModel.decodeAvatarDataUriOrBytes(dataUri, null);
                    assertNotNull(decoded);
                    assertEquals("fakeavatar", new String(decoded, StandardCharsets.UTF_8));
                } finally {
                    OctiAppViewModel.avatarEncoderForTest = null;
                }

                // Test 2: In-memory avatar caching
                OctiAppViewModel.clearAvatarBytesCacheForTest();
                server.avatarFetchCount = 0;

                String avatarPath = "/api/v1/avatars/cached_test.png";
                byte[] fetch1 = OctiAppViewModel.decodeAvatarDataUriOrBytes(avatarPath, client);
                assertNotNull(fetch1);
                assertEquals(4, fetch1.length);
                assertEquals(1, server.avatarFetchCount);

                // Second fetch must hit in-memory cache without hitting server
                byte[] fetch2 = OctiAppViewModel.decodeAvatarDataUriOrBytes(avatarPath, client);
                assertNotNull(fetch2);
                assertEquals(4, fetch2.length);
                assertEquals("Second fetch must be served from in-memory cache", 1, server.avatarFetchCount);

                // Calling with null netClient must also return from cache
                byte[] fetchCached = OctiAppViewModel.decodeAvatarDataUriOrBytes(avatarPath, null);
                assertNotNull(fetchCached);
                assertEquals(4, fetchCached.length);
                assertEquals(1, server.avatarFetchCount);

                // Clearing cache forces subsequent fetch to hit server
                OctiAppViewModel.clearAvatarBytesCacheForTest();
                byte[] fetch3 = OctiAppViewModel.decodeAvatarDataUriOrBytes(avatarPath, client);
                assertNotNull(fetch3);
                assertEquals(2, server.avatarFetchCount);
            }
        }
    }

    @Test
    public void testStartupAndDialogProfileSyncParity() throws Exception {
        File cfgFile = new File(_tempDir, "client_config_sync.json");
        Files.write(cfgFile.toPath(), "{}".getBytes(StandardCharsets.UTF_8));

        try (MockSsoHarnessServer server = new MockSsoHarnessServer()) {
            server.currentHandle = "synchandle";
            server.currentDisplayName = "Server User";
            server.currentAvatarUrl = "/api/v1/avatars/synced_avatar.png";
            server.currentBio = "Server bio for sync verification";
            server.currentLocation = "Sapporo, Japan";

            try (OctiNetworkClient client = OctiNetworkClient.forHermeticTest(server.port())) {
                ClientConfigStore configStore = new ClientConfigStore(cfgFile);
                configStore.preferences().setSessionToken("sess-valid-token");
                configStore.preferences().setHandle("stale_handle");
                configStore.preferences().setBio("Stale local bio");
                configStore.save();

                try (OctiAppViewModel vm = new OctiAppViewModel(configStore, client)) {
                    vm.setStorageRootDir(_tempDir);
                    vm.setAsyncBotThinkingEnabled(false);

                    // 1. refreshMyProfileAsync pulls server bio, avatarUrl, location, displayName, and identities
                    CountDownLatch latch = new CountDownLatch(1);
                    vm.refreshMyProfileAsync(ok -> {
                        assertTrue(ok);
                        latch.countDown();
                    });
                    assertTrue("refreshMyProfileAsync timed out", latch.await(5, TimeUnit.SECONDS));

                    assertEquals("synchandle", vm.getOnlineProfile().getValue().handle);
                    assertEquals("Server User", vm.getOnlineProfile().getValue().displayName);
                    assertEquals("/api/v1/avatars/synced_avatar.png", vm.getOnlineProfile().getValue().avatarUrl);
                    assertEquals("Server bio for sync verification", vm.getOnlineProfile().getValue().bio);
                    assertEquals("Sapporo, Japan", vm.getOnlineProfile().getValue().location);
                    assertFalse(vm.getLinkedIdentities().getValue().isEmpty());

                    // Verify ClientConfigStore was synced
                    assertEquals("synchandle", configStore.preferences().getHandle());
                    assertEquals("Server bio for sync verification", configStore.preferences().getBio());
                    assertEquals("/api/v1/avatars/synced_avatar.png", configStore.preferences().getAvatarUrl());

                    // 2. setShowAccountDialog(true) triggers refreshMyProfileAsync
                    server.currentBio = "Updated Server Bio After Dialog Opened";
                    server.currentAvatarUrl = "/api/v1/avatars/synced_avatar_v2.png";

                    CountDownLatch dialogLatch = new CountDownLatch(1);
                    vm.setShowAccountDialog(true);
                    vm.refreshMyProfileAsync(ok -> {
                        assertTrue(ok);
                        dialogLatch.countDown();
                    });
                    assertTrue("refreshMyProfileAsync after showAccountDialog timed out", dialogLatch.await(5, TimeUnit.SECONDS));

                    assertEquals("Updated Server Bio After Dialog Opened", vm.getOnlineProfile().getValue().bio);
                    assertEquals("/api/v1/avatars/synced_avatar_v2.png", vm.getOnlineProfile().getValue().avatarUrl);
                    assertEquals("Updated Server Bio After Dialog Opened", configStore.preferences().getBio());
                }
            }
        }
    }

    @Test
    public void testLobbyChatLocalTimestampFormattingAndLeaveOnlineTableAsync() throws Exception {
        File cfgFile = new File(_tempDir, "client_config_lobby_leave.json");
        Files.write(cfgFile.toPath(), "{}".getBytes(StandardCharsets.UTF_8));

        // 1. Verify local-timezone [HH:mm] chat formatting
        long sampleEpochMs = 1775005200000L;
        String expectedHhMm = new java.text.SimpleDateFormat("HH:mm", java.util.Locale.getDefault()).format(
                new java.util.Date(sampleEpochMs));
        OctiNetworkClient.LobbyChatEntry entry = new OctiNetworkClient.LobbyChatEntry();
        entry.seq = 10L;
        entry.handle = "Alice";
        entry.text = "Ready for 2P table!";
        entry.atMs = sampleEpochMs;
        entry.createdAtMs = sampleEpochMs;

        String formatted = OctiAppViewModel.formatLobbyChatEntry(entry);
        assertEquals("[" + expectedHhMm + "] Alice: Ready for 2P table!", formatted);
        assertTrue(formatted.matches("^\\[\\d{2}:\\d{2}\\] Alice: Ready for 2P table!$"));

        // 2. Verify createOnlineTableAsync in WAITING_FOR_PLAYERS and leaveOnlineTableAsync clearing room state
        try (MockSsoHarnessServer server = new MockSsoHarnessServer()) {
            try (OctiNetworkClient client = OctiNetworkClient.forHermeticTest(server.port())) {
                ClientConfigStore configStore = new ClientConfigStore(cfgFile);
                configStore.preferences().setSessionToken("sess-valid-token");
                configStore.save();

                try (OctiAppViewModel vm = new OctiAppViewModel(configStore, client)) {
                    vm.setStorageRootDir(_tempDir);
                    vm.setAsyncBotThinkingEnabled(false);

                    CountDownLatch createLatch = new CountDownLatch(1);
                    OctiNetworkClient.CreateTableRequest req = new OctiNetworkClient.CreateTableRequest();
                    req.mode = "2P";
                    req.bases = 3;
                    req.timeControl = "10+5";
                    vm.createOnlineTableAsync(req, res -> {
                        assertTrue(res.ok);
                        createLatch.countDown();
                    });
                    assertTrue("createOnlineTableAsync timed out", createLatch.await(5, TimeUnit.SECONDS));
                    assertEquals("room-leave-test-456", vm.getActiveRoomId().getValue());
                    assertEquals(OctiAppViewModel.MatchType.ONLINE_ROOM, vm.getMatchType().getValue());
                    assertNotNull(vm.getActiveGameView().getValue());
                    assertEquals("WAITING_FOR_PLAYERS", vm.getActiveGameView().getValue().phase);

                    CountDownLatch leaveLatch = new CountDownLatch(1);
                    vm.leaveOnlineTableAsync("room-leave-test-456", ok -> {
                        assertTrue(ok);
                        leaveLatch.countDown();
                    });
                    assertTrue("leaveOnlineTableAsync timed out", leaveLatch.await(5, TimeUnit.SECONDS));
                    assertEquals("", vm.getActiveRoomId().getValue());
                    assertNull(vm.getActiveGameView().getValue());
                    assertNotEquals(OctiAppViewModel.MatchType.ONLINE_ROOM, vm.getMatchType().getValue());
                }
            }
        }
    }

    @Test
    public void testFinalWinningBotMovePreservesAiPlyMetadataInTranscriptAndUpload() throws Exception {
        File cfgFile = new File(_tempDir, "client_config_bot_win_meta.json");
        String json = "{\n"
                + "  \"telemetry_consent\": \"share\",\n"
                + "  \"telemetry_consent_version\": 1,\n"
                + "  \"install_id\": \"inst-bot-win-meta-999\"\n"
                + "}\n";
        Files.write(cfgFile.toPath(), json.getBytes(StandardCharsets.UTF_8));

        try (MockTelemetryServer telemetryServer = new MockTelemetryServer()) {
            try (OctiNetworkClient client = OctiNetworkClient.forHermeticTest(telemetryServer.port())) {
                ClientConfigStore configStore = new ClientConfigStore(cfgFile);

                try (OctiAppViewModel vm = new OctiAppViewModel(configStore, client)) {
                    vm.setStorageRootDir(_tempDir);
                    vm.setAsyncBotThinkingEnabled(false);

                    // Start a 2P 1-Base game where Human is Player A and Bot is Player B
                    vm.startNewLocalGame(
                            OctiGameMode.GAME_MODE_2,
                            false,
                            false,
                            OctiGameBases.GAME_BASES_1,
                            true,
                            OctiBotEngine.BotDifficulty.CASUAL,
                            OctiPlayer.PLAYER_A);
                    vm.confirmStartMatch();

                    // Advance board to a position where Player B (Bot) at (5,5) has S prong 'e'
                    // and Player A (Human) is at (5,4), leaving A's home base (5,3) empty.
                    OctiGame g = vm.getBoard().getGame();
                    g.move("53+a");
                    g.move("57+e");
                    g.move("53a-54");
                    g.move("57e-56");
                    g.move("33+a");
                    g.move("56e-55");

                    // Human (Player A) adds a prong at (7,3), allowing Bot (Player B) at (5,5)
                    // to jump over (5,4) onto A's base (5,3) for an immediate base-capture victory!
                    assertTrue(vm.applyMoveNotation("73+a"));
                    String botWinMove = vm.playBotTurnSynchronously();
                    assertFalse("Bot must execute a move", botWinMove.isEmpty());
                    assertTrue("Game must be over after bot's winning jump (" + botWinMove + ")", g.isGameOver());
                    assertEquals(OctiPlayer.PLAYER_B, g.getWinner());

                    // Inspect the final ply metadata in OctiGame
                    List<OctiGame.OctiPlyMetadata> allMetas = g.getAllPlyMetadata();
                    assertFalse(allMetas.isEmpty());
                    OctiGame.OctiPlyMetadata finalPlyMeta = allMetas.get(allMetas.size() - 1);
                    assertNotNull("Final bot ply must have OctiPlyMetadata attached", finalPlyMeta);
                    assertTrue("Final bot ply must have isAiMove = true", finalPlyMeta.isAiMove);
                    assertTrue("Final bot ply must record aiDifficulty > 0", finalPlyMeta.aiDifficulty > 0);
                    assertTrue("Final bot ply must record aiDepthReached >= 1", finalPlyMeta.aiDepthReached >= 1);
                    assertTrue("Final bot ply must record aiNodesVisited >= 1", finalPlyMeta.aiNodesVisited >= 1);

                    // Inspect buildCurrentGameTranscriptJson(true)
                    String transcriptJson = vm.buildCurrentGameTranscriptJson(true);
                    Map<String, Object> doc = OctiMiniJson.parseObject(transcriptJson);
                    List<Object> plies = OctiMiniJson.getArray(doc, "plies");
                    assertNotNull(plies);
                    assertFalse(plies.isEmpty());
                    @SuppressWarnings("unchecked")
                    Map<String, Object> lastPlyObj = (Map<String, Object>) plies.get(plies.size() - 1);
                    Map<String, Object> aiObj = OctiMiniJson.getObject(lastPlyObj, "ai");
                    assertNotNull("Final winning bot move in transcript must include 'ai' metadata object", aiObj);
                    assertTrue(OctiMiniJson.getInt(aiObj, "difficulty", 0) > 0);
                    assertTrue(OctiMiniJson.getInt(aiObj, "depthReached", 0) >= 1);
                    assertTrue(OctiMiniJson.getLong(aiObj, "nodesVisited", 0L) >= 1L);

                    // Also verify the uploaded payload received by MockTelemetryServer has 'ai' on the final ply
                    long deadline = System.currentTimeMillis() + 3000L;
                    while (telemetryServer.receivedUploadBodies().isEmpty() && System.currentTimeMillis() < deadline) {
                        Thread.sleep(50L);
                    }
                    List<String> uploads = telemetryServer.receivedUploadBodies();
                    assertFalse("Completed bot game must be uploaded", uploads.isEmpty());
                    Map<String, Object> uploadEnvelope = OctiMiniJson.parseObject(uploads.get(uploads.size() - 1));
                    Map<String, Object> uploadedDoc = OctiMiniJson.getObject(uploadEnvelope, "transcript");
                    if (uploadedDoc == null) {
                        String innerTranscriptStr = OctiMiniJson.getString(uploadEnvelope, "transcript", "");
                        uploadedDoc = innerTranscriptStr.isEmpty()
                                ? uploadEnvelope
                                : OctiMiniJson.parseObject(innerTranscriptStr);
                    }
                    List<Object> uploadedPlies = OctiMiniJson.getArray(uploadedDoc, "plies");
                    assertNotNull(uploadedPlies);
                    assertFalse(uploadedPlies.isEmpty());
                    @SuppressWarnings("unchecked")
                    Map<String, Object> uploadedLastPly = (Map<String, Object>) uploadedPlies.get(uploadedPlies.size() - 1);
                    Map<String, Object> uploadedAiObj = OctiMiniJson.getObject(uploadedLastPly, "ai");
                    assertNotNull("Uploaded transcript final winning bot move must include 'ai' metadata", uploadedAiObj);
                    assertTrue(OctiMiniJson.getInt(uploadedAiObj, "difficulty", 0) > 0);
                    assertTrue(OctiMiniJson.getInt(uploadedAiObj, "depthReached", 0) >= 1);
                    assertTrue(OctiMiniJson.getLong(uploadedAiObj, "nodesVisited", 0L) >= 1L);
                }
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
