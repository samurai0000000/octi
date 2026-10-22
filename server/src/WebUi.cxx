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
  <div class="panel">
    <div class="row">
      <label>Mode <select id="ar-mode"><option value="">Any</option>
        <option>2P</option><option>4P</option><option>OFK</option></select>
      </label>
      <label>Player id <input id="ar-player" size="36"
        placeholder="player uuid (optional)"></label>
      <button class="btn" id="ar-go">Search</button>
    </div>
    <div id="ar-msg" class="dim"></div>
    <table id="ar-table"><thead><tr><th>Game</th><th>Mode</th>
      <th>Players</th><th>Result</th><th>Plies</th><th>Ended</th></tr></thead>
      <tbody></tbody></table>
    <div class="row" style="margin-top:10px">
      <button class="btn" id="ar-prev">Previous</button>
      <button class="btn" id="ar-next">Next</button>
      <span id="ar-page" class="dim"></span>
    </div>
  </div>
  <div class="panel" id="ar-detail" style="display:none">
    <div class="row"><strong id="ar-title"></strong>
      <a class="btn" id="ar-dl" href="#">Download .octi</a></div>
    <div class="cols">
      <div><div id="ar-board" class="board"></div>
        <div id="ar-info" class="dim"></div></div>
      <div><h3>Transcript</h3><pre id="ar-transcript"></pre></div>
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
Array.prototype.forEach.call($("nav").querySelectorAll("button"), function (b) {
  b.addEventListener("click", function () {
    Array.prototype.forEach.call($("nav").querySelectorAll("button"),
      function (o) { o.classList.remove("on"); });
    b.classList.add("on");
    Object.keys(tabs).forEach(function (k) {
      tabs[k].classList.toggle("on", k === b.getAttribute("data-tab"));
    });
    if (b.getAttribute("data-tab") === "archive" && !arLoaded) { arSearch(0); }
    if (b.getAttribute("data-tab") === "board") { lbLoad(); }
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

function resultText(g) {
  return g.result + " (" + g.termination + ")";
}

function arSearch(offset) {
  arLoaded = true;
  arOffset = Math.max(0, offset);
  var q = "?limit=" + arLimit + "&offset=" + arOffset;
  if ($("ar-mode").value) { q += "&mode=" + encodeURIComponent($("ar-mode").value); }
  var p = $("ar-player").value.trim();
  if (p) { q += "&player=" + encodeURIComponent(p); }
  api("/api/v1/archive/games" + q).then(function (j) {
    arTotal = j.total;
    var body = $("ar-table").querySelector("tbody");
    clear(body);
    $("ar-msg").textContent = j.games.length ? "" : "No archived games.";
    j.games.forEach(function (g) {
      var tr = el("tr", null, "click");
      cellRow(tr, [g.game_id, g.mode,
        g.players.map(function (p) { return p.handle; }).join(" / "),
        resultText(g), g.ply_count, fmtTime(g.ended_at)]);
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

function arOpen(id) {
  api("/api/v1/archive/games/" + encodeURIComponent(id)).then(function (g) {
    $("ar-detail").style.display = "block";
    $("ar-title").textContent = g.game_id + " - " + g.mode;
    $("ar-dl").setAttribute("href", "/api/v1/archive/games/" +
      encodeURIComponent(id) + "/octi");
    $("ar-dl").setAttribute("download", id + ".octi");
    renderBoard($("ar-board"), g.final_fen);
    $("ar-info").textContent = g.players.map(function (p) {
      return "Seat " + p.seat + ": " + p.handle;
    }).join("  |  ") + "  Result: " + resultText(g);
    $("ar-transcript").textContent = g.octi_transcript;
  }).catch(function (e) {
    $("ar-msg").textContent = "Error: " + e.message;
  });
}

$("ar-go").addEventListener("click", function () { arSearch(0); });
$("ar-prev").addEventListener("click", function () { arSearch(arOffset - arLimit); });
$("ar-next").addEventListener("click", function () { arSearch(arOffset + arLimit); });

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
