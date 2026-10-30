/*
 * WebUi.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "WebUi.hxx"

namespace octi {

namespace {

const char kPage[] = R"OCTIUI(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>OCTI Archive</title>
<style>
:root { --bg:#14181f; --panel:#1d232d; --line:#303949; --fg:#e6eaf0;
        --dim:#97a3b6; --acc:#4da3ff; --a:#e5604d; --b:#4da3ff;
        --c:#e8c24a; --d:#58c27d; }
* { box-sizing: border-box; }
body { margin:0; background:var(--bg); color:var(--fg);
       font:15px/1.45 system-ui, -apple-system, "Segoe UI", sans-serif; }
header { display:flex; flex-wrap:wrap; align-items:center; gap:12px;
         padding:10px 16px; background:var(--panel);
         border-bottom:1px solid var(--line); }
header h1 { margin:0; font-size:18px; letter-spacing:.5px; }
nav { display:flex; gap:6px; flex-wrap:wrap; }
nav button, .btn { background:#27303d; color:var(--fg);
       border:1px solid var(--line); border-radius:6px; padding:6px 12px;
       cursor:pointer; font:inherit; text-decoration:none; }
nav button.on { background:var(--acc); color:#06121f; border-color:var(--acc); }
main { max-width:1100px; margin:0 auto; padding:16px; }
section { display:none; }
section.on { display:block; }
.panel { background:var(--panel); border:1px solid var(--line);
         border-radius:8px; padding:12px; margin-bottom:14px; }
.row { display:flex; flex-wrap:wrap; gap:8px; align-items:center;
       margin-bottom:10px; }
select, input { background:#10141a; color:var(--fg);
       border:1px solid var(--line); border-radius:6px; padding:6px 8px;
       font:inherit; }
table { width:100%; border-collapse:collapse; }
th, td { text-align:left; padding:6px 8px; border-bottom:1px solid var(--line);
         vertical-align:top; }
th { color:var(--dim); font-weight:600; font-size:13px; }
tr.click { cursor:pointer; }
tr.click:hover { background:#222a36; }
.dim { color:var(--dim); }
.err { color:#ff8a80; }
pre { background:#10141a; border:1px solid var(--line); border-radius:6px;
      padding:10px; overflow:auto; max-height:320px; margin:8px 0; }
.cols { display:flex; flex-wrap:wrap; gap:14px; }
.cols > div { flex:1 1 320px; min-width:0; }
.board { display:grid; gap:2px; width:100%; max-width:560px; }
.cell { aspect-ratio:1/1; background:#10141a; border:1px solid #232b37;
        display:flex; flex-direction:column; align-items:center;
        justify-content:center; font-size:11px; overflow:hidden;
        text-align:center; line-height:1.1; }
.cell.A { color:var(--a); } .cell.B { color:var(--b); }
.cell.C { color:var(--c); } .cell.D { color:var(--d); }
.cell span { display:block; }
.log { max-height:260px; overflow:auto; font-size:13px; }
.log div { border-bottom:1px solid #232b37; padding:2px 0; }
.badge { font-size:11px; padding:2px 6px; border-radius:4px; font-weight:600; text-transform:uppercase; display:inline-block; }
.badge.online { background:#223c2d; color:#7ce39f; border:1px solid #2e593e; }
.badge.client_upload { background:#31374a; color:#a3b7e8; border:1px solid #434b66; }
.replay-table-wrap { max-height:260px; overflow-y:auto; border:1px solid var(--line); border-radius:6px; margin:8px 0; }
.ply-row.active { background:#2c3747; font-weight:bold; }
.license-box { margin-top:14px; font-size:12px; color:var(--dim); border-top:1px solid var(--line); padding-top:8px; }
.license-box a { color:var(--acc); text-decoration:none; }
@media (max-width:600px) { main { padding:8px; } th, td { padding:4px; } }
</style>
</head>
<body>
<header>
  <h1>OCTI</h1>
  <nav id="nav">
    <button data-tab="live" class="on">Live</button>
    <button data-tab="archive">Archive</button>
    <button data-tab="board">Leaderboard</button>
  </nav>
  <span id="health" class="dim"></span>
</header>
<main>
<section id="tab-live" class="on">
  <div class="panel">
    <h2>Open and live tables</h2>
    <div id="tables-msg" class="dim">Loading...</div>
    <table id="tables"><thead><tr><th>Table</th><th>Mode</th><th>State</th>
      <th>Players</th><th>Watching</th><th></th></tr></thead>
      <tbody></tbody></table>
  </div>
  <div class="panel" id="watch" style="display:none">
    <div class="row"><strong id="watch-title"></strong>
      <button class="btn" id="watch-close">Stop watching</button></div>
    <div class="cols">
      <div><div id="live-board" class="board"></div>
        <div id="live-info" class="dim"></div></div>
      <div><h3>Events</h3><div id="live-log" class="log"></div>
        <h3>Moves</h3><pre id="live-moves"></pre></div>
    </div>
  </div>
</section>
<section id="tab-archive">
  <div class="panel" id="ar-stats-panel">
    <h3>Archive Statistics</h3>
    <div id="ar-stats-summary" class="dim">Loading stats...</div>
    <div id="ar-stats-ai" class="dim" style="margin-top:4px;"></div>
  </div>
  <div class="panel">
    <div class="row">
      <label>Mode <select id="ar-mode"><option value="">Any</option>
        <option>2P</option><option>4P</option><option>OFK</option></select>
      </label>
      <label>Source <select id="ar-source"><option value="">Any</option>
        <option value="online">Online</option><option value="client_upload">Client Upload</option></select>
      </label>
      <label>Status <select id="ar-status"><option value="">Any</option>
        <option value="GAME_OVER">Game Over</option><option value="RESIGNED">Resigned</option></select>
      </label>
      <label>Winner <select id="ar-winner"><option value="">Any</option>
        <option value="A">Player A</option><option value="B">Player B</option>
        <option value="C">Player C</option><option value="D">Player D</option>
        <option value="NONE">None</option></select>
      </label>
      <label>AI <select id="ar-ai"><option value="">Any</option>
        <option value="1">L1</option><option value="2">L2</option><option value="3">L3</option>
        <option value="4">L4</option><option value="5">L5</option></select>
      </label>
      <label>Sort <select id="ar-sort">
        <option value="ended_at:desc">Ended (Newest)</option>
        <option value="ended_at:asc">Ended (Oldest)</option>
        <option value="uploaded_at:desc">Uploaded (Newest)</option>
        <option value="uploaded_at:asc">Uploaded (Oldest)</option>
        <option value="ply_count:desc">Plies (Most)</option>
        <option value="ply_count:asc">Plies (Least)</option></select>
      </label>
      <label>Player id <input id="ar-player" size="28"
        placeholder="uuid (optional)"></label>
      <button class="btn" id="ar-go">Filter</button>
      <a class="btn" id="ar-export-btn" href="/api/v1/archive/export" target="_blank">Export Filtered (NDJSON)</a>
    </div>
    <div id="ar-msg" class="dim"></div>
    <table id="ar-table"><thead><tr><th>Game</th><th>Mode</th><th>Source</th>
      <th>Players</th><th>Winner</th><th>Result</th><th>Plies</th><th>Date</th></tr></thead>
      <tbody></tbody></table>
    <div class="row" style="margin-top:10px">
      <button class="btn" id="ar-prev">Previous</button>
      <button class="btn" id="ar-next">Next</button>
      <span id="ar-page" class="dim"></span>
    </div>
  </div>
  <div class="panel" id="ar-detail" style="display:none">
    <div class="row"><strong id="ar-title"></strong>
      <button class="btn" id="ar-copy-json">Copy JSON</button>
      <a class="btn" id="ar-dl" href="#">Download .json</a>
      <button class="btn" id="ar-close">Close</button>
    </div>
    <div class="cols">
      <div>
        <div id="ar-board" class="board"></div>
        <div class="row" style="margin-top:8px; justify-content:center;">
          <button class="btn" id="rp-start">&lt;&lt;</button>
          <button class="btn" id="rp-prev">&lt; Prev</button>
          <button class="btn" id="rp-play">Play</button>
          <button class="btn" id="rp-next">Next &gt;</button>
          <button class="btn" id="rp-end">&gt;&gt;</button>
          <span id="rp-ply-label" class="dim" style="margin-left:8px;"></span>
        </div>
        <div id="ar-info" class="dim" style="margin-top:6px;"></div>
      </div>
      <div>
        <h3>Move Replay</h3>
        <div class="replay-table-wrap">
          <table id="rp-table"><thead><tr><th>#</th><th>Ply</th><th>Move</th><th>Time</th><th>AI Search</th></tr></thead>
          <tbody></tbody></table>
        </div>
        <h3>Transcript JSON</h3><pre id="ar-transcript"></pre>
      </div>
    </div>
    <div class="license-box">
      Archived game data is published under Creative Commons Attribution 4.0 International (CC-BY 4.0). Attribution: <strong>Octi game data, Charles Chiou (selfso.com)</strong>.
    </div>
  </div>
</section>
<section id="tab-board">
  <div class="panel">
    <div class="row">
      <label>Mode <select id="lb-mode"><option>2P</option><option>4P</option>
        <option>OFK</option></select></label>
      <button class="btn" id="lb-go">Refresh</button>
    </div>
    <div id="lb-msg" class="dim"></div>
    <table id="lb-table"><thead><tr><th>#</th><th>Player</th><th>Elo</th>
      <th>Games</th><th>W</th><th>L</th><th>D</th></tr></thead>
      <tbody></tbody></table>
  </div>
</section>
</main>
<script>
(function () {
"use strict";

function $(id) { return document.getElementById(id); }

function el(tag, text, cls) {
  var e = document.createElement(tag);
  if (text !== undefined && text !== null) { e.textContent = String(text); }
  if (cls) { e.className = cls; }
  return e;
}

function clear(node) {
  while (node.firstChild) { node.removeChild(node.firstChild); }
}

function api(path) {
  return fetch(path, { headers: { "Accept": "application/json" } })
    .then(function (r) {
      return r.json().catch(function () { return {}; }).then(function (j) {
        if (!r.ok) {
          throw new Error((j && j.error ? j.error : "HTTP " + r.status));
        }
        return j;
      });
    });
}

function fmtTime(sec) {
  if (!sec) { return ""; }
  return new Date(sec * 1000).toISOString().replace("T", " ").slice(0, 16);
}

function cellRow(tr, values) {
  values.forEach(function (v) { tr.appendChild(el("td", v)); });
}

/* ---- board rendering from an OCTI-FEN position string ---- */
function renderBoard(container, fen) {
  clear(container);
  if (!fen) { return; }
  var f = fen.split(":");
  var mode = f[0];
  var squares = f.length > 0 ? f[f.length - 1] : "";
  var maxX = 9, maxY = 9;
  if (mode === "OFK") { maxX = 6; maxY = 7; }
  var map = {};
  if (squares && squares !== "-") {
    squares.split(",").forEach(function (tok) {
      var m = /^([A-D])([1-9])([1-9])([a-h!]*)$/.exec(tok);
      if (!m) { return; }
      var key = m[2] + m[3];
      if (!map[key]) { map[key] = []; }
      map[key].push({ owner: m[1], prongs: m[4] });
    });
  }
  container.style.gridTemplateColumns = "repeat(" + maxX + ", 1fr)";
  for (var y = maxY; y >= 1; y--) {
    for (var x = 1; x <= maxX; x++) {
      var stack = map[String(x) + String(y)] || [];
      var cls = "cell" + (stack.length ? " " + stack[stack.length - 1].owner : "");
      var cell = el("div", null, cls);
      cell.title = String(x) + String(y);
      stack.forEach(function (p) {
        cell.appendChild(el("span", p.owner + (p.prongs ? " " + p.prongs : "")));
      });
      container.appendChild(cell);
    }
  }
}

