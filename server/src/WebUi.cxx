/*
 * WebUi.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "WebUi.hxx"

namespace octi {

namespace {

const char kIndexHtml[] = R"OCTIUI(<!DOCTYPE html>
<!--
  Copyright (C) 2026, Charles Chiou
-->
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>OCTI &mdash; Official Platform, Web Player, Archive &amp; Leaderboard</title>
<style>
:root {
  --bg: #fbfbfa;
  --panel: #ffffff;
  --panel-elev: #f4f4f5;
  --line: #e4e4e7;
  --line-dim: #e4e4e7;
  --fg: #18181b;
  --dim: #52525b;
  --acc: #18181b;
  --acc-bright: #2563eb;
  --input-bg: #ffffff;
  --a: #10b981;
  --b: #f43f5e;
  --c: #3b82f6;
  --d: #f59e0b;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  background: var(--bg);
  color: var(--fg);
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  font-size: 14px;
  line-height: 1.5;
  -webkit-font-smoothing: antialiased;
}
header {
  position: sticky;
  top: 0;
  z-index: 100;
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 12px;
  padding: 12px 24px;
  background: rgba(251, 251, 250, 0.94);
  backdrop-filter: blur(12px);
  -webkit-backdrop-filter: blur(12px);
  border-bottom: 1px solid var(--line);
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.04), 0 4px 12px rgba(0, 0, 0, 0.03);
}
.brand {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  text-decoration: none !important;
}
.brand-badge {
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
  font-size: 0.82rem;
  font-weight: 700;
  color: #ffffff;
  background: var(--fg);
  padding: 0.25rem 0.6rem;
  border-radius: 4px;
  display: inline-flex;
  align-items: center;
  gap: 0.45rem;
  letter-spacing: 0.05em;
}
.brand-badge::before {
  content: '';
  display: inline-block;
  width: 6px;
  height: 6px;
  background-color: #dc2626;
  border-radius: 50%;
}
.brand-text {
  display: flex;
  flex-direction: column;
}
.brand-name {
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
  font-size: 0.95rem;
  font-weight: 700;
  color: var(--fg);
  letter-spacing: 0.05em;
  line-height: 1.2;
}
.brand-sub {
  font-size: 11px;
  color: var(--dim);
}
nav { display: flex; gap: 6px; flex-wrap: wrap; }
nav button, .btn {
  background: #ffffff;
  color: var(--fg);
  border: 1px solid var(--line);
  border-radius: 6px;
  padding: 6px 14px;
  cursor: pointer;
  font: inherit;
  font-size: 13.5px;
  font-weight: 600;
  text-decoration: none !important;
  touch-action: manipulation;
  transition: all 0.2s cubic-bezier(0.16, 1, 0.3, 1);
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
}
nav button:hover:not(:disabled), .btn:hover:not(:disabled) {
  background: #f4f4f5;
  border-color: #d4d4d8;
  color: var(--fg);
}
nav button.on, .btn.btn-primary {
  background: var(--fg);
  color: #ffffff;
  border-color: var(--fg);
}
nav button.on:hover, .btn.btn-primary:hover {
  background: #27272a;
  border-color: #27272a;
  color: #ffffff;
}
.btn:disabled {
  opacity: 0.42;
  cursor: not-allowed;
}
.header-right {
  margin-left: auto;
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}
#nav-auth-pill {
  border-color: #3b82f6;
  color: #1d4ed8;
  background: #eff6ff;
  font-size: 13px;
}
#nav-auth-pill:hover {
  background: #dbeafe;
}
main { max-width: 1260px; margin: 0 auto; padding: 20px 16px 40px 16px; }
section.tab { display: none; }
section.tab.on { display: block; }
.panel {
  background: var(--panel);
  border: 1px solid var(--line);
  border-radius: 10px;
  padding: 18px 20px;
  margin-bottom: 16px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.04), 0 4px 12px rgba(0, 0, 0, 0.03);
}
.panel h2, .panel h3, .panel h4 {
  margin: 0 0 12px 0;
  color: var(--fg);
  font-weight: 600;
  letter-spacing: -0.01em;
}
.row {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  align-items: center;
  margin-bottom: 12px;
}
select, input, textarea {
  background: var(--input-bg);
  color: var(--fg);
  border: 1px solid var(--line);
  border-radius: 6px;
  padding: 6px 10px;
  font: inherit;
  transition: border-color 0.15s ease;
}
select:focus, input:focus, textarea:focus {
  outline: none;
  border-color: var(--acc-bright);
  box-shadow: 0 0 0 2px rgba(37, 99, 235, 0.15);
}
table { width: 100%; border-collapse: collapse; }
th, td {
  text-align: left;
  padding: 8px 10px;
  border-bottom: 1px solid var(--line);
  vertical-align: middle;
}
th {
  color: var(--dim);
  font-weight: 600;
  font-size: 11.5px;
  text-transform: uppercase;
  letter-spacing: 0.05em;
  background: #fafafa;
}
tr.click { cursor: pointer; }
tr.click:hover { background: #f4f4f5; }
.dim { color: var(--dim); }
.err { color: #dc2626; }
pre {
  background: #f8fafc;
  border: 1px solid var(--line);
  border-radius: 6px;
  padding: 12px;
  overflow: auto;
  max-height: 280px;
  margin: 8px 0;
  font-size: 12px;
  color: var(--fg);
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
}
.cols {
  display: flex;
  flex-wrap: wrap;
  gap: 20px;
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
  gap: 10px;
}
.board {
  width: 100%;
  max-width: 560px;
  aspect-ratio: 1 / 1;
  position: relative;
  border-radius: 12px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.12), 0 1px 3px rgba(0, 0, 0, 0.08);
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
  background: #ffffff;
  border: 1px solid var(--line);
  border-radius: 8px;
  padding: 8px 12px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.04);
}
.hud-card.active-turn {
  border-color: #2563eb;
  box-shadow: 0 0 0 2px rgba(37, 99, 235, 0.25);
}
.hud-left {
  display: flex;
  align-items: center;
  gap: 8px;
  font-weight: 700;
}
.pod-dot {
  width: 20px;
  height: 20px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  font-size: 11px;
  font-weight: 800;
  color: #fff;
  border: 1.5px solid rgba(0, 0, 0, 0.2);
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
  min-height: 32px;
  background: #f8fafc;
  border: 1px solid var(--line);
  border-radius: 8px;
  padding: 6px 12px;
  font-size: 12px;
  color: var(--dim);
  display: flex;
  align-items: center;
  justify-content: space-between;
}
.badge {
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
  font-size: 11px;
  padding: 2px 7px;
  border-radius: 4px;
  font-weight: 600;
  text-transform: uppercase;
  display: inline-block;
}
.badge-online { background: rgba(16, 185, 129, 0.1); color: #059669; border: 1px solid rgba(5, 150, 105, 0.3); }
.badge-upload { background: rgba(37, 99, 235, 0.1); color: #2563eb; border: 1px solid rgba(37, 99, 235, 0.3); }
.badge-winner { background: rgba(220, 38, 38, 0.1); color: #dc2626; border: 1px solid rgba(220, 38, 38, 0.3); }
.badge-active { background: rgba(16, 185, 129, 0.12); color: #059669; border: 1px solid rgba(5, 150, 105, 0.3); }
.badge-revoked { background: rgba(220, 38, 38, 0.12); color: #dc2626; border: 1px solid rgba(220, 38, 38, 0.3); }
.replay-table-wrap {
  max-height: 320px;
  overflow-y: auto;
  border: 1px solid var(--line);
  border-radius: 8px;
  margin: 8px 0;
  background: #ffffff;
}
tr.active-ply {
  background: rgba(37, 99, 235, 0.1) !important;
  font-weight: 700;
  color: var(--acc-bright);
}
.stats-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(135px, 1fr));
  gap: 10px;
  margin-top: 8px;
}
.stat-box {
  background: #ffffff;
  border: 1px solid var(--line);
  border-radius: 8px;
  padding: 10px 14px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.04);
}
.stat-box .lbl { font-size: 11px; color: var(--dim); text-transform: uppercase; font-weight: 600; }
.stat-box .val { font-size: 20px; font-weight: 700; color: var(--fg); margin-top: 2px; }
.license-box {
  margin-top: 14px;
  font-size: 12px;
  color: var(--dim);
  border-top: 1px solid var(--line);
  padding-top: 10px;
}
/* Features grid */
.features-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
  gap: 16px;
  margin-top: 16px;
}
.feature-card {
  background: #ffffff;
  border: 1px solid var(--line);
  border-radius: 8px;
  padding: 16px;
  box-shadow: 0 1px 3px rgba(0,0,0,0.04);
}
.feature-card h3 {
  margin: 0 0 6px 0;
  font-size: 16px;
  color: var(--fg);
}
.feature-card p {
  margin: 0;
  color: var(--dim);
  font-size: 13.5px;
}
/* Rules grid */
.rules-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
  gap: 14px;
  margin-top: 12px;
}
.rule-box {
  background: #fbfbfa;
  border: 1px solid var(--line);
  border-radius: 8px;
  padding: 14px;
}
.rule-box h4 {
  margin: 0 0 6px 0;
  font-size: 14px;
  color: var(--acc-bright);
}
.rule-box p {
  margin: 0;
  font-size: 13px;
  color: var(--dim);
}
@media (max-width: 640px) {
  main { padding: 10px 8px; }
  header { padding: 10px 14px; }
  th, td { padding: 6px 8px; font-size: 12px; }
}
</style>
</head>
<body>
<header>
  <div class="brand">
    <span class="brand-badge">OCTI</span>
    <div class="brand-text">
      <span class="brand-name">OCTI</span>
      <span class="brand-sub">octi.selfso.com &mdash; Official Platform, Web Player, Players, Archive, Leaderboard &amp; Robot Registry</span>
    </div>
  </div>
  <nav id="nav">
    <button type="button" data-tab="home" id="nav-home-btn" class="btn on">Home</button>
    <button type="button" data-tab="play" id="nav-play-btn" class="btn">Web Player</button>
    <button type="button" data-tab="users" id="nav-users-btn" class="btn">Players</button>
    <button type="button" data-tab="archive" id="nav-archive-btn" class="btn">Game Archive</button>
    <button type="button" data-tab="board" id="nav-board-btn" class="btn">Leaderboard</button>
    <button type="button" data-tab="robots" id="nav-robots-btn" class="btn">Robot Registration</button>
    <button type="button" data-tab="account" id="nav-account-btn" class="btn">Account</button>
  </nav>
  <div class="header-right">
    <button type="button" class="btn" id="btn-theme" title="Switch Board Palette">Theme: Obsidian Brass</button>
    <select class="btn" id="theme-select" style="display:none">
      <option value="obsidian">Obsidian Brass</option>
      <option value="rosewood">Rosewood Ivory</option>
    </select>
    <button type="button" class="btn" id="btn-rotate" title="Rotate Board 90 degrees CW">Rotate 90&deg;</button>
    <span id="health" class="dim"></span>
    <button type="button" class="btn" id="nav-auth-pill" title="Click to view Account">Guest (Not Signed In)</button>
  </div>
</header>
<main>

<!-- Tab 1: Home (Default Top Page) -->
<section id="tab-home" class="tab on">
  <div class="panel" style="background:linear-gradient(180deg, #ffffff 0%, #f9fafb 100%);">
    <div style="max-width:800px;margin:10px 0;">
      <h1 style="margin:0 0 8px 0;font-size:28px;letter-spacing:-0.02em;">OCTI &amp; OCTI for Kids (OFK)</h1>
      <p style="font-size:15px;color:var(--dim);margin:0 0 16px 0;line-height:1.6;">
        The timeless strategy game of tactical growth, prong directional control, multi-hop jumping, and base infiltration.
        Compete directly in your browser against client-side minimax JS robots, join real-time multiplayer tables, explore grandmaster replays, or register automated bots.
      </p>
      <div class="row">
        <button type="button" class="btn btn-primary" id="home-cta-play" style="padding:10px 18px;font-size:14px;">Play in Browser (vs. JS Robot or Online)</button>
        <button type="button" class="btn" id="home-cta-users" style="padding:10px 18px;font-size:14px;">Browse Registered Players</button>
        <button type="button" class="btn" id="home-cta-account" style="padding:10px 18px;font-size:14px;">Sign In with Google</button>
      </div>
    </div>
  </div>

  <div class="panel">
    <h2>Platform Features</h2>
    <div class="features-grid">
      <div class="feature-card">
        <h3>Web Player (2P / OFK / 4P)</h3>
        <p>Play locally against asynchronous minimax JS robots (0% server load) or join online multiplayer tables and observe active matches.</p>
      </div>
      <div class="feature-card">
        <h3>Registered Players &amp; Profiles</h3>
        <p>Public member directory, detailed player dossiers, Elo rating curves, win/loss breakdowns, and linked Google authentication.</p>
      </div>
      <div class="feature-card">
        <h3>Game Archive &amp; Replays</h3>
        <p>Comprehensive transcript dataset with step-by-step visual move replays, ply timeline navigation, AI search metrics, and NDJSON exports.</p>
      </div>
      <div class="feature-card">
        <h3>Global ELO Leaderboard</h3>
        <p>Official rankings across 2-Player (9x9 3-Base), 4-Player, and OCTI for Kids (7x7) modes updated continuously.</p>
      </div>
      <div class="feature-card">
        <h3>Bot &amp; Robot Token Registry</h3>
        <p>Registered players can register up to 3 automated bots, generate secret bearer tokens, and compete programmatically via the table API.</p>
      </div>
      <div class="feature-card">
        <h3>Desktop &amp; Android Apps</h3>
        <p>Native client applications featuring hardware-accelerated 3D boards, AI engine analysis, and offline matches. Download: <a href="/software/windows/" target="_blank">Windows Desktop</a> | <a href="/software/android/" target="_blank">Android APK</a></p>
      </div>
    </div>
  </div>

  <div class="panel">
    <h2>How to Play OCTI</h2>
    <div class="rules-grid">
      <div class="rule-box">
        <h4>1. Pods &amp; Reserves</h4>
        <p>Players begin with pods on their home base squares and in reserve. Pods are octagonal pieces that can hold prongs and stack upon each other.</p>
      </div>
      <div class="rule-box">
        <h4>2. Prongs &amp; Direction</h4>
        <p>On your turn, you can allocate a directional prong (North, North-East, East, etc.) to one of your pods, granting movement and jumping vectors.</p>
      </div>
      <div class="rule-box">
        <h4>3. Move, Jump &amp; Capture</h4>
        <p>Pods can step into adjacent empty squares or jump over adjacent pods along prong directions. Jumping enemy pods allows capturing or stacking.</p>
      </div>
      <div class="rule-box">
        <h4>4. Victory Conditions</h4>
        <p>Win immediately by moving any of your pods onto any opponent's home base square, or by capturing all opponent pods so none remain.</p>
      </div>
    </div>
  </div>
</section>

