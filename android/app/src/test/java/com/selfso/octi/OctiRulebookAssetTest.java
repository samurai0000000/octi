/*
 * OctiRulebookAssetTest.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import com.selfso.octi.ui.OctiRulebookAssets;

import org.junit.Test;

import java.io.File;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.List;

public class OctiRulebookAssetTest {

    private static final byte[] PNG_SIG = new byte[] {
        (byte) 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
    };

    private File resolveRepoRulebookDir() {
        File cwd = new File(System.getProperty("user.dir", ".")).getAbsoluteFile();
        File candidate1 = new File(cwd, "../../assets/rulebook");
        if (candidate1.isDirectory()) {
            return candidate1;
        }
        File candidate2 = new File(cwd, "../assets/rulebook");
        if (candidate2.isDirectory()) {
            return candidate2;
        }
        return new File(cwd, "assets/rulebook");
    }

    private File resolveAppMainDir() {
        File cwd = new File(System.getProperty("user.dir", ".")).getAbsoluteFile();
        if (new File(cwd, "src/main").isDirectory()) {
            return new File(cwd, "src/main");
        }
        return new File(cwd, "app/src/main");
    }

    @Test
    public void testZeroDuplicateRulebookAssetsInAndroidModule() {
        File mainDir = resolveAppMainDir();
        File dupAssetsRulebook = new File(mainDir, "assets/rulebook");
        File dupResRulebook = new File(mainDir, "resources/rulebook");
        assertFalse("Rulebook assets must not be duplicated under app/src/main/assets/rulebook",
            dupAssetsRulebook.exists());
        assertFalse("Rulebook assets must not be duplicated under app/src/main/resources/rulebook",
            dupResRulebook.exists());
    }

    @Test
    public void testClasspathRulebookPngsAndMobileHtmlParity() throws Exception {
        File repoDir = resolveRepoRulebookDir();
        assertTrue("Canonical repo assets/rulebook dir must exist", repoDir.isDirectory());

        List<String> files = OctiRulebookAssets.IMAGE_FILES;
        assertEquals(12, files.size());

        long totalBytes = 0L;
        for (String fileName : files) {
            byte[] cpBytes = OctiRulebookAssets.loadRulebookImageBytes(fileName);
            assertNotNull("Missing classpath PNG: " + fileName, cpBytes);
            assertTrue("PNG too small: " + fileName, cpBytes.length >= 1024);
            assertTrue("PNG exceeds 80 KB budget: " + fileName, cpBytes.length <= 80 * 1024);
            assertArrayEquals(PNG_SIG, Arrays.copyOfRange(cpBytes, 0, 8));
            totalBytes += cpBytes.length;

            File diskFile = new File(repoDir, fileName);
            assertTrue("Missing canonical disk PNG: " + diskFile, diskFile.isFile());
            byte[] diskBytes = Files.readAllBytes(diskFile.toPath());
            assertArrayEquals("Classpath vs disk byte mismatch for " + fileName, diskBytes, cpBytes);
        }
        assertTrue("Total PNG size must be < 1 MB", totalBytes < 1024L * 1024L);

        assertNull(OctiRulebookAssets.loadRulebookImageBytes("nonexistent.png"));
        assertNull(OctiRulebookAssets.loadRulebookImageBytes("../sounds/pod_place.wav"));

        String html = OctiRulebookAssets.loadRulebookHtml();
        assertNotNull(html);
        assertTrue(html.length() > 4000);
        for (int ch = 1; ch <= 7; ch++) {
            assertTrue("Missing Chapter " + ch, html.contains("Chapter " + ch));
        }
        for (String fileName : files) {
            assertTrue("Missing illustration in mobile HTML: " + fileName,
                html.contains("https://octi.local/assets/rulebook/" + fileName));
        }
        String[] variants = {
            "2P_3B", "2P_3B_SP", "2P_3B_ED", "2P_3B_SPED",
            "2P_1B", "2P_1B_SP", "2P_1B_ED", "2P_1B_SPED",
            "4P", "4P_SP", "4P_ED", "4P_SPED", "OFK"
        };
        for (String v : variants) {
            assertTrue("Missing variant code in mobile HTML: " + v, html.contains(v));
        }
        assertFalse("External http:// forbidden in offline rulebook HTML", html.contains("http://"));
    }

    @Test
    public void testUrlInterceptorStrictAllowlistAnd403Block() {
        for (String fileName : OctiRulebookAssets.IMAGE_FILES) {
            OctiRulebookAssets.InterceptDecision ok = OctiRulebookAssets.resolveInterceptedUrl(
                "https://octi.local/assets/rulebook/" + fileName);
            assertEquals(200, ok.getStatusCode());
            assertEquals("image/png", ok.getMimeType());
            assertTrue(ok.getBytes().length >= 1024);
        }

        String[] forbiddenUrls = {
            "https://evil.example/assets/rulebook/rule_01_2p_3b_setup.png",
            "http://octi.local/assets/rulebook/rule_01_2p_3b_setup.png",
            "file:///etc/passwd",
            "content://media/external/images/media/1",
            "https://octi.local/assets/rulebook/../sounds/pod_place.wav",
            "https://octi.local/assets/rulebook/..%2fsounds%2fpod_place.wav",
            "https://octi.local/assets/rulebook/nonexistent.png",
            "https://octi.local/assets/rulebook/rule_01_2p_3b_setup.png?leak=1",
            ""
        };
        for (String badUrl : forbiddenUrls) {
            OctiRulebookAssets.InterceptDecision blocked =
                OctiRulebookAssets.resolveInterceptedUrl(badUrl);
            assertEquals("Expected 403 for " + badUrl, 403, blocked.getStatusCode());
            assertEquals(0, blocked.getBytes().length);
        }
    }
}