/* ---- tabs ---- */
var tabs = { live: $("tab-live"), archive: $("tab-archive"),
             board: $("tab-board") };

function switchTab(name) {
  Array.prototype.forEach.call($("nav").querySelectorAll("button"),
    function (o) { o.classList.toggle("on", o.getAttribute("data-tab") === name); });
  Object.keys(tabs).forEach(function (k) {
    tabs[k].classList.toggle("on", k === name);
  });
  if (name === "archive") {
    if (!arLoaded) { arSearch(0); }
    loadArchiveStats();
  }
  if (name === "board") { lbLoad(); }
}

Array.prototype.forEach.call($("nav").querySelectorAll("button"), function (b) {
  b.addEventListener("click", function () {
    switchTab(b.getAttribute("data-tab"));
  });
});

/* ---- health ---- */
function loadHealth() {
  api("/api/v1/health").then(function (h) {
    $("health").textContent = "v" + h.version + " | " + h.active_tables +
      " tables | " + h.online_players + " online";
  }).catch(function () { $("health").textContent = "server unreachable"; });
}

/* ---- live tables ---- */
function loadTables() {
  api("/api/v1/tables").then(function (j) {
    var body = $("tables").querySelector("tbody");
    clear(body);
    $("tables-msg").textContent = j.tables.length ? "" : "No tables right now.";
    j.tables.forEach(function (t) {
      var tr = el("tr");
      cellRow(tr, [t.room_id, t.mode, t.phase,
        t.players.join(", ") + " (" + t.seats_filled + "/" + t.seats_total + ")",
        t.observer_count]);
      var td = el("td");
      var b = el("button", "Watch", "btn");
      b.addEventListener("click", function () { watch(t.room_id); });
      td.appendChild(b);
      tr.appendChild(td);
      body.appendChild(tr);
    });
  }).catch(function (e) { $("tables-msg").textContent = "Error: " + e.message; });
}

