/*
 * OctiAndroidAppIntegrationTest.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import android.view.KeyEvent;

import com.selfso.octi.ui.OctiAppViewModel;
import com.selfso.selfupdate.UpdateManifest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.HashMap;
import java.util.Map;
import java.util.Random;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
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
        assertEquals("trumpet.selfso.com:6874", OctiAppViewModel.canonicalServerEndpoint());
        assertEquals("trumpet.selfso.com", OctiAppViewModel.canonicalServerHost());
        assertEquals(6874, OctiAppViewModel.canonicalServerPort());
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
            assertEquals("trumpet.selfso.com", vm.getNetClient().serverHost());
            assertEquals(6874, vm.getNetClient().serverPort());
            assertEquals("trumpet.selfso.com:6874", OctiAppViewModel.canonicalServerEndpoint());
            assertEquals(OctiThemeId.ROSEWOOD_IVORY, vm.getTheme().getValue());

            Map<String, String> rogueExtras = new HashMap<>();
            rogueExtras.put("server_url", "http://127.0.0.1:1111");
            rogueExtras.put("server_host", "127.0.0.1");
            rogueExtras.put("server_port", "1111");
            rogueExtras.put("tab", "LOBBY");
            vm.applyExternalArguments(rogueExtras);

            assertEquals("trumpet.selfso.com", vm.getNetClient().serverHost());
            assertEquals(6874, vm.getNetClient().serverPort());
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

            // 4. Export .octi transcript and reload into VCR replay mode
            String transcript = vm.exportCurrentTranscript();
            assertNotNull(transcript);
            assertTrue(transcript.contains("GAME_MODE"));

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
            assertEquals("GrandmasterVega", vm.getLinkConflictProfile().getValue().handle);

            // 4. Confirm switching to the conflict account
            OctiNetworkClient.ApiResult<OctiNetworkClient.SsoLoginData> switched =
                    vm.confirmSwitchToConflictAccount();
            assertTrue(switched.ok);
            assertNull(vm.getLinkConflictProfile().getValue());
            assertEquals("p-microsoft", vm.getCurrentProfile().getValue().uuid);
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
