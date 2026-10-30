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
<title>OCTI Web Player &amp; Archive</title>
<style>
:root {
  --bg: #0b0f17;
  --panel: #141b29;
  --panel-elev: #1a2336;
  --line: #b49148;
  --line-dim: rgba(180, 145, 72, 0.36);
  --fg: #f2ece0;
  --dim: #a8b2c6;
  --acc: #c8a253;
  --acc-bright: #e4c070;
  --input-bg: #090d14;
  --a: #10b981;
  --b: #f43f5e;
  --c: #3b82f6;
  --d: #f59e0b;
}
body.theme-rosewood {
  --bg: #2a1812;
  --panel: #3e241b;
  --panel-elev: #4d2d22;
  --line: #cea660;
  --line-dim: rgba(206, 166, 96, 0.40);
  --fg: #fbf5e8;
  --dim: #d6c2a8;
  --acc: #d6ac5c;
  --acc-bright: #eecc84;
  --input-bg: #1f110c;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  background: var(--bg);
  color: var(--fg);
  font: 14px/1.45 "Segoe UI", system-ui, -apple-system, sans-serif;
}
header {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 12px;
  padding: 10px 18px;
  background: var(--panel);
  border-bottom: 1px solid var(--line);
  box-shadow: 0 3px 14px rgba(0, 0, 0, 0.45);
}
.brand {
  display: flex;
  align-items: center;
  gap: 10px;
}
.brand-gem {
  width: 28px;
  height: 28px;
  background: radial-gradient(circle at 35% 35%, #6ee7b7, #10b981 55%, #064e3b);
  border: 2px solid var(--acc-bright);
  clip-path: polygon(30% 0%, 70% 0%, 100% 30%, 100% 70%, 70% 100%, 30% 100%, 0% 70%, 0% 30%);
}
header h1 {
  margin: 0;
  font-size: 18px;
  letter-spacing: 1.2px;
  color: var(--acc-bright);
  font-weight: 700;
}
nav { display: flex; gap: 6px; flex-wrap: wrap; }
nav button, .btn {
  background: var(--panel-elev);
  color: var(--fg);
  border: 1px solid var(--line);
  border-radius: 6px;
  padding: 6px 12px;
  cursor: pointer;
  font: inherit;
  font-weight: 600;
  text-decoration: none;
  transition: background 0.15s, color 0.15s, border-color 0.15s;
}
nav button:hover, .btn:hover:not(:disabled) {
  background: var(--acc);
  color: #0b0f17;
  border-color: var(--acc-bright);
}
nav button.on {
  background: linear-gradient(180deg, var(--acc-bright), var(--acc));
  color: #0b0f17;
  border-color: var(--acc-bright);
}
.btn:disabled {
  opacity: 0.42;
  cursor: default;
}
.header-right {
  margin-left: auto;
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}
main { max-width: 1260px; margin: 0 auto; padding: 16px; }
section { display: none; }
section.on { display: block; }
.panel {
  background: var(--panel);
  border: 1px solid var(--line);
  border-radius: 8px;
  padding: 14px;
  margin-bottom: 14px;
  box-shadow: 0 6px 20px rgba(0, 0, 0, 0.35);
}
.panel h2, .panel h3 {
  margin: 0 0 10px 0;
  color: var(--acc-bright);
  letter-spacing: 0.4px;
}
.row {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
  margin-bottom: 10px;
}
select, input {
  background: var(--input-bg);
  color: var(--fg);
  border: 1px solid var(--line-dim);
  border-radius: 5px;
  padding: 6px 8px;
  font: inherit;
}
select:focus, input:focus {
  outline: none;
  border-color: var(--acc-bright);
}
table { width: 100%; border-collapse: collapse; }
th, td {
  text-align: left;
  padding: 7px 9px;
  border-bottom: 1px solid var(--line-dim);
  vertical-align: middle;
}
th {
  color: var(--acc-bright);
  font-weight: 700;
  font-size: 12px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}
tr.click { cursor: pointer; }
tr.click:hover { background: rgba(200, 162, 83, 0.12); }
.dim { color: var(--dim); }
.err { color: #ff8a80; }
pre {
  background: var(--input-bg);
  border: 1px solid var(--line-dim);
  border-radius: 6px;
  padding: 10px;
  overflow: auto;
  max-height: 280px;
  margin: 8px 0;
  font-size: 12px;
}
.cols {
  display: flex;
  flex-wrap: wrap;
  gap: 18px;
  align-items: flex-start;
}
.col-board {
  flex: 1 1 540px;
  min-width: 300px;
  display: flex;
  flex-direction: column;
  align-items: center;
}
.col-side {
  flex: 1 1 380px;
  min-width: 280px;
}
.board-stage {
  width: 100%;
  max-width: 600px;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
}
.board {
  width: 100%;
  max-width: 560px;
  aspect-ratio: 1 / 1;
  position: relative;
  border-radius: 12px;
  box-shadow: 0 8px 26px rgba(0, 0, 0, 0.55);
}
.board canvas {
  width: 100%;
  height: 100%;
  display: block;
  border-radius: 12px;
  cursor: pointer;
}
.hud-strip {
  width: 100%;
  max-width: 560px;
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
  gap: 8px;
}
.hud-card {
  background: var(--input-bg);
  border: 1px solid var(--line-dim);
  border-radius: 7px;
  padding: 7px 10px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
}
.hud-card.active-turn {
  border-color: #ff69b4;
  box-shadow: 0 0 10px rgba(255, 105, 180, 0.35);
}
.hud-left {
  display: flex;
  align-items: center;
  gap: 8px;
  font-weight: 700;
}
.pod-dot {
  width: 18px;
  height: 18px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  font-size: 11px;
  font-weight: 800;
  color: #fff;
  border: 1.5px solid var(--acc-bright);
  clip-path: polygon(30% 0%, 70% 0%, 100% 30%, 100% 70%, 70% 100%, 30% 100%, 0% 70%, 0% 30%);
}
.pod-dot.A { background: #10b981; }
.pod-dot.B { background: #f43f5e; }
.pod-dot.C { background: #3b82f6; }
.pod-dot.D { background: #f59e0b; }
.hud-metrics {
  display: flex;
  gap: 8px;
  font-size: 12px;
  color: var(--dim);
}
.hud-metrics strong { color: var(--fg); }
.sq-inspector {
  width: 100%;
  max-width: 560px;
  min-height: 28px;
  background: var(--input-bg);
  border: 1px solid var(--line-dim);
  border-radius: 6px;
  padding: 5px 10px;
  font-size: 12px;
  color: var(--dim);
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.log {
  max-height: 240px;
  overflow: auto;
  font-size: 13px;
  background: var(--input-bg);
  border: 1px solid var(--line-dim);
  border-radius: 6px;
  padding: 6px 8px;
}
.log div { border-bottom: 1px solid var(--line-dim); padding: 3px 0; }
.badge {
  font-size: 11px;
  padding: 2px 7px;
  border-radius: 4px;
  font-weight: 700;
  text-transform: uppercase;
  display: inline-block;
}
.badge-online { background: rgba(16, 185, 129, 0.18); color: #6ee7b7; border: 1px solid #059669; }
.badge-upload { background: rgba(59, 130, 246, 0.18); color: #93c5fd; border: 1px solid #2563eb; }
.badge-winner { background: rgba(200, 162, 83, 0.22); color: var(--acc-bright); border: 1px solid var(--line); }
.replay-table-wrap {
  max-height: 310px;
  overflow-y: auto;
  border: 1px solid var(--line-dim);
  border-radius: 6px;
  margin: 8px 0;
  background: var(--input-bg);
}
tr.active-ply {
  background: rgba(200, 162, 83, 0.28) !important;
  font-weight: 700;
}
.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(135px, 1fr));
  gap: 8px;
  margin-top: 6px;
}
.stat-box {
  background: var(--input-bg);
  border: 1px solid var(--line-dim);
  border-radius: 6px;
  padding: 8px 10px;
}
.stat-box .lbl { font-size: 11px; color: var(--dim); text-transform: uppercase; }
.stat-box .val { font-size: 18px; font-weight: 700; color: var(--acc-bright); margin-top: 2px; }
.license-box {
  margin-top: 14px;
  font-size: 12px;
  color: var(--dim);
  border-top: 1px solid var(--line-dim);
  padding-top: 8px;
}
@media (max-width: 640px) {
  main { padding: 8px; }
  th, td { padding: 5px; font-size: 12px; }
}
</style>
</head>
<body>
<header>
  <div class="brand">
    <div class="brand-gem"></div>
    <h1>OCTI ARENA &amp; ARCHIVE</h1>
  </div>
  <nav id="nav">
    <button data-tab="live" class="on">Live Arena</button>
    <button data-tab="archive">Game Archive</button>
    <button data-tab="board">Leaderboard</button>
  </nav>
  <div class="header-right">
    <button class="btn" id="btn-theme" title="Switch Visual Theme">Theme: Obsidian Brass</button>
    <select class="btn" id="theme-select" style="display:none">
      <option value="obsidian">Obsidian Brass</option>
      <option value="rosewood">Rosewood Ivory</option>
    </select>
    <button class="btn" id="btn-rotate" title="Rotate Board 90 degrees CW">Rotate 90&deg;</button>
    <span id="health" class="dim"></span>
  </div>
</header>
<main>
<section id="tab-live" class="on">
  <div class="panel">
    <h2>Open &amp; Live Multiplayer Tables</h2>
    <div id="tables-msg" class="dim">Loading...</div>
    <table id="tables"><thead><tr><th>Table</th><th>Mode</th><th>State</th>
      <th>Players</th><th>Watching</th><th></th></tr></thead>
      <tbody></tbody></table>
  </div>
  <div class="panel" id="watch" style="display:none">
    <div class="row"><strong id="watch-title"></strong>
      <button class="btn" id="watch-close">Stop Watching</button></div>
    <div class="cols">
      <div class="col-board">
        <div class="board-stage">
          <div id="live-hud" class="hud-strip">
            <div id="res-card-a" class="res-card" style="display:none"></div>
            <div id="res-card-b" class="res-card" style="display:none"></div>
          </div>
          <div id="live-board" class="board"><canvas id="board-canvas"></canvas></div>
          <div id="live-inspector" class="sq-inspector"><span id="stack-inspector-body">Hover or tap any square to inspect pod stacks &amp; prongs.</span></div>
          <div id="live-info" class="dim"></div>
        </div>
      </div>
      <div class="col-side">
        <h3>Live Event Stream</h3>
        <div id="live-log" class="log"></div>
        <h3 style="margin-top:12px;">Move Notation</h3>
        <pre id="live-moves"></pre>
      </div>
    </div>
  </div>
</section>
<section id="tab-archive">
  <div class="panel" id="ar-stats-panel">
    <h3>Archive Telemetry &amp; Dataset Statistics</h3>
    <div id="ar-stats-summary" class="dim">Loading stats...</div>
    <div id="ar-stats-cards" class="stats-grid"></div>
    <div id="ar-stats-ai" class="dim" style="margin-top:8px;"></div>
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
      <label>Player <input id="ar-player" size="20"
        placeholder="uuid (optional)"></label>
      <button class="btn" id="ar-go">Filter</button>
      <a class="btn" id="ar-export-btn" href="/api/v1/archive/export" target="_blank">Export Filtered (NDJSON)</a>
    </div>
    <div id="ar-msg" class="dim"></div>
    <table id="ar-table"><thead><tr><th>Game ID</th><th>Mode</th><th>Source</th>
      <th>Players</th><th>Winner</th><th>Outcome</th><th>Plies</th><th>Date (UTC)</th></tr></thead>
      <tbody></tbody></table>
    <div class="row" style="margin-top:10px">
      <button class="btn" id="ar-prev">Previous</button>
      <button class="btn" id="ar-next">Next</button>
      <span id="ar-page" class="dim"></span>
    </div>
  </div>
  <div class="panel" id="ar-detail" style="display:none">
    <div class="row">
      <strong id="ar-title" style="color:var(--acc-bright);font-size:15px;"></strong>
      <div style="margin-left:auto;display:flex;gap:8px;flex-wrap:wrap;">
        <button class="btn" id="ar-copy-json">Copy JSON</button>
        <a class="btn" id="ar-dl" href="#">Download .json</a>
        <button class="btn" id="ar-close">Close</button>
      </div>
    </div>
    <div class="cols">
      <div class="col-board">
        <div class="board-stage">
          <div id="ar-hud" class="hud-strip"></div>
          <div id="ar-board" class="board"></div>
          <div id="ar-inspector" class="sq-inspector">Hover or tap any square to inspect pod stacks &amp; prongs.</div>
          <div class="row" style="margin-top:4px; justify-content:center;">
            <button class="btn" id="rp-start" title="First Ply (Home)">|&lt;</button>
            <button class="btn" id="rp-prev" title="Previous Ply (Left Arrow)">&lt; Prev</button>
            <button class="btn" id="rp-play" title="Play / Pause (Space)">Play</button>
            <button class="btn" id="rp-next" title="Next Ply (Right Arrow)">Next &gt;</button>
            <button class="btn" id="rp-end" title="Final Ply (End)">&gt;|</button>
            <select id="rp-speed" title="Playback Speed">
              <option value="1100">0.5x</option>
              <option value="650" selected>1.0x</option>
              <option value="320">2.0x</option>
            </select>
            <span id="rp-ply-label" class="dim" style="margin-left:6px;font-weight:700;"></span>
          </div>
          <div id="ar-info" class="dim" style="margin-top:2px;text-align:center;"></div>
        </div>
      </div>
      <div class="col-side">
        <h3>Archive Replay &amp; Interactive Move Replay</h3>
        <div class="replay-table-wrap">
          <table id="rp-table"><thead><tr><th>#</th><th>Seat</th><th>Canonical Move</th><th>Time</th><th>AI Telemetry</th></tr></thead>
          <tbody></tbody></table>
        </div>
        <h3>Transcript JSON (v3.0)</h3>
        <pre id="ar-transcript"></pre>
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
  if (!node) { return; }
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

/* =========================================================================
 * Desktop & Android Theme Palettes + HTML5 2D Canvas Board Renderer
 * ========================================================================= */
var PALETTES = {
  obsidian: {
    name: "Obsidian Brass",
    windowBg: "#0B0F17",
    boardOuterTop: "#182133",
    boardOuterMid: "#101622",
    boardOuterBot: "#0B0F18",
    bevelLight: "#E4C070",
    bevelDark: "#664C1E",
    cellDark: "#151E30",
    cellLight: "#1D2940",
    socketShadow: "rgba(8, 12, 20, 0.75)",
    gridLine: "rgba(184, 148, 76, 0.45)",
    coordLabel: "#D8B66C",
    homeOuter: "#0A0E16",
    homeInner: "#070A10",
    neutralFill: "#2D3A54",
    neutralRim: "#DCB869",
    portalGlow: "#38BDF8",
    captureGlow: "#F43F5E",
    accentBrass: "#C8A253",
    prongMetal: "#D6DCE8",
    prongTip: "#F8FAFC",
    prongSuperGold: "#FDE047",
    prongSuperCore: "#FFFBEB",
    pods: {
      A: { core: "#10B981", rim: "#059669", hi: "#6EE7B7", sh: "#064E3B", txt: "#ECFDF5" },
      B: { core: "#F43F5E", rim: "#BE123C", hi: "#FDA4AF", sh: "#881337", txt: "#FFF1F2" },
      C: { core: "#3B82F6", rim: "#1D4ED8", hi: "#93C5FD", sh: "#1E3A8A", txt: "#EFF6FF" },
      D: { core: "#F59E0B", rim: "#B45309", hi: "#FDE68A", sh: "#78350F", txt: "#FFFBEB" }
    }
  },
  rosewood: {
    name: "Rosewood Ivory",
    windowBg: "#2A1812",
    boardOuterTop: "#5C3021",
    boardOuterMid: "#4A261A",
    boardOuterBot: "#361B12",
    bevelLight: "#EECC84",
    bevelDark: "#70441C",
    cellDark: "#623626",
    cellLight: "#7C4832",
    socketShadow: "rgba(34, 16, 10, 0.72)",
    gridLine: "rgba(222, 186, 116, 0.50)",
    coordLabel: "#F4DEB0",
    homeOuter: "#140E0B",
    homeInner: "#0E0A08",
    neutralFill: "#92603E",
    neutralRim: "#F0CE84",
    portalGlow: "#2DD4BF",
    captureGlow: "#FB7185",
    accentBrass: "#D6AC5C",
    prongMetal: "#E8E0D0",
    prongTip: "#FFFCF5",
    prongSuperGold: "#FDE047",
    prongSuperCore: "#FFFFF0",
    pods: {
      A: { core: "#0D9468", rim: "#EEE4CC", hi: "#6EE7B7", sh: "#064431", txt: "#FCF8EC" },
      B: { core: "#D82E4E", rim: "#EEE4CC", hi: "#FDA4AF", sh: "#76102A", txt: "#FCF8EC" },
      C: { core: "#2563EB", rim: "#EEE4CC", hi: "#93C5FD", sh: "#183076", txt: "#FCF8EC" },
      D: { core: "#D97706", rim: "#EEE4CC", hi: "#FDE68A", sh: "#6C300C", txt: "#FCF8EC" }
    }
  }
};

var currentTheme = "obsidian";
var THEME_PALETTES = PALETTES;
var LAST_MOVE_DISPLAY_DURATION_MS = 5000;
var boardRotationDeg = 0;
var boardStates = {};

var DIR_DELTAS = [
  [ 0,  1], /* a: N  */
  [ 1,  1], /* b: NE */
  [ 1,  0], /* c: E  */
  [ 1, -1], /* d: SE */
  [ 0, -1], /* e: S  */
  [-1, -1], /* f: SW */
  [-1,  0], /* g: W  */
  [-1,  1]  /* h: NW */
];

function screenUnitForDir(dirIdx, rotDeg) {
  var d = DIR_DELTAS[dirIdx] || [0, 1];
  var sx0 = d[0], sy0 = -d[1];
  var len = Math.hypot(sx0, sy0);
  if (len > 1e-9) { sx0 /= len; sy0 /= len; }
  var r = ((rotDeg % 360) + 360) % 360;
  if (r === 90)  { return [ sy0, -sx0]; }
  if (r === 180) { return [-sx0, -sy0]; }
  if (r === 270) { return [-sy0,  sx0]; }
  return [sx0, sy0];
}

function rotationForSeat(mode, owner) {
  if (mode === "4" || mode === "4P") {
    if (owner === "A") { return 0; }
    if (owner === "B") { return 90; }
    if (owner === "C") { return 180; }
    if (owner === "D") { return 270; }
  }
  return (owner === "B") ? 180 : 0;
}

function activeTurnScreenEdge(mode, activeTurn, rotDeg) {
  var seatRot = rotationForSeat(mode, activeTurn);
  var rel = ((seatRot - rotDeg) % 360 + 360) % 360;
  if (rel === 90)  { return 1; } /* Left */
  if (rel === 180) { return 2; } /* Top */
  if (rel === 270) { return 3; } /* Right */
  return 0; /* Bottom */
}

function squareTypeAt(mode, x, y) {
  if (mode === "OFK") {
    if (y === 2 && x >= 2 && x <= 5) { return "A_HOME"; }
    if (y === 6 && x >= 2 && x <= 5) { return "B_HOME"; }
    return "NEUTRAL";
  }
  if (mode === "4" || mode === "4P") {
    if (x === 3 && y === 3) { return "A_HOME"; }
    if (x === 3 && y === 7) { return "B_HOME"; }
    if (x === 7 && y === 7) { return "C_HOME"; }
    if (x === 7 && y === 3) { return "D_HOME"; }
    if ((x === 5 && y === 3) || (x === 3 && y === 5) ||
        (x === 7 && y === 5) || (x === 5 && y === 7) ||
        (x === 5 && y === 5)) {
      return "NEUTRAL_OCTI";
    }
    return "NEUTRAL";
  }
  if (y === 3 && (x === 3 || x === 5 || x === 7)) { return "A_HOME"; }
  if (y === 7 && (x === 3 || x === 5 || x === 7)) { return "B_HOME"; }
  return "NEUTRAL";
}

function parseProngConfig(str) {
  var prongs = [0, 0, 0, 0, 0, 0, 0, 0];
  var count = 0;
  if (!str) { return { prongs: prongs, count: 0, text: "" }; }
  for (var i = 0; i < str.length; i++) {
    var ch = str.charAt(i);
    var lower = ch.toLowerCase();
    var idx = lower.charCodeAt(0) - 97;
    if (idx >= 0 && idx < 8) {
      var isSuper = (ch >= "A" && ch <= "H");
      if (i + 1 < str.length && str.charAt(i + 1) === "!") {
        isSuper = true;
        i++;
      }
      prongs[idx] = isSuper ? 2 : 1;
      count++;
    }
  }
  return { prongs: prongs, count: count, text: str };
}

function parseOctiFen(fen) {
  if (!fen) { return null; }
  var f = fen.split(":");
  if (f.length < 11) { return null; }
  var mode = f[0];
  var gw = (mode === "OFK") ? 6 : 9;
  var gh = (mode === "OFK") ? 7 : 9;
  var numPlayers = (mode === "4" || mode === "4P") ? 4 : 2;
  var reserves = [];
  var seatLetters = ["A", "B", "C", "D"];
  for (var p = 0; p < numPlayers; p++) {
    var rTok = (f[8 + p] || "0,0,0,0").split(",");
    reserves.push({
      player: seatLetters[p],
      freePods: parseInt(rTok[0] || "0", 10),
      capturedPods: parseInt(rTok[1] || "0", 10),
      freeProngs: parseInt(rTok[2] || "0", 10),
      freeSuper: parseInt(rTok[3] || "0", 10)
    });
  }
  var squaresTok = f[8 + numPlayers] || "-";
  var map = {};
  if (squaresTok && squaresTok !== "-") {
    squaresTok.split(",").forEach(function (tok) {
      var m = /^([A-D])([1-9])([1-9])([a-hA-H!]*)$/.exec(tok);
      if (!m) { return; }
      var key = m[2] + "," + m[3];
      if (!map[key]) { map[key] = []; }
      map[key].push({
        owner: m[1],
        cfg: parseProngConfig(m[4])
      });
    });
  }
  return {
    mode: mode,
    bases: parseInt(f[1] || "3", 10),
    superProngs: f[2] === "1",
    edgeless: f[3] === "1",
    status: f[4] || "P",
    turn: f[5] || "-",
    winner: f[6] || "-",
    elimMask: parseInt(f[7] || "0", 10),
    reserves: reserves,
    gw: gw,
    gh: gh,
    map: map
  };
}

function extractModifiedSquaresFromMove(moveStr) {
  var out = [];
  var seen = {};
  function addSq(x, y) {
    var k = x + "," + y;
    if (!seen[k] && x >= 1 && x <= 9 && y >= 1 && y <= 9) {
      seen[k] = true;
      out.push({ x: x, y: y });
    }
  }
  if (!moveStr || moveStr.charAt(0) === "(") { return out; }
  moveStr.split(",").forEach(function (op) {
    var clean = op.trim();
    if (!clean) { return; }
    if (clean.indexOf("-") !== -1) {
      var hops = clean.split("-");
      var firstM = /^([1-9])([1-9])/.exec(hops[0]);
      var prevX = firstM ? parseInt(firstM[1], 10) : 0;
      var prevY = firstM ? parseInt(firstM[2], 10) : 0;
      for (var i = 1; i < hops.length; i++) {
        var hm = /^([1-9])([1-9])(x?)/.exec(hops[i]);
        if (hm) {
          var nx = parseInt(hm[1], 10);
          var ny = parseInt(hm[2], 10);
          if (hm[3] === "x" && prevX && prevY) {
            var dx = nx - prevX, dy = ny - prevY;
            if (Math.abs(dx) === 2 || Math.abs(dy) === 2) {
              addSq(prevX + dx / 2, prevY + dy / 2);
            }
          }
          prevX = nx;
          prevY = ny;
        }
      }
      if (prevX && prevY) { addSq(prevX, prevY); }
    } else {
      var sm = /^\(?([1-9])([1-9])\)?/.exec(clean);
      if (sm) {
        addSq(parseInt(sm[1], 10), parseInt(sm[2], 10));
      }
    }
  });
  return out;
}

function boardToScreenGrid(x, y, gw, gh, rotDeg) {
  var r = ((rotDeg % 360) + 360) % 360;
  if (r === 90)  { return { col: gh - y, row: gw - x }; }
  if (r === 180) { return { col: gw - x, row: y - 1 }; }
  if (r === 270) { return { col: y - 1,  row: x - 1 }; }
  return { col: x - 1, row: gh - y };
}

function computeBoardGeometry(w, h, gw, gh, rotDeg) {
  var r = ((rotDeg % 360) + 360) % 360;
  var cols = (r === 90 || r === 270) ? gh : gw;
  var rows = (r === 90 || r === 270) ? gw : gh;
  var pad = Math.min(w, h) * 0.025;
  var availW = w - pad * 2;
  var availH = h - pad * 2;
  var cell = Math.min(availW / (cols + 0.9), availH / (rows + 0.9));
  var gridW = cols * cell;
  var gridH = rows * cell;
  var gutter = cell * 0.45;
  var outerW = gridW + gutter * 2;
  var outerH = gridH + gutter * 2;
  var outerX = (w - outerW) * 0.5;
  var outerY = (h - outerH) * 0.5;
  var gridX = outerX + gutter;
  var gridY = outerY + gutter;
  return {
    w: w, h: h, gw: gw, gh: gh, rotDeg: r,
    cols: cols, rows: rows, cell: cell, gutter: gutter,
    outer: { x: outerX, y: outerY, w: outerW, h: outerH },
    grid:  { x: gridX,  y: gridY,  w: gridW,  h: gridH },
    squareRect: function (x, y) {
      var sg = boardToScreenGrid(x, y, gw, gh, r);
      return {
        x: gridX + sg.col * cell,
        y: gridY + sg.row * cell,
        w: cell,
        h: cell,
        cx: gridX + (sg.col + 0.5) * cell,
        ry: gridY + (sg.row + 0.5) * cell
      };
    },
    screenToSquare: function (px, py) {
      if (px < gridX || py < gridY || px >= gridX + gridW || py >= gridY + gridH) {
        return null;
      }
      var col = Math.floor((px - gridX) / cell);
      var row = Math.floor((py - gridY) / cell);
      if (r === 90)  { return { x: gw - row, y: gh - col }; }
      if (r === 180) { return { x: gw - col, y: row + 1 }; }
      if (r === 270) { return { x: row + 1,  y: col + 1 }; }
      return { x: col + 1, y: gh - row };
    }
  };
}

function roundRectPath(ctx, x, y, w, h, rad) {
  var r = Math.min(rad, w * 0.5, h * 0.5);
  ctx.beginPath();
  ctx.moveTo(x + r, y);
  ctx.arcTo(x + w, y, x + w, y + h, r);
  ctx.arcTo(x + w, y + h, x, y + h, r);
  ctx.arcTo(x, y + h, x, y, r);
  ctx.arcTo(x, y, x + w, y, r);
  ctx.closePath();
}

function drawOctagonPath(ctx, cx, cy, radius) {
  ctx.beginPath();
  for (var i = 0; i < 8; i++) {
    var ang = (i * 45 + 22.5) * Math.PI / 180;
    var px = cx + radius * Math.cos(ang);
    var py = cy + radius * Math.sin(ang);
    if (i === 0) { ctx.moveTo(px, py); } else { ctx.lineTo(px, py); }
  }
  ctx.closePath();
}

function drawProngs(ctx, cx, cy, radius, cfg, pal, rotDeg) {
  var prongInnerR = radius * 0.65;
  var prongOuterR = radius * 1.46;
  var superOuterR = radius * 1.56;

  /* 1. Prongs & Superprongs */
  for (var d = 0; d < 8; d++) {
    var pt = cfg.prongs[d];
    if (!pt) { continue; }
    var u = screenUnitForDir(d, rotDeg);
    var ux = u[0], uy = u[1];
    var px = -uy, py = ux;
    if (pt === 1) {
      var hw = Math.max(2.4, radius * 0.20);
      ctx.beginPath();
      ctx.moveTo(cx + ux * prongInnerR - px * hw, cy + uy * prongInnerR - py * hw);
      ctx.lineTo(cx + ux * (prongOuterR * 0.86) - px * (hw * 0.85), cy + uy * (prongOuterR * 0.86) - py * (hw * 0.85));
      ctx.lineTo(cx + ux * prongOuterR, cy + uy * prongOuterR);
      ctx.lineTo(cx + ux * (prongOuterR * 0.86) + px * (hw * 0.85), cy + uy * (prongOuterR * 0.86) + py * (hw * 0.85));
      ctx.lineTo(cx + ux * prongInnerR + px * hw, cy + uy * prongInnerR + py * hw);
      ctx.closePath();
      var gProng = ctx.createLinearGradient(cx + ux * prongOuterR, cy + uy * prongOuterR, cx + ux * prongInnerR, cy + uy * prongInnerR);
      gProng.addColorStop(0, "#FFFFFF");
      gProng.addColorStop(0.45, pal.prongTip);
      gProng.addColorStop(1, pal.prongMetal);
      ctx.fillStyle = gProng;
      ctx.fill();
      ctx.lineWidth = Math.max(1.1, radius * 0.055);
      ctx.strokeStyle = "rgba(15, 20, 28, 0.92)";
      ctx.stroke();

      ctx.beginPath();
      ctx.moveTo(cx + ux * (prongInnerR + radius * 0.08), cy + uy * (prongInnerR + radius * 0.08));
      ctx.lineTo(cx + ux * (prongOuterR * 0.92), cy + uy * (prongOuterR * 0.92));
      ctx.lineWidth = Math.max(0.9, radius * 0.04);
      ctx.strokeStyle = "rgba(255, 255, 255, 0.88)";
      ctx.stroke();
    } else {
      var shw = Math.max(3.2, radius * 0.26);
      ctx.beginPath();
      ctx.moveTo(cx + ux * prongInnerR - px * (shw * 0.75), cy + uy * prongInnerR - py * (shw * 0.75));
      ctx.lineTo(cx + ux * (superOuterR * 0.72) - px * (shw * 1.25), cy + uy * (superOuterR * 0.72) - py * (shw * 1.25));
      ctx.lineTo(cx + ux * superOuterR, cy + uy * superOuterR);
      ctx.lineTo(cx + ux * (superOuterR * 0.72) + px * (shw * 1.25), cy + uy * (superOuterR * 0.72) + py * (shw * 1.25));
      ctx.lineTo(cx + ux * prongInnerR + px * (shw * 0.75), cy + uy * prongInnerR + py * (shw * 0.75));
      ctx.closePath();
      ctx.fillStyle = pal.prongSuperGold;
      ctx.fill();
      ctx.lineWidth = Math.max(1.3, radius * 0.07);
      ctx.strokeStyle = pal.prongSuperCore;
      ctx.stroke();
    }
  }
}

function drawOctagonalPod(ctx, sqRect, owner, cfg, pal, rotDeg, highlighted, centerLabel) {
  var fp = pal.pods[owner] || pal.pods.A;
  var cx = sqRect.x + sqRect.w * 0.5;
  var cy = sqRect.y + sqRect.h * 0.5;
  var radius = Math.min(sqRect.w, sqRect.h) * 0.36;

  /* 1. Prongs & Superprongs */
  drawProngs(ctx, cx, cy, radius, cfg, pal, rotDeg);

  /* 2. Drop shadow */
  drawOctagonPath(ctx, cx + radius * 0.07, cy + radius * 0.10, radius * 1.02);
  ctx.fillStyle = "rgba(0, 0, 0, 0.48)";
  ctx.fill();

  /* 3. Outer bevel octagon */
  drawOctagonPath(ctx, cx, cy, radius);
  var rimGrad = ctx.createLinearGradient(sqRect.x, sqRect.y, sqRect.x + sqRect.w, sqRect.y + sqRect.h);
  rimGrad.addColorStop(0, fp.hi);
  rimGrad.addColorStop(0.45, fp.rim);
  rimGrad.addColorStop(1, fp.sh);
  ctx.fillStyle = rimGrad;
  ctx.fill();
  ctx.lineWidth = highlighted ? Math.max(2.0, radius * 0.11) : Math.max(1.1, radius * 0.06);
  ctx.strokeStyle = highlighted ? pal.prongSuperGold : pal.accentBrass;
  ctx.stroke();

  /* 4. Inner gem dome octagon */
  drawOctagonPath(ctx, cx, cy, radius * 0.78);
  var domeGrad = ctx.createRadialGradient(
    cx - radius * 0.22, cy - radius * 0.22, radius * 0.05,
    cx, cy, radius * 0.90
  );
  domeGrad.addColorStop(0, fp.hi);
  domeGrad.addColorStop(0.5, fp.core);
  domeGrad.addColorStop(1, fp.sh);
  ctx.fillStyle = domeGrad;
  ctx.fill();
  ctx.lineWidth = Math.max(0.8, radius * 0.04);
  ctx.strokeStyle = fp.hi;
  ctx.stroke();

  /* 5. Recessed 8-direction prong sockets */
  var socketDist = radius * 0.86;
  var socketR = Math.max(1.2, radius * 0.08);
  for (var sd = 0; sd < 8; sd++) {
    var su = screenUnitForDir(sd, rotDeg);
    ctx.beginPath();
    ctx.arc(cx + su[0] * socketDist, cy + su[1] * socketDist, socketR, 0, Math.PI * 2);
    ctx.fillStyle = cfg.prongs[sd] ? pal.prongTip : "rgba(8, 12, 18, 0.78)";
    ctx.fill();
  }

  /* 6. Center emblem / stack badge */
  var label = centerLabel || owner;
  ctx.font = "bold " + Math.max(9, Math.round(radius * (centerLabel ? 0.68 : 0.78))) + "px 'Segoe UI', sans-serif";
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillStyle = "rgba(0, 0, 0, 0.55)";
  ctx.fillText(label, cx + 0.8, cy + 1.0);
  ctx.fillStyle = centerLabel ? pal.prongSuperGold : fp.txt;
  ctx.fillText(label, cx, cy);
}

var drawOctagonPod = drawOctagonalPod;

function pointOnRectPerimeter(r, s) {
  var sw = ((s % 4) + 4) % 4;
  var edge = Math.floor(sw) % 4;
  var t = sw - Math.floor(sw);
  if (edge === 0) { return [r.x + t * r.w, r.y]; }
  if (edge === 1) { return [r.x + r.w, r.y + t * r.h]; }
  if (edge === 2) { return [r.x + r.w - t * r.w, r.y + r.h]; }
  return [r.x, r.y + r.h - t * r.h];
}

function drawBoardState(state) {
  if (!state || !state.parsed) { return; }
  var canvas = state.canvas;
  var parsed = state.parsed;
  var pal = PALETTES[currentTheme] || PALETTES.obsidian;
  var rect = state.container.getBoundingClientRect();
  var cssSize = Math.max(260, Math.min(rect.width || 520, 560));
  var dpr = window.devicePixelRatio || 1;
  var pxSize = Math.round(cssSize * dpr);
  if (canvas.width !== pxSize || canvas.height !== pxSize) {
    canvas.width = pxSize;
    canvas.height = pxSize;
  }
  var ctx = canvas.getContext("2d");
  ctx.save();
  ctx.scale(dpr, dpr);

  ctx.fillStyle = pal.windowBg;
  ctx.fillRect(0, 0, cssSize, cssSize);

  var geom = computeBoardGeometry(cssSize, cssSize, parsed.gw, parsed.gh, boardRotationDeg);
  state.lastGeom = geom;

  /* 1. Outer luxury board frame with 3D brass bevel */
  var frameGrad = ctx.createLinearGradient(
    geom.outer.x, geom.outer.y,
    geom.outer.x + geom.outer.w, geom.outer.y + geom.outer.h
  );
  frameGrad.addColorStop(0, pal.boardOuterTop);
  frameGrad.addColorStop(0.5, pal.boardOuterMid);
  frameGrad.addColorStop(1, pal.boardOuterBot);
  roundRectPath(ctx, geom.outer.x, geom.outer.y, geom.outer.w, geom.outer.h, 10);
  ctx.fillStyle = frameGrad;
  ctx.fill();
  ctx.lineWidth = 2.4;
  ctx.strokeStyle = pal.bevelLight;
  ctx.stroke();

  roundRectPath(ctx, geom.outer.x + 3, geom.outer.y + 3, geom.outer.w - 6, geom.outer.h - 6, 8);
  ctx.lineWidth = 1.5;
  ctx.strokeStyle = pal.bevelDark;
  ctx.stroke();

  /* 2. Coordinate gutter labels (1..gw, 1..gh) */
  ctx.font = "bold " + Math.max(9, Math.round(geom.cell * 0.24)) + "px 'Segoe UI', sans-serif";
  ctx.fillStyle = pal.coordLabel;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  var rot = geom.rotDeg;
  for (var x = 1; x <= parsed.gw; x++) {
    var rBot = geom.squareRect(x, 1);
    var rTop = geom.squareRect(x, parsed.gh);
    if (rot === 0 || rot === 180) {
      ctx.fillText(String(x), rBot.cx, geom.grid.y + geom.grid.h + geom.gutter * 0.52);
      ctx.fillText(String(x), rTop.cx, geom.outer.y + geom.gutter * 0.48);
    } else {
      ctx.fillText(String(x), geom.outer.x + geom.gutter * 0.48, rBot.ry);
      ctx.fillText(String(x), geom.grid.x + geom.grid.w + geom.gutter * 0.52, rTop.ry);
    }
  }
  for (var y = 1; y <= parsed.gh; y++) {
    var rLeft = geom.squareRect(1, y);
    var rRight = geom.squareRect(parsed.gw, y);
    if (rot === 0 || rot === 180) {
      ctx.fillText(String(y), geom.outer.x + geom.gutter * 0.48, rLeft.ry);
      ctx.fillText(String(y), geom.grid.x + geom.grid.w + geom.gutter * 0.52, rRight.ry);
    } else {
      ctx.fillText(String(y), rLeft.cx, geom.grid.y + geom.grid.h + geom.gutter * 0.52);
      ctx.fillText(String(y), rRight.cx, geom.outer.y + geom.gutter * 0.48);
    }
  }

  /* 3. Edgeless portal glow if enabled */
  if (parsed.edgeless) {
    ctx.save();
    ctx.setLineDash([6, 4]);
    ctx.strokeStyle = pal.portalGlow;
    ctx.lineWidth = 2.0;
    ctx.strokeRect(geom.grid.x - 3, geom.grid.y - 3, geom.grid.w + 6, geom.grid.h + 6);
    ctx.restore();
  }

  /* 4. Grid squares, recessed socket rings, home OCTI shields & neutral medallions */
  for (var sy = 1; sy <= parsed.gh; sy++) {
    for (var sx = 1; sx <= parsed.gw; sx++) {
      var sqRect = geom.squareRect(sx, sy);
      var isDark = ((sx + sy) % 2 === 0);
      ctx.fillStyle = isDark ? pal.cellDark : pal.cellLight;
      ctx.fillRect(sqRect.x, sqRect.y, sqRect.w, sqRect.h);
      ctx.lineWidth = 1.0;
      ctx.strokeStyle = pal.gridLine;
      ctx.strokeRect(sqRect.x, sqRect.y, sqRect.w, sqRect.h);

      /* Recessed socket ring */
      ctx.beginPath();
      ctx.arc(sqRect.cx, sqRect.ry, sqRect.w * 0.34, 0, Math.PI * 2);
      ctx.lineWidth = 1.1;
      ctx.strokeStyle = pal.socketShadow;
      ctx.stroke();

      var st = squareTypeAt(parsed.mode, sx, sy);
      var stack = parsed.map[sx + "," + sy] || [];
      if (st === "A_HOME" || st === "B_HOME" || st === "C_HOME" || st === "D_HOME") {
        var homeOwner = st.charAt(0);
        ctx.fillStyle = pal.homeOuter;
        ctx.fillRect(sqRect.x + 1, sqRect.y + 1, sqRect.w - 2, sqRect.h - 2);
        roundRectPath(ctx, sqRect.x + 3.5, sqRect.y + 3.5, sqRect.w - 7, sqRect.h - 7, 4);
        ctx.fillStyle = pal.homeInner;
        ctx.fill();
        var isCaptured = stack.some(function (p) { return p.owner !== homeOwner; });
        ctx.lineWidth = isCaptured ? 2.2 : 1.2;
        ctx.strokeStyle = isCaptured ? pal.captureGlow : pal.bevelDark;
        ctx.stroke();

        if (!stack.length) {
          var seatRot = rotationForSeat(parsed.mode, homeOwner);
          var txtAng = ((seatRot - rot) % 360 + 360) % 360;
          ctx.save();
          ctx.translate(sqRect.cx, sqRect.ry);
          ctx.rotate(txtAng * Math.PI / 180);
          ctx.font = "bold " + Math.max(8, Math.round(sqRect.h * 0.20)) + "px 'Segoe UI', sans-serif";
          ctx.fillStyle = pal.coordLabel;
          ctx.textAlign = "center";
          ctx.textBaseline = "middle";
          ctx.fillText("OCTI", 0, 0);
          ctx.restore();
        }
      } else if (st === "NEUTRAL_OCTI") {
        ctx.save();
        roundRectPath(ctx, sqRect.x + 4, sqRect.y + 4, sqRect.w - 8, sqRect.h - 8, 6);
        ctx.fillStyle = pal.neutralFill;
        ctx.fill();
        ctx.setLineDash([4, 3]);
        ctx.lineWidth = 1.8;
        ctx.strokeStyle = pal.neutralRim;
        ctx.stroke();
        if (!stack.length) {
          ctx.font = "bold " + Math.max(8, Math.round(sqRect.h * 0.18)) + "px 'Segoe UI', sans-serif";
          ctx.fillStyle = pal.neutralRim;
          ctx.fillText("OCTI*", sqRect.cx, sqRect.ry);
        }
        ctx.restore();
      }
    }
  }

  /* 5. Pods, prongs, superprongs & layered 3D multi-pod stacks */
  for (var py = 1; py <= parsed.gh; py++) {
    for (var px = 1; px <= parsed.gw; px++) {
      var podStack = parsed.map[px + "," + py];
      if (!podStack || !podStack.length) { continue; }
      var baseRect = geom.squareRect(px, py);
      var sorted = podStack.slice().sort(function (a, b) {
        return a.cfg.count - b.cfg.count;
      });
      var maxLayers = Math.min(sorted.length, 4);
      var isHover = state.hoverSq && state.hoverSq.x === px && state.hoverSq.y === py;
      for (var l = 0; l < maxLayers; l++) {
        var pod = (l === maxLayers - 1) ? sorted[sorted.length - 1] : sorted[l];
        var off = (maxLayers - 1 - l) * 2.4;
        var layerRect = {
          x: baseRect.x - off,
          y: baseRect.y + off,
          w: baseRect.w,
          h: baseRect.h
        };
        var topBadge = (sorted.length >= 2 && l === maxLayers - 1) ? ("x" + sorted.length) : "";
        drawOctagonalPod(ctx, layerRect, pod.owner, pod.cfg, pal, rot,
          isHover && l === maxLayers - 1, topBadge);
      }
    }
  }

  var nowMs = Date.now();

  /* 6. Clockwise orange move indicator (#FF8C00) with 5000 ms auto-expiry */
  if (state.modifiedSquares && state.modifiedSquares.length &&
      state.moveSetAtMs && (nowMs - state.moveSetAtMs) < LAST_MOVE_DISPLAY_DURATION_MS) {
    var elapsed = Math.max(0, nowMs - state.moveSetAtMs);
    var phase = (elapsed % 2000) / 2000.0;
    var s0 = phase * 4.0;
    var s1 = s0 + 1.2;
    ctx.save();
    ctx.strokeStyle = "#FF8C00";
    ctx.lineWidth = 3.2;
    ctx.lineCap = "round";
    ctx.lineJoin = "round";
    state.modifiedSquares.forEach(function (sq) {
      var cr = geom.squareRect(sq.x, sq.y);
      var r = { x: cr.x + 1.5, y: cr.y + 1.5, w: cr.w - 3, h: cr.h - 3 };
      ctx.beginPath();
      var pStart = pointOnRectPerimeter(r, s0);
      ctx.moveTo(pStart[0], pStart[1]);
      var cur = s0;
      while (cur < s1 - 1e-6) {
        var nextCorner = Math.floor(cur) + 1.0;
        var stepEnd = Math.min(s1, nextCorner);
        var pt = pointOnRectPerimeter(r, stepEnd);
        ctx.lineTo(pt[0], pt[1]);
        cur = stepEnd;
      }
      ctx.stroke();
    });
    ctx.restore();
  }

  /* 7. Pink (#FF69B4) active-player board-edge turn indicator */
  if (parsed.status === "P" && parsed.turn && parsed.turn !== "-") {
    var edge = activeTurnScreenEdge(parsed.mode, parsed.turn, rot);
    var gr = geom.grid;
    var inset = 1.0;
    var startPt, endPt;
    if (edge === 1) {
      startPt = [gr.x + inset, gr.y];
      endPt   = [gr.x + inset, gr.y + gr.h];
    } else if (edge === 2) {
      startPt = [gr.x + gr.w, gr.y + inset];
      endPt   = [gr.x, gr.y + inset];
    } else if (edge === 3) {
      startPt = [gr.x + gr.w - inset, gr.y + gr.h];
      endPt   = [gr.x + gr.w - inset, gr.y];
    } else {
      startPt = [gr.x, gr.y + gr.h - inset];
      endPt   = [gr.x + gr.w, gr.y + gr.h - inset];
    }
    var tPhase = (nowMs % 2000) / 2000.0;
    var segFrac = 0.30;
    var t0 = tPhase, t1 = tPhase + segFrac;
    var strokeW = Math.max(3.5, geom.cell * 0.065);

    function drawPinkSub(a, b) {
      if (a >= b) { return; }
      var x0 = startPt[0] + a * (endPt[0] - startPt[0]);
      var y0 = startPt[1] + a * (endPt[1] - startPt[1]);
      var x1 = startPt[0] + b * (endPt[0] - startPt[0]);
      var y1 = startPt[1] + b * (endPt[1] - startPt[1]);
      ctx.save();
      ctx.lineCap = "round";
      ctx.beginPath();
      ctx.moveTo(x0, y0);
      ctx.lineTo(x1, y1);
      ctx.lineWidth = strokeW + 3.0;
      ctx.strokeStyle = "rgba(255, 105, 180, 0.28)";
      ctx.stroke();

      ctx.beginPath();
      ctx.moveTo(x0, y0);
      ctx.lineTo(x1, y1);
      ctx.lineWidth = strokeW;
      ctx.strokeStyle = "#FF69B4";
      ctx.stroke();
      ctx.restore();
    }

    if (t1 <= 1.0) {
      drawPinkSub(t0, t1);
    } else {
      drawPinkSub(t0, 1.0);
      drawPinkSub(0.0, t1 - 1.0);
    }
  }

  /* 8. Persistent Victory / Game Over Banner */
  if (parsed.status === "O" || parsed.status === "R") {
    var banW = Math.min(geom.outer.w * 0.88, 420);
    var banH = 36;
    var banX = geom.outer.x + (geom.outer.w - banW) * 0.5;
    var banY = geom.outer.y + 8;
    roundRectPath(ctx, banX, banY, banW, banH, 8);
    ctx.fillStyle = "rgba(12, 16, 26, 0.92)";
    ctx.fill();
    ctx.lineWidth = 2.0;
    ctx.strokeStyle = pal.prongSuperGold;
    ctx.stroke();

    var winTxt = parsed.winner && parsed.winner !== "-"
      ? ("VICTORY: PLAYER " + parsed.winner + (parsed.status === "R" ? " (BY RESIGNATION)" : ""))
      : "GAME OVER";
    ctx.font = "bold 13px 'Segoe UI', sans-serif";
    ctx.fillStyle = pal.prongSuperGold;
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(winTxt, banX + banW * 0.5, banY + banH * 0.5);
  }

  ctx.restore();
}

function updateHudCards(hudContainer, parsed, playerHandles) {
  if (!hudContainer || !parsed) { return; }
  clear(hudContainer);
  parsed.reserves.forEach(function (r, idx) {
    var isTurn = (parsed.status === "P" && parsed.turn === r.player);
    var card = el("div", null, "hud-card" + (isTurn ? " active-turn" : ""));
    var left = el("div", null, "hud-left");
    var dot = el("span", r.player, "pod-dot " + r.player);
    var name = (playerHandles && playerHandles[idx]) ? playerHandles[idx] : ("Player " + r.player);
    var nameSpan = el("span", name);
    left.appendChild(dot);
    left.appendChild(nameSpan);

    var metrics = el("div", null, "hud-metrics");
    function addMetric(lbl, val) {
      var s = el("span", lbl + ": ");
      s.appendChild(el("strong", val));
      metrics.appendChild(s);
    }
    addMetric("Pods", r.freePods);
    addMetric("Cap", r.capturedPods);
    addMetric("Prongs", r.freeProngs);
    if (parsed.superProngs || r.freeSuper > 0) {
      addMetric("Super", r.freeSuper);
    }
    card.appendChild(left);
    card.appendChild(metrics);
    hudContainer.appendChild(card);
  });
}

function updateInspectorText(inspectorEl, parsed, sq) {
  if (!inspectorEl || !parsed) { return; }
  if (!sq) {
    inspectorEl.textContent = "Hover or tap any square to inspect pod stacks & prongs.";
    return;
  }
  var key = sq.x + "," + sq.y;
  var stack = parsed.map[key] || [];
  var st = squareTypeAt(parsed.mode, sq.x, sq.y);
  var baseTag = st !== "NEUTRAL" ? (" [" + st.replace("_", " ") + "]") : "";
  if (!stack.length) {
    inspectorEl.textContent = "Square " + sq.x + sq.y + baseTag + " — Empty";
    return;
  }
  var desc = stack.map(function (p, i) {
    return "#" + (i + 1) + " Pod " + p.owner + (p.cfg.text ? (" (prongs: " + p.cfg.text + ")") : " (no prongs)");
  }).join("  |  ");
  inspectorEl.textContent = "Square " + sq.x + sq.y + baseTag + " (" + stack.length +
    (stack.length === 1 ? " pod): " : " pods): ") + desc;
}

var renderBoardCanvas = drawBoardState;

function ensureBoardState(container, hudId, inspectorId) {
  var id = container.id;
  if (boardStates[id]) { return boardStates[id]; }
  var canvas = container.querySelector("canvas");
  if (!canvas) {
    clear(container);
    canvas = document.createElement("canvas");
    canvas.id = "board-canvas";
    container.appendChild(canvas);
  } else if (!canvas.id) {
    canvas.id = "board-canvas";
  }
  var state = {
    id: id,
    container: container,
    canvas: canvas,
    hudEl: hudId ? $(hudId) : null,
    inspectorEl: inspectorId ? $(inspectorId) : null,
    fen: "",
    parsed: null,
    handles: [],
    modifiedSquares: [],
    moveSetAtMs: 0,
    hoverSq: null,
    lastGeom: null
  };
  function onPointer(ev) {
    if (!state.lastGeom || !state.parsed) { return; }
    var r = canvas.getBoundingClientRect();
    var sq = state.lastGeom.screenToSquare(ev.clientX - r.left, ev.clientY - r.top);
    state.hoverSq = sq;
    updateInspectorText(state.inspectorEl, state.parsed, sq);
    drawBoardState(state);
  }
  canvas.addEventListener("mousemove", onPointer);
  canvas.addEventListener("click", onPointer);
  canvas.addEventListener("mouseleave", function () {
    state.hoverSq = null;
    updateInspectorText(state.inspectorEl, state.parsed, null);
    drawBoardState(state);
  });
  boardStates[id] = state;
  return state;
}

function renderBoard(container, fen, moveStr, playerHandles) {
  if (!container || !fen) { return; }
  var hudId = container.id === "live-board" ? "live-hud" : "ar-hud";
  var inspId = container.id === "live-board" ? "live-inspector" : "ar-inspector";
  var state = ensureBoardState(container, hudId, inspId);
  var parsed = parseOctiFen(fen);
  if (!parsed) { return; }
  state.fen = fen;
  state.parsed = parsed;
  if (playerHandles && playerHandles.length) {
    state.handles = playerHandles;
  }
  if (moveStr) {
    state.modifiedSquares = extractModifiedSquaresFromMove(moveStr);
    state.moveSetAtMs = Date.now();
  } else {
    state.modifiedSquares = [];
    state.moveSetAtMs = 0;
  }
  updateHudCards(state.hudEl, parsed, state.handles);
  updateInspectorText(state.inspectorEl, parsed, state.hoverSq);
  drawBoardState(state);
}

/* Continuous 60fps animation loop for active turn & 5s move indicators */
function animationTick() {
  var now = Date.now();
  Object.keys(boardStates).forEach(function (k) {
    var st = boardStates[k];
    if (!st || !st.parsed || !st.container.offsetParent) { return; }
    var hasActiveMoveInd = st.modifiedSquares.length > 0 && (now - st.moveSetAtMs) <= 5200;
    var hasActiveTurnInd = st.parsed.status === "P";
    if (hasActiveMoveInd || hasActiveTurnInd) {
      drawBoardState(st);
    }
  });
  requestAnimationFrame(animationTick);
}
requestAnimationFrame(animationTick);

window.addEventListener("resize", function () {
  Object.keys(boardStates).forEach(function (k) {
    drawBoardState(boardStates[k]);
  });
});

$("btn-theme").addEventListener("click", function () {
  currentTheme = (currentTheme === "obsidian") ? "rosewood" : "obsidian";
  document.body.classList.toggle("theme-rosewood", currentTheme === "rosewood");
  $("btn-theme").textContent = "Theme: " + PALETTES[currentTheme].name;
  Object.keys(boardStates).forEach(function (k) {
    drawBoardState(boardStates[k]);
  });
});

$("btn-rotate").addEventListener("click", function () {
  boardRotationDeg = (boardRotationDeg + 90) % 360;
  Object.keys(boardStates).forEach(function (k) {
    drawBoardState(boardStates[k]);
  });
});

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
    var handles = (g.seats || []).map(function (s) { return s.handle || ("Seat " + s.seat); });
    var lastMove = (g.moves && g.moves.length) ? g.moves[g.moves.length - 1] : "";
    renderBoard($("live-board"), g.fen, lastMove, handles);
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
var rpPlies = [], rpIdx = 0, rpTimer = null, rpHandles = [];

function resultText(g) {
  var s = "";
  if (g.winner && g.winner !== "NONE") { s += "Winner: " + g.winner + "  "; }
  s += (g.status || g.result) + " (" + (g.termination || "") + ")";
  return s;
}

function loadArchiveStats() {
  api("/api/v1/archive/stats").then(function (s) {
    var sm = $("ar-stats-summary");
    var cards = $("ar-stats-cards");
    var ai = $("ar-stats-ai");
    if (!sm) { return; }
    sm.textContent = "License: " + (s.license || "CC-BY-4.0") + " — " +
      (s.attribution || "Octi game data, Charles Chiou (selfso.com)");
    if (cards) {
      clear(cards);
      var items = [
        ["Total Games", s.total_games || 0],
        ["Online Games", s.by_source ? (s.by_source.online || 0) : 0],
        ["Client Uploads", s.by_source ? (s.by_source.client_upload || 0) : 0],
        ["2-Player", s.by_mode ? (s.by_mode["2P"] || 0) : 0],
        ["4-Player", s.by_mode ? (s.by_mode["4P"] || 0) : 0],
        ["OCTI for Kids", s.by_mode ? (s.by_mode["OFK"] || 0) : 0],
        ["Average Plies", s.average_plies !== undefined ? s.average_plies.toFixed(1) : "0.0"]
      ];
      items.forEach(function (it) {
        var box = el("div", null, "stat-box");
        box.appendChild(el("div", it[0], "lbl"));
        box.appendChild(el("div", String(it[1]), "val"));
        cards.appendChild(box);
      });
    }
    if (s.ai_win_rates && s.ai_win_rates.length) {
      ai.textContent = "AI Win Rates by Difficulty: " + s.ai_win_rates.map(function (w) {
        var tot = (w.total_games !== undefined) ? w.total_games : (w.ai_total || 0);
        return "Level " + w.difficulty + ": " + (w.win_rate * 100).toFixed(1) + "% (" +
               w.ai_wins + "/" + tot + " AI wins)";
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
      var playersStr = (g.players || []).map(function (p) { return p.handle; }).join(" vs ");
      var shortId = g.game_id.length > 16 ? (g.game_id.slice(0, 12) + "...") : g.game_id;
      var tdGame = el("td", shortId);
      tdGame.title = g.game_id;
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
  renderBoard($("ar-board"), p.fenAfter, p.move, rpHandles);
  var totalMoves = Math.max(0, rpPlies.length - 1);
  $("rp-ply-label").textContent = "Ply " + p.plyIndex + " / " + totalMoves;
  var rows = $("rp-table").querySelectorAll("tbody tr");
  for (var i = 0; i < rows.length; i++) {
    var active = (i === idx);
    rows[i].classList.toggle("active-ply", active);
    if (active) {
      rows[i].scrollIntoView({ block: "nearest" });
    }
  }
}

function arOpen(id) {
  stopReplayPlayback();
  location.hash = "#archive/" + encodeURIComponent(id);
  api("/api/v1/archive/games/" + encodeURIComponent(id)).then(function (g) {
    $("ar-detail").style.display = "block";
    $("ar-title").textContent = "Game " + g.game_id.slice(0, 16) + "... (" + g.mode + ")";
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
    rpHandles = (g.players || []).map(function (p) { return p.handle; });
    $("ar-info").textContent = (g.players || []).map(function (p) {
      return "Seat " + p.seat + ": " + p.handle;
    }).join("  |  ") + "  |  " + resultText(g);

    api("/api/v1/archive/games/" + encodeURIComponent(id) + "/plies").then(function (pRes) {
      var rawPlies = pRes.plies || [];
      rpPlies = [];
      if (pRes.initialFen) {
        rpPlies.push({
          plyIndex: 0,
          player: "-",
          move: "",
          elapsedMs: -1,
          fenAfter: pRes.initialFen
        });
      }
      rawPlies.forEach(function (ply, idx) {
        var fen = ply.fenAfter || ply.fen || "";
        if (idx === rawPlies.length - 1 && g.final_fen) {
          fen = g.final_fen;
        }
        rpPlies.push({
          plyIndex: ply.plyIndex !== undefined ? ply.plyIndex : (ply.ply || (idx + 1)),
          player: ply.player || "-",
          move: ply.move || "",
          elapsedMs: ply.elapsedMs,
          ai: ply.ai,
          fenAfter: fen
        });
      });

      var tbody = $("rp-table").querySelector("tbody");
      clear(tbody);
      rpPlies.forEach(function (ply, i) {
        var tr = el("tr", null, "click");
        var aiInfo = "";
        if (ply.ai) {
          var d = ply.ai.depthReached !== undefined ? ply.ai.depthReached : (ply.ai.depth || 0);
          var sc = ply.ai.bestScore !== undefined ? ply.ai.bestScore : (ply.ai.evalScore || 0);
          var nd = ply.ai.nodesVisited !== undefined ? ply.ai.nodesVisited : (ply.ai.nodesEvaluated || 0);
          aiInfo = "D" + d + " • " + Number(nd).toLocaleString() + "n • score " + sc;
        }
        var elapsed = (ply.elapsedMs !== undefined && ply.elapsedMs >= 0)
          ? ((ply.elapsedMs >= 1000) ? ((ply.elapsedMs / 1000).toFixed(2) + "s") : (ply.elapsedMs + "ms"))
          : "";
        cellRow(tr, [
          ply.plyIndex,
          ply.player,
          ply.move || "(Initial Setup)",
          elapsed,
          aiInfo
        ]);
        tr.addEventListener("click", function () {
          stopReplayPlayback();
          showReplayPly(i);
        });
        tbody.appendChild(tr);
      });
      showReplayPly(rpPlies.length ? rpPlies.length - 1 : 0);
    }).catch(function () {
      renderBoard($("ar-board"), g.final_fen, "", rpHandles);
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

function startReplayTimer() {
  var delay = parseInt($("rp-speed").value || "650", 10);
  rpTimer = setInterval(function () {
    if (rpIdx < rpPlies.length - 1) {
      showReplayPly(rpIdx + 1);
    } else {
      stopReplayPlayback();
    }
  }, delay);
}

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
    startReplayTimer();
  }
});
$("rp-speed").addEventListener("change", function () {
  if (rpTimer) {
    clearInterval(rpTimer);
    startReplayTimer();
  }
});

window.addEventListener("keydown", function (ev) {
  if ($("ar-detail").style.display === "none" || !rpPlies.length) { return; }
  var tag = (ev.target && ev.target.tagName) ? ev.target.tagName.toLowerCase() : "";
  if (tag === "input" || tag === "select" || tag === "textarea") { return; }
  if (ev.key === "ArrowLeft") {
    ev.preventDefault();
    stopReplayPlayback();
    showReplayPly(rpIdx - 1);
  } else if (ev.key === "ArrowRight") {
    ev.preventDefault();
    stopReplayPlayback();
    showReplayPly(rpIdx + 1);
  } else if (ev.key === "Home") {
    ev.preventDefault();
    stopReplayPlayback();
    showReplayPly(0);
  } else if (ev.key === "End") {
    ev.preventDefault();
    stopReplayPlayback();
    showReplayPly(rpPlies.length - 1);
  } else if (ev.key === " ") {
    ev.preventDefault();
    $("rp-play").click();
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