var es = null;
var watchedId = null;
var refreshTimer = null;
var EVENT_TYPES = ["ROOM_CREATED", "SEAT_JOINED", "SEAT_LEFT", "SEAT_KICKED",
  "HOST_CHANGED", "OBSERVER_JOINED", "OBSERVER_LEFT", "GAME_STARTED", "MOVE",
  "CHAT", "VOICE_META", "OBSERVER_BANNED", "USER_MUTED", "POLICY_UPDATED",
  "PROPOSAL_OFFERED", "PROPOSAL_ACCEPTED", "PROPOSAL_DECLINED",
  "PROPOSAL_EXPIRED", "GAME_PAUSED", "GAME_RESUMED", "UNDO_APPLIED",
  "REMATCH_CREATED", "GAME_OVER", "PLAYER_DISCONNECTED",
  "PLAYER_RECONNECTED"];

function stopWatching() {
  if (es) { es.close(); es = null; }
  if (refreshTimer) { clearTimeout(refreshTimer); refreshTimer = null; }
  watchedId = null;
  $("watch").style.display = "none";
}

function describeEvent(ev) {
  var who = ev.handle ? ev.handle + " " : "";
  if (ev.type === "CHAT") { return (ev.handle || "?") + ": " + ev.text; }
  if (ev.type === "MOVE") { return who + "moved " + ev.move; }
  if (ev.type === "GAME_OVER") { return "Game over: " + ev.result + " (" + ev.termination + ")"; }
  return ev.type + (who ? " - " + who : "");
}