<!-- Tab 2: Web Player (Local vs. JS Robot & Online Play/Observe) -->
<section id="tab-play" class="tab">
  <div class="panel">
    <div class="row">
      <button type="button" class="btn btn-primary" id="subtab-play-local">Play vs. Browser JS Robot</button>
      <button type="button" class="btn" id="subtab-play-online">Online Lobby &amp; Observation</button>
    </div>

    <!-- Sub-Panel: Play vs. Browser JS Robot -->
    <div id="play-local-panel">
      <div class="row" style="background:#f9fafb;padding:12px;border-radius:8px;border:1px solid var(--line);">
        <label>Mode:
          <select id="web-play-mode">
            <option value="2P" selected>2P (9x9, 3-Base)</option>
            <option value="2P_1BASE">2P 1-Base (9x9, Rapid)</option>
            <option value="OFK">OFK (7x7, OCTI for Kids)</option>
            <option value="4P">4P (9x9, Human vs. 3 JS Robots)</option>
          </select>
        </label>
        <label>Difficulty:
          <select id="web-bot-difficulty">
            <option value="1">Easy (Depth 1, ~120ms)</option>
            <option value="2" selected>Medium (Depth 2, ~350ms)</option>
            <option value="3">Hard (Depth 3, ~900ms)</option>
          </select>
        </label>
        <label>Human Side:
          <select id="web-human-side">
            <option value="A" selected>Red (First)</option>
            <option value="B">Green (Second)</option>
          </select>
        </label>
        <button type="button" class="btn btn-primary" id="btn-web-new-bot-game">New Game vs. JS Robot</button>
        <button type="button" class="btn" id="btn-web-resign">Resign</button>
      </div>

      <div id="web-play-status" style="margin:12px 0 8px 0;font-size:15px;font-weight:700;padding:8px 12px;background:#f4f4f5;border-radius:6px;">
        Starting game...
      </div>
      <div id="web-play-archive-status" class="dim" style="margin-bottom:12px;font-size:13px;"></div>

      <div class="cols">
        <div class="col-board">
          <div class="board-stage">
            <div id="web-play-reserves" class="hud-strip"></div>
            <div id="web-play-board-wrap" style="width:100%;max-width:580px;aspect-ratio:1/1;margin:0 auto;background:#182133;border-radius:12px;padding:6px;box-shadow:0 4px 16px rgba(0,0,0,0.12);">
              <svg id="web-play-svg" viewBox="0 0 580 580" style="width:100%;height:100%;display:block;border-radius:8px;cursor:pointer;"></svg>
            </div>
            <div class="panel" style="width:100%;max-width:580px;margin-top:10px;padding:12px;">
              <div class="row" style="margin-bottom:8px;">
                <strong style="margin-right:6px;">Action:</strong>
                <button type="button" class="btn on" id="btn-act-move">Move / Jump Pod</button>
                <button type="button" class="btn" id="btn-act-prong">Add Prong</button>
                <button type="button" class="btn" id="btn-act-place">Place Pod</button>
              </div>
              <div id="prong-dir-picker" class="row" style="display:none;margin-bottom:8px;">
                <span class="dim" style="font-size:12px;">Prong:</span>
                <button type="button" class="btn" data-dir="a">N (&uarr;)</button>
                <button type="button" class="btn" data-dir="b">NE (&nearr;)</button>
                <button type="button" class="btn" data-dir="c">E (&rarr;)</button>
                <button type="button" class="btn" data-dir="d">SE (&searr;)</button>
                <button type="button" class="btn" data-dir="e">S (&darr;)</button>
                <button type="button" class="btn" data-dir="f">SW (&swarr;)</button>
                <button type="button" class="btn" data-dir="g">W (&larr;)</button>
                <button type="button" class="btn" data-dir="h">NW (&nwarr;)</button>
              </div>
              <div class="row">
                <label style="font-size:13px;"><input type="checkbox" id="web-play-capture-toggle" checked> Capture jumped pods</label>
                <span id="web-selected-move-text" style="margin-left:auto;font-weight:700;color:var(--acc-bright);">Selected: (None)</span>
                <button type="button" class="btn btn-primary" id="btn-web-commit-move" disabled>Commit Move</button>
              </div>
            </div>
          </div>
        </div>
        <div class="col-side">
          <h3>Move List</h3>
          <div id="web-play-moves" style="max-height:480px;overflow-y:auto;background:#fff;border:1px solid var(--line);border-radius:8px;padding:8px;font-family:ui-monospace,SFMono-Regular,monospace;font-size:13px;">
            <div class="dim">Game started.</div>
          </div>
        </div>
      </div>
    </div>

    <!-- Sub-Panel: Online Lobby, Live Tables & Observation -->
    <div id="play-online-panel" style="display:none;">
      <div id="play-online-lobby-container">
        <div class="row">
          <button type="button" class="btn" id="btn-web-refresh-tables">Refresh Tables</button>
          <button type="button" class="btn btn-primary" id="btn-web-create-table" title="Sign in to play online">Create Online Table</button>
          <span id="web-online-notice" class="dim" style="margin-left:auto;"></span>
        </div>

        <div id="web-my-table-banner" style="display:none;background:#eff6ff;border:1px solid #93c5fd;border-radius:8px;padding:10px 14px;margin:10px 0;align-items:center;justify-content:space-between;gap:12px;">
          <strong id="web-my-table-banner-text" style="color:#1e3a8a;font-size:14px;">Seated at Table</strong>
          <button type="button" class="btn btn-primary" id="btn-web-leave-my-table" style="background:#dc2626;border-color:#dc2626;">Leave Table</button>
        </div>

        <div id="web-create-table-panel" class="panel" style="margin:10px 0;background:#f9fafb;">
          <h4 style="margin:0 0 8px 0;">Create New Table</h4>
          <div class="row">
            <label>Mode:
              <select id="web-create-mode">
                <option value="2P" selected>2P (2-Player)</option>
                <option value="4P">4P (4-Player)</option>
                <option value="OFK">OFK (OCTI for Kids)</option>
              </select>
            </label>
            <label>Bases:
              <select id="web-create-bases">
                <option value="1">1-Base (Fast)</option>
                <option value="3" selected>3-Base (Standard)</option>
                <option value="4">4-Base (OFK)</option>
              </select>
            </label>
            <label>Time Control:
              <select id="web-create-tc">
                <option value="untimed" selected>Untimed</option>
                <option value="300000+0">5m + 0s</option>
                <option value="600000+5000">10m + 5s</option>
                <option value="900000+10000">15m + 10s</option>
              </select>
            </label>
            <label><input type="checkbox" id="web-create-super-prongs"> Superprongs</label>
            <label><input type="checkbox" id="web-create-edgeless"> Edgeless</label>
            <label><input type="checkbox" id="web-create-rated" checked> Rated</label>
            <label>Passcode:
              <input id="web-create-passcode" type="password" placeholder="Optional" maxlength="32" style="width:110px;">
            </label>
            <button type="button" class="btn btn-primary" id="btn-web-create-table-submit">Host Table</button>
          </div>
        </div>

        <div style="background:#f4f4f5;border:1px solid var(--line);border-radius:6px;padding:8px 12px;margin:10px 0;font-size:13px;">
          <strong>Active Room Observers:</strong> <span id="web-room-observers">anonymous-127.0.0.1</span>
        </div>

        <div id="web-online-tables" style="margin-top:12px;">
          <table id="web-lobby-tables">
            <thead>
              <tr>
                <th>Table / Host</th>
                <th>Mode &amp; Rules</th>
                <th>Phase</th>
                <th>Seat Roster</th>
                <th>Observers</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody id="web-lobby-tables-body">
              <tr><td colspan="6" class="dim" style="text-align:center;padding:12px;">Loading live tables...</td></tr>
            </tbody>
          </table>
        </div>

        <div class="cols" style="margin-top:18px;">
          <div class="col-side" style="flex:1;" id="web-lobby-chat-box">
            <h4>Lobby Chat</h4>
            <div id="web-lobby-chat-log" style="height:180px;overflow-y:auto;border:1px solid var(--line);border-radius:6px;padding:8px;background:#fff;margin-bottom:8px;font-size:13px;">
              <div class="dim">Connected to lobby chat.</div>
            </div>
          </div>
          <div class="col-side" style="flex:1;">
            <h4>Game Room Chat</h4>
            <div id="web-room-chat-log" style="height:180px;overflow-y:auto;border:1px solid var(--line);border-radius:6px;padding:8px;background:#fff;margin-bottom:8px;font-size:13px;">
              <div class="dim">No active table selected.</div>
            </div>
          </div>
        </div>
        <div class="row">
          <input id="web-chat-input" placeholder="Sign in to chat (observing as anonymous-127.0.0.1)" style="flex:1;" disabled>
          <button type="button" class="btn" id="btn-web-chat-send" disabled>Send</button>
        </div>
      </div>
    </div>
  </div>
</section>

<!-- Tab 3: Users (Public Directory & Detailed Profiles) -->
<section id="tab-users" class="tab">
  <div class="panel">
    <div class="row">
      <input id="users-search-input" placeholder="Search handle, display name, location, or bio..." style="min-width:280px;flex:1;">
      <select id="users-sort-select">
        <option value="elo_2p" selected>2P ELO</option>
        <option value="elo_4p">4P ELO</option>
        <option value="elo_ofk">OFK ELO</option>
        <option value="games_played">Games Played</option>
        <option value="wins">Wins</option>
        <option value="handle">Handle</option>
        <option value="display_name">Display Name</option>
        <option value="created_at">Joined Date</option>
        <option value="last_seen_at">Last Seen</option>
      </select>
      <select id="users-order-select">
        <option value="desc" selected>Descending</option>
        <option value="asc">Ascending</option>
      </select>
      <button type="button" class="btn" id="btn-users-search">Search</button>
    </div>

    <div id="users-msg" class="dim" style="margin-bottom:8px;"></div>
    <table id="users-table">
      <thead>
        <tr>
          <th>Avatar</th>
          <th>Handle</th>
          <th>Display Name</th>
          <th>Location</th>
          <th>Bio</th>
          <th>2P ELO</th>
          <th>4P ELO</th>
          <th>OFK ELO</th>
          <th>Games (W-L-D)</th>
          <th>Robots</th>
          <th>Joined</th>
        </tr>
      </thead>
      <tbody id="users-tbody"></tbody>
    </table>

    <div class="row" style="margin-top:12px;">
      <button type="button" class="btn" id="users-prev-btn">Prev</button>
      <span id="users-page-info" class="dim"></span>
      <button type="button" class="btn" id="users-next-btn">Next</button>
    </div>
  </div>

  <!-- Detailed User Profile Panel -->
  <div class="panel" id="user-profile-detail" style="display:none;">
    <div class="row" style="align-items:flex-start;">
      <div id="user-detail-avatar" style="width:96px;height:96px;border-radius:50%;overflow:hidden;flex-shrink:0;"></div>
      <div style="flex:1;">
        <h2 id="user-detail-name" style="margin:0 0 4px 0;"></h2>
        <div id="user-detail-handle" style="font-weight:700;color:var(--acc-bright);"></div>
        <div id="user-detail-location" class="dim" style="font-size:12px;"></div>
        <p id="user-detail-bio" style="margin:8px 0;"></p>
        <div id="user-detail-meta" class="dim" style="font-size:12px;"></div>
      </div>
      <div>
        <button type="button" class="btn" id="btn-user-edit-self" style="display:none;margin-right:8px;">Edit My Profile &amp; Avatar</button>
        <button type="button" class="btn" id="user-detail-close">Close</button>
      </div>
    </div>

    <div id="user-detail-stats" class="stats-grid" style="margin-top:14px;"></div>

    <h3 style="margin-top:18px;">Registered Robots</h3>
    <div id="user-detail-robots" class="dim">Loading robots...</div>

    <h3 style="margin-top:18px;">Recent Match History</h3>
    <div id="user-detail-games" class="dim">Loading recent matches...</div>
  </div>
</section>

<!-- Tab 4: Game Archive (100% Preserved) -->
<section id="tab-archive" class="tab">
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
      <button type="button" class="btn" id="ar-go">Filter</button>
      <a class="btn" id="ar-export-btn" href="/api/v1/archive/export" target="_blank">Export Filtered (NDJSON)</a>
    </div>
    <div id="ar-msg" class="dim"></div>
    <table id="ar-table"><thead><tr><th>Game ID</th><th>Mode</th><th>Source</th>
      <th>Players</th><th>Winner</th><th>Outcome</th><th>Plies</th><th>Date (UTC)</th></tr></thead>
      <tbody></tbody></table>
    <div class="row" style="margin-top:10px">
      <button type="button" class="btn" id="ar-prev">Previous</button>
      <button type="button" class="btn" id="ar-next">Next</button>
      <span id="ar-page" class="dim"></span>
    </div>
  </div>
  <div class="panel" id="ar-detail" style="display:none">
    <div class="row">
      <strong id="ar-title" style="color:var(--acc-bright);font-size:15px;"></strong>
      <div style="margin-left:auto;display:flex;gap:8px;flex-wrap:wrap;">
        <button type="button" class="btn" id="ar-copy-json">Copy JSON</button>
        <a class="btn" id="ar-dl" href="#">Download .json</a>
        <button type="button" class="btn" id="ar-close">Close</button>
      </div>
    </div>
    <div class="cols">
      <div class="col-board">
        <div class="board-stage">
          <div id="ar-hud" class="hud-strip">
            <div id="res-card-a" class="res-card" style="display:none"></div>
            <div id="res-card-b" class="res-card" style="display:none"></div>
          </div>
          <div id="ar-board" class="board"><canvas id="board-canvas"></canvas></div>
          <div id="ar-inspector" class="sq-inspector"><span id="stack-inspector-body">Hover or tap any square to inspect pod stacks &amp; prongs.</span></div>
          <div class="row" style="margin-top:4px; justify-content:center;">
            <button type="button" class="btn" id="rp-start" title="First Ply (Home)">|&lt;</button>
            <button type="button" class="btn" id="rp-prev" title="Previous Ply (Left Arrow)">&lt; Prev</button>
            <button type="button" class="btn" id="rp-play" title="Play / Pause (Space)">Play</button>
            <button type="button" class="btn" id="rp-next" title="Next Ply (Right Arrow)">Next &gt;</button>
            <button type="button" class="btn" id="rp-end" title="Final Ply (End)">&gt;|</button>
            <select id="rp-speed" title="Playback Speed">
              <option value="2200">0.5x</option>
              <option value="1100" selected>1.0x</option>
              <option value="550">2.0x</option>
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

<!-- Tab 5: Leaderboard (100% Preserved) -->
<section id="tab-board" class="tab">
  <div class="panel">
    <div class="row">
      <label>Mode <select id="lb-mode"><option>2P</option><option>4P</option>
        <option>OFK</option></select></label>
      <button type="button" class="btn" id="lb-go">Refresh</button>
    </div>
    <div id="lb-msg" class="dim"></div>
    <table id="lb-table"><thead><tr><th>#</th><th>Player</th><th>Elo</th>
      <th>Games</th><th>W</th><th>L</th><th>D</th></tr></thead>
      <tbody></tbody></table>
  </div>
</section>

<!-- Tab 6: Robot Registration & Token Management -->
<section id="tab-robots" class="tab">
  <div class="panel">
    <h2>OCTI Robot Registration &amp; Token Management</h2>
    <p class="dim">Each registered user can register up to <strong>3 active robots</strong> (<code>octi_bot_&lt;token&gt;</code>) with a globally unique <code>robot_name</code>.</p>
  </div>

  <div class="panel" id="robots-guest-card">
    <h3>Sign In Required</h3>
    <p class="dim">Please sign in with your Google account on the Account tab to register and manage your automated bots.</p>
    <button type="button" class="btn btn-primary" id="btn-robots-to-account">Go to Account Tab</button>
  </div>

  <div id="robots-owner-card" style="display:none;">
    <div class="panel">
      <div class="row">
        <h3>Register a New Robot</h3>
        <span id="robots-quota-badge" class="badge" style="margin-left:auto;">Active Robots: 0 / 3</span>
      </div>
      <div class="row">
        <input id="robot-name-input" placeholder="Robot Name (3..24 chars [A-Za-z0-9_-])" maxlength="24" style="flex:1;">
        <input id="robot-desc-input" placeholder="Robot Description (optional, max 256 chars)" maxlength="256" style="flex:2;">
        <button type="button" class="btn btn-primary" id="btn-create-robot">Create Robot &amp; Generate Token</button>
      </div>
      <div id="robot-new-token-box" style="display:none;background:#eff6ff;border:1px solid #93c5fd;padding:12px;border-radius:8px;margin-top:10px;"></div>
    </div>

    <div class="panel">
      <h3>My Registered Robots</h3>
      <table id="robots-table">
        <thead>
          <tr>
            <th>Robot Name</th>
            <th>Description</th>
            <th>Secret Token</th>
            <th>Status</th>
            <th>Created</th>
            <th>Actions</th>
          </tr>
        </thead>
        <tbody id="robots-tbody"></tbody>
      </table>
    </div>
  </div>
</section>

<!-- Tab 7: Account (Sign In, Profile/Avatar Editor & Linked Identities) -->
<section id="tab-account" class="tab">
  <div class="panel" id="account-guest-card" style="max-width:480px;margin:24px auto;text-align:center;">
    <h2>Sign In to OCTI Platform</h2>
    <p class="dim">Sign in using your Google account to create a player profile, track Elo ratings, play online games, and register automated bots.</p>
    <div style="display:flex;flex-direction:column;gap:12px;margin-top:20px;">
      <button type="button" class="btn btn-primary" id="btn-web-google" style="padding:10px 16px;font-size:14px;">Sign in with Google</button>
      <button type="button" class="btn btn-primary" id="btn-web-facebook" style="display:none;padding:10px 16px;font-size:14px;background:#1877f2;border-color:#1877f2;">Sign in with Facebook</button>
    </div>
  </div>

  <div id="account-profile-card" style="display:none;">
    <div class="panel">
      <div class="row">
        <h2>Authenticated Profile &amp; Avatar Editor</h2>
        <button type="button" class="btn" id="btn-web-logout" style="margin-left:auto;">Sign Out</button>
      </div>
      <div class="cols">
        <div style="display:flex;flex-direction:column;align-items:center;gap:10px;">
          <div id="web-avatar-preview" style="width:96px;height:96px;border-radius:50%;overflow:hidden;background:#e4e4e7;display:flex;align-items:center;justify-content:center;"></div>
          <label class="btn" style="cursor:pointer;">
            Upload Avatar
            <input type="file" id="web-avatar-file" accept="image/*" style="display:none;">
          </label>
          <button type="button" class="btn" id="btn-web-clear-avatar">Clear Avatar</button>
        </div>
        <div style="flex:1;">
          <div class="row">
            <label style="flex:1;">Handle <input id="web-handle-input" style="width:100%;"></label>
          </div>
          <div style="font-size:12px;color:var(--dim);margin-top:2px;margin-bottom:8px;">Handle changes are limited to once every 24 hours.</div>
          <div class="row">
            <label style="flex:1;">Display Name <input id="web-display-name-input" style="width:100%;"></label>
            <label style="flex:1;">Location <input id="web-location-input" style="width:100%;"></label>
          </div>
          <div class="row">
            <label style="flex:1;">Avatar URL / Data URI <input id="web-avatar-url" style="width:100%;" readonly></label>
          </div>
          <div class="row">
            <label style="width:100%;">Bio <textarea id="web-bio-input" maxlength="500" style="width:100%;height:80px;"></textarea></label>
          </div>
          <div id="web-profile-status" style="font-size:13px;margin-top:8px;min-height:18px;"></div>
          <div class="row" style="margin-top:10px;">
            <button type="button" class="btn btn-primary" id="btn-web-save-profile">Save Profile</button>
            <button type="button" class="btn" id="btn-web-view-public-profile">View My Public Profile</button>
          </div>
        </div>
      </div>
    </div>

    <div class="panel" id="account-identities-card">
      <h3>Linked Sign-In Identities</h3>
      <div id="web-link-conflict-box" style="display:none;background:#fef2f2;border:1px solid #f87171;color:#991b1b;padding:12px;border-radius:8px;margin-bottom:12px;">
        <div><strong>Account Conflict (409 identity_already_bound):</strong> This social identity is already associated with another OCTI user.</div>
        <div class="row" style="margin-top:8px;">
          <button type="button" class="btn" id="btn-conflict-switch">Switch to That Account</button>
          <button type="button" class="btn btn-primary" id="btn-conflict-merge">Merge Empty Account Into Current Account</button>
        </div>
      </div>
      <table id="web-identities-table">
        <thead>
          <tr>
            <th>Provider</th>
            <th>Status / Email</th>
            <th>Action</th>
          </tr>
        </thead>
        <tbody id="web-identities-tbody"></tbody>
      </table>
    </div>
  </div>
