/*
 * OctiCrashReporter.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.io.File;
import java.io.FileOutputStream;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;

public final class OctiCrashReporter {

    private static volatile File s_storageRootDir = null;
    private static volatile String s_appVersion = "2.1.2";
    private static volatile String s_installId = "";
    private static final AtomicBoolean s_installed = new AtomicBoolean(false);
    private static volatile Thread.UncaughtExceptionHandler s_previousHandler = null;

    private static final Object s_contextLock = new Object();
    private static String s_gameMatchMode = "LOCAL_PASS_AND_PLAY";
    private static int s_gamePlyCount = 0;
    private static String s_gameFen = "";

    private OctiCrashReporter() {
    }

    public static void setStorageRootDir(File dir) {
        s_storageRootDir = dir;
    }

    public static File getStorageRootDir() {
        File r = s_storageRootDir;
        return r != null ? r : new File(".");
    }

    public static File getCrashesDir(File rootDir) {
        File root = rootDir != null ? rootDir : getStorageRootDir();
        File d = new File(root, "crashes");
        if (!d.exists()) {
            d.mkdirs();
        }
        return d;
    }

    public static File getPendingUploadsDir(File rootDir) {
        File root = rootDir != null ? rootDir : getStorageRootDir();
        File d = new File(root, "pending_uploads");
        if (!d.exists()) {
            d.mkdirs();
        }
        return d;
    }

    public static void updateGameContext(String matchMode, int plyCount, String fen) {
        synchronized (s_contextLock) {
            if (matchMode != null) {
                s_gameMatchMode = matchMode;
            }
            s_gamePlyCount = Math.max(0, plyCount);
            if (fen != null) {
                s_gameFen = fen;
            }
        }
    }

    public static String getGameContextJson() {
        synchronized (s_contextLock) {
            Map<String, Object> map = new LinkedHashMap<>();
            map.put("match_mode", s_gameMatchMode != null ? s_gameMatchMode : "");
            map.put("ply_count", s_gamePlyCount);
            map.put("fen", s_gameFen != null ? s_gameFen : "");
            return OctiMiniJson.stringify(map);
        }
    }

    public static void pruneDirectory(File dir, int maxFiles) {
        if (dir == null || !dir.exists() || !dir.isDirectory()) {
            return;
        }
        File[] files = dir.listFiles((d, name) -> name.endsWith(".json"));
        if (files == null || files.length <= maxFiles) {
            return;
        }
        Arrays.sort(files, Comparator.comparingLong(File::lastModified));
        int toDelete = files.length - maxFiles;
        for (int i = 0; i < toDelete; i++) {
            try {
                files[i].delete();
            } catch (Exception ignored) {
            }
        }
    }

    public static String sanitizeFilenamePart(String in) {
        if (in == null || in.isEmpty()) {
            return "game";
        }
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < in.length(); i++) {
            char c = in.charAt(i);
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '-' || c == '_') {
                sb.append(c);
            } else {
                sb.append('_');
            }
        }
        return sb.length() == 0 ? "game" : sb.toString();
    }

    public static String computeSha256Hex(String text) {
        try {
            MessageDigest md = MessageDigest.getInstance("SHA-256");
            byte[] digest = md.digest(text.getBytes(StandardCharsets.UTF_8));
            StringBuilder sb = new StringBuilder(64);
            for (byte b : digest) {
                sb.append(String.format("%02x", b & 0xFF));
            }
            return sb.toString();
        } catch (Exception e) {
            return Long.toHexString(System.currentTimeMillis()) + Long.toHexString(System.nanoTime());
        }
    }

    public static String formatStackTrace(Throwable throwable) {
        if (throwable == null) {
            return "";
        }
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        throwable.printStackTrace(pw);
        pw.flush();
        return sw.toString();
    }

    public static String currentOsVersion() {
        try {
            Class<?> buildClass = Class.forName("android.os.Build");
            Class<?> versionClass = Class.forName("android.os.Build$VERSION");
            Object release = versionClass.getField("RELEASE").get(null);
            Object sdkInt = versionClass.getField("SDK_INT").get(null);
            return "Android " + release + " (API " + sdkInt + ")";
        } catch (Throwable ignored) {
            return "Android " + System.getProperty("os.name", "Linux") + " " + System.getProperty("os.version", "");
        }
    }

    public static void installGlobalHandler(File storageRootDir, String appVersion, String installId) {
        if (storageRootDir != null) {
            setStorageRootDir(storageRootDir);
        }
        if (appVersion != null && !appVersion.isEmpty()) {
            s_appVersion = appVersion;
        }
        if (installId != null) {
            s_installId = installId;
        }
        if (s_installed.compareAndSet(false, true)) {
            s_previousHandler = Thread.getDefaultUncaughtExceptionHandler();
            Thread.setDefaultUncaughtExceptionHandler((thread, throwable) -> {
                try {
                    recordHandledException(
                        getStorageRootDir(),
                        thread != null ? thread.getName() : "main",
                        throwable,
                        s_appVersion,
                        s_installId
                    );
                } catch (Throwable ignored) {
                }
                if (s_previousHandler != null) {
                    s_previousHandler.uncaughtException(thread, throwable);
                }
            });
        }
    }

    public static void uninstallGlobalHandler() {
        if (s_installed.compareAndSet(true, false)) {
            if (s_previousHandler != null) {
                Thread.setDefaultUncaughtExceptionHandler(s_previousHandler);
                s_previousHandler = null;
            }
        }
    }

    public static File recordHandledException(String threadName, Throwable throwable, String installId) {
        return recordHandledException(getStorageRootDir(), threadName, throwable, s_appVersion, installId);
    }

    public static File recordHandledException(
            File rootDir,
            String threadName,
            Throwable throwable,
            String appVersion,
            String installId) {
        File crashesDir = getCrashesDir(rootDir);
        long epochSec = System.currentTimeMillis() / 1000L;
        long epochMs = System.currentTimeMillis();
        String version = (appVersion != null && !appVersion.isEmpty()) ? appVersion : s_appVersion;
        String instId = (installId != null) ? installId : s_installId;
        String exType = throwable != null ? throwable.getClass().getName() : "UnknownException";
        String exMsg = (throwable != null && throwable.getMessage() != null) ? throwable.getMessage() : "";
        String thName = (threadName != null && !threadName.isEmpty()) ? threadName : "main";
        String stackTrace = formatStackTrace(throwable);
        String platform = "android";

        String seed = platform + "|" + version + "|" + exType + "|" + exMsg + "|" + stackTrace + "|" + epochSec;
        String crashId = computeSha256Hex(seed);
        String hexSuffix = crashId.length() >= 8 ? crashId.substring(0, 8) : Long.toHexString(epochMs);

        File crashFile = new File(crashesDir, "crash_" + epochMs + "_" + hexSuffix + ".json");

        Map<String, Object> root = new LinkedHashMap<>();
        root.put("crash_id", crashId);
        root.put("created_at", epochSec);
        root.put("platform", platform);
        root.put("app_version", version);
        root.put("os_version", currentOsVersion());
        root.put("exception_type", exType);
        root.put("exception_message", exMsg);
        root.put("thread_name", thName);
        root.put("stack_trace", stackTrace);

        String ctxJson = getGameContextJson();
        try {
            Object parsed = OctiMiniJson.parse(ctxJson);
            root.put("context", parsed != null ? parsed : ctxJson);
        } catch (Exception ignored) {
            root.put("context", ctxJson);
        }
        if (instId != null && !instId.isEmpty()) {
            root.put("install_id", instId);
        }

        String jsonString = OctiMiniJson.stringify(root);
        writeAtomicFile(crashFile, jsonString);
        pruneDirectory(crashesDir, 32);

        return crashFile;
    }

    public static File spoolTrainingTranscript(
            File rootDir,
            String gameId,
            String transcriptJson,
            String clientId) {
        File pendingDir = getPendingUploadsDir(rootDir);
        long epochMs = System.currentTimeMillis();
        String safeId = (gameId != null && !gameId.trim().isEmpty())
                ? sanitizeFilenamePart(gameId)
                : ("game_" + epochMs);
        File spoolFile = new File(pendingDir, "transcript_" + safeId + ".json");
        if (spoolFile.exists()) {
            spoolFile = new File(pendingDir, "transcript_" + safeId + "_" + epochMs + ".json");
        }

        Map<String, Object> root = new LinkedHashMap<>();
        root.put("game_id", gameId != null ? gameId : "");
        root.put("created_at", epochMs / 1000L);
        if (transcriptJson != null && transcriptJson.trim().startsWith("{")) {
            try {
                Object parsed = OctiMiniJson.parse(transcriptJson);
                root.put("transcript", parsed != null ? parsed : transcriptJson);
            } catch (Exception ignored) {
                root.put("transcript", transcriptJson);
            }
        } else {
            root.put("transcript", transcriptJson != null ? transcriptJson : "");
        }
        root.put("transcript_json", transcriptJson != null ? transcriptJson : "");
        if (clientId != null && !clientId.isEmpty()) {
            root.put("client_id", clientId);
        }

        String jsonString = OctiMiniJson.stringify(root);
        writeAtomicFile(spoolFile, jsonString);
        pruneDirectory(pendingDir, 32);

        return spoolFile;
    }

    public static List<File> listPendingCrashFiles(File rootDir) {
        File dir = getCrashesDir(rootDir);
        File[] files = dir.listFiles((d, name) -> name.endsWith(".json"));
        if (files == null || files.length == 0) {
            return Collections.emptyList();
        }
        List<File> list = new ArrayList<>(Arrays.asList(files));
        list.sort(Comparator.comparing(File::getName));
        return list;
    }

    public static List<File> listPendingTranscriptFiles(File rootDir) {
        File dir = getPendingUploadsDir(rootDir);
        File[] files = dir.listFiles((d, name) -> name.endsWith(".json"));
        if (files == null || files.length == 0) {
            return Collections.emptyList();
        }
        List<File> list = new ArrayList<>(Arrays.asList(files));
        list.sort(Comparator.comparing(File::getName));
        return list;
    }

    public static int flushPendingCrashReports(
            File rootDir,
            OctiNetworkClient client,
            String fallbackInstallId) {
        if (client == null) {
            return 0;
        }
        List<File> files = listPendingCrashFiles(rootDir);
        int flushedCount = 0;
        for (File file : files) {
            try {
                long length = file.length();
                if (length == 0 || length > 65536) {
                    file.delete();
                    continue;
                }
                byte[] bytes = Files.readAllBytes(file.toPath());
                String content = new String(bytes, StandardCharsets.UTF_8);
                Object parsed = OctiMiniJson.parse(content);
                if (!(parsed instanceof Map)) {
                    file.delete();
                    continue;
                }
                @SuppressWarnings("unchecked")
                Map<String, Object> map = (Map<String, Object>) parsed;

                OctiNetworkClient.CrashReportRequest req = new OctiNetworkClient.CrashReportRequest();
                Object cid = map.get("crash_id");
                if (cid != null) req.crashId = cid.toString();

                Object ca = map.get("created_at");
                if (ca instanceof Number) req.createdAt = ((Number) ca).longValue();

                Object plat = map.get("platform");
                if (plat != null) req.platform = plat.toString();

                Object ver = map.get("app_version");
                if (ver != null) req.appVersion = ver.toString();

                Object os = map.get("os_version");
                if (os != null) req.osVersion = os.toString();

                Object et = map.get("exception_type");
                if (et != null) req.exceptionType = et.toString();

                Object em = map.get("exception_message");
                if (em != null) req.exceptionMessage = em.toString();

                Object tn = map.get("thread_name");
                if (tn != null) req.threadName = tn.toString();

                Object st = map.get("stack_trace");
                if (st != null) req.stackTrace = st.toString();

                Object ctx = map.get("context");
                if (ctx instanceof Map || ctx instanceof List) {
                    req.contextJson = OctiMiniJson.stringify(ctx);
                } else if (ctx != null) {
                    req.contextJson = ctx.toString();
                } else {
                    req.contextJson = "{}";
                }

                Object iid = map.get("install_id");
                if (iid != null && !iid.toString().isEmpty()) {
                    req.installId = iid.toString();
                } else if (fallbackInstallId != null && !fallbackInstallId.isEmpty()) {
                    req.installId = fallbackInstallId;
                }

                OctiNetworkClient.CrashReportResponse res = client.uploadCrashReport(req);
                if (res.ok || res.httpStatus == 400 || res.httpStatus == 413) {
                    file.delete();
                    if (res.ok) {
                        flushedCount++;
                    }
                } else if (res.httpStatus <= 0 || res.httpStatus >= 500) {
                    // Offline or server 5xx: retain on disk for retry, stop iteration
                    break;
                } else {
                    file.delete();
                }
            } catch (Exception e) {
            }
        }
        return flushedCount;
    }

    public static int flushPendingTranscripts(
            File rootDir,
            OctiNetworkClient client,
            String fallbackClientId) {
        if (client == null) {
            return 0;
        }
        List<File> files = listPendingTranscriptFiles(rootDir);
        int flushedCount = 0;
        for (File file : files) {
            try {
                long length = file.length();
                if (length == 0 || length > 131072) {
                    file.delete();
                    continue;
                }
                byte[] bytes = Files.readAllBytes(file.toPath());
                String content = new String(bytes, StandardCharsets.UTF_8);
                Object parsed = OctiMiniJson.parse(content);
                if (!(parsed instanceof Map)) {
                    file.delete();
                    continue;
                }
                @SuppressWarnings("unchecked")
                Map<String, Object> map = (Map<String, Object>) parsed;

                String transcriptJson = "";
                Object tj = map.get("transcript_json");
                if (tj != null && !tj.toString().isEmpty()) {
                    transcriptJson = tj.toString();
                } else {
                    Object t = map.get("transcript");
                    if (t instanceof Map || t instanceof List) {
                        transcriptJson = OctiMiniJson.stringify(t);
                    } else if (t != null) {
                        transcriptJson = t.toString();
                    } else {
                        transcriptJson = content;
                    }
                }

                String clientId = fallbackClientId != null ? fallbackClientId : "";
                Object cid = map.get("client_id");
                if (cid == null) cid = map.get("clientId");
                if (cid != null && !cid.toString().isEmpty()) {
                    clientId = cid.toString();
                }

                OctiNetworkClient.ApiResult<OctiNetworkClient.UploadTranscriptResponse> res =
                        client.uploadTrainingTranscript(transcriptJson, clientId);

                if (res.ok || res.httpStatus == 400) {
                    file.delete();
                    if (res.ok) {
                        flushedCount++;
                    }
                } else if (res.httpStatus <= 0 || res.httpStatus >= 500) {
                    // Offline or server 5xx: retain on disk for retry, stop iteration
                    break;
                } else {
                    file.delete();
                }
            } catch (Exception e) {
            }
        }
        return flushedCount;
    }

    private static void writeAtomicFile(File targetFile, String content) {
        File parent = targetFile.getParentFile();
        if (parent != null && !parent.exists()) {
            parent.mkdirs();
        }
        File tmpFile = new File(parent, targetFile.getName() + ".tmp");
        try {
            try (FileOutputStream fos = new FileOutputStream(tmpFile)) {
                fos.write(content.getBytes(StandardCharsets.UTF_8));
                fos.flush();
            }
            if (!tmpFile.renameTo(targetFile)) {
                Files.move(tmpFile.toPath(), targetFile.toPath(), StandardCopyOption.REPLACE_EXISTING);
            }
        } catch (Exception e) {
            try {
                if (tmpFile.exists()) {
                    tmpFile.delete();
                }
            } catch (Exception ignored) {
            }
        }
    }
}