function loadGame(id) {
  api("/api/v1/games/" + encodeURIComponent(id)).then(function (g) {
    if (watchedId !== id) { return; }
    $("watch-title").textContent = "Table " + g.room_id + " - " + g.phase;
    renderBoard($("live-board"), g.fen);
    var seats = g.seats.map(function (s) {
      return "Seat " + s.seat + ": " + (s.handle || "(empty)") +
        (s.clock_ms >= 0 ? " [" + Math.floor(s.clock_ms / 1000) + "s]" : "");
    }).join("  |  ");
    $("live-info").textContent = seats + (g.paused ? "  (paused)" : "") +
      (g.result ? "  Result: " + g.result : "");
    $("live-moves").textContent = g.moves.join(" ");
  }).catch(function (e) { $("live-info").textContent = "Error: " + e.message; });
}

function scheduleRefresh(id) {
  if (refreshTimer) { return; }
  refreshTimer = setTimeout(function () {
    refreshTimer = null;
    if (watchedId === id) { loadGame(id); }
  }, 250);
}

function watch(id) {
  stopWatching();
  watchedId = id;
  $("watch").style.display = "block";
  clear($("live-log"));
  loadGame(id);
  es = new EventSource("/api/v1/games/" + encodeURIComponent(id) +
                       "/events?stream=1&since_seq=0");
  EVENT_TYPES.forEach(function (t) {
    es.addEventListener(t, function (m) {
      var ev = {};
      try { ev = JSON.parse(m.data); } catch (e) { return; }
      var log = $("live-log");
      log.appendChild(el("div", describeEvent(ev)));
      while (log.childNodes.length > 200) { log.removeChild(log.firstChild); }
      log.scrollTop = log.scrollHeight;
      scheduleRefresh(id);
    });
  });
  es.addEventListener("closed", function () { if (es) { es.close(); } });
}