</section>

<div id="web-confirm-modal" style="display:none;position:fixed;inset:0;background:rgba(15,23,42,0.55);z-index:9999;align-items:center;justify-content:center;padding:16px;">
  <div style="background:#ffffff;border:1px solid var(--line);border-radius:12px;max-width:440px;width:100%;padding:20px;box-shadow:0 20px 35px rgba(0,0,0,0.22);">
    <h3 id="web-confirm-title" style="margin:0 0 8px 0;font-size:18px;color:var(--fg);">Confirm Action</h3>
    <p id="web-confirm-message" style="margin:0 0 18px 0;font-size:14px;color:var(--dim);line-height:1.5;"></p>
    <div class="row" style="justify-content:flex-end;gap:10px;margin:0;">
      <button type="button" class="btn" id="btn-web-confirm-cancel">Cancel</button>
      <button type="button" class="btn btn-primary" id="btn-web-confirm-ok">Confirm</button>
    </div>
  </div>
</div>

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

function escapeHtml(s) {
  return String(s || "").replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

function getWebInstallId() {
  var id = null;
  try { id = localStorage.getItem("octi_install_id"); } catch (e) {}
  if (!id) {
    if (typeof crypto !== "undefined" && typeof crypto.randomUUID === "function") {
      id = crypto.randomUUID();
    } else {
      id = "web-" + Math.random().toString(36).substring(2, 10) + "-" + Date.now().toString(36);
    }
    try { localStorage.setItem("octi_install_id", id); } catch (e) {}
  }
  return id;
}

function showToast(msg, type) {
  var st = $("web-profile-status");
  if (st) {
    st.textContent = msg;
    st.style.color = (type === "error") ? "#ef4444" : "#10b981";
  }
}

function api(path, opts) {
  opts = opts || {};
  opts.headers = opts.headers || {};
  if (!opts.headers["Accept"]) { opts.headers["Accept"] = "application/json"; }
  var token = null;
  try { token = localStorage.getItem("octi_jwt_token"); } catch (e) {}
  if (token && !opts.headers["Authorization"]) {
    opts.headers["Authorization"] = "Bearer " + token;
  }
  var installId = getWebInstallId();
  if (installId && !opts.headers["X-Octi-Install-Id"]) {
    opts.headers["X-Octi-Install-Id"] = installId;
  }
  if (opts.body && typeof opts.body === "object" && !(opts.body instanceof FormData)) {
    opts.headers["Content-Type"] = "application/json";
    opts.body = JSON.stringify(opts.body);
  }
  return fetch(path, opts).then(function (r) {
    return r.json().catch(function () { return {}; }).then(function (j) {
      if (!r.ok) {
        var msg = (j && j.message)
          ? (j.error ? (j.message + " (" + j.error + ")") : j.message)
          : ((j && j.error) ? j.error : ("HTTP " + r.status));
        var err = new Error(msg);
        err.status = r.status;
        err.data = j;
        throw err;
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
  values.forEach(function (v) {
    if (v instanceof Node) {
      var td = document.createElement("td");
      td.appendChild(v);
      tr.appendChild(td);
    } else {
      tr.appendChild(el("td", v));
    }
  });
}

function createAvatarEl(url, nameOrHandle, sizePx) {
  sizePx = sizePx || 32;
  if (url && (url.indexOf("http") === 0 || url.indexOf("data:image/") === 0 || url.indexOf("/") === 0)) {
    var img = document.createElement("img");
    img.src = (url.indexOf("/") === 0 && API_BASE) ? (API_BASE + url) : url;
    img.width = sizePx;
    img.height = sizePx;
    img.style.width = sizePx + "px";
    img.style.height = sizePx + "px";
    img.style.borderRadius = "50%";
    img.style.objectFit = "cover";
    img.alt = nameOrHandle || "avatar";
    return img;
  }
  var ch = (nameOrHandle ? nameOrHandle.charAt(0) : "U").toUpperCase();
  var svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  svg.setAttribute("width", sizePx);
  svg.setAttribute("height", sizePx);
  svg.setAttribute("viewBox", "0 0 " + sizePx + " " + sizePx);
  svg.style.borderRadius = "50%";
  svg.style.verticalAlign = "middle";
  svg.style.display = "inline-block";
  var circle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
  circle.setAttribute("cx", sizePx / 2);
  circle.setAttribute("cy", sizePx / 2);
  circle.setAttribute("r", sizePx / 2);
  circle.setAttribute("fill", "#2563eb");
  var text = document.createElementNS("http://www.w3.org/2000/svg", "text");
  text.setAttribute("x", sizePx / 2);
  text.setAttribute("y", sizePx * 0.65);
  text.setAttribute("text-anchor", "middle");
  text.setAttribute("fill", "#ffffff");
  text.setAttribute("font-size", Math.round(sizePx * 0.5));
  text.setAttribute("font-weight", "bold");
  text.textContent = ch;
  svg.appendChild(circle);
  svg.appendChild(text);
  return svg;
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
  var hudId = "ar-hud";
  var inspId = "ar-inspector";
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


/* =========================================================================
 * Bot Minimax & Heuristic Search Engine
 * ========================================================================= */
var DIR_CHARS = ["a", "b", "c", "d", "e", "f", "g", "h"];

function cloneState(s) {
  var newRes = s.reserves.map(function (r) {
    return {
      player: r.player,
      freePods: r.freePods,
      capturedPods: r.capturedPods,
      freeProngs: r.freeProngs,
      freeSuper: r.freeSuper
    };
  });
  var newMap = {};
  Object.keys(s.map).forEach(function (k) {
    newMap[k] = s.map[k].map(function (pod) {
      return {
        owner: pod.owner,
        cfg: {
          prongs: pod.cfg.prongs.slice(),
          count: pod.cfg.count,
          text: pod.cfg.text
        }
      };
    });
  });
  return {
    mode: s.mode,
    bases: s.bases,
    superProngs: s.superProngs,
    edgeless: s.edgeless,
    status: s.status,
    turn: s.turn,
    winner: s.winner,
    elimMask: s.elimMask,
    reserves: newRes,
    gw: s.gw,
    gh: s.gh,
    map: newMap
  };
}

function serializeOctiFen(s) {
  var parts = [];
  parts.push(s.mode);
  parts.push(String(s.bases));
  parts.push(s.superProngs ? "1" : "0");
  parts.push(s.edgeless ? "1" : "0");
  parts.push(s.status || "P");
  parts.push(s.turn || "-");
  parts.push(s.winner || "-");
  parts.push(String(s.elimMask || 0));
  s.reserves.forEach(function (r) {
    parts.push(r.freePods + "," + r.capturedPods + "," + r.freeProngs + "," + r.freeSuper);
  });
  var sqTokens = [];
  Object.keys(s.map).forEach(function (k) {
    var p = k.split(",");
    var px = p[0], py = p[1];
    s.map[k].forEach(function (pod) {
      sqTokens.push(pod.owner + px + py + (pod.cfg.text || ""));
    });
  });
  parts.push(sqTokens.length ? sqTokens.join(",") : "-");
  return parts.join(":");
}

function getHomeSquares(mode, player) {
  if (mode === "OFK") {
    return (player === "A")
      ? [{x:2, y:2}, {x:3, y:2}, {x:4, y:2}, {x:5, y:2}]
      : [{x:2, y:6}, {x:3, y:6}, {x:4, y:6}, {x:5, y:6}];
  }
  if (mode === "4" || mode === "4P") {
    if (player === "A") return [{x:3, y:3}];
    if (player === "B") return [{x:3, y:7}];
    if (player === "C") return [{x:7, y:7}];
    return [{x:7, y:3}];
  }
  if (mode === "2P_1BASE") {
    return (player === "A") ? [{x:5, y:3}] : [{x:5, y:7}];
  }
  return (player === "A")
    ? [{x:3, y:3}, {x:5, y:3}, {x:7, y:3}]
    : [{x:3, y:7}, {x:5, y:7}, {x:7, y:7}];
}

function getOpponentBases(mode, player) {
  if (mode === "4" || mode === "4P") {
    var all = ["A", "B", "C", "D"];
    var out = [];
    all.forEach(function (p) {
      if (p !== player) {
        out = out.concat(getHomeSquares(mode, p));
      }
    });
    return out;
  }
  return getHomeSquares(mode, player === "A" ? "B" : "A");
}

function generateMoves(s, player) {
  var moves = [];
  var pIdx = (player === "A" ? 0 : (player === "B" ? 1 : (player === "C" ? 2 : 3)));
  var res = s.reserves[pIdx];

  /* 1. Pod placements */
  if (res && res.freePods > 0) {
    var homes = getHomeSquares(s.mode, player);
    homes.forEach(function (h) {
      var k = h.x + "," + h.y;
      if (!s.map[k] || !s.map[k].length) {
        moves.push("(" + h.x + h.y + ")");
      }
    });
  }

  /* 2. Prong additions */
  if (res && res.freeProngs > 0) {
    Object.keys(s.map).forEach(function (k) {
      var stack = s.map[k];
      if (!stack || !stack.length) return;
      var top = stack[stack.length - 1];
      if (top.owner === player) {
        var coords = k.split(",");
        for (var d = 0; d < 8; d++) {
          if (!top.cfg.prongs[d]) {
            moves.push(coords[0] + coords[1] + DIR_CHARS[d]);
          }
        }
      }
    });
  }

  /* 3. Moves and Jumps */
  Object.keys(s.map).forEach(function (k) {
    var stack = s.map[k];
    if (!stack || !stack.length) return;
    var top = stack[stack.length - 1];
    if (top.owner === player) {
      var px = parseInt(k.split(",")[0], 10);
      var py = parseInt(k.split(",")[1], 10);
      for (var d = 0; d < 8; d++) {
        if (top.cfg.prongs[d]) {
          var dx = DIR_DELTAS[d][0], dy = DIR_DELTAS[d][1];
          var stepX = px + dx, stepY = py + dy;
          if (stepX >= 1 && stepX <= s.gw && stepY >= 1 && stepY <= s.gh) {
            var stepK = stepX + "," + stepY;
            var stepStack = s.map[stepK];
            if (!stepStack || !stepStack.length) {
              moves.push(px + "" + py + "-" + stepX + "" + stepY);
            } else {
              var jumpX = stepX + dx, jumpY = stepY + dy;
              if (jumpX >= 1 && jumpX <= s.gw && jumpY >= 1 && jumpY <= s.gh) {
                var jumpK = jumpX + "," + jumpY;
                var jumpStack = s.map[jumpK];
                if (!jumpStack || !jumpStack.length) {
                  var jumpedTop = stepStack[stepStack.length - 1];
                  if (jumpedTop.owner !== player) {
                    moves.push(px + "" + py + "-" + jumpX + "" + jumpY + "x");
                  }
                  moves.push(px + "" + py + "-" + jumpX + "" + jumpY);
                }
              }
            }
          }
        }
      }
    }
  });

  return moves;
}

function applyMoveToState(s, move, player) {
  var pIdx = (player === "A" ? 0 : (player === "B" ? 1 : (player === "C" ? 2 : 3)));
  var res = s.reserves[pIdx];

  if (move.charAt(0) === "(") {
    var x = parseInt(move.charAt(1), 10);
    var y = parseInt(move.charAt(2), 10);
    if (res) { res.freePods = Math.max(0, res.freePods - 1); }
    var k = x + "," + y;
    if (!s.map[k]) { s.map[k] = []; }
    s.map[k].push({
      owner: player,
      cfg: { prongs: [0, 0, 0, 0, 0, 0, 0, 0], count: 0, text: "" }
    });
  } else if (/^([1-9])([1-9])([a-hA-H])$/.test(move)) {
    var mx = RegExp.$1, my = RegExp.$2, dirCh = RegExp.$3.toLowerCase();
    var dIdx = DIR_CHARS.indexOf(dirCh);
    var mk = mx + "," + my;
    if (s.map[mk] && s.map[mk].length) {
      var top = s.map[mk][s.map[mk].length - 1];
      if (dIdx >= 0 && !top.cfg.prongs[dIdx]) {
        top.cfg.prongs[dIdx] = 1;
        top.cfg.count++;
        top.cfg.text += dirCh;
        if (res) { res.freeProngs = Math.max(0, res.freeProngs - 1); }
      }
    }
  } else if (/^([1-9])([1-9])-([1-9])([1-9])(x?)$/.test(move)) {
    var x1 = parseInt(RegExp.$1, 10), y1 = parseInt(RegExp.$2, 10);
    var x2 = parseInt(RegExp.$3, 10), y2 = parseInt(RegExp.$4, 10);
    var isCap = (RegExp.$5 === "x");
    var fromK = x1 + "," + y1;
    var toK = x2 + "," + y2;
    if (s.map[fromK] && s.map[fromK].length) {
      var pod = s.map[fromK].pop();
      if (!s.map[fromK].length) { delete s.map[fromK]; }
      if (Math.abs(x2 - x1) === 2 || Math.abs(y2 - y1) === 2) {
        var midK = ((x1 + x2) / 2) + "," + ((y1 + y2) / 2);
        if (isCap && s.map[midK] && s.map[midK].length) {
          s.map[midK].pop();
          if (!s.map[midK].length) { delete s.map[midK]; }
          if (res) { res.capturedPods++; }
        }
      }
      if (!s.map[toK]) { s.map[toK] = []; }
      s.map[toK].push(pod);

      /* Check Victory */
      var oppBases = getOpponentBases(s.mode, player);
      for (var b = 0; b < oppBases.length; b++) {
        if (x2 === oppBases[b].x && y2 === oppBases[b].y) {
          s.status = "O";
          s.winner = player;
          return s;
        }
      }
    }
  }

  if (s.mode === "4" || s.mode === "4P") {
    var seats = ["A", "B", "C", "D"];
    s.turn = seats[(seats.indexOf(player) + 1) % 4];
  } else {
    s.turn = (player === "A") ? "B" : "A";
  }
  return s;
}

function evaluateState(s, player) {
  if (s.status === "O") {
    if (s.winner === player) return 100000;
    if (s.winner && s.winner !== "-") return -100000;
  }
  var oppBases = getOpponentBases(s.mode, player);
  var myBases = getHomeSquares(s.mode, player);
  var score = 0;

  Object.keys(s.map).forEach(function (k) {
    var stack = s.map[k];
    if (!stack || !stack.length) return;
    var top = stack[stack.length - 1];
    var px = parseInt(k.split(",")[0], 10);
    var py = parseInt(k.split(",")[1], 10);

    if (top.owner === player) {
      score += 200;
      score += top.cfg.count * 30;
      var minDist = 999;
      oppBases.forEach(function (b) {
        var d = Math.abs(px - b.x) + Math.abs(py - b.y);
        if (d < minDist) minDist = d;
      });
      score += (18 - minDist) * 120;
    } else {
      score -= 200;
      score -= top.cfg.count * 30;
      var threatDist = 999;
      myBases.forEach(function (b) {
        var td = Math.abs(px - b.x) + Math.abs(py - b.y);
        if (td < threatDist) threatDist = td;
      });
      score -= (18 - threatDist) * 160;
    }
  });

  var pIdx = (player === "A" ? 0 : (player === "B" ? 1 : (player === "C" ? 2 : 3)));
  var oppIdx = (pIdx === 0 ? 1 : 0);
  if (s.reserves[pIdx]) score += s.reserves[pIdx].capturedPods * 150;
  if (s.reserves[oppIdx]) score -= s.reserves[oppIdx].capturedPods * 150;

  return score;
}

function alphaBeta(s, depth, alpha, beta, isMaximizing, botSide, curPlayer, nodes) {
  nodes.count++;
  if (s.status === "O") {
    if (s.winner === botSide) return 100000;
    if (s.winner && s.winner !== "-") return -100000;
  }
  if (depth <= 0) {
    return evaluateState(s, botSide);
  }

  var moves = generateMoves(s, curPlayer);
  if (!moves.length) {
    return isMaximizing ? -50000 : 50000;
  }

  if (isMaximizing) {
    var maxVal = -Infinity;
    for (var i = 0; i < moves.length; i++) {
      var nextS = applyMoveToState(cloneState(s), moves[i], curPlayer);
      var val = alphaBeta(nextS, depth - 1, alpha, beta, false, botSide, nextS.turn, nodes);
      if (val > maxVal) maxVal = val;
      if (val > alpha) alpha = val;
      if (beta <= alpha) break;
    }
    return maxVal;
  } else {
    var minVal = Infinity;
    for (var j = 0; j < moves.length; j++) {
      var nextS2 = applyMoveToState(cloneState(s), moves[j], curPlayer);
      var val2 = alphaBeta(nextS2, depth - 1, alpha, beta, true, botSide, nextS2.turn, nodes);
      if (val2 < minVal) minVal = val2;
      if (val2 < beta) beta = val2;
      if (beta <= alpha) break;
    }
    return minVal;
  }
}

function botSearch(fen, depth, mode, botSide) {
  var s = parseOctiFen(fen);
  if (!s) return { bestMove: null, score: 0, nodes: 0 };
  var moves = generateMoves(s, botSide);
  if (!moves.length) return { bestMove: null, score: -100000, nodes: 1 };

  moves.sort(function (a, b) {
    function rank(m) {
      if (m.indexOf("x") !== -1) return 5;
      if (m.indexOf("-") !== -1) {
        var p = m.split("-");
        var d = Math.abs(p[0].charAt(0) - p[1].charAt(0)) + Math.abs(p[0].charAt(1) - p[1].charAt(1));
        return d > 1 ? 4 : 3;
      }
      if (m.charAt(0) !== "(") return 2;
      return 1;
    }
    return rank(b) - rank(a);
  });

  var nodes = { count: 0 };
  var bestMove = moves[0];
  var bestScore = -Infinity;

  for (var i = 0; i < moves.length; i++) {
    var mv = moves[i];
    var nextS = applyMoveToState(cloneState(s), mv, botSide);
    nodes.count++;
    var sc;
    if (nextS.status === "O" && nextS.winner === botSide) {
      sc = 100000;
    } else if (depth <= 1) {
      sc = evaluateState(nextS, botSide);
    } else {
      sc = -alphaBeta(nextS, depth - 1, -100000, 100000, false, botSide, nextS.turn, nodes);
    }
    if (sc > bestScore) {
      bestScore = sc;
      bestMove = mv;
    }
  }

  return { bestMove: bestMove, score: bestScore, nodes: nodes.count };
}

/* Inline Web Worker Initialization */
var botWorker = null;
try {
  var workerParts = [
    "self.onmessage = function (e) {",
    "  var d = e.data;",
    "  if (d.type === 'search') {",
    "    var t0 = Date.now();",
    "    var res = (" + botSearch.toString() + ")(d.fen, d.depth, d.mode, d.side);",
    "    var elapsed = Date.now() - t0;",
    "    self.postMessage({",
    "      type: 'result',",
    "      bestMove: res.bestMove,",
    "      score: res.score,",
    "      nodes: res.nodes,",
    "      elapsedMs: elapsed",
    "    });",
    "  }",
    "};",
    "var DIR_DELTAS = " + JSON.stringify(DIR_DELTAS) + ";",
    "var DIR_CHARS = " + JSON.stringify(DIR_CHARS) + ";",
    parseProngConfig.toString(),
    parseOctiFen.toString(),
    cloneState.toString(),
    getHomeSquares.toString(),
    getOpponentBases.toString(),
    generateMoves.toString(),
    applyMoveToState.toString(),
    evaluateState.toString(),
    alphaBeta.toString()
  ];
  var workerCode = workerParts.join(String.fromCharCode(10));
  var blob = new Blob([workerCode], { type: "application/javascript" });
  botWorker = new Worker(URL.createObjectURL(blob));
} catch (e) {
  botWorker = null;
}



/* =========================================================================
 * 7 Navigation Tabs & URL Hash Router
 * ========================================================================= */
var tabs = {
  home: $("tab-home"),
  play: $("tab-play"),
  users: $("tab-users"),
  archive: $("tab-archive"),
  board: $("tab-board"),
  robots: $("tab-robots"),
  account: $("tab-account")
};

function switchTab(name) {
  if (!tabs[name]) { name = "home"; }
  Array.prototype.forEach.call($("nav").querySelectorAll("button"), function (b) {
    b.classList.toggle("on", b.getAttribute("data-tab") === name);
  });
  Object.keys(tabs).forEach(function (k) {
    if (tabs[k]) {
      tabs[k].classList.toggle("on", k === name);
    }
  });

  if (name === "archive") {
    if (!arLoaded) { arSearch(0); }
    loadArchiveStats();
  } else if (name === "board") {
    lbLoad();
  } else if (name === "users") {
    usersSearch(usersOffset || 0);
  } else if (name === "robots") {
    if (currentUser) { loadMyRobots(); }
  } else if (name === "account") {
    if (currentUser || localStorage.getItem("octi_jwt_token")) { loadMyProfile(); }
  } else if (name === "play") {
    if (!localBotState) { initLocalBotGame(); }
  }
}

Array.prototype.forEach.call($("nav").querySelectorAll("button"), function (b) {
  b.addEventListener("click", function () {
    var targetTab = b.getAttribute("data-tab");
    if (history.replaceState) {
      history.replaceState(null, "", window.location.pathname + "#" + targetTab);
    } else {
      location.hash = "#" + targetTab;
    }
    switchTab(targetTab);
  });
});

$("nav-auth-pill").addEventListener("click", function () {
  switchTab("account");
});

function handleAuthHash() {
  var params = null;
  var token = null;

  if (window.location.search) {
    var searchParams = new URLSearchParams(window.location.search);
    if (searchParams.get("auth_token")) {
      params = searchParams;
      token = searchParams.get("auth_token");
    }
  }

  if (!token && window.location.hash) {
    var hashStr = window.location.hash.substring(1);
    if (hashStr.indexOf("auth_token=") !== -1) {
      var qIdx = hashStr.indexOf("?");
      var rawHash = (qIdx !== -1) ? hashStr.substring(qIdx + 1) : hashStr;
      var hashParams = new URLSearchParams(rawHash);
      if (hashParams.get("auth_token")) {
        params = hashParams;
        token = hashParams.get("auth_token");
      }
    }
  }

  if (token) {
    try {
      localStorage.setItem("octi_jwt_token", token);
    } catch (e) {}

    if (history.replaceState) {
      history.replaceState(null, null, window.location.pathname);
    } else {
      location.hash = "";
    }

    switchTab("account");

    var isNewAccount = params && (params.get("is_new_account") === "true");

    loadMyProfile().then(function () {
      if (isNewAccount) {
        var modal = document.getElementById("web-account-modal");
        if (modal) {
          modal.style.display = "flex";
        }
        var handleInput = document.getElementById("web-handle-input");
        if (handleInput) {
          handleInput.focus();
        }
        loadIdentities();
        var handleStatus = document.getElementById("web-profile-status") || document.getElementById("web-handle-status");
        if (handleStatus) {
          handleStatus.textContent = "Welcome! Choose your OCTI player handle and click Save Profile.";
          handleStatus.style.color = "#10b981";
        }
      }
    }).catch(function () {});

    return true;
  }
  return false;
}

function checkHash() {
  if (handleAuthHash()) return;
  var h = location.hash;
  if (!h || h === "#") {
    if (window.location.pathname && window.location.pathname.indexOf("/archives") === 0) {
      switchTab("archive");
    } else {
      switchTab("home");
    }
  } else if (h === "#home") {
    switchTab("home");
  } else if (h.indexOf("#archive/") === 0) {
    var id = decodeURIComponent(h.substring(9));
    switchTab("archive");
    if (id) { arOpen(id); }
  } else if (h.indexOf("#users/") === 0) {
    var uid = decodeURIComponent(h.substring(7));
    switchTab("users");
    if (uid) { openUserProfile(uid); }
  } else {
    var tabName = h.substring(1);
    if (tabs[tabName]) {
      switchTab(tabName);
    } else {
      switchTab("home");
    }
  }
}
window.addEventListener("hashchange", checkHash);

/* =========================================================================
 * Tab 1: Home (Default Top Page)
 * ========================================================================= */
$("home-cta-play").addEventListener("click", function () { switchTab("play"); });
$("home-cta-users").addEventListener("click", function () { switchTab("users"); });
$("home-cta-account").addEventListener("click", function () { switchTab("account"); });

/* =========================================================================
 * Tab 7: Account & Authentication
 * ========================================================================= */
var currentUser = null;

function updateAuthPill() {
  var pill = $("nav-auth-pill");
  if (!pill) return;
  clear(pill);
  if (currentUser && currentUser.handle) {
    var av = createAvatarEl(currentUser.avatar_url, currentUser.handle, 20);
    pill.appendChild(av);
    var sp = el("span", " @" + currentUser.handle);
    sp.style.marginLeft = "4px";
    pill.appendChild(sp);
    pill.title = "Signed in as @" + currentUser.handle + ". Click to view Account.";
  } else {
    pill.textContent = "Guest (Not Signed In)";
    pill.title = "Not signed in. Click to sign in.";
  }
}

function renderGuestState() {
  if ($("account-guest-card")) $("account-guest-card").style.display = "block";
  if ($("account-profile-card")) $("account-profile-card").style.display = "none";
  if ($("robots-guest-card")) $("robots-guest-card").style.display = "block";
  if ($("robots-owner-card")) $("robots-owner-card").style.display = "none";

  if ($("btn-web-create-table")) {
    $("btn-web-create-table").disabled = true;
    $("btn-web-create-table").title = "Sign in to play online";
  }
  if ($("btn-web-create-table-submit")) {
    $("btn-web-create-table-submit").disabled = true;
    $("btn-web-create-table-submit").title = "Sign in to play online";
  }
  if ($("web-chat-input")) {
    $("web-chat-input").disabled = true;
    $("web-chat-input").placeholder = "Sign in to chat (observing as anonymous-127.0.0.1)";
  }
  if ($("btn-web-chat-send")) {
    $("btn-web-chat-send").disabled = true;
  }
}

function renderAuthState() {
  if (!currentUser) return;
  if ($("account-guest-card")) $("account-guest-card").style.display = "none";
  if ($("account-profile-card")) $("account-profile-card").style.display = "block";
  if ($("robots-guest-card")) $("robots-guest-card").style.display = "none";
  if ($("robots-owner-card")) $("robots-owner-card").style.display = "block";

  $("web-handle-input").value = currentUser.handle || "";
  $("web-display-name-input").value = currentUser.display_name || "";
  $("web-location-input").value = currentUser.location || "";
  $("web-avatar-url").value = currentUser.avatar_url || "";
  $("web-bio-input").value = currentUser.bio || "";

  var prev = $("web-avatar-preview");
  clear(prev);
  prev.appendChild(createAvatarEl(currentUser.avatar_url, currentUser.handle, 96));

  if ($("btn-web-create-table")) {
    $("btn-web-create-table").disabled = false;
    $("btn-web-create-table").title = "";
  }
  if ($("btn-web-create-table-submit")) {
    $("btn-web-create-table-submit").disabled = false;
    $("btn-web-create-table-submit").title = "";
  }
  if ($("web-chat-input")) {
    $("web-chat-input").disabled = false;
    $("web-chat-input").placeholder = "Type message and press Enter...";
  }
  if ($("btn-web-chat-send")) {
    $("btn-web-chat-send").disabled = false;
  }
}

function loadMyProfile() {
  var token = null;
  try { token = localStorage.getItem("octi_jwt_token"); } catch (e) {}
  if (!token) {
    currentUser = null;
    updateAuthPill();
    renderGuestState();
    return Promise.resolve(null);
  }
  return api("/api/v1/profile/me").then(function (res) {
    currentUser = res.user || res;
    updateAuthPill();
    renderAuthState();
    loadIdentities();
    loadMyRobots();
    return currentUser;
  }).catch(function (err) {
    try { localStorage.removeItem("octi_jwt_token"); } catch (e) {}
    currentUser = null;
    updateAuthPill();
    renderGuestState();
    throw err;
  });
}

$("btn-web-google").addEventListener("click", function () {
  window.location.href = "/api/v1/auth/authorize?provider=google&redirect_uri=" + encodeURIComponent("/#account");
});

$("btn-web-facebook").addEventListener("click", function () {
  window.location.href = "/api/v1/auth/authorize?provider=facebook&redirect_uri=" + encodeURIComponent("/#account");
});

$("btn-robots-to-account").addEventListener("click", function () {
  switchTab("account");
});

function getAuthHeaders() {
  var headers = { "Accept": "application/json" };
  var token = null;
  try { token = localStorage.getItem("octi_jwt_token"); } catch (e) {}
  if (token) {
    headers["Authorization"] = "Bearer " + token;
  }
  return headers;
}

$("btn-web-logout").addEventListener("click", function () {
  fetch(`${API_BASE}/auth/logout`, { method: "POST", headers: getAuthHeaders() }).catch(function () {});
  fetch(`${API_BASE}/api/v1/auth/logout`, { method: "POST", headers: getAuthHeaders() }).catch(function () {});
  try { localStorage.removeItem("octi_jwt_token"); } catch (e) {}
  currentUser = null;
  loadMyProfile();
  switchTab("account");
});

$("web-avatar-file").addEventListener("change", function (ev) {
  var file = ev.target.files && ev.target.files[0];
  if (!file) return;
  var statusEl = $("web-profile-status");
  if (statusEl) {
    statusEl.textContent = "Processing image...";
    statusEl.style.color = "var(--dim)";
  }
  var reader = new FileReader();
  reader.onload = function (re) {
    var img = new Image();
    img.onload = function () {
      var minDim = Math.min(img.width, img.height);
      var sx = (img.width - minDim) / 2;
      var sy = (img.height - minDim) / 2;
      var sizes = [192, 128, 96];
      var jpegQualities = [0.85, 0.70, 0.55];
      var bestDataUri = null;

      for (var s = 0; s < sizes.length && !bestDataUri; ++s) {
        var targetSize = sizes[s];
        var canvas = document.createElement("canvas");
        canvas.width = targetSize;
        canvas.height = targetSize;
        var ctx = canvas.getContext("2d");
        ctx.imageSmoothingEnabled = true;
        ctx.imageSmoothingQuality = "high";
        ctx.drawImage(img, sx, sy, minDim, minDim, 0, 0, targetSize, targetSize);

        // Try PNG first (lossless)
        var pngUri = canvas.toDataURL("image/png");
        if (pngUri && pngUri.length <= 200000) {
          bestDataUri = pngUri;
          break;
        }

        // Fall back to JPEG at various qualities
        for (var q = 0; q < jpegQualities.length; ++q) {
          var uri = canvas.toDataURL("image/jpeg", jpegQualities[q]);
          if (uri && uri.length <= 200000) {
            bestDataUri = uri;
            break;
          }
        }
      }

      if (!bestDataUri) {
        var canvas = document.createElement("canvas");
        canvas.width = 96;
        canvas.height = 96;
        var ctx = canvas.getContext("2d");
        ctx.imageSmoothingEnabled = true;
        ctx.imageSmoothingQuality = "high";
        ctx.drawImage(img, sx, sy, minDim, minDim, 0, 0, 96, 96);
        bestDataUri = canvas.toDataURL("image/jpeg", 0.5);
      }

      $("web-avatar-url").value = bestDataUri;
      var prev = $("web-avatar-preview");
      clear(prev);
      prev.appendChild(createAvatarEl(bestDataUri, $("web-handle-input").value || "U", 96));
      if (statusEl) {
        statusEl.textContent = "Avatar processed and ready to save (" + Math.round(bestDataUri.length * 3 / 4 / 1024) + " KB). Click Save Profile to apply.";
        statusEl.style.color = "#10b981";
      }
    };
    img.onerror = function () {
      if (statusEl) {
        statusEl.textContent = "Failed to load selected image file.";
        statusEl.style.color = "#ef4444";
      }
    };
    img.src = re.target.result;
  };
  reader.onerror = function () {
    if (statusEl) {
      statusEl.textContent = "Failed to read image file.";
      statusEl.style.color = "#ef4444";
    }
  };
  reader.readAsDataURL(file);
});

var pendingConfirmFn = null;

function closeConfirmModal() {
  var modal = $("web-confirm-modal");
  if (modal) modal.style.display = "none";
  pendingConfirmFn = null;
}

function showConfirmModal(title, message, confirmText, onConfirm) {
  var modal = $("web-confirm-modal");
  if (!modal) {
    if (typeof onConfirm === "function") onConfirm();
    return;
  }
  if ($("web-confirm-title")) $("web-confirm-title").textContent = title || "Confirm Action";
  if ($("web-confirm-message")) $("web-confirm-message").textContent = message || "Are you sure?";
  if ($("btn-web-confirm-ok")) $("btn-web-confirm-ok").textContent = confirmText || "Confirm";
  pendingConfirmFn = onConfirm || null;
  modal.style.display = "flex";
}

if ($("btn-web-confirm-cancel")) {
  $("btn-web-confirm-cancel").addEventListener("click", closeConfirmModal);
}
if ($("btn-web-confirm-ok")) {
  $("btn-web-confirm-ok").addEventListener("click", function () {
    var fn = pendingConfirmFn;
    closeConfirmModal();
    if (typeof fn === "function") fn();
  });
}

$("btn-web-clear-avatar").addEventListener("click", function () {
  showConfirmModal(
    "Clear Avatar?",
    "Are you sure you want to clear your avatar? You will still need to click Save Profile to apply.",
    "Clear Avatar",
    function () {
      $("web-avatar-url").value = "";
      $("web-avatar-file").value = "";
      var prev = $("web-avatar-preview");
      clear(prev);
      prev.appendChild(createAvatarEl("", $("web-handle-input").value || "U", 96));
      var statusEl = $("web-profile-status");
      if (statusEl) {
        statusEl.textContent = "Avatar cleared. Click Save Profile to apply.";
        statusEl.style.color = "var(--dim)";
      }
    }
  );
});

$("btn-web-save-profile").addEventListener("click", function () {
  var statusEl = $("web-profile-status");
  var handleVal = $("web-handle-input").value.trim();
  if (handleVal && !/^[A-Za-z0-9_.-]{3,32}$/.test(handleVal)) {
    var msg = "Invalid handle. Handle must be 3-32 characters and contain only letters, numbers, underscores, dots, or hyphens.";
    if (statusEl) {
      statusEl.textContent = msg;
      statusEl.style.color = "#ef4444";
    }
    showToast(msg, "error");
    return;
  }

  var payload = {
    handle: handleVal,
    display_name: $("web-display-name-input").value.trim(),
    location: $("web-location-input").value.trim(),
    avatar_url: $("web-avatar-url").value.trim(),
    bio: $("web-bio-input").value.trim()
  };

  showConfirmModal(
    "Apply Profile Settings?",
    "Are you sure you want to save and apply these changes to your profile?",
    "Save Changes",
    function () {
      if (statusEl) {
        statusEl.textContent = "Saving profile...";
        statusEl.style.color = "var(--dim)";
      }

      api("/api/v1/profile/me", { method: "PATCH", body: payload }).then(function (res) {
        var updated = res.user || res;
        if (currentUser) {
          currentUser.handle = (updated.handle !== undefined) ? updated.handle : payload.handle;
          currentUser.display_name = (updated.display_name !== undefined) ? updated.display_name : payload.display_name;
          currentUser.location = (updated.location !== undefined) ? updated.location : payload.location;
          currentUser.avatar_url = (updated.avatar_url !== undefined) ? updated.avatar_url : payload.avatar_url;
          currentUser.bio = (updated.bio !== undefined) ? updated.bio : payload.bio;
          try {
            localStorage.setItem("octi_current_user", JSON.stringify(currentUser));
          } catch (e) {}
        } else {
          currentUser = updated;
        }
        updateAuthPill();
        renderAuthState();
        if (typeof loadLeaderboard === "function") { loadLeaderboard(); }
        if (typeof loadGames === "function") { loadGames(); }
        if (typeof lbLoad === "function") { lbLoad(); }
        if (typeof arSearch === "function") { arSearch(0); }
        if (typeof usersSearch === "function" && usersLoaded) { usersSearch(usersOffset || 0); }
        if (statusEl) {
          statusEl.textContent = "Profile saved.";
          statusEl.style.color = "#10b981";
        }
        showToast("Profile saved.", "success");
      }).catch(function (e) {
        var errMsg = e.message || "Failed to save profile.";
        if (statusEl) {
          statusEl.textContent = errMsg;
          statusEl.style.color = "#ef4444";
        }
        showToast(errMsg, "error");
      });
    }
  );
});

$("btn-web-view-public-profile").addEventListener("click", function () {
  if (currentUser) {
    switchTab("users");
    openUserProfile(currentUser.uuid || currentUser.user_id || currentUser.handle);
  }
});

function refreshAccountIfIdle() {
  var accTab = $("tab-account");
  if (!accTab || !accTab.classList.contains("on")) {
    return;
  }
  var active = document.activeElement;
  var editing = active && (
    active.id === "web-handle-input" ||
    active.id === "web-display-name-input" ||
    active.id === "web-location-input" ||
    active.id === "web-bio-input"
  );
  if (!editing && (currentUser || localStorage.getItem("octi_jwt_token"))) {
    loadMyProfile();
  }
}

window.addEventListener("focus", function () {
  refreshAccountIfIdle();
});

document.addEventListener("visibilitychange", function () {
  if (document.visibilityState === "visible") {
    refreshAccountIfIdle();
  }
});

var serverAuthProviders = ["google"];

function loadAuthProviders() {
  return api("/api/v1/auth/providers").then(function (res) {
    var list = [];
    if (Array.isArray(res)) {
      list = res;
    } else if (res && Array.isArray(res.providers)) {
      list = res.providers;
    } else if (res && typeof res.providers === "object") {
      list = Object.keys(res.providers).filter(function (k) { return res.providers[k]; });
    }
    if (list.length > 0) {
      serverAuthProviders = list.map(function (x) {
        return (typeof x === "string" ? x : (x.name || x.id || x.provider || x.key || "")).toLowerCase();
      });
    }
    var fbBtn = $("btn-web-facebook");
    if (fbBtn) {
      fbBtn.style.display = (serverAuthProviders.indexOf("facebook") !== -1) ? "block" : "none";
    }
    return serverAuthProviders;
  }).catch(function () {
    var fbBtn = $("btn-web-facebook");
    if (fbBtn) {
      fbBtn.style.display = (serverAuthProviders.indexOf("facebook") !== -1) ? "block" : "none";
    }
    return serverAuthProviders;
  });
}

function loadIdentities() {
  loadAuthProviders().then(function (enabledProviders) {
    function renderList(ids) {
      var tbody = $("web-identities-tbody");
      if (!tbody) return;
      clear(tbody);
      var allProviders = [
        { key: "google", name: "Google" },
        { key: "facebook", name: "Facebook" }
      ];
      var activeProviders = enabledProviders || serverAuthProviders;
      var providers = allProviders.filter(function (p) {
        var isLinked = ids.some(function (x) { return (x.provider || "").toLowerCase() === p.key; });
        var isEnabled = activeProviders.indexOf(p.key) !== -1;
        return isLinked || isEnabled;
      });
      providers.forEach(function (p) {
        var tr = el("tr");
        var matched = ids.find(function (x) { return (x.provider || "").toLowerCase() === p.key; });
        tr.appendChild(el("td", p.name));
        if (matched) {
          var emailHint = matched.provider_email || matched.email || matched.display_hint || "Linked";
          tr.appendChild(el("td", "Linked (" + emailHint + ")"));
          var actTd = el("td");
          var unlinkBtn = el("button", "Unlink", "btn");
          unlinkBtn.disabled = (ids.length <= 1);
          if (unlinkBtn.disabled) { unlinkBtn.title = "Cannot unlink the only active identity."; }
          unlinkBtn.addEventListener("click", function () {
            if (!confirm("Are you sure you want to unlink your " + p.name + " account?")) return;
            api("/api/v1/auth/identities/" + p.key, { method: "DELETE" }).then(function () {
              loadIdentities();
            }).catch(function (e) {
              var errData = e.data || {};
              var msg = errData.message || errData.error || e.message || "Failed to unlink identity.";
              alert("Unlink failed: " + msg);
            });
          });
          actTd.appendChild(unlinkBtn);
          tr.appendChild(actTd);
        } else {
          tr.appendChild(el("td", "Not Linked", "dim"));
          var actTd2 = el("td");
          var linkBtn = el("button", "Link " + p.name, "btn btn-primary");
          linkBtn.addEventListener("click", function () {
            window.location.href = "/api/v1/auth/authorize?provider=" + p.key + "&redirect_uri=" + encodeURIComponent("/#account");
          });
          actTd2.appendChild(linkBtn);
          tr.appendChild(actTd2);
        }
        tbody.appendChild(tr);
      });
    }

    api("/api/v1/auth/identities").then(function (res) {
      var ids = (res && res.identities && res.identities.length > 0)
        ? res.identities
        : ((currentUser && currentUser.linked_identities) || []);
      renderList(ids);
    }).catch(function () {
      var ids = (currentUser && currentUser.linked_identities) || [];
      renderList(ids);
    });
  });
}

$("btn-conflict-switch").addEventListener("click", function () {
  try { localStorage.removeItem("octi_jwt_token"); } catch (e) {}
  currentUser = null;
  loadMyProfile();
  $("web-link-conflict-box").style.display = "none";
});

$("btn-conflict-merge").addEventListener("click", function () {
  api("/api/v1/auth/link", { method: "POST", body: { force_relink: true } }).then(function () {
    $("web-link-conflict-box").style.display = "none";
    loadIdentities();
    alert("Account merged successfully!");
  }).catch(function (e) {
    alert("Merge failed: " + e.message);
  });
});

/* =========================================================================
 * Tab 6: Robot Registration & Token Management
 * ========================================================================= */
function loadMyRobots() {
  if (!currentUser) return;
  api("/api/v1/robots").then(function (res) {
    var robots = res.robots || [];
    var activeCount = 0;
    robots.forEach(function (r) {
      if (!r.revoked && (r.status || "ACTIVE").toUpperCase() === "ACTIVE") activeCount++;
    });
    var badge = $("robots-quota-badge");
    if (badge) {
      badge.textContent = "Active Robots: " + activeCount + " / 3";
      badge.className = "badge " + (activeCount >= 3 ? "badge-revoked" : "badge-active");
    }
    var createBtn = $("btn-create-robot");
    if (createBtn) {
      createBtn.disabled = (activeCount >= 3);
      createBtn.title = activeCount >= 3 ? "Maximum quota of 3 active robots reached" : "";
    }

    var tbody = $("robots-tbody");
    if (!tbody) return;
    clear(tbody);
    if (!robots.length) {
      var trEmpty = el("tr");
      var tdEmpty = el("td", "No robots registered yet.", "dim");
      tdEmpty.colSpan = 6;
      trEmpty.appendChild(tdEmpty);
      tbody.appendChild(trEmpty);
      return;
    }

    robots.forEach(function (r) {
      var tr = el("tr");
      var tdName = el("td", r.robot_name);
      tdName.style.fontWeight = "700";

      /* Description with inline editing */
      var tdDesc = el("td");
      var descSpan = el("span", r.description || "(no description)", r.description ? "" : "dim");
      var descInput = el("input");
      descInput.value = r.description || "";
      descInput.style.display = "none";
      descInput.style.width = "160px";
      var editBtn = el("button", "Edit", "btn");
      editBtn.style.marginLeft = "6px";
      editBtn.style.padding = "2px 8px";
      editBtn.style.fontSize = "12px";
      var saveBtn = el("button", "Save", "btn btn-primary");
      saveBtn.style.marginLeft = "6px";
      saveBtn.style.padding = "2px 8px";
      saveBtn.style.fontSize = "12px";
      saveBtn.style.display = "none";

      editBtn.addEventListener("click", function () {
        descSpan.style.display = "none";
        editBtn.style.display = "none";
        descInput.style.display = "inline-block";
        saveBtn.style.display = "inline-block";
        descInput.focus();
      });

      saveBtn.addEventListener("click", function () {
        var newDesc = descInput.value.trim();
        var rid = r.robot_uuid || r.robot_id || r.robot_name;
        api("/api/v1/robots/" + encodeURIComponent(rid), {
          method: "PATCH",
          body: { description: newDesc }
        }).then(function () {
          descSpan.textContent = newDesc || "(no description)";
          descSpan.className = newDesc ? "" : "dim";
          descSpan.style.display = "inline";
          editBtn.style.display = "inline-block";
          descInput.style.display = "none";
          saveBtn.style.display = "none";
        }).catch(function (e) { alert("Save failed: " + e.message); });
      });

      tdDesc.appendChild(descSpan);
      tdDesc.appendChild(descInput);
      tdDesc.appendChild(editBtn);
      tdDesc.appendChild(saveBtn);

      /* Secret Token with Show/Hide & Copy */
      var tdTok = el("td");
      var tokMasked = true;
      var tokSpan = el("span", "••••••••••••••••");
      tokSpan.style.fontFamily = "ui-monospace,SFMono-Regular,monospace";
      tokSpan.style.fontSize = "12px";
      var toggleBtn = el("button", "Show", "btn");
      toggleBtn.style.marginLeft = "6px";
      toggleBtn.style.padding = "2px 8px";
      toggleBtn.style.fontSize = "12px";
      var copyBtn = el("button", "Copy", "btn");
      copyBtn.style.marginLeft = "4px";
      copyBtn.style.padding = "2px 8px";
      copyBtn.style.fontSize = "12px";

      toggleBtn.addEventListener("click", function () {
        tokMasked = !tokMasked;
        tokSpan.textContent = tokMasked ? "••••••••••••••••" : (r.robot_token || r.token || r.secret_token || "octi_bot_hidden");
        toggleBtn.textContent = tokMasked ? "Show" : "Hide";
      });

      copyBtn.addEventListener("click", function () {
        var val = r.robot_token || r.token || r.secret_token || "";
        if (navigator.clipboard && navigator.clipboard.writeText) {
          navigator.clipboard.writeText(val).then(function () {
            copyBtn.textContent = "Copied!";
            setTimeout(function () { copyBtn.textContent = "Copy"; }, 1500);
          });
        }
      });

      tdTok.appendChild(tokSpan);
      tdTok.appendChild(toggleBtn);
      tdTok.appendChild(copyBtn);

      /* Status */
      var isAct = !r.revoked && ((r.status || "ACTIVE").toUpperCase() === "ACTIVE");
      var tdStat = el("td");
      var statBadge = el("span", isAct ? "ACTIVE" : "REVOKED", isAct ? "badge badge-active" : "badge badge-revoked");
      tdStat.appendChild(statBadge);

      /* Created */
      var tdCreated = el("td", fmtTime(r.created_at));

      /* Revoke Action */
      var tdAct = el("td");
      if (isAct) {
        var revokeBtn = el("button", "Revoke", "btn");
        revokeBtn.style.padding = "4px 10px";
        revokeBtn.style.fontSize = "12px";
        revokeBtn.addEventListener("click", function () {
          if (!confirm("Are you sure you want to revoke robot " + r.robot_name + "? This cannot be undone.")) return;
          var rid = r.robot_uuid || r.robot_id || r.robot_name;
          api("/api/v1/robots/" + encodeURIComponent(rid), { method: "DELETE" }).then(function () {
            loadMyRobots();
          }).catch(function (e) { alert("Revocation failed: " + e.message); });
        });
        tdAct.appendChild(revokeBtn);
      } else {
        tdAct.textContent = "-";
      }

      [tdName, tdDesc, tdTok, tdStat, tdCreated, tdAct].forEach(function (td) { tr.appendChild(td); });
      tbody.appendChild(tr);
    });
  }).catch(function () {});
}

$("btn-create-robot").addEventListener("click", function () {
  var name = $("robot-name-input").value.trim();
  var desc = $("robot-desc-input").value.trim();
  if (!name || name.length < 3 || name.length > 24) {
    alert("Robot name must be between 3 and 24 characters.");
    return;
  }
  if (!/^[A-Za-z0-9_-]+$/.test(name)) {
    alert("Robot name can only contain letters, numbers, hyphens, and underscores.");
    return;
  }
  api("/api/v1/robots", {
    method: "POST",
    body: { robot_name: name, description: desc }
  }).then(function (res) {
    $("robot-name-input").value = "";
    $("robot-desc-input").value = "";
    var box = $("robot-new-token-box");
    if (box) {
      clear(box);
      box.style.display = "block";
      var rawTok = res.robot_token || res.token || res.secret_token || "";
      var tSpan = el("div");
      tSpan.innerHTML = "<strong>Robot Created!</strong> Save your secret bearer token now: <code style='font-size:13px;background:#fff;padding:2px 6px;border-radius:4px;'>" +
        escapeHtml(rawTok) + "</code>";
      var cp = el("button", "Copy Token", "btn btn-primary");
      cp.style.marginTop = "8px";
      cp.addEventListener("click", function () {
        if (navigator.clipboard) {
          navigator.clipboard.writeText(rawTok);
          cp.textContent = "Copied!";
          setTimeout(function () { cp.textContent = "Copy Token"; }, 1500);
        }
      });
      box.appendChild(tSpan);
      box.appendChild(cp);
    }
    loadMyRobots();
  }).catch(function (e) { alert("Failed to create robot: " + e.message); });
});

/* =========================================================================
 * Tab 3: Users (Directory & Profiles)
 * ========================================================================= */
var usersOffset = 0, USERS_PAGE_LIMIT = 50, usersTotal = 0, usersLoaded = false;

function usersSearch(offset) {
  usersLoaded = true;
  usersOffset = Math.max(0, offset);
  var q = $("users-search-input").value.trim();
  var sort = $("users-sort-select").value;
  var order = $("users-order-select").value;
  var url = "/api/v1/users?limit=" + USERS_PAGE_LIMIT + "&offset=" + usersOffset +
    "&sort=" + encodeURIComponent(sort) + "&order=" + encodeURIComponent(order);
  if (q) { url += "&q=" + encodeURIComponent(q); }

  api(url).then(function (res) {
    usersTotal = res.total !== undefined ? res.total : (res.users ? res.users.length : 0);
    var tbody = $("users-tbody");
    clear(tbody);
    var users = res.users || [];
    $("users-msg").textContent = users.length ? "" : "No registered players found matching query.";

    users.forEach(function (u) {
      var tr = el("tr", null, "click");
      var tdAv = el("td");
      tdAv.appendChild(createAvatarEl(u.avatar_url, u.handle, 32));
      var tdHandle = el("td", "@" + u.handle);
      tdHandle.style.fontWeight = "700";
      var tdName = el("td", u.display_name || "-");
      var tdLoc = el("td", u.location || "-");
      var bioRaw = (u.bio || "").trim();
      var bioDisplay = bioRaw ? (bioRaw.length > 60 ? bioRaw.substring(0, 57) + "..." : bioRaw) : "—";
      var tdBio = el("td", bioDisplay);
      if (bioRaw) { tdBio.title = bioRaw; }
      var tdElo2p = el("td", u.elo_2p !== undefined ? u.elo_2p : 1200);
      var tdElo4p = el("td", u.elo_4p !== undefined ? u.elo_4p : 1200);
      var tdEloOfk = el("td", u.elo_ofk !== undefined ? u.elo_ofk : 1200);
      var wld = (u.wins || 0) + "-" + (u.losses || 0) + "-" + (u.draws || 0);
      var tdGames = el("td", wld);
      var tdRobots = el("td", u.active_robots_count !== undefined ? u.active_robots_count : 0);
      var tdJoined = el("td", fmtTime(u.created_at));

      [tdAv, tdHandle, tdName, tdLoc, tdBio, tdElo2p, tdElo4p, tdEloOfk, tdGames, tdRobots, tdJoined].forEach(function (td) {
        tr.appendChild(td);
      });

      tr.addEventListener("click", function () {
        openUserProfile(u.uuid || u.user_id || u.handle);
      });
      tbody.appendChild(tr);
    });

    var last = Math.min(usersOffset + users.length, usersTotal);
    $("users-page-info").textContent = usersTotal ? (usersOffset + 1) + "-" + last + " of " + usersTotal : "";
    $("users-prev-btn").disabled = usersOffset <= 0;
    $("users-next-btn").disabled = usersOffset + USERS_PAGE_LIMIT >= usersTotal;
  }).catch(function (e) {
    $("users-msg").textContent = "Error: " + e.message;
    $("users-msg").className = "err";
  });
}

$("btn-users-search").addEventListener("click", function () { usersSearch(0); });
$("users-sort-select").addEventListener("change", function () { usersSearch(0); });
$("users-order-select").addEventListener("change", function () { usersSearch(0); });
$("users-prev-btn").addEventListener("click", function () { usersSearch(usersOffset - USERS_PAGE_LIMIT); });
$("users-next-btn").addEventListener("click", function () { usersSearch(usersOffset + USERS_PAGE_LIMIT); });
$("users-search-input").addEventListener("keydown", function (ev) {
  if (ev.key === "Enter") { usersSearch(0); }
});

function openUserProfile(id) {
  location.hash = "#users/" + encodeURIComponent(id);
  api("/api/v1/users/" + encodeURIComponent(id)).then(function (u) {
    var panel = $("user-profile-detail");
    panel.style.display = "block";
    panel.scrollIntoView({ behavior: "smooth" });

    var avBox = $("user-detail-avatar");
    clear(avBox);
    avBox.appendChild(createAvatarEl(u.avatar_url, u.handle, 96));

    $("user-detail-name").textContent = u.display_name || u.handle;
    $("user-detail-handle").textContent = "@" + u.handle;
    $("user-detail-location").textContent = u.location ? ("📍 " + u.location) : "";
    $("user-detail-bio").textContent = u.bio || "No bio written yet.";
    $("user-detail-meta").textContent = "Joined: " + fmtTime(u.created_at) +
      (u.last_seen_at ? (" • Last Seen: " + fmtTime(u.last_seen_at)) : "");

    var isSelf = currentUser && ((currentUser.uuid && currentUser.uuid === u.uuid) || currentUser.handle === u.handle);
    $("btn-user-edit-self").style.display = isSelf ? "inline-block" : "none";

    var statsBox = $("user-detail-stats");
    clear(statsBox);
    var statItems = [
      ["2P Elo", u.elo_2p !== undefined ? u.elo_2p : 1200],
      ["4P Elo", u.elo_4p !== undefined ? u.elo_4p : 1200],
      ["OFK Elo", u.elo_ofk !== undefined ? u.elo_ofk : 1200],
      ["Games", u.games_played || 0],
      ["Wins", u.wins || 0],
      ["Losses", u.losses || 0],
      ["Draws", u.draws || 0],
      ["Win Rate", (u.games_played ? ((u.wins / u.games_played) * 100).toFixed(1) + "%" : "0.0%")]
    ];
    statItems.forEach(function (it) {
      var box = el("div", null, "stat-box");
      box.appendChild(el("div", it[0], "lbl"));
      box.appendChild(el("div", String(it[1]), "val"));
      statsBox.appendChild(box);
    });

    var rBox = $("user-detail-robots");
    clear(rBox);
    var rList = u.robots || [];
    if (!rList.length) {
      rBox.textContent = "No active public robots.";
    } else {
      var rTable = el("table");
      rTable.innerHTML = "<thead><tr><th>Robot Name</th><th>Description</th><th>Created</th></tr></thead>";
      var rTbody = el("tbody");
      rList.forEach(function (rb) {
        var rtr = el("tr");
        rtr.appendChild(el("td", rb.robot_name));
        rtr.appendChild(el("td", rb.description || "-"));
        rtr.appendChild(el("td", fmtTime(rb.created_at)));
        rTbody.appendChild(rtr);
      });
      rTable.appendChild(rTbody);
      rBox.appendChild(rTable);
    }

    var gBox = $("user-detail-games");
    clear(gBox);
    var gList = u.recent_games || u.games || [];
    if (!gList.length) {
      gBox.textContent = "No match history available.";
    } else {
      var gTable = el("table");
      gTable.innerHTML = "<thead><tr><th>Outcome</th><th>Mode</th><th>Players</th><th>Result</th><th>Plies</th><th>Date</th><th>Action</th></tr></thead>";
      var gTbody = el("tbody");
      gList.forEach(function (gm) {
        var gtr = el("tr");
        var outStr = gm.player_outcome || gm.outcome || "";
        var isWin = (outStr === "WIN" || (gm.winner && gm.player_seat && gm.winner === gm.player_seat));
        var isLoss = (outStr === "LOSS" || (gm.winner && gm.winner !== "NONE" && gm.player_seat && gm.winner !== gm.player_seat));
        var badgeCls = isWin ? "badge badge-active" : (isLoss ? "badge badge-revoked" : "badge");
        var tdOut = el("td");
        tdOut.appendChild(el("span", outStr || (isWin ? "WIN" : (isLoss ? "LOSS" : "DRAW")), badgeCls));
        var tdMode = el("td", gm.mode || "2P");
        var pList = (gm.players && gm.players.length)
          ? gm.players.map(function (p) { return p.handle; }).join(" vs ")
          : [gm.p1_handle, gm.p2_handle, gm.p3_handle, gm.p4_handle].filter(Boolean).join(" vs ");
        var tdPl = el("td", pList || "-");
        var tdRes = el("td", (gm.status || gm.result || "") + (gm.termination ? (" (" + gm.termination + ")") : ""));
        var tdPlies = el("td", gm.ply_count !== undefined ? gm.ply_count : "-");
        var tdDate = el("td", fmtTime(gm.ended_at || gm.created_at));
        var tdAct = el("td");
        var rpBtn = el("button", "View Replay", "btn");
        rpBtn.style.padding = "3px 8px";
        rpBtn.style.fontSize = "12px";
        rpBtn.addEventListener("click", function () {
          switchTab("archive");
          arOpen(gm.game_id);
        });
        tdAct.appendChild(rpBtn);

        [tdOut, tdMode, tdPl, tdRes, tdPlies, tdDate, tdAct].forEach(function (td) { gtr.appendChild(td); });
        gTbody.appendChild(gtr);
      });
      gTable.appendChild(gTbody);
      gBox.appendChild(gTable);
    }
  }).catch(function (e) {
    alert("Failed to load user profile: " + e.message);
  });
}

$("btn-user-edit-self").addEventListener("click", function () {
  switchTab("account");
});
$("user-detail-close").addEventListener("click", function () {
  $("user-profile-detail").style.display = "none";
});

/* =========================================================================
 * Tab 2: Web Player (Local vs. JS Robot & Online Lobby)
 * ========================================================================= */
var localBotState = null;
var localGameMoves = [];
var localGamePlies = [];
var localGameInitialFen = "";
var localGameStartedAt = 0;
var localSelectedSq = null;
var localSelectedMove = null;
var localLegalDests = {};
var localActionMode = "move";
var localSelectedProngDir = "a";

$("subtab-play-local").addEventListener("click", function () {
  $("subtab-play-local").className = "btn btn-primary";
  $("subtab-play-online").className = "btn";
  $("play-local-panel").style.display = "block";
  $("play-online-panel").style.display = "none";
});

$("subtab-play-online").addEventListener("click", function () {
  $("subtab-play-online").className = "btn btn-primary";
  $("subtab-play-local").className = "btn";
  $("play-online-panel").style.display = "block";
  $("play-local-panel").style.display = "none";
  loadOnlineTables();
});

function initLocalBotGame() {
  var mode = $("web-play-mode").value;
  var humanSide = $("web-human-side").value;
  var fen = "";
  if (mode === "OFK") {
    fen = "OFK:4:0:0:P:A:-:0:0,0,12,0:0,0,12,0:A22,A32,A42,A52,B26,B36,B46,B56";
  } else if (mode === "2P_1BASE") {
    fen = "2P:1:0:0:P:A:-:0:6,0,25,0:6,0,25,0:A53,B57";
  } else if (mode === "4P") {
    fen = "4P:1:0:0:P:A:-:0:3,0,15,0:3,0,15,0:3,0,15,0:3,0,15,0:A33,B37,C77,D73";
  } else {
    fen = "2P:3:0:0:P:A:-:0:4,0,25,0:4,0,25,0:A33,A53,A73,B37,B57,B77";
  }
  localBotState = parseOctiFen(fen);
  localGameInitialFen = fen;
  localGameStartedAt = Math.floor(Date.now() / 1000);
  localGameMoves = [];
  localGamePlies = [{
    plyIndex: 0,
    player: "-",
    move: "(Initial Setup)",
    fenAfter: fen
  }];
  localSelectedSq = null;
  localSelectedMove = null;
  localLegalDests = {};
  $("web-selected-move-text").textContent = "Selected: (None)";
  $("btn-web-commit-move").disabled = true;
  $("web-play-archive-status").textContent = "";

  var movesBox = $("web-play-moves");
  clear(movesBox);
  movesBox.appendChild(el("div", "Game initialized. Mode: " + mode));

  updatePlayHud();
  renderSvgBoard();
  checkPlayTurn();
}

function updatePlayHud() {
  var container = $("web-play-reserves");
  if (!container || !localBotState) return;
  clear(container);
  var names = {
    A: $("web-human-side").value === "A" ? (currentUser ? currentUser.handle : "Human (Red)") : "JS Robot A",
    B: $("web-human-side").value === "B" ? (currentUser ? currentUser.handle : "Human (Green)") : "JS Robot B",
    C: "JS Robot C",
    D: "JS Robot D"
  };
  localBotState.reserves.forEach(function (r) {
    var card = el("div", null, "hud-card" + (localBotState.turn === r.player ? " active-turn" : ""));
    var left = el("div", null, "hud-left");
    left.appendChild(el("span", r.player, "pod-dot " + r.player));
    left.appendChild(el("span", names[r.player] || ("Player " + r.player)));

    var metrics = el("div", null, "hud-metrics");
    function addM(lbl, val) {
      var s = el("span", lbl + ": ");
      s.appendChild(el("strong", val));
      metrics.appendChild(s);
    }
    addM("Pods", r.freePods);
    addM("Cap", r.capturedPods);
    addM("Prongs", r.freeProngs);
    card.appendChild(left);
    card.appendChild(metrics);
    container.appendChild(card);
  });
}

function checkPlayTurn() {
  if (!localBotState) return;
  var humanSide = $("web-human-side").value;
  var statusBanner = $("web-play-status");

  if (localBotState.status === "O" || localBotState.status === "R") {
    var winStr = localBotState.status === "R" ? "Game Over - Resigned" :
      (localBotState.winner === humanSide ? "Victory! You won the match!" : ("Game Over - Player " + localBotState.winner + " won"));
    statusBanner.textContent = winStr;
    statusBanner.style.background = (localBotState.winner === humanSide ? "#ecfdf5" : "#fff1f2");
    uploadLocalGameArchive();
    return;
  }

  if (localBotState.turn === humanSide) {
    statusBanner.textContent = "Your Turn (" + (humanSide === "A" ? "Red" : "Green") + ")";
    statusBanner.style.background = "#eff6ff";
  } else {
    statusBanner.textContent = "Bot is thinking (" + localBotState.turn + ")...";
    statusBanner.style.background = "#f4f4f5";
    triggerBotTurn();
  }
}

function triggerBotTurn() {
  var fen = serializeOctiFen(localBotState);
  var diff = parseInt($("web-bot-difficulty").value || "2", 10);
  var side = localBotState.turn;
  var mode = localBotState.mode;

  function onBotResult(res) {
    if (!localBotState || localBotState.status === "O") return;
    if (res.bestMove) {
      applyMoveToState(localBotState, res.bestMove, side);
      localGameMoves.push(res.bestMove);
      var plyIdx = localGamePlies.length;
      localGamePlies.push({
        plyIndex: plyIdx,
        player: side,
        move: res.bestMove,
        fenAfter: serializeOctiFen(localBotState),
        ai: { depthReached: diff, nodesVisited: res.nodes, evalScore: res.score }
      });
      var movesBox = $("web-play-moves");
      var moveDiv = el("div", "#" + plyIdx + " [" + side + "] " + res.bestMove + " (" + (res.elapsedMs || 0) + "ms, " + (res.nodes || 0) + " nodes)");
      movesBox.appendChild(moveDiv);
      movesBox.scrollTop = movesBox.scrollHeight;
    }
    updatePlayHud();
    renderSvgBoard();
    checkPlayTurn();
  }

  if (botWorker) {
    botWorker.onmessage = function (e) {
      if (e.data && e.data.type === "result") {
        onBotResult(e.data);
      }
    };
    botWorker.postMessage({ type: "search", fen: fen, depth: diff, mode: mode, side: side });
  } else {
    setTimeout(function () {
      var t0 = Date.now();
      var r = botSearch(fen, diff, mode, side);
      r.elapsedMs = Date.now() - t0;
      onBotResult(r);
    }, 80);
  }
}

function uploadLocalGameArchive() {
  if (!localBotState || localBotState.archiveUploaded) return;
  localBotState.archiveUploaded = true;
  var humanSide = $("web-human-side").value;
  var diff = $("web-bot-difficulty").value;
  var diffName = diff === "1" ? "Easy" : (diff === "3" ? "Hard" : "Medium");
  var nowSec = Math.floor(Date.now() / 1000);
  var gId = "web-" + Date.now().toString(36) + "-" + Math.random().toString(36).substr(2, 6);

  var humanHandle = (currentUser && currentUser.handle) ? currentUser.handle : "Player";
  var robotHandle = "Browser JS Robot (" + diffName + ")";
  var p1Handle = (humanSide === "A") ? humanHandle : robotHandle;
  var p2Handle = (humanSide === "B") ? humanHandle : robotHandle;
  var p1Type = (humanSide === "A") ? "HUMAN" : "AI";
  var p2Type = (humanSide === "B") ? "HUMAN" : "AI";
  var winnerSeat = (localBotState.winner === "A") ? 1 : ((localBotState.winner === "B") ? 2 : 0);
  var termStr = (localBotState.status === "R") ? "RESIGNATION" : "BASE_OCCUPATION";
  var modeStr = (localBotState.mode === "2P_1BASE" ? "2P" : localBotState.mode) || "2P";
  var basesVal = (localBotState.bases === 1 || localBotState.bases === 3) ? localBotState.bases : 1;
  var finalFenStr = serializeOctiFen(localBotState);
  var installId = getWebInstallId();

  var movesList = [];
  var pliesList = [];
  for (var i = 1; i < localGamePlies.length; i++) {
    var p = localGamePlies[i];
    movesList.push(p.move);
    var plyObj = {
      ply: i,
      player: p.player,
      move: p.move
    };
    if (p.ai) {
      plyObj.ai = {
        difficulty: p.ai.depthReached || parseInt(diff, 10),
        depthReached: p.ai.depthReached || 0,
        nodesVisited: p.ai.nodesVisited || 0
      };
    }
    pliesList.push(plyObj);
  }

  if (pliesList.length === 0) {
    $("web-play-archive-status").textContent = "Game ended before any moves were recorded.";
    return;
  }

  var playersList = [
    {
      seat: 1,
      player: "A",
      type: p1Type,
      handle: p1Handle,
      aiDifficulty: (humanSide === "A" ? 0 : parseInt(diff, 10))
    },
    {
      seat: 2,
      player: "B",
      type: p2Type,
      handle: p2Handle,
      aiDifficulty: (humanSide === "B" ? 0 : parseInt(diff, 10))
    }
  ];

  var payload = {
    format: "octi.transcript.v3",
    schema: "octi.transcript.v3",
    version: 3,
    gameId: gId,
    game_id: gId,
    install_id: installId,
    source: "LOCAL",
    mode: modeStr,
    bases: basesVal,
    superProngs: false,
    edgeless: false,
    startPlayer: "A",
    status: (localBotState.status === "R" ? "RESIGNED" : "GAME_OVER"),
    winner: localBotState.winner || "NONE",
    termination: termStr,
    finalFen: finalFenStr,
    final_fen: finalFenStr,
    initial_fen: localGameInitialFen,
    client: {
      platform: "web",
      appVersion: "1.0.0",
      extended: {
        startedAt: localGameStartedAt,
        endedAt: nowSec,
        installId: installId
      }
    },
    game: {
      game_id: gId,
      mode: modeStr,
      board_size: (modeStr === "OFK" ? 6 : 9),
      edgeless: false,
      prong_cap: 4,
      pod_cap: 7,
      super_pods: false,
      jump_capture: false,
      started_at: localGameStartedAt,
      ended_at: nowSec,
      result: {
        status: "FINISHED",
        winner_seat: winnerSeat,
        termination: termStr
      },
      players: playersList
    },
    players: playersList,
    moves: movesList,
    plies: pliesList,
    started_at: localGameStartedAt,
    ended_at: nowSec
  };

  $("web-play-archive-status").innerHTML = "Uploading game transcript to archive...";
  api("/api/v1/archive/upload", { method: "POST", body: payload }).then(function () {
    $("web-play-archive-status").innerHTML = "Game uploaded to archive! <a href='#archive/" + encodeURIComponent(gId) + "' style='font-weight:700;color:var(--acc-bright);'>View in Game Archive &rarr;</a>";
  }).catch(function (e) {
    $("web-play-archive-status").textContent = "Archive upload notice: " + e.message;
  });
}

$("btn-web-new-bot-game").addEventListener("click", function () {
  initLocalBotGame();
});

$("btn-web-resign").addEventListener("click", function () {
  if (!localBotState || localBotState.status === "O") return;
  if (!confirm("Are you sure you want to resign this game?")) return;
  var humanSide = $("web-human-side").value;
  localBotState.status = "R";
  localBotState.winner = (humanSide === "A" ? "B" : "A");
  checkPlayTurn();
});

/* Action Mode Buttons */
$("btn-act-move").addEventListener("click", function () {
  localActionMode = "move";
  $("btn-act-move").className = "btn on";
  $("btn-act-prong").className = "btn";
  $("btn-act-place").className = "btn";
  $("prong-dir-picker").style.display = "none";
  localSelectedMove = null;
  $("web-selected-move-text").textContent = "Selected: (None)";
  $("btn-web-commit-move").disabled = true;
  renderSvgBoard();
});

$("btn-act-prong").addEventListener("click", function () {
  localActionMode = "prong";
  $("btn-act-move").className = "btn";
  $("btn-act-prong").className = "btn on";
  $("btn-act-place").className = "btn";
  $("prong-dir-picker").style.display = "flex";
  localSelectedMove = null;
  $("web-selected-move-text").textContent = "Selected: (None)";
  $("btn-web-commit-move").disabled = true;
  renderSvgBoard();
});

$("btn-act-place").addEventListener("click", function () {
  localActionMode = "place";
  $("btn-act-move").className = "btn";
  $("btn-act-prong").className = "btn";
  $("btn-act-place").className = "btn on";
  $("prong-dir-picker").style.display = "none";
  localSelectedMove = null;
  $("web-selected-move-text").textContent = "Selected: (None)";
  $("btn-web-commit-move").disabled = true;
  renderSvgBoard();
});

Array.prototype.forEach.call($("prong-dir-picker").querySelectorAll("button"), function (b) {
  b.addEventListener("click", function () {
    Array.prototype.forEach.call($("prong-dir-picker").querySelectorAll("button"), function (x) { x.classList.remove("on"); });
    b.classList.add("on");
    localSelectedProngDir = b.getAttribute("data-dir") || "a";
    if (localSelectedSq) {
      localSelectedMove = localSelectedSq.x + "" + localSelectedSq.y + localSelectedProngDir;
      $("web-selected-move-text").textContent = "Selected: " + localSelectedMove;
      $("btn-web-commit-move").disabled = false;
    }
  });
});

$("btn-web-commit-move").addEventListener("click", function () {
  if (!localSelectedMove || !localBotState) return;
  var humanSide = $("web-human-side").value;
  applyMoveToState(localBotState, localSelectedMove, humanSide);
  localGameMoves.push(localSelectedMove);
  var plyIdx = localGamePlies.length;
  localGamePlies.push({
    plyIndex: plyIdx,
    player: humanSide,
    move: localSelectedMove,
    fenAfter: serializeOctiFen(localBotState)
  });
  var movesBox = $("web-play-moves");
  var mDiv = el("div", "#" + plyIdx + " [" + humanSide + "] " + localSelectedMove);
  mDiv.style.fontWeight = "700";
  movesBox.appendChild(mDiv);
  movesBox.scrollTop = movesBox.scrollHeight;

  localSelectedMove = null;
  localSelectedSq = null;
  localLegalDests = {};
  $("web-selected-move-text").textContent = "Selected: (None)";
  $("btn-web-commit-move").disabled = true;

  updatePlayHud();
  renderSvgBoard();
  checkPlayTurn();
});

function onPlaySquareClick(x, y) {
  if (!localBotState || localBotState.status === "O" || localBotState.status === "R") return;
  var humanSide = $("web-human-side").value;
  if (localBotState.turn !== humanSide) return;

  var k = x + "," + y;
  var stack = localBotState.map[k] || [];
  var top = stack.length ? stack[stack.length - 1] : null;

  if (localActionMode === "place") {
    var homes = getHomeSquares(localBotState.mode, humanSide);
    var isHome = homes.some(function (h) { return h.x === x && h.y === y; });
    var pIdx = humanSide === "A" ? 0 : 1;
    var freePods = localBotState.reserves[pIdx] ? localBotState.reserves[pIdx].freePods : 0;
    if (!isHome || freePods <= 0 || stack.length > 0) return;
    localSelectedSq = { x: x, y: y };
    localSelectedMove = "(" + x + y + ")";
    $("web-selected-move-text").textContent = "Selected: Place Pod at (" + x + "," + y + ")";
    $("btn-web-commit-move").disabled = false;
    renderSvgBoard();
    return;
  }

  if (localActionMode === "prong") {
    if (!top || top.owner !== humanSide) return;
    var pIdx2 = humanSide === "A" ? 0 : 1;
    var freeProngs = localBotState.reserves[pIdx2] ? localBotState.reserves[pIdx2].freeProngs : 0;
    if (freeProngs <= 0) {
      alert("No free prongs left in reserve!");
      return;
    }
    localSelectedSq = { x: x, y: y };
    localSelectedMove = x + "" + y + localSelectedProngDir;
    $("web-selected-move-text").textContent = "Selected: Add Prong " + localSelectedProngDir + " at (" + x + "," + y + ")";
    $("btn-web-commit-move").disabled = false;
    renderSvgBoard();
    return;
  }

  /* Action Mode: Move / Jump */
  if (localLegalDests[k]) {
    localSelectedMove = localLegalDests[k];
    $("web-selected-move-text").textContent = "Selected: " + localSelectedMove;
    $("btn-web-commit-move").disabled = false;
    renderSvgBoard();
    return;
  }

  if (top && top.owner === humanSide) {
    localSelectedSq = { x: x, y: y };
    localLegalDests = {};
    localSelectedMove = null;
    $("btn-web-commit-move").disabled = true;
    $("web-selected-move-text").textContent = "Selected Pod (" + x + "," + y + ")";

    var capChecked = $("web-play-capture-toggle").checked;
    for (var d = 0; d < 8; d++) {
      if (top.cfg.prongs[d]) {
        var dx = DIR_DELTAS[d][0], dy = DIR_DELTAS[d][1];
        var sx = x + dx, sy = y + dy;
        if (sx >= 1 && sx <= localBotState.gw && sy >= 1 && sy <= localBotState.gh) {
          var stepK = sx + "," + sy;
          var stepStack = localBotState.map[stepK];
          if (!stepStack || !stepStack.length) {
            localLegalDests[stepK] = x + "" + y + "-" + sx + "" + sy;
          } else {
            var jx = sx + dx, jy = sy + dy;
            if (jx >= 1 && jx <= localBotState.gw && jy >= 1 && jy <= localBotState.gh) {
              var jumpK = jx + "," + jy;
              var jumpStack = localBotState.map[jumpK];
              if (!jumpStack || !jumpStack.length) {
                var jumpedTop = stepStack[stepStack.length - 1];
                var isCap = (jumpedTop.owner !== humanSide && capChecked);
                localLegalDests[jumpK] = x + "" + y + "-" + jx + "" + jy + (isCap ? "x" : "");
              }
            }
          }
        }
      }
    }
    renderSvgBoard();
  }
}

window.onPlaySquareClick = onPlaySquareClick;

function renderSvgBoard() {
  var svg = $("web-play-svg");
  if (!svg || !localBotState) return;
  clear(svg);

  var gw = localBotState.gw, gh = localBotState.gh;
  var pad = 36;
  var cell = Math.min((580 - pad * 2) / gw, (580 - pad * 2) / gh);
  var gridW = gw * cell, gridH = gh * cell;
  var originX = (580 - gridW) / 2, originY = (580 - gridH) / 2;

  /* Board Frame */
  var bgRect = document.createElementNS("http://www.w3.org/2000/svg", "rect");
  bgRect.setAttribute("x", "0");
  bgRect.setAttribute("y", "0");
  bgRect.setAttribute("width", "580");
  bgRect.setAttribute("height", "580");
  bgRect.setAttribute("rx", "14");
  bgRect.setAttribute("fill", "#101622");
  bgRect.setAttribute("stroke", "#E4C070");
  bgRect.setAttribute("stroke-width", "3");
  svg.appendChild(bgRect);

  /* Coordinate labels */
  for (var cx = 1; cx <= gw; cx++) {
    var sqCol = cx - 1;
    var xMid = originX + (sqCol + 0.5) * cell;
    var tTop = document.createElementNS("http://www.w3.org/2000/svg", "text");
    tTop.setAttribute("x", xMid);
    tTop.setAttribute("y", originY - 10);
    tTop.setAttribute("fill", "#D8B66C");
    tTop.setAttribute("font-size", "11");
    tTop.setAttribute("font-weight", "bold");
    tTop.setAttribute("text-anchor", "middle");
    tTop.textContent = String(cx);
    svg.appendChild(tTop);

    var tBot = document.createElementNS("http://www.w3.org/2000/svg", "text");
    tBot.setAttribute("x", xMid);
    tBot.setAttribute("y", originY + gridH + 18);
    tBot.setAttribute("fill", "#D8B66C");
    tBot.setAttribute("font-size", "11");
    tBot.setAttribute("font-weight", "bold");
    tBot.setAttribute("text-anchor", "middle");
    tBot.textContent = String(cx);
    svg.appendChild(tBot);
  }

  for (var cy = 1; cy <= gh; cy++) {
    var sqRow = gh - cy;
    var yMid = originY + (sqRow + 0.5) * cell;
    var tLeft = document.createElementNS("http://www.w3.org/2000/svg", "text");
    tLeft.setAttribute("x", originX - 12);
    tLeft.setAttribute("y", yMid + 4);
    tLeft.setAttribute("fill", "#D8B66C");
    tLeft.setAttribute("font-size", "11");
    tLeft.setAttribute("font-weight", "bold");
    tLeft.setAttribute("text-anchor", "middle");
    tLeft.textContent = String(cy);
    svg.appendChild(tLeft);

    var tRight = document.createElementNS("http://www.w3.org/2000/svg", "text");
    tRight.setAttribute("x", originX + gridW + 12);
    tRight.setAttribute("y", yMid + 4);
    tRight.setAttribute("fill", "#D8B66C");
    tRight.setAttribute("font-size", "11");
    tRight.setAttribute("font-weight", "bold");
    tRight.setAttribute("text-anchor", "middle");
    tRight.textContent = String(cy);
    svg.appendChild(tRight);
  }

  /* Grid Squares */
  for (var y = 1; y <= gh; y++) {
    for (var x = 1; x <= gw; x++) {
      var col = x - 1, row = gh - y;
      var sqX = originX + col * cell;
      var sqY = originY + row * cell;
      var isDark = ((x + y) % 2 === 0);

      var cellRect = document.createElementNS("http://www.w3.org/2000/svg", "rect");
      cellRect.setAttribute("x", sqX);
      cellRect.setAttribute("y", sqY);
      cellRect.setAttribute("width", cell);
      cellRect.setAttribute("height", cell);
      cellRect.setAttribute("fill", isDark ? "#151E30" : "#1D2940");
      cellRect.setAttribute("stroke", "rgba(184, 148, 76, 0.4)");
      cellRect.setAttribute("stroke-width", "1");
      svg.appendChild(cellRect);

      /* Socket Ring */
      var socket = document.createElementNS("http://www.w3.org/2000/svg", "circle");
      socket.setAttribute("cx", sqX + cell / 2);
      socket.setAttribute("cy", sqY + cell / 2);
      socket.setAttribute("r", cell * 0.33);
      socket.setAttribute("fill", "none");
      socket.setAttribute("stroke", "rgba(8, 12, 20, 0.7)");
      socket.setAttribute("stroke-width", "1");
      svg.appendChild(socket);

      /* Home Base indicator */
      var st = squareTypeAt(localBotState.mode, x, y);
      if (st.indexOf("HOME") !== -1) {
        var hRect = document.createElementNS("http://www.w3.org/2000/svg", "rect");
        hRect.setAttribute("x", sqX + 2);
        hRect.setAttribute("y", sqY + 2);
        hRect.setAttribute("width", cell - 4);
        hRect.setAttribute("height", cell - 4);
        hRect.setAttribute("rx", "4");
        hRect.setAttribute("fill", "none");
        hRect.setAttribute("stroke", "#C8A253");
        hRect.setAttribute("stroke-width", "1.5");
        svg.appendChild(hRect);

        var stack = localBotState.map[x + "," + y];
        if (!stack || !stack.length) {
          var hTxt = document.createElementNS("http://www.w3.org/2000/svg", "text");
          hTxt.setAttribute("x", sqX + cell / 2);
          hTxt.setAttribute("y", sqY + cell / 2 + 3);
          hTxt.setAttribute("fill", "#D8B66C");
          hTxt.setAttribute("font-size", "9");
          hTxt.setAttribute("font-weight", "bold");
          hTxt.setAttribute("text-anchor", "middle");
          hTxt.textContent = "OCTI";
          svg.appendChild(hTxt);
        }
      }

      /* Highlight destination squares */
      var k = x + "," + y;
      if (localLegalDests[k]) {
        var dCircle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
        dCircle.setAttribute("cx", sqX + cell / 2);
        dCircle.setAttribute("cy", sqY + cell / 2);
        dCircle.setAttribute("r", cell * 0.28);
        dCircle.setAttribute("fill", "rgba(16, 185, 129, 0.35)");
        dCircle.setAttribute("stroke", "#10B981");
        dCircle.setAttribute("stroke-width", "2");
        svg.appendChild(dCircle);
      }

      /* Highlight selected square */
      if (localSelectedSq && localSelectedSq.x === x && localSelectedSq.y === y) {
        var sRect = document.createElementNS("http://www.w3.org/2000/svg", "rect");
        sRect.setAttribute("x", sqX + 1);
        sRect.setAttribute("y", sqY + 1);
        sRect.setAttribute("width", cell - 2);
        sRect.setAttribute("height", cell - 2);
        sRect.setAttribute("rx", "4");
        sRect.setAttribute("fill", "none");
        sRect.setAttribute("stroke", "#FDE047");
        sRect.setAttribute("stroke-width", "2.5");
        svg.appendChild(sRect);
      }
    }
  }

  /* Draw Pods */
  Object.keys(localBotState.map).forEach(function (mk) {
    var stack = localBotState.map[mk];
    if (!stack || !stack.length) return;
    var top = stack[stack.length - 1];
    var pcoords = mk.split(",");
    var px = parseInt(pcoords[0], 10), py = parseInt(pcoords[1], 10);
    var pcol = px - 1, prow = gh - py;
    var pcx = originX + (pcol + 0.5) * cell;
    var pcy = originY + (prow + 0.5) * cell;
    var radius = cell * 0.36;

    /* Radial Prongs */
    for (var d = 0; d < 8; d++) {
      if (top.cfg.prongs[d]) {
        var u = DIR_DELTAS[d];
        var ux = u[0], uy = -u[1];
        var ulen = Math.hypot(ux, uy);
        if (ulen > 1e-6) { ux /= ulen; uy /= ulen; }
        var perpX = -uy, perpY = ux;
        var rInner = radius * 0.65;
        var rOuter = radius * 1.45;
        var hw = radius * 0.22;

        var prongPoly = document.createElementNS("http://www.w3.org/2000/svg", "polygon");
        var pts = [
          (pcx + ux * rInner - perpX * hw) + "," + (pcy + uy * rInner - perpY * hw),
          (pcx + ux * (rOuter * 0.85) - perpX * (hw * 0.8)) + "," + (pcy + uy * (rOuter * 0.85) - perpY * (hw * 0.8)),
          (pcx + ux * rOuter) + "," + (pcy + uy * rOuter),
          (pcx + ux * (rOuter * 0.85) + perpX * (hw * 0.8)) + "," + (pcy + uy * (rOuter * 0.85) + perpY * (hw * 0.8)),
          (pcx + ux * rInner + perpX * hw) + "," + (pcy + uy * rInner + perpY * hw)
        ].join(" ");
        prongPoly.setAttribute("points", pts);
        prongPoly.setAttribute("fill", "#F8FAFC");
        prongPoly.setAttribute("stroke", "#18181b");
        prongPoly.setAttribute("stroke-width", "1");
        svg.appendChild(prongPoly);
      }
    }

    /* Octagon Pod Body */
    var podPoly = document.createElementNS("http://www.w3.org/2000/svg", "polygon");
    var polyPts = [];
    for (var i = 0; i < 8; i++) {
      var ang = (i * 45 + 22.5) * Math.PI / 180;
      polyPts.push((pcx + radius * Math.cos(ang)).toFixed(1) + "," + (pcy + radius * Math.sin(ang)).toFixed(1));
    }
    podPoly.setAttribute("points", polyPts.join(" "));
    var podColor = (top.owner === "A" ? "#10B981" : (top.owner === "B" ? "#F43F5E" : (top.owner === "C" ? "#3B82F6" : "#F59E0B")));
    podPoly.setAttribute("fill", podColor);
    podPoly.setAttribute("stroke", "#C8A253");
    podPoly.setAttribute("stroke-width", "1.5");
    svg.appendChild(podPoly);

    /* Center Emblem */
    var centerTxt = document.createElementNS("http://www.w3.org/2000/svg", "text");
    centerTxt.setAttribute("x", pcx);
    centerTxt.setAttribute("y", pcy + 4);
    centerTxt.setAttribute("fill", "#ffffff");
    centerTxt.setAttribute("font-size", String(Math.round(radius * 0.75)));
    centerTxt.setAttribute("font-weight", "bold");
    centerTxt.setAttribute("text-anchor", "middle");
    centerTxt.textContent = stack.length > 1 ? ("x" + stack.length) : top.owner;
    svg.appendChild(centerTxt);
  });

  /* Click Handler rects */
  for (var y2 = 1; y2 <= gh; y2++) {
    for (var x2 = 1; x2 <= gw; x2++) {
      var c2 = x2 - 1, r2 = gh - y2;
      var cx2 = originX + c2 * cell, cy2 = originY + r2 * cell;
      var clickRect = document.createElementNS("http://www.w3.org/2000/svg", "rect");
      clickRect.setAttribute("x", cx2);
      clickRect.setAttribute("y", cy2);
      clickRect.setAttribute("width", cell);
      clickRect.setAttribute("height", cell);
      clickRect.setAttribute("fill", "transparent");
      clickRect.setAttribute("cursor", "pointer");
      clickRect.setAttribute("onclick", "window.onPlaySquareClick(" + x2 + "," + y2 + ")");
      svg.appendChild(clickRect);
    }
  }
}

/* Online Lobby & Tables */
var currentMyTableRoomId = "";

function seatLetterOf(seat) {
  return seat === 1 ? "A" : (seat === 2 ? "B" : (seat === 3 ? "C" : (seat === 4 ? "D" : String(seat))));
}

function formatLobbyChatTime(ms) {
  var num = Number(ms || 0);
  if (!num || num < 100000000000) {
    return new Date().toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
  }
  return new Date(num).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
}

function renderLobbyChatMessages(chatList) {
  var log = $("web-lobby-chat-log");
  if (!log) return;
  clear(log);
  var arr = Array.isArray(chatList) ? chatList : [];
  if (!arr.length) {
    log.appendChild(el("div", "Connected to lobby chat. (Messages older than 6 hours are automatically purged.)", "dim"));
    return;
  }
  arr.forEach(function (m) {
    var ts = formatLobbyChatTime(m.created_at_ms || m.at_ms);
    var h = m.handle ? ("@" + m.handle) : "player";
    var line = el("div", "[" + ts + "] " + h + ": " + (m.text || ""));
    line.style.marginBottom = "4px";
    log.appendChild(line);
  });
  log.scrollTop = log.scrollHeight;
}

function formatTableRulesetSummary(t) {
  var parts = [t.mode || "2P"];
  if (t.bases) parts.push(t.bases + "-Base");
  if (t.super_prongs) parts.push("Superprongs");
  if (t.edgeless) parts.push("Edgeless");
  parts.push(t.rated ? "Rated" : "Casual");
  if (t.time_control && typeof t.time_control === "object") {
    if (t.time_control.enabled && t.time_control.initial_ms > 0) {
      var mins = Math.round(t.time_control.initial_ms / 60000);
      var inc = Math.round((t.time_control.increment_ms || 0) / 1000);
      parts.push(mins + "m+" + inc + "s");
    } else {
      parts.push("Untimed");
    }
  }
  return parts.join(" \u2022 ");
}

function isUserSeatedAtTable(t) {
  if (!currentUser) return false;
  var myUuid = currentUser.uuid || currentUser.user_id || "";
  var myHandle = currentUser.handle || "";
  if (Array.isArray(t.seats)) {
    for (var i = 0; i < t.seats.length; i++) {
      var s = t.seats[i];
      if ((myUuid && s.player_uuid === myUuid) || (myHandle && s.handle === myHandle)) {
        return true;
      }
    }
  }
  if (Array.isArray(t.players)) {
    for (var j = 0; j < t.players.length; j++) {
      var p = t.players[j];
      var ph = (typeof p === "string") ? p : (p && p.handle ? p.handle : "");
      if (myHandle && ph === myHandle) return true;
    }
  }
  return false;
}

function leaveTableById(roomId) {
  if (!roomId) return;
  api("/api/v1/tables/" + encodeURIComponent(roomId) + "/leave", { method: "POST" }).then(function () {
    if ($("web-online-notice")) {
      $("web-online-notice").textContent = "Left table " + roomId + ".";
    }
    refreshLobbyTables();
  }).catch(function (e) {
    alert("Failed to leave table: " + e.message);
  });
}

function refreshLobbyTables() {
  api("/api/v1/lobby/state").then(function (res) {
    if (res && Array.isArray(res.recent_chat)) {
      renderLobbyChatMessages(res.recent_chat);
    }
    var tables = (res && Array.isArray(res.tables)) ? res.tables : [];
    var tbody = $("web-lobby-tables-body");
    if (!tbody) return;
    clear(tbody);

    currentMyTableRoomId = "";
    var myTableObj = null;
    for (var ti = 0; ti < tables.length; ti++) {
      var tblItem = tables[ti];
      if (isUserSeatedAtTable(tblItem)) {
        currentMyTableRoomId = tblItem.room_id || tblItem.table_id || "";
        myTableObj = tblItem;
        break;
      }
    }

    var banner = $("web-my-table-banner");
    var bannerText = $("web-my-table-banner-text");
    if (banner && bannerText) {
      if (currentMyTableRoomId && myTableObj) {
        bannerText.textContent = "Seated at Table #" + currentMyTableRoomId + " (" + (myTableObj.phase || "WAITING_FOR_PLAYERS") + ")";
        banner.style.display = "flex";
      } else {
        banner.style.display = "none";
      }
    }

    if (!tables.length) {
      var emptyTr = el("tr");
      var emptyTd = el("td", "No active online tables found. Use Create New Table above to host one.", "dim");
      emptyTd.colSpan = 6;
      emptyTd.style.textAlign = "center";
      emptyTd.style.padding = "12px";
      emptyTr.appendChild(emptyTd);
      tbody.appendChild(emptyTr);
      return;
    }

    tables.forEach(function (t) {
      var roomId = t.room_id || t.table_id || "";
      var phase = t.phase || t.state || "WAITING_FOR_PLAYERS";
      var tr = el("tr");

      var tdHost = el("td");
      var idLine = el("div");
      var idStrong = el("strong", roomId);
      idLine.appendChild(idStrong);
      if (t.has_passcode) {
        var lockSpan = el("span", " \uD83D\uDD12 Passcode", "badge");
        lockSpan.style.marginLeft = "6px";
        idLine.appendChild(lockSpan);
      }
      tdHost.appendChild(idLine);
      var hostSub = el("div", "Host: @" + (t.host_handle || "unknown"), "dim");
      hostSub.style.fontSize = "12px";
      tdHost.appendChild(hostSub);
      tr.appendChild(tdHost);

      tr.appendChild(el("td", formatTableRulesetSummary(t)));

      var tdPhase = el("td");
      var phaseBadge = el("span", phase, phase === "IN_PROGRESS" ? "badge badge-online" : "badge badge-upload");
      tdPhase.appendChild(phaseBadge);
      tr.appendChild(tdPhase);

      var tdSeats = el("td");
      var seatsTotal = t.seats_total || (t.mode === "4P" ? 4 : 2);
      var seatsArr = Array.isArray(t.seats) && t.seats.length ? t.seats : [];
      var rosterWrap = el("div");
      rosterWrap.style.display = "flex";
      rosterWrap.style.flexWrap = "wrap";
      rosterWrap.style.gap = "6px";
      if (seatsArr.length) {
        seatsArr.forEach(function (s) {
          var sNum = s.seat || 1;
          var side = seatLetterOf(sNum);
          var occupied = !!(s.handle || s.player_uuid);
          var txt = "Seat " + sNum + " (" + side + "): " +
            (occupied ? ("@" + s.handle + (s.rating ? " (" + s.rating + ")" : "")) : "Open");
          var pill = el("span", txt, "badge");
          pill.style.background = occupied ? "#dbeafe" : "#f4f4f5";
          pill.style.color = occupied ? "#1e40af" : "#71717a";
          rosterWrap.appendChild(pill);
        });
      } else {
        var pls = Array.isArray(t.players) ? t.players : [];
        for (var si = 1; si <= seatsTotal; si++) {
          var sideL = seatLetterOf(si);
          var pItem = pls[si - 1];
          var pHandle = typeof pItem === "string" ? pItem : (pItem && pItem.handle ? pItem.handle : "");
          var pillTxt = "Seat " + si + " (" + sideL + "): " + (pHandle ? ("@" + pHandle) : "Open");
          var pillEl = el("span", pillTxt, "badge");
          rosterWrap.appendChild(pillEl);
        }
      }
      tdSeats.appendChild(rosterWrap);
      tr.appendChild(tdSeats);

      var obsCount = (t.observer_count !== undefined) ? t.observer_count : (t.observers_count || 0);
      tr.appendChild(el("td", String(obsCount)));

      var tdAct = el("td");
      var amSeatedHere = isUserSeatedAtTable(t);
      var seatsFilled = (t.seats_filled !== undefined) ? t.seats_filled : 0;

      if (currentUser && !amSeatedHere && phase === "WAITING_FOR_PLAYERS" && seatsFilled < seatsTotal) {
        var joinBtn = el("button", "Join Table", "btn btn-primary");
        joinBtn.style.marginRight = "6px";
        joinBtn.addEventListener("click", function () {
          var pass = "";
          if (t.has_passcode) {
            var prompted = window.prompt("Enter table passcode for " + roomId + ":", "");
            if (prompted === null) return;
            pass = prompted;
          }
          api("/api/v1/tables/" + encodeURIComponent(roomId) + "/join", {
            method: "POST",
            body: { seat: 0, passcode: pass }
          }).then(function () {
            if ($("web-online-notice")) {
              $("web-online-notice").textContent = "Joined table " + roomId + ".";
            }
            refreshLobbyTables();
          }).catch(function (e) { alert("Failed to join: " + e.message); });
        });
        tdAct.appendChild(joinBtn);
      }

      if (currentUser && amSeatedHere) {
        var leaveBtn = el("button", "Leave Table", "btn");
        leaveBtn.style.marginRight = "6px";
        leaveBtn.style.borderColor = "#dc2626";
        leaveBtn.style.color = "#dc2626";
        leaveBtn.addEventListener("click", function () {
          leaveTableById(roomId);
        });
        tdAct.appendChild(leaveBtn);
      }

      var obsBtn = el("button", "Observe", "btn");
      obsBtn.addEventListener("click", function () {
        api("/api/v1/tables/" + encodeURIComponent(roomId) + "/observe", { method: "POST" }).then(function (oRes) {
          $("web-room-observers").textContent = (oRes.observer_handles || t.observer_handles || ["anonymous-127.0.0.1"]).join(", ");
          var roomLog = $("web-room-chat-log");
          clear(roomLog);
          roomLog.appendChild(el("div", "Observing table " + roomId));
          refreshLobbyTables();
        }).catch(function (e) { alert("Failed to observe: " + e.message); });
      });
      tdAct.appendChild(obsBtn);

      tr.appendChild(tdAct);
      tbody.appendChild(tr);
    });
  }).catch(function () {});
}

var loadOnlineTables = refreshLobbyTables;

$("btn-web-refresh-tables").addEventListener("click", refreshLobbyTables);

if ($("btn-web-leave-my-table")) {
  $("btn-web-leave-my-table").addEventListener("click", function () {
    if (currentMyTableRoomId) {
      leaveTableById(currentMyTableRoomId);
    }
  });
}

function submitCreateOnlineTable() {
  if (!currentUser) return;
  var mode = $("web-create-mode") ? $("web-create-mode").value : "2P";
  var bases = $("web-create-bases") ? parseInt($("web-create-bases").value, 10) : 3;
  if (mode === "OFK") bases = 4;
  if (mode === "4P" && bases === 4) bases = 1;
  var superProngs = $("web-create-super-prongs") ? !!$("web-create-super-prongs").checked : false;
  var edgeless = $("web-create-edgeless") ? !!$("web-create-edgeless").checked : false;
  var rated = $("web-create-rated") ? !!$("web-create-rated").checked : true;
  var tcVal = $("web-create-tc") ? $("web-create-tc").value : "untimed";
  var passcode = $("web-create-passcode") ? $("web-create-passcode").value : "";

  var body = {
    mode: mode,
    bases: bases,
    super_prongs: superProngs,
    edgeless: edgeless,
    rated: rated,
    time_control: tcVal
  };
  if (passcode) {
    body.passcode = passcode;
  }
  api("/api/v1/tables", { method: "POST", body: body }).then(function (res) {
    if ($("web-create-passcode")) $("web-create-passcode").value = "";
    if ($("web-online-notice") && res && res.room_id) {
      $("web-online-notice").textContent = "Created table " + res.room_id + ".";
    }
    refreshLobbyTables();
  }).catch(function (e) { alert("Failed to create table: " + e.message); });
}

$("btn-web-create-table").addEventListener("click", submitCreateOnlineTable);
if ($("btn-web-create-table-submit")) {
  $("btn-web-create-table-submit").addEventListener("click", submitCreateOnlineTable);
}

$("btn-web-chat-send").addEventListener("click", function () {
  var input = $("web-chat-input");
  var msg = input.value.trim();
  if (!msg || !currentUser) return;
  input.value = "";
  api("/api/v1/lobby/chat", { method: "POST", body: { text: msg } }).then(function () {
    refreshLobbyTables();
  }).catch(function (e) { alert("Chat failed: " + e.message); });
});

$("web-chat-input").addEventListener("keydown", function (ev) {
  if (ev.key === "Enter") { $("btn-web-chat-send").click(); }
});


/* ---- archive ---- */
var arOffset = 0, arLimit = 20, arTotal = 0, arLoaded = false;
var rpPlies = [], rpIdx = 0, rpTimer = null, rpHandles = [];

function winnerHandleOf(g) {
  if (!g || !g.winner || g.winner === "NONE") return "";
  var targetSeat = g.winner === "A" ? 1 : (g.winner === "B" ? 2 : (g.winner === "C" ? 3 : (g.winner === "D" ? 4 : 0)));
  var pls = g.players || [];
  for (var i = 0; i < pls.length; i++) {
    if ((pls[i].player && pls[i].player === g.winner) || (targetSeat > 0 && pls[i].seat === targetSeat)) {
      return pls[i].handle || "";
    }
  }
  if (targetSeat > 0 && pls.length >= targetSeat && pls[targetSeat - 1]) {
    return pls[targetSeat - 1].handle || "";
  }
  return "";
}

function resultText(g) {
  var s = "";
  if (g.winner && g.winner !== "NONE") {
    var winHandle = winnerHandleOf(g);
    s += "Winner: " + g.winner + (winHandle ? " (" + winHandle + ")  " : "  ");
  }
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
      var winHandle = winnerHandleOf(g);
      var winText = (g.winner && g.winner !== "NONE")
        ? (g.winner + (winHandle ? " (" + winHandle + ")" : ""))
        : (g.winner || "NONE");
      var winBadge = el("span", winText,
        (g.winner === "A" || g.winner === "B" || g.winner === "C" || g.winner === "D")
          ? "badge badge-winner" : "badge");
      var playersStr = (g.players || []).map(function (p, idx) {
        var sNum = p.seat || (idx + 1);
        var side = p.player || seatLetterOf(sNum);
        return side + ": " + p.handle;
      }).join(" vs ");
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
      var wrap = $("rp-table").parentNode;
      if (wrap) {
        var rowTop = rows[i].offsetTop;
        var rowBot = rowTop + rows[i].offsetHeight;
        if (rowTop < wrap.scrollTop) {
          wrap.scrollTop = rowTop;
        } else if (rowBot > wrap.scrollTop + wrap.clientHeight) {
          wrap.scrollTop = rowBot - wrap.clientHeight;
        }
      }
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
    $("ar-info").textContent = (g.players || []).map(function (p, idx) {
      var sNum = p.seat || (idx + 1);
      var side = p.player || seatLetterOf(sNum);
      return "Seat " + sNum + " (" + side + "): " + p.handle;
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
  var delay = parseInt($("rp-speed").value || "1100", 10);
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


/* =========================================================================
 * Application Initialization
 * ========================================================================= */
var API_BASE = "";

async function loadHealth() {
  var pill = document.getElementById("server-health-pill") || document.getElementById("health");
  try {
    var res = await fetch(`${API_BASE}/api/v1/health`);
    if (!res.ok) throw new Error("Health HTTP " + res.status);
    var data = await res.json();
    if (pill) {
      pill.textContent = (data && data.status === "ok") ? "Server Online" : "Server Degraded";
      pill.style.color = "#10b981";
    }
  } catch (e) {
    if (pill) {
      pill.textContent = "Server Offline";
      pill.style.color = "#ef4444";
    }
  }
}

loadHealth();
loadArchiveStats();
checkHash();
loadMyProfile();
loadAuthProviders();
setInterval(loadHealth, 15000);
})();
</script>
</body>
</html>

)OCTIUI";

} // namespace

const char *webUiHtml()
{
    return kIndexHtml;
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