$("watch-close").addEventListener("click", stopWatching);

/* ---- archive ---- */
var arOffset = 0, arLimit = 20, arTotal = 0, arLoaded = false;
var rpPlies = [], rpIdx = 0, rpTimer = null;

function resultText(g) {
  var s = "";
  if (g.winner && g.winner !== "NONE") { s += "Winner: " + g.winner + "  "; }
  s += (g.status || g.result) + " (" + (g.termination || "") + ")";
  return s;
}

function loadArchiveStats() {
  api("/api/v1/archive/stats").then(function (s) {
    var sm = $("ar-stats-summary");
    var ai = $("ar-stats-ai");
    if (!sm) { return; }
    sm.textContent = "Total Games: " + (s.total_games || 0) +
      "  |  Online: " + (s.by_source ? (s.by_source.online || 0) : 0) +
      "  |  Client Uploads: " + (s.by_source ? (s.by_source.client_upload || 0) : 0) +
      "  |  2P: " + (s.by_mode ? (s.by_mode["2P"] || 0) : 0) +
      "  |  4P: " + (s.by_mode ? (s.by_mode["4P"] || 0) : 0) +
      "  |  OFK: " + (s.by_mode ? (s.by_mode["OFK"] || 0) : 0) +
      "  |  Avg Plies: " + (s.average_plies !== undefined ? s.average_plies.toFixed(1) : "0");
    if (s.ai_win_rates && s.ai_win_rates.length) {
      ai.textContent = "AI Win Rates: " + s.ai_win_rates.map(function (w) {
        return "L" + w.difficulty + ": " + (w.win_rate * 100).toFixed(1) + "% (" +
               w.ai_wins + "/" + w.ai_total + ")";
      }).join("  |  ");
    } else {
      ai.textContent = "";
    }
  }).catch(function () {});
}

function buildArchiveQuery(limit, offset) {
  var q = "?limit=" + limit + "&offset=" + offset;
  var mode = $("ar-mode").value;
  var src = $("ar-source").value;
  var st = $("ar-status").value;
  var win = $("ar-winner").value;
  var ai = $("ar-ai").value;
  var srt = $("ar-sort").value;
  var p = $("ar-player").value.trim();
  if (mode) { q += "&mode=" + encodeURIComponent(mode); }
  if (src) { q += "&source=" + encodeURIComponent(src); }
  if (st) { q += "&status=" + encodeURIComponent(st); }
  if (win) { q += "&winner=" + encodeURIComponent(win); }
  if (ai) { q += "&aiDifficulty=" + encodeURIComponent(ai); }
  if (srt) { q += "&sort=" + encodeURIComponent(srt); }
  if (p) { q += "&player=" + encodeURIComponent(p); }
  return q;
}

function arSearch(offset) {
  arLoaded = true;
  arOffset = Math.max(0, offset);
  var q = buildArchiveQuery(arLimit, arOffset);
  var expQ = buildArchiveQuery(1000, 0);
  $("ar-export-btn").setAttribute("href", "/api/v1/archive/export" + expQ);
  api("/api/v1/archive/games" + q).then(function (j) {
    arTotal = j.total;
    var body = $("ar-table").querySelector("tbody");
    clear(body);
    $("ar-msg").textContent = j.games.length ? "" : "No archived games.";
    j.games.forEach(function (g) {
      var tr = el("tr", null, "click");
      var srcBadge = g.source === "client_upload"
        ? el("span", "upload", "badge badge-upload")
        : el("span", "online", "badge badge-online");
      var winBadge = el("span", g.winner || "NONE",
        (g.winner === "A" || g.winner === "B" || g.winner === "C" || g.winner === "D")
          ? "badge badge-winner" : "badge");
      var playersStr = (g.players || []).map(function (p) { return p.handle; }).join(" / ");
      var tdGame = el("td", g.game_id);
      var tdMode = el("td", g.mode);
      var tdSrc = el("td"); tdSrc.appendChild(srcBadge);
      var tdPl = el("td", playersStr);
      var tdWin = el("td"); tdWin.appendChild(winBadge);
      var tdRes = el("td", (g.status || g.result) + " (" + (g.termination || "") + ")");
      var tdPlies = el("td", g.ply_count);
      var tdDate = el("td", fmtTime(g.ended_at || g.uploaded_at));
      [tdGame, tdMode, tdSrc, tdPl, tdWin, tdRes, tdPlies, tdDate].forEach(function (td) {
        tr.appendChild(td);
      });
      tr.addEventListener("click", function () { arOpen(g.game_id); });
      body.appendChild(tr);
    });
    var last = Math.min(arOffset + j.games.length, arTotal);
    $("ar-page").textContent = arTotal ? (arOffset + 1) + "-" + last + " of " + arTotal : "";
    $("ar-prev").disabled = arOffset <= 0;
    $("ar-next").disabled = arOffset + arLimit >= arTotal;
  }).catch(function (e) {
    $("ar-msg").textContent = "Error: " + e.message;
    $("ar-msg").className = "err";
  });
}

function stopReplayPlayback() {
  if (rpTimer) {
    clearInterval(rpTimer);
    rpTimer = null;
    $("rp-play").textContent = "Play";
  }
}

function showReplayPly(idx) {
  if (!rpPlies.length) { return; }
  idx = Math.max(0, Math.min(idx, rpPlies.length - 1));
  rpIdx = idx;
  var p = rpPlies[idx];
  renderBoard($("ar-board"), p.fenAfter);
  $("rp-ply-label").textContent = "Ply " + p.plyIndex + " / " + (rpPlies.length - 1);
  var rows = $("rp-table").querySelectorAll("tbody tr");
  for (var i = 0; i < rows.length; i++) {
    rows[i].classList.toggle("active-ply", i === idx);
  }
}

function arOpen(id) {
  stopReplayPlayback();
  location.hash = "#archive/" + encodeURIComponent(id);
  api("/api/v1/archive/games/" + encodeURIComponent(id)).then(function (g) {
    $("ar-detail").style.display = "block";
    $("ar-title").textContent = g.game_id + " - " + g.mode;
    $("ar-dl").setAttribute("href", "/api/v1/archive/games/" +
      encodeURIComponent(id) + "/transcript");
    $("ar-dl").setAttribute("download", id + ".json");
    var tJson = g.transcript_json || g.octi_transcript;
    if (typeof tJson === "object") {
      $("ar-transcript").textContent = JSON.stringify(tJson, null, 2);
    } else {
      try {
        $("ar-transcript").textContent = JSON.stringify(JSON.parse(tJson), null, 2);
      } catch (e) {
        $("ar-transcript").textContent = tJson;
      }
    }
    $("ar-info").textContent = (g.players || []).map(function (p) {
      return "Seat " + p.seat + ": " + p.handle;
    }).join("  |  ") + "  Result: " + resultText(g);

    api("/api/v1/archive/games/" + encodeURIComponent(id) + "/plies").then(function (pRes) {
      rpPlies = pRes.plies || [];
      var tbody = $("rp-table").querySelector("tbody");
      clear(tbody);
      rpPlies.forEach(function (ply, i) {
        var tr = el("tr", null, "click");
        var aiInfo = "";
        if (ply.ai) {
          aiInfo = "d:" + (ply.ai.depthReached || ply.ai.depth || 0) +
                   " ev:" + (ply.ai.evalScore || ply.ai.score || 0) +
                   " n:" + (ply.ai.nodesEvaluated || ply.ai.nodes || 0);
        }
        var elapsed = (ply.elapsedMs !== undefined && ply.elapsedMs >= 0) ? (ply.elapsedMs + "ms") : "";
        cellRow(tr, [ply.plyIndex, ply.player, ply.move || "(start)", elapsed, aiInfo]);
        tr.addEventListener("click", function () {
          stopReplayPlayback();
          showReplayPly(i);
        });
        tbody.appendChild(tr);
      });
      showReplayPly(rpPlies.length ? rpPlies.length - 1 : 0);
    }).catch(function () {
      renderBoard($("ar-board"), g.final_fen);
    });
  }).catch(function (e) {
    $("ar-msg").textContent = "Error: " + e.message;
  });
}

$("ar-go").addEventListener("click", function () { arSearch(0); });
$("ar-prev").addEventListener("click", function () { arSearch(arOffset - arLimit); });
$("ar-next").addEventListener("click", function () { arSearch(arOffset + arLimit); });
$("ar-close").addEventListener("click", function () {
  stopReplayPlayback();
  $("ar-detail").style.display = "none";
});

$("ar-copy-json").addEventListener("click", function () {
  var text = $("ar-transcript").textContent;
  if (navigator.clipboard && navigator.clipboard.writeText) {
    navigator.clipboard.writeText(text).then(function () {
      var orig = $("ar-copy-json").textContent;
      $("ar-copy-json").textContent = "Copied!";
      setTimeout(function () { $("ar-copy-json").textContent = orig; }, 1500);
    });
  }
});

$("rp-start").addEventListener("click", function () { stopReplayPlayback(); showReplayPly(0); });
$("rp-prev").addEventListener("click", function () { stopReplayPlayback(); showReplayPly(rpIdx - 1); });
$("rp-next").addEventListener("click", function () { stopReplayPlayback(); showReplayPly(rpIdx + 1); });
$("rp-end").addEventListener("click", function () { stopReplayPlayback(); showReplayPly(rpPlies.length - 1); });
$("rp-play").addEventListener("click", function () {
  if (rpTimer) {
    stopReplayPlayback();
  } else {
    if (rpIdx >= rpPlies.length - 1) { rpIdx = 0; showReplayPly(0); }
    $("rp-play").textContent = "Pause";
    rpTimer = setInterval(function () {
      if (rpIdx < rpPlies.length - 1) {
        showReplayPly(rpIdx + 1);
      } else {
        stopReplayPlayback();
      }
    }, 700);
  }
});

function checkHash() {
  if (location.hash && location.hash.indexOf("#archive/") === 0) {
    var id = decodeURIComponent(location.hash.substring(9));
    switchTab("archive");
    if (id) { arOpen(id); }
  }
}
window.addEventListener("hashchange", checkHash);

/* ---- leaderboard ---- */
function lbLoad() {
  api("/api/v1/leaderboard?mode=" + encodeURIComponent($("lb-mode").value) +
      "&limit=50").then(function (j) {
    var body = $("lb-table").querySelector("tbody");
    clear(body);
    $("lb-msg").textContent = j.entries.length ? "" : "No ranked players yet.";
    j.entries.forEach(function (e) {
      var tr = el("tr");
      cellRow(tr, [e.rank, e.display_name || e.handle, e.elo, e.games_played,
                   e.wins, e.losses, e.draws]);
      body.appendChild(tr);
    });
  }).catch(function (e) { $("lb-msg").textContent = "Error: " + e.message; });
}

$("lb-go").addEventListener("click", lbLoad);
$("lb-mode").addEventListener("change", lbLoad);

loadHealth();
loadTables();
checkHash();
setInterval(loadTables, 4000);
setInterval(loadHealth, 15000);
})();
</script>
</body>
</html>
)OCTIUI";

} // namespace

const char *webUiHtml()
{
    return kPage;
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
