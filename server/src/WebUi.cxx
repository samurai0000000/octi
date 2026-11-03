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
  flex-wrap: nowrap;
  white-space: nowrap;
  align-items: center;
  gap: 12px;
  padding: 10px 20px;
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
nav { display: flex; gap: 6px; flex-wrap: nowrap; white-space: nowrap; }
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
  flex-wrap: nowrap;
  white-space: nowrap;
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

/* --- Tabletop Web Player 3-Column Layout & Overlays --- */
@keyframes commitPulse1Hz {
  0%, 100% {
    background: #C8A253;
    border-color: #E5C158;
    box-shadow: 0 0 16px rgba(229, 193, 88, 0.7);
    transform: scale(1.0);
  }
  50% {
    background: #4A3A1A;
    border-color: #A67C2E;
    box-shadow: 0 0 4px rgba(166, 124, 46, 0.3);
    transform: scale(0.985);
  }
}
.commit-pulse {
  animation: commitPulse1Hz 1s infinite ease-in-out !important;
}
.web-tabletop-wrap {
  display: flex;
  flex-direction: row;
  align-items: stretch;
  justify-content: center;
  gap: 16px;
  margin: 12px auto;
  width: 100%;
  max-width: 1380px;
}
#web-play-reserves-col {
  width: 250px;
  flex: 0 0 250px;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  gap: 12px;
}
#web-play-board-wrap {
  flex: 0 1 760px;
  width: 100%;
  max-width: 760px;
  aspect-ratio: 1 / 1;
  position: relative;
  background: #0B0F17;
  border-radius: 14px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.35);
  overflow: hidden;
  user-select: none;
  -webkit-user-select: none;
}
#web-play-canvas {
  width: 100%;
  height: 100%;
  display: block;
  cursor: default;
  border-radius: 14px;
}
#web-play-side-col {
  width: 320px;
  flex: 0 0 320px;
  display: flex;
  flex-direction: column;
  gap: 12px;
}
@media (max-width: 1180px) {
  .web-tabletop-wrap {
    flex-direction: column;
    align-items: center;
  }
  #web-play-reserves-col, #web-play-side-col {
    width: 100%;
    max-width: 680px;
  }
}
.web-res-card {
  background: #101622;
  border: 1.5px solid #2D3A54;
  border-radius: 10px;
  padding: 10px 12px;
  color: #F8FAFC;
  box-shadow: 0 2px 8px rgba(0,0,0,0.25);
  transition: border-color 0.2s ease, box-shadow 0.2s ease;
}
.web-res-card.active-turn {
  border-color: #C8A253;
  box-shadow: 0 0 12px rgba(200, 162, 83, 0.35);
}
.web-res-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-size: 13px;
  font-weight: 700;
  margin-bottom: 8px;
  color: #C8A253;
}
.web-res-wells {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
}
.web-res-well {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 6px;
  padding: 4px 8px;
  font-size: 13px;
  font-weight: 700;
  cursor: default;
  transition: background 0.15s ease, border-color 0.15s ease;
}
.web-res-well.interactive {
  cursor: pointer;
  border-color: #C8A253;
  background: rgba(200, 162, 83, 0.12);
}
.web-res-well.interactive:hover {
  background: rgba(200, 162, 83, 0.25);
}
.web-quick-guide-card {
  background: #101622;
  border: 1px solid #1E293B;
  border-radius: 10px;
  padding: 10px 12px;
  font-size: 11.5px;
  color: #94A3B8;
  line-height: 1.5;
}
.web-quick-guide-card strong {
  color: #E2E8F0;
  display: block;
  margin-bottom: 4px;
  font-size: 12px;
}
#web-ply-confirm-bar {
  position: absolute;
  bottom: 12px;
  left: 12px;
  right: 12px;
  background: rgba(16, 22, 34, 0.95);
  border: 2px solid #C8A253;
  border-radius: 10px;
  padding: 10px 16px;
  display: none;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.45);
  z-index: 40;
  pointer-events: auto;
}
#web-staged-notation {
  font-family: ui-monospace, SFMono-Regular, monospace;
  font-size: 14px;
  font-weight: 700;
  color: #FDE047;
}
#web-start-match-overlay {
  position: absolute;
  inset: 24px;
  background: #161D26;
  border: 2px solid #C8A253;
  border-radius: 14px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  text-align: center;
  padding: 24px;
  z-index: 50;
  opacity: 1.0;
  box-shadow: 0 10px 40px rgba(0,0,0,0.6);
}
#web-start-summary-line1 {
  font-size: 19px;
  font-weight: 700;
  color: #F8FAFC;
  margin-bottom: 6px;
}
#web-start-summary-line2 {
  font-size: 14px;
  color: #94A3B8;
  margin-bottom: 24px;
}
#web-thinking-overlay {
  position: absolute;
  inset: 24px;
  background: #161D26;
  border: 2px solid #C8A253;
  border-radius: 14px;
  display: none;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  text-align: center;
  padding: 24px;
  z-index: 55;
  opacity: 1.0;
  box-shadow: 0 10px 40px rgba(0,0,0,0.6);
}
.web-thinking-progress-track {
  width: 80%;
  max-width: 440px;
  height: 26px;
  background: #0B0F17;
  border: 1px solid #C8A253;
  border-radius: 13px;
  position: relative;
  overflow: hidden;
  margin: 16px 0 10px 0;
}
#web-thinking-progress-fill {
  height: 100%;
  width: 0%;
  background: linear-gradient(90deg, #A67C2E 0%, #E5C158 100%);
  border-radius: 13px;
  transition: width 0.15s ease;
}
#web-thinking-progress-text {
  position: absolute;
  inset: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 12px;
  font-weight: 700;
  color: #F8FAFC;
  text-shadow: 0 1px 2px rgba(0,0,0,0.8);
}
#web-capture-overlay {
  position: absolute;
  inset: 0;
  background: rgba(6, 9, 14, 0.7);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 60;
}
#web-capture-card {
  background: #101622;
  border: 2px solid #C8A253;
  border-radius: 14px;
  padding: 20px 24px;
  max-width: 380px;
  width: 90%;
  text-align: center;
  box-shadow: 0 12px 36px rgba(0,0,0,0.55);
}
#web-gameover-overlay {
  position: absolute;
  inset: 0;
  background: rgba(6, 9, 14, 0.75);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 65;
}
#web-gameover-card {
  background: #101622;
  border: 2px solid #C8A253;
  border-radius: 14px;
  padding: 24px 28px;
  max-width: 440px;
  width: 90%;
  text-align: center;
  box-shadow: 0 16px 44px rgba(0,0,0,0.6);
}
#web-gameover-title {
  font-size: 22px;
  font-weight: 800;
  color: #FDE047;
  margin: 0 0 8px 0;
}
#web-gameover-subtitle {
  font-size: 14px;
  color: #CBD5E1;
  margin: 0 0 20px 0;
}
#web-zoom-overlay {
  position: fixed;
  inset: 0;
  background: rgba(6, 9, 14, 0.65);
  display: none;
  align-items: center;
  justify-content: center;
  z-index: 9990;
}
#web-zoom-card {
  background: #101622;
  border: 2px solid #C8A253;
  border-radius: 14px;
  padding: 16px 20px;
  max-width: 420px;
  width: 92%;
  display: flex;
  flex-direction: column;
  align-items: center;
  box-shadow: 0 16px 48px rgba(0,0,0,0.7);
  color: #F8FAFC;
  user-select: none;
  -webkit-user-select: none;
}
#web-zoom-canvas {
  width: 320px;
  height: 320px;
  display: block;
  cursor: crosshair;
}
#web-zoom-pod-tabs {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  justify-content: center;
  margin: 8px 0;
}
.zoom-pod-tab-btn {
  background: rgba(255, 255, 255, 0.08);
  border: 1px solid rgba(255, 255, 255, 0.15);
  color: #E2E8F0;
  border-radius: 6px;
  padding: 3px 8px;
  font-size: 12px;
  cursor: pointer;
}
.zoom-pod-tab-btn.active {
  background: #C8A253;
  border-color: #E5C158;
  color: #0B0F17;
  font-weight: 700;
}
#web-zoom-prong-tray {
  display: flex;
  gap: 12px;
  align-items: center;
  margin: 8px 0;
}
.zoom-prong-well {
  display: flex;
  align-items: center;
  gap: 6px;
  background: rgba(255, 255, 255, 0.06);
  border: 1px solid #2D3A54;
  border-radius: 6px;
  padding: 4px 8px;
  font-size: 12px;
  cursor: pointer;
}
.zoom-prong-well.active {
  border-color: #C8A253;
  background: rgba(200, 162, 83, 0.2);
}
#web-zoom-actions {
  display: flex;
  gap: 8px;
  width: 100%;
  margin-top: 10px;
}
#web-zoom-actions button {
  flex: 1;
}
</style>
</head>
<body>
<header>
  <div class="brand">
    <span class="brand-badge">OCTI</span>
    <div class="brand-text">
      <span class="brand-name">OCTI</span>
      <span class="brand-sub">octi.selfso.com &mdash; Official Platform</span>
    </div>
  </div>
  <nav id="nav">
    <button type="button" data-tab="home" id="nav-home-btn" class="btn on">Home</button>
    <button type="button" data-tab="play" id="tab-btn-play" class="btn">Web Player</button>
    <button type="button" data-tab="users" id="nav-users-btn" class="btn">Players</button>
    <button type="button" data-tab="archive" id="nav-archive-btn" class="btn">Game Archive</button>
    <button type="button" data-tab="board" id="nav-board-btn" class="btn">Leaderboard</button>
    <button type="button" data-tab="account" id="nav-account-btn" class="btn">Account</button>
  </nav>
  <div class="header-right">
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
        <p>Official rankings across 2-Player (9x9 3-Base), 4-Player, and OCTI for Kids (6x7) modes updated continuously.</p>
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
    <div class="row" style="align-items:center;justify-content:space-between;margin-bottom:12px;">
      <div style="display:flex;gap:8px;">
        <button type="button" class="btn btn-primary" id="subtab-play-local">Play vs. Browser JS Robot</button>
        <button type="button" class="btn" id="subtab-play-online">Online Lobby &amp; Observation</button>
      </div>
      <div style="display:flex;align-items:center;gap:10px;">
        <label style="display:flex;align-items:center;gap:4px;font-size:13px;">Theme:
          <select id="web-play-theme-select">
            <option value="obsidian" selected>Obsidian Brass</option>
            <option value="rosewood">Rosewood Ivory</option>
          </select>
        </label>
        <button type="button" class="btn" id="btn-web-rotate" title="Rotate Board 90&deg; CW">Rotate 90&deg;</button>
        <button type="button" class="btn" id="btn-sfx" title="Toggle Procedural Audio FX">SFX: On</button>
      </div>
    </div>

    <!-- Sub-Panel: Play vs. Browser JS Robot / Local -->
    <div id="play-local-panel">
      <!-- Tabletop 3-column stage -->
      <div class="web-tabletop-wrap">
        <!-- Left Column: Reserve HUD Cards (Player B top, Player A bottom, Quick Guide middle) -->
        <div id="web-play-reserves-col">
          <div id="web-res-card-b" class="web-res-card"></div>
          <div id="web-res-card-c" class="web-res-card" style="display:none;"></div>
          <div id="web-res-card-d" class="web-res-card" style="display:none;"></div>
          <div class="web-quick-guide-card">
            <strong style="color:#C8A253;font-size:12px;margin-bottom:4px;display:block;">Quick Guide</strong>
            <div>&bull; <strong>Right-click / Double-click:</strong> Zoom Square</div>
            <div>&bull; <strong>Click Pod:</strong> Guided Move / Jump</div>
            <div>&bull; <strong>Click Outer Socket:</strong> Add Prong</div>
            <div>&bull; <strong>Wheel / Pinch:</strong> Zoom Board (1.0x&ndash;3.0x)</div>
            <div>&bull; <strong>Drag Empty Space:</strong> Rotate 90&deg; Snap</div>
          </div>
          <div id="web-res-card-a" class="web-res-card"></div>
        </div>

        <!-- Center Column: Board Wrap & Overlays -->
        <div id="web-play-board-wrap">
          <canvas id="web-play-canvas" width="760" height="760"></canvas>

          <!-- Non-Modal Board-Scoped Start Match Overlay -->
          <div id="web-start-match-overlay">
            <div id="web-start-summary-line1">vs AI Bot (Lv2 &bull; 8s) &bull; Seat A (Emerald)</div>
            <div id="web-start-summary-line2">2P (9x9) &bull; 3 Bases &bull; Standard Rules</div>
            <div class="row" style="gap:12px;margin:0;">
              <button type="button" class="btn" id="btn-web-change-type" style="padding:10px 18px;font-size:14px;">Change Type</button>
              <button type="button" class="btn btn-primary" id="btn-web-start-match" style="padding:10px 22px;font-size:14px;background:#C8A253;border-color:#E5C158;color:#0B0F17;font-weight:700;">Start Match</button>
            </div>
          </div>

          <!-- 100% Opaque AI Thinking Overlay -->
          <div id="web-thinking-overlay">
            <h3 style="margin:0;font-size:18px;color:#FDE047;">Browser JS Robot Thinking...</h3>
            <div class="web-thinking-progress-track">
              <div id="web-thinking-progress-fill"></div>
              <div id="web-thinking-progress-text">Depth 1 &bull; 0 nodes &bull; 0.0s / 8.0s (0%)</div>
            </div>
          </div>

          <!-- Non-Blocking Staged Ply Confirmation Bar -->
          <div id="web-ply-confirm-bar">
            <div>
              <span class="dim" style="font-size:12px;margin-right:6px;">Staged Ply:</span>
              <span id="web-staged-notation">-</span>
            </div>
            <div class="row" style="gap:8px;margin:0;">
              <button type="button" class="btn" id="btn-web-cancel-ply" style="padding:6px 14px;">Cancel [Esc]</button>
              <button type="button" class="btn btn-primary commit-pulse" id="btn-web-commit-ply" style="padding:6px 18px;font-weight:700;">Commit [Enter]</button>
            </div>
          </div>

          <!-- Borderless Capture Prompt Dialog -->
          <div id="web-capture-overlay">
            <div id="web-capture-card">
              <h4 id="web-capture-prompt-text" style="margin:0 0 10px 0;font-size:16px;color:#F8FAFC;">Capture jumped pod(s)?</h4>
              <p class="dim" style="font-size:12px;margin:0 0 16px 0;">You may also drag the jumped pod off the board edge to toggle capture.</p>
              <div class="row" style="gap:10px;justify-content:center;margin:0;">
                <button type="button" class="btn btn-primary" id="btn-capture-yes" style="background:#dc2626;border-color:#dc2626;padding:8px 18px;">Capture</button>
                <button type="button" class="btn" id="btn-capture-no" style="padding:8px 18px;">Leave on Board</button>
              </div>
            </div>
          </div>

          <!-- Borderless Game Over Dialog -->
          <div id="web-gameover-overlay">
            <div id="web-gameover-card">
              <div id="web-gameover-title">PLAYER A WINS</div>
              <div id="web-gameover-subtitle">VICTORY BY BASE OCCUPATION</div>
              <div class="row" style="gap:10px;justify-content:center;margin:0;">
                <button type="button" class="btn btn-primary" id="btn-gameover-new" style="padding:8px 18px;background:#C8A253;border-color:#E5C158;color:#0B0F17;font-weight:700;">New Game</button>
                <button type="button" class="btn" id="btn-gameover-review" style="padding:8px 18px;">Review Board</button>
              </div>
            </div>
          </div>
        </div>

        <!-- Right Column: Sidebar (Status & Clocks, Move History + VCR, Match Actions) -->
        <div id="web-play-side-col">
          <!-- Top Card: Match Status & Clocks -->
          <div class="panel" style="margin:0;padding:12px;background:#101622;border:1px solid #1E293B;color:#F8FAFC;">
            <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px;">
              <span id="web-play-mode-badge" class="badge" style="background:#1E293B;color:#94A3B8;">2P (9x9)</span>
              <span id="web-play-turn-badge" class="badge" style="background:#064E3B;color:#6EE7B7;">Player A Turn</span>
            </div>
            <div id="web-play-clock-bar" style="display:flex;gap:6px;margin-top:6px;">
              <div id="web-clock-a" style="flex:1;background:#064E3B;color:#ECFDF5;padding:6px;border-radius:6px;text-align:center;font-family:ui-monospace,SFMono-Regular,monospace;font-size:14px;font-weight:700;">A: --:--</div>
              <div id="web-clock-b" style="flex:1;background:#881337;color:#FFF1F2;padding:6px;border-radius:6px;text-align:center;font-family:ui-monospace,SFMono-Regular,monospace;font-size:14px;font-weight:700;">B: --:--</div>
              <div id="web-clock-c" style="display:none;flex:1;background:#1E3A8A;color:#EFF6FF;padding:6px;border-radius:6px;text-align:center;font-family:ui-monospace,SFMono-Regular,monospace;font-size:14px;font-weight:700;">C: --:--</div>
              <div id="web-clock-d" style="display:none;flex:1;background:#78350F;color:#FFFBEB;padding:6px;border-radius:6px;text-align:center;font-family:ui-monospace,SFMono-Regular,monospace;font-size:14px;font-weight:700;">D: --:--</div>
            </div>
            <div id="web-play-archive-status" class="dim" style="margin-top:6px;font-size:12px;"></div>
          </div>

          <!-- Middle Card: Color-Coded Move History & VCR Replay -->
          <div class="panel" style="margin:0;padding:12px;background:#101622;border:1px solid #1E293B;flex:1;display:flex;flex-direction:column;">
            <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:8px;">
              <strong style="color:#C8A253;font-size:13px;">Move History</strong>
              <button type="button" class="btn" id="btn-web-export-moves" title="Export Move History (JSON v3.0)" style="width:36px;height:32px;padding:0;display:inline-flex;align-items:center;justify-content:center;background:#161D26;border:1px solid #C8A253;border-radius:6px;cursor:pointer;">
                <svg width="16" height="16" viewBox="0 0 16 16" fill="none" xmlns="http://www.w3.org/2000/svg">
                  <path d="M2.5 9.5V13.5H13.5V9.5" stroke="#C8A253" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/>
                  <path d="M8 10.5V2.5M8 2.5L4.5 6M8 2.5L11.5 6" stroke="#C8A253" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/>
                </svg>
              </button>
            </div>
            <div id="web-play-moves" style="flex:1;min-height:180px;max-height:300px;overflow-y:auto;background:#0B0F17;border:1px solid #1E293B;border-radius:6px;padding:6px;font-family:ui-monospace,SFMono-Regular,monospace;font-size:12.5px;">
              <div class="dim" style="padding:4px;">No moves recorded yet.</div>
            </div>
            <!-- VCR Replay Bar -->
            <div id="web-vcr-bar" style="display:flex;align-items:center;justify-content:space-between;margin-top:8px;gap:4px;">
              <button type="button" class="btn" id="web-vcr-first" style="padding:4px 8px;font-size:12px;" title="First Move">|&lt;&lt;</button>
              <button type="button" class="btn" id="web-vcr-prev" style="padding:4px 8px;font-size:12px;" title="Previous Move">&lt;</button>
              <span id="web-vcr-label" style="font-size:12px;color:#94A3B8;font-family:ui-monospace,SFMono-Regular,monospace;flex:1;text-align:center;">Ply 0/0</span>
              <button type="button" class="btn" id="web-vcr-next" style="padding:4px 8px;font-size:12px;" title="Next Move">&gt;</button>
              <button type="button" class="btn" id="web-vcr-last" style="padding:4px 8px;font-size:12px;" title="Latest Move">&gt;&gt;|</button>
            </div>
          </div>

          <!-- Bottom Card: Match Actions -->
          <div class="panel" style="margin:0;padding:12px;background:#101622;border:1px solid #1E293B;">
            <div class="row" style="gap:8px;margin:0;">
              <button type="button" class="btn btn-primary" id="btn-web-new-game-setup" style="flex:1;background:#C8A253;border-color:#E5C158;color:#0B0F17;font-weight:700;">New Game...</button>
              <button type="button" class="btn" id="btn-web-undo">Undo</button>
              <button type="button" class="btn" id="btn-web-resign" style="border-color:#dc2626;color:#dc2626;">Resign</button>
            </div>
            <div id="web-online-table-actions" style="display:none;margin-top:8px;" class="row">
              <button type="button" class="btn" id="btn-web-leave-table" style="flex:1;border-color:#dc2626;color:#dc2626;">Leave Table</button>
              <button type="button" class="btn" id="btn-web-back-lobby" style="flex:1;">Back to Lobby</button>
            </div>
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
          <div class="row" style="margin:0;gap:8px;">
            <button type="button" class="btn btn-primary" id="btn-web-open-my-table" style="background:#2563eb;border-color:#2563eb;">Open Board</button>
            <button type="button" class="btn btn-primary" id="btn-web-leave-my-table" style="background:#dc2626;border-color:#dc2626;">Leave Table</button>
          </div>
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
</section>

<!-- Standalone Tab: Replay Viewer (Spawned in New Browser Tab) -->
<section id="tab-replay" class="tab">
  <div class="panel" id="ar-detail" style="display:none">
    <div class="row" style="align-items:center;">
      <strong id="ar-title" style="color:var(--acc-bright);font-size:15px;"></strong>
      <div style="margin-left:auto;display:flex;gap:8px;flex-wrap:wrap;align-items:center;">
        <select class="btn" id="replay-theme-select" title="Switch Replay Board Palette">
          <option value="obsidian">Obsidian Brass</option>
          <option value="rosewood">Rosewood Ivory</option>
        </select>
        <button type="button" class="btn" id="btn-replay-rotate" title="Rotate Board 90 degrees CW">Rotate 90&deg;</button>
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

    <div class="panel" id="account-robots-panel" style="margin-top:16px;">
      <div class="row" style="align-items:center;">
        <h3 style="margin:0;">Robot Registration &amp; Token Management</h3>
        <span id="robots-quota-badge" class="badge" style="margin-left:12px;">Active Robots: 0 / 3</span>
        <button type="button" class="btn" id="btn-toggle-robots-section" aria-expanded="false" style="margin-left:auto;">Show Robot Registration &#9662;</button>
      </div>
      <div id="account-robots-expand-body" style="display:none;margin-top:14px;">
        <p class="dim" style="margin-top:0;">Each registered user can register up to <strong>3 active robots</strong> (<code>octi_bot_&lt;token&gt;</code>) with a globally unique <code>robot_name</code>.</p>
        <div class="row" style="margin-top:10px;">
          <input id="robot-name-input" placeholder="Robot Name (3..24 chars [A-Za-z0-9_-])" maxlength="24" style="flex:1;">
          <input id="robot-desc-input" placeholder="Robot Description (optional, max 256 chars)" maxlength="256" style="flex:2;">
          <button type="button" class="btn btn-primary" id="btn-create-robot">Create Robot &amp; Generate Token</button>
        </div>
        <div id="robot-new-token-box" style="display:none;background:#eff6ff;border:1px solid #93c5fd;padding:12px;border-radius:8px;margin-top:10px;"></div>
        <div style="margin-top:14px;">
          <h4 style="margin:0 0 8px 0;">My Registered Robots</h4>
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
    </div>
  </div>
</section>


<!-- Setup Match Modal -->
<div id="web-match-setup-modal" style="display:none;position:fixed;inset:0;background:rgba(6,9,14,0.7);z-index:9995;align-items:center;justify-content:center;padding:16px;">
  <div style="background:#101622;border:2px solid #C8A253;border-radius:14px;max-width:480px;width:100%;padding:22px;color:#F8FAFC;box-shadow:0 16px 48px rgba(0,0,0,0.7);">
    <h3 style="margin:0 0 16px 0;font-size:18px;color:#C8A253;">Match Configuration</h3>
    <div style="display:flex;flex-direction:column;gap:12px;">
      <label style="display:flex;justify-content:space-between;align-items:center;">Play Mode:
        <select id="setup-play-mode" style="background:#161D26;color:#fff;border-color:#2D3A54;">
          <option value="bot" selected>vs AI Bot</option>
          <option value="pass">Pass-and-Play (Local 2P)</option>
        </select>
      </label>
      <label style="display:flex;justify-content:space-between;align-items:center;">Bot Difficulty:
        <select id="setup-bot-difficulty" style="background:#161D26;color:#fff;border-color:#2D3A54;">
          <option value="1">Lv1: Fast (2s)</option>
          <option value="2" selected>Lv2: Balanced (8s)</option>
          <option value="3">Lv3: Deep (20s)</option>
        </select>
      </label>
      <label style="display:flex;justify-content:space-between;align-items:center;">Human Seat:
        <select id="setup-human-seat" style="background:#161D26;color:#fff;border-color:#2D3A54;">
          <option value="A" selected>Seat A (Bottom &bull; Emerald)</option>
          <option value="B">Seat B (Top &bull; Ruby)</option>
          <option value="C" id="setup-seat-opt-c" style="display:none;">Seat C (Top-Right &bull; Sapphire)</option>
          <option value="D" id="setup-seat-opt-d" style="display:none;">Seat D (Bottom-Left &bull; Amber)</option>
        </select>
      </label>
      <label style="display:flex;justify-content:space-between;align-items:center;">Game Mode:
        <select id="setup-game-mode" style="background:#161D26;color:#fff;border-color:#2D3A54;">
          <option value="2P" selected>2P (9x9)</option>
          <option value="4P">4P (9x9)</option>
          <option value="OFK">OFK (6x7)</option>
        </select>
      </label>
      <label style="display:flex;justify-content:space-between;align-items:center;">Bases to Win:
        <select id="setup-bases" style="background:#161D26;color:#fff;border-color:#2D3A54;">
          <option value="3" selected>3 Bases (Standard)</option>
          <option value="1">1 Base (Rapid)</option>
        </select>
      </label>
      <div style="display:flex;gap:18px;">
        <label><input type="checkbox" id="setup-superprongs"> Super-Prongs</label>
        <label><input type="checkbox" id="setup-edgeless"> Edgeless Board</label>
      </div>
      <label style="display:flex;justify-content:space-between;align-items:center;">Time Control:
        <select id="setup-time-control" style="background:#161D26;color:#fff;border-color:#2D3A54;">
          <option value="untimed" selected>Untimed</option>
          <option value="300+3">5m + 3s</option>
          <option value="600+5">10m + 5s</option>
          <option value="900+10">15m + 10s</option>
        </select>
      </label>
    </div>
    <div class="row" style="margin-top:20px;justify-content:flex-end;gap:10px;">
      <button type="button" class="btn" id="btn-setup-cancel">Cancel</button>
      <button type="button" class="btn btn-primary" id="btn-setup-apply" style="background:#C8A253;border-color:#E5C158;color:#0B0F17;font-weight:700;">Save Type</button>
    </div>
  </div>
</div>

<!-- Borderless Zoomed Square Overlay -->
<div id="web-zoom-overlay">
  <div id="web-zoom-card">
    <div style="font-size:14px;font-weight:700;color:#C8A253;margin-bottom:6px;" id="web-zoom-sq-title">Square (3, 3)</div>
    <canvas id="web-zoom-canvas" width="320" height="320"></canvas>
    <div id="web-zoom-pod-tabs"></div>
    <div id="web-zoom-prong-tray">
      <div class="zoom-prong-well active" id="zoom-well-normal" title="Normal Prong">
        <svg width="14" height="18" viewBox="0 0 14 18"><path d="M7 1L12 14L7 17L2 14Z" fill="#F8FAFC" stroke="#0B0F17" stroke-width="1"/></svg>
        <span id="zoom-prong-count">0</span>
      </div>
      <div class="zoom-prong-well" id="zoom-well-super" title="Superprong" style="display:none;">
        <svg width="16" height="18" viewBox="0 0 16 18"><path d="M8 1L15 13L8 17L1 13Z" fill="#FDE047" stroke="#78350F" stroke-width="1"/></svg>
        <span id="zoom-super-count">0</span>
      </div>
    </div>
    <div id="web-zoom-actions">
      <button type="button" class="btn btn-primary" id="btn-zoom-move-pod" style="background:#C8A253;border-color:#E5C158;color:#0B0F17;font-weight:700;">Move / Jump This Pod</button>
      <button type="button" class="btn" id="btn-zoom-place-pod" style="display:none;">Place Reserve Pod</button>
      <button type="button" class="btn btn-primary" id="btn-zoom-liberate-pod" style="display:none;background:#10B981;border-color:#059669;">Liberate (*)</button>
    </div>
  </div>
</div>

<!-- Move Export Modal -->
<div id="web-export-modal" style="display:none;position:fixed;inset:0;background:rgba(6,9,14,0.7);z-index:9995;align-items:center;justify-content:center;padding:16px;">
  <div style="background:#101622;border:2px solid #C8A253;border-radius:14px;max-width:540px;width:100%;padding:20px;color:#F8FAFC;box-shadow:0 16px 48px rgba(0,0,0,0.7);">
    <h3 style="margin:0 0 12px 0;font-size:16px;color:#C8A253;">Export Move History (JSON v3.0)</h3>
    <textarea id="web-export-json-text" readonly style="width:100%;height:220px;background:#0B0F17;color:#E2E8F0;font-family:ui-monospace,SFMono-Regular,monospace;font-size:12px;border:1px solid #2D3A54;border-radius:6px;padding:8px;resize:vertical;"></textarea>
    <div class="row" style="margin-top:14px;justify-content:flex-end;gap:10px;">
      <button type="button" class="btn" id="btn-web-close-export">Close</button>
      <button type="button" class="btn btn-primary" id="btn-web-copy-transcript" style="background:#C8A253;border-color:#E5C158;color:#0B0F17;font-weight:700;">Copy to Clipboard</button>
    </div>
  </div>
</div>
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
    if ((x === 5 && y === 3) || (x === 5 && y === 7)) {
      return "NEUTRAL_OCTI";
    }
    return "NEUTRAL";
  }
  if (y === 3 && (x === 3 || x === 5 || x === 7)) { return "A_HOME"; }
  if (y === 7 && (x === 3 || x === 5 || x === 7)) { return "B_HOME"; }
  return "NEUTRAL";
}

function formatProngConfig(prongs) {
  if (!prongs) return "";
  var letters = ["a", "b", "c", "d", "e", "f", "g", "h"];
  var text = "";
  for (var i = 0; i < 8; i++) {
    if (prongs[i] === 1) {
      text += letters[i];
    } else if (prongs[i] === 2) {
      text += letters[i] + "!";
    }
  }
  return text;
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
  return { prongs: prongs, count: count, text: formatProngConfig(prongs) };
}

function parseOctiFen(fen) {
  if (!fen) { return null; }
  var f = fen.split(":");
  if (f.length < 11) { return null; }
  var mode = f[0];
  if (mode === "2") { mode = "2P"; }
  else if (mode === "4") { mode = "4P"; }
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
    var oppIdx = (idx === 0 ? 1 : 0);
    var oppRes = parsed.reserves[oppIdx];
    var enemyPodsCaptured = (parsed.mode === "4P") ? r.capturedPods : (oppRes ? oppRes.capturedPods : 0);
    addMetric("Pods", r.freePods);
    addMetric("Cap", enemyPodsCaptured);
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
    /*WebPlayBoard*/
    if ($("tab-play") && $("tab-play").classList.contains("on") && activePlayState) {
      var hasPlayMoveInd = lastMoveHighlightSquares && lastMoveHighlightSquares.length > 0 && (now - lastMoveSetAtMs) <= 5200;
      var hasPlayTurnInd = activePlayState.status === "P";
      if (hasPlayMoveInd || hasPlayTurnInd) {
        renderPlayBoardCanvas();
      }
    }
    requestAnimationFrame(animationTick);
  }
  requestAnimationFrame(animationTick);

window.addEventListener("resize", function () {
  Object.keys(boardStates).forEach(function (k) {
    drawBoardState(boardStates[k]);
  });
});

if ($("btn-theme")) {
  $("btn-theme").addEventListener("click", function () {
    currentTheme = (currentTheme === "obsidian") ? "rosewood" : "obsidian";
    $("btn-theme").textContent = "Theme: " + PALETTES[currentTheme].name;
    Object.keys(boardStates).forEach(function (k) {
      drawBoardState(boardStates[k]);
    });
  });
}

if ($("btn-rotate")) {
  $("btn-rotate").addEventListener("click", function () {
    boardRotationDeg = (boardRotationDeg + 90) % 360;
    Object.keys(boardStates).forEach(function (k) {
      drawBoardState(boardStates[k]);
    });
  });
}

if ($("replay-theme-select")) {
  $("replay-theme-select").value = currentTheme;
  $("replay-theme-select").addEventListener("change", function () {
    currentTheme = $("replay-theme-select").value;
    Object.keys(boardStates).forEach(function (k) {
      drawBoardState(boardStates[k]);
    });
  });
}

if ($("btn-replay-rotate")) {
  $("btn-replay-rotate").addEventListener("click", function () {
    boardRotationDeg = (boardRotationDeg + 90) % 360;
    Object.keys(boardStates).forEach(function (k) {
      drawBoardState(boardStates[k]);
    });
  });
}


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

function wrapCoord(val, max) {
  return ((val - 1) % max + max) % max + 1;
}

function playerIndex(p) {
  if (p === "A") return 0;
  if (p === "B") return 1;
  if (p === "C") return 2;
  if (p === "D") return 3;
  return 0;
}

function partnerOf(mode, p) {
  if (mode === "4" || mode === "4P") {
    if (p === "A") return "C";
    if (p === "B") return "D";
    if (p === "C") return "A";
    if (p === "D") return "B";
  }
  return null;
}

function partnerIndex(mode, pIdx) {
  if (mode === "4" || mode === "4P") {
    if (pIdx === 0) return 2;
    if (pIdx === 1) return 3;
    if (pIdx === 2) return 0;
    if (pIdx === 3) return 1;
  }
  return -1;
}

function isPartnerHome(mode, partnerIdx, x, y) {
  if (mode !== "4" && mode !== "4P") return false;
  if (partnerIdx === 0) return (x === 3 && y === 3);
  if (partnerIdx === 1) return (x === 3 && y === 7);
  if (partnerIdx === 2) return (x === 7 && y === 7);
  if (partnerIdx === 3) return (x === 7 && y === 3);
  return false;
}

function areAllied(mode, p1, p2) {
  if (p1 === p2) return true;
  if (mode === "4" || mode === "4P") {
    if ((p1 === "A" || p1 === "C" || p1 === "TEAM_13_WIN") &&
        (p2 === "A" || p2 === "C" || p2 === "TEAM_13_WIN")) return true;
    if ((p1 === "B" || p1 === "D" || p1 === "TEAM_24_WIN") &&
        (p2 === "B" || p2 === "D" || p2 === "TEAM_24_WIN")) return true;
  }
  return false;
}

function isSquareOccupiedBy(state, x, y, player) {
  if (!state || !state.map) return false;
  var stack = state.map[x + "," + y];
  if (!stack || !stack.length) return false;
  for (var i = 0; i < stack.length; i++) {
    if (stack[i].owner === player) return true;
  }
  return false;
}

function squareHasEnemyOf(state, x, y, player) {
  if (!state || !state.map) return false;
  var stack = state.map[x + "," + y];
  if (!stack || !stack.length) return false;
  for (var i = 0; i < stack.length; i++) {
    if (!areAllied(state.mode, stack[i].owner, player)) return true;
  }
  return false;
}

function canLiberate(state, player) {
  if (typeof player === "object" && player !== null) {
    var tmp = state; state = player; player = tmp;
  }
  if (!state || !state.map) return false;
  if (state.mode === "OFK") return false;
  if (state.mode === "2" || state.mode === "2P") {
    if (player === "A") {
      return isSquareOccupiedBy(state, 3, 7, "A") ||
             isSquareOccupiedBy(state, 5, 7, "A") ||
             isSquareOccupiedBy(state, 7, 7, "A");
    }
    if (player === "B") {
      return isSquareOccupiedBy(state, 3, 3, "B") ||
             isSquareOccupiedBy(state, 5, 3, "B") ||
             isSquareOccupiedBy(state, 7, 3, "B");
    }
    return false;
  }
  if (state.mode === "4" || state.mode === "4P") {
    if (isSquareOccupiedBy(state, 5, 3, player) ||
        isSquareOccupiedBy(state, 5, 7, player)) {
      return true;
    }
    if (player === "A" || player === "C") {
      return isSquareOccupiedBy(state, 3, 7, player) ||
             isSquareOccupiedBy(state, 7, 3, player);
    }
    if (player === "B" || player === "D") {
      return isSquareOccupiedBy(state, 3, 3, player) ||
             isSquareOccupiedBy(state, 7, 7, player);
    }
  }
  return false;
}

function canPlaceReservePodOnSquare(state, x, y, side) {
  if (!state || !state.reserves) return false;
  if (state.mode === "OFK") return false;
  var pIdx = playerIndex(side);
  var res = state.reserves[pIdx];
  if (!res || res.freePods <= 0) return false;

  if (state.mode === "2" || state.mode === "2P") {
    var friendlyY = (side === "A" ? 3 : 7);
    var enemyY = (side === "A" ? 7 : 3);
    var isFriendlyBase = (y === friendlyY && (x === 3 || x === 5 || x === 7));
    if (isFriendlyBase) {
      return !squareHasEnemyOf(state, x, y, side);
    }
    var isEnemyBase = (y === enemyY && (x === 3 || x === 5 || x === 7));
    if (isEnemyBase) {
      return isSquareOccupiedBy(state, x, y, side);
    }
    return false;
  }

  if (state.mode === "4" || state.mode === "4P") {
    var ownCoord = (side === "A" ? {x:3,y:3} : (side === "B" ? {x:3,y:7} : (side === "C" ? {x:7,y:7} : {x:7,y:3})));
    if (x === ownCoord.x && y === ownCoord.y) {
      return !squareHasEnemyOf(state, x, y, side);
    }
    var partnerIdx = partnerIndex(state.mode, pIdx);
    if (isPartnerHome(state.mode, partnerIdx, x, y)) return false;
    if ((x === 5 && y === 3) || (x === 5 && y === 7)) return false;

    var isEnemyBase = (side === "A" || side === "C")
      ? ((x === 3 && y === 7) || (x === 7 && y === 3))
      : ((x === 3 && y === 3) || (x === 7 && y === 7));
    if (isEnemyBase) {
      return isSquareOccupiedBy(state, x, y, side);
    }
    return false;
  }

  return false;
}

function canLiberateOnSquare(state, x, y, side) {
  if (!state || !state.reserves) return false;
  if (state.mode === "OFK") return false;
  var pIdx = playerIndex(side);
  var res = state.reserves[pIdx];

  if (state.mode === "2" || state.mode === "2P") {
    if (!res || res.capturedPods <= 0) return false;
    if (!canLiberate(state, side)) return false;
    var isBase = (y === 3 || y === 7) && (x === 3 || x === 5 || x === 7);
    return isBase && isSquareOccupiedBy(state, x, y, side);
  }

  if (state.mode === "4" || state.mode === "4P") {
    var partnerIdx = partnerIndex(state.mode, pIdx);
    var isPartnerRevival = (state.elimMask & (1 << partnerIdx)) !== 0 &&
                           isPartnerHome(state.mode, partnerIdx, x, y);
    if (isPartnerRevival) {
      return state.reserves[partnerIdx] &&
             state.reserves[partnerIdx].capturedPods > 0 &&
             isSquareOccupiedBy(state, x, y, side);
    }

    if (!res || res.capturedPods <= 0) return false;

    if ((x === 5 && y === 3) || (x === 5 && y === 7)) {
      return isSquareOccupiedBy(state, x, y, side);
    }

    var isEnemyBase = (side === "A" || side === "C")
      ? ((x === 3 && y === 7) || (x === 7 && y === 3))
      : ((x === 3 && y === 3) || (x === 7 && y === 7));
    if (isEnemyBase) {
      return isSquareOccupiedBy(state, x, y, side);
    }

    var isOwnHome = (side === "A" && x === 3 && y === 3) ||
                    (side === "B" && x === 3 && y === 7) ||
                    (side === "C" && x === 7 && y === 7) ||
                    (side === "D" && x === 7 && y === 3);
    var isPartnerBase = isPartnerHome(state.mode, partnerIdx, x, y);
    if (isOwnHome || isPartnerBase) {
      return canLiberate(state, side) && isSquareOccupiedBy(state, x, y, side);
    }

    return false;
  }

  return false;
}

function serializeOctiFen(s) {
  var parts = [];
  var outMode = s.mode;
  if (outMode === "2P" || outMode === "2") outMode = "2";
  else if (outMode === "4P" || outMode === "4") outMode = "4";
  parts.push(outMode);
  parts.push(String(s.bases));
  parts.push(s.superProngs ? "1" : "0");
  parts.push(s.edgeless ? "1" : "0");
  var st = s.status || "P";
  parts.push(st);
  parts.push((st === "O" || st === "R") ? "-" : (s.turn || "-"));
  var winTok = s.winner || "-";
  if (winTok === "TEAM_13_WIN") winTok = "A";
  else if (winTok === "TEAM_24_WIN") winTok = "B";
  parts.push(winTok);
  parts.push(String(s.elimMask || 0));
  s.reserves.forEach(function (r) {
    parts.push(r.freePods + "," + r.capturedPods + "," + r.freeProngs + "," + r.freeSuper);
  });
  var sqTokens = [];
  var sortedKeys = Object.keys(s.map).sort(function (a, b) {
    var pA = a.split(","), pB = b.split(",");
    var xA = parseInt(pA[0], 10), yA = parseInt(pA[1], 10);
    var xB = parseInt(pB[0], 10), yB = parseInt(pB[1], 10);
    if (yA !== yB) return yA - yB;
    return xA - xB;
  });
  sortedKeys.forEach(function (k) {
    var p = k.split(",");
    var px = p[0], py = p[1];
    s.map[k].forEach(function (pod) {
      var cfgStr = (pod.cfg && pod.cfg.prongs) ? formatProngConfig(pod.cfg.prongs) : ((pod.cfg && pod.cfg.text) ? pod.cfg.text : "");
      sqTokens.push(pod.owner + px + py + cfgStr);
    });
  });
  parts.push(sqTokens.length ? sqTokens.join(",") : "-");
  return parts.join(":");
}

function getHomeSquares(mode, player, bases) {
  if (typeof mode === "object" && mode !== null) {
    bases = mode.bases;
    player = player || mode.turn;
    mode = mode.mode;
  }
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
  return (player === "A")
    ? [{x:3, y:3}, {x:5, y:3}, {x:7, y:3}]
    : [{x:3, y:7}, {x:5, y:7}, {x:7, y:7}];
}

function getOpponentBases(mode, player, bases) {
  if (typeof mode === "object" && mode !== null) {
    bases = mode.bases;
    player = player || mode.turn;
    mode = mode.mode;
  }
  if (mode === "4" || mode === "4P") {
    if (player === "A" || player === "C") {
      return [{x:3, y:7}, {x:7, y:3}];
    }
    return [{x:3, y:3}, {x:7, y:7}];
  }
  if (mode === "OFK") {
    return (player === "A")
      ? [{x:2, y:6}, {x:3, y:6}, {x:4, y:6}, {x:5, y:6}]
      : [{x:2, y:2}, {x:3, y:2}, {x:4, y:2}, {x:5, y:2}];
  }
  return (player === "A")
    ? [{x:3, y:7}, {x:5, y:7}, {x:7, y:7}]
    : [{x:3, y:3}, {x:5, y:3}, {x:7, y:3}];
}

function dfsJumpChains(s, startX, startY, curX, curY, pod, jumpedSquares, currentMoveStr, results, seenMap, onlyTactical, isFirstHop) {
  if (jumpedSquares.length >= 16) return;
  var oppBases = getOpponentBases(s.mode, pod.owner, s.bases);

  for (var d = 0; d < 8; d++) {
    if (!pod.cfg.prongs[d]) continue;
    var dx = DIR_DELTAS[d][0], dy = DIR_DELTAS[d][1];
    var jx = curX + dx, jy = curY + dy;
    var landX = curX + dx * 2, landY = curY + dy * 2;

    if (s.edgeless) {
      jx = wrapCoord(jx, s.gw); jy = wrapCoord(jy, s.gh);
      landX = wrapCoord(landX, s.gw); landY = wrapCoord(landY, s.gh);
    } else {
      if (jx < 1 || jx > s.gw || jy < 1 || jy > s.gh) continue;
      if (landX < 1 || landX > s.gw || landY < 1 || landY > s.gh) continue;
    }

    var alreadyJumped = jumpedSquares.some(function (j) { return j.x === jx && j.y === jy; });
    if (alreadyJumped) continue;

    var jStack = s.map[jx + "," + jy];
    var jCount = jStack ? ((jx === startX && jy === startY) ? (jStack.length - 1) : jStack.length) : 0;
    if (jCount <= 0) continue;

    var landK = landX + "," + landY;
    var landStack = s.map[landK];
    var canLand = (!landStack || !landStack.length) ||
      (landX === startX && landY === startY && s.map[startX + "," + startY] && s.map[startX + "," + startY].length === 1);
    if (!canLand) continue;

    var hasEnemy = squareHasEnemyOf(s, jx, jy, pod.owner);
    var cfgStr = (pod.cfg && pod.cfg.prongs) ? formatProngConfig(pod.cfg.prongs) : (pod.cfg.text || "");
    var hopPrefix = isFirstHop
      ? (startX + "" + startY + cfgStr + "-" + landX + "" + landY)
      : (currentMoveStr + "-" + landX + "" + landY);

    var options = [];
    if (hasEnemy) {
      options.push({ moveStr: hopPrefix + "x", isCap: true });
      options.push({ moveStr: hopPrefix, isCap: false });
    } else {
      options.push({ moveStr: hopPrefix, isCap: false });
    }

    for (var o = 0; o < options.length; o++) {
      var opt = options[o];
      var isBaseEntry = oppBases.some(function (b) { return b.x === landX && b.y === landY; });

      if (!onlyTactical || opt.isCap || isBaseEntry) {
        if (!seenMap[opt.moveStr]) {
          seenMap[opt.moveStr] = true;
          results.push(opt.moveStr);
        }
      }

      dfsJumpChains(s, startX, startY, landX, landY, pod,
        jumpedSquares.concat([{ x: jx, y: jy }]), opt.moveStr, results, seenMap, onlyTactical, false);
    }
  }
}

function generateMoves(s, player, onlyTactical) {
  var moves = [];
  var seenMap = {};
  var pIdx = playerIndex(player);
  var res = s.reserves[pIdx];
  var oppBases = getOpponentBases(s.mode, player, s.bases);

  /* 1. Pod placements & Liberations */
  if (!onlyTactical && res && s.mode !== "OFK") {
    var candidateCoords = [
      {x:3, y:3}, {x:5, y:3}, {x:7, y:3},
      {x:3, y:7}, {x:5, y:7}, {x:7, y:7}
    ];
    candidateCoords.forEach(function (c) {
      if (canPlaceReservePodOnSquare(s, c.x, c.y, player)) {
        var mStr = c.x + "" + c.y;
        if (!seenMap[mStr]) { seenMap[mStr] = true; moves.push(mStr); }
      }
      if (canLiberateOnSquare(s, c.x, c.y, player)) {
        var mStrLib = c.x + "" + c.y + "*";
        if (!seenMap[mStrLib]) { seenMap[mStrLib] = true; moves.push(mStrLib); }
      }
    });
  }

  /* 2. Prong additions & Repositions */
  if (!onlyTactical) {
    Object.keys(s.map).forEach(function (k) {
      var stack = s.map[k] || [];
      var coords = k.split(",");
      var px = coords[0], py = coords[1];

      for (var p = 0; p < stack.length; p++) {
        var pod = stack[p];
        if (pod.owner !== player) continue;
        var cfgText = pod.cfg.text || "";

        // Prong additions
        if (res && res.freeProngs > 0) {
          for (var d = 0; d < 8; d++) {
            if (!pod.cfg.prongs[d]) {
              var addMove = px + "" + py + cfgText + "+" + DIR_CHARS[d];
              if (!seenMap[addMove]) { seenMap[addMove] = true; moves.push(addMove); }
            }
          }
        }
        if (s.superProngs && res && res.freeSuper > 0) {
          for (var sd = 0; sd < 8; sd++) {
            if (!pod.cfg.prongs[sd]) {
              var addSupMove = px + "" + py + cfgText + "+" + DIR_CHARS[sd] + "!";
              if (!seenMap[addSupMove]) { seenMap[addSupMove] = true; moves.push(addSupMove); }
            }
          }
        }

        // Prong repositions
        if (s.mode !== "OFK" && pod.cfg.count > 0) {
          for (var src = 0; src < 8; src++) {
            if (pod.cfg.prongs[src] === 1) {
              for (var dst = 0; dst < 8; dst++) {
                if (!pod.cfg.prongs[dst]) {
                  var repoMove = px + "" + py + cfgText + "+" + DIR_CHARS[dst] + "-" + DIR_CHARS[src];
                  if (!seenMap[repoMove]) { seenMap[repoMove] = true; moves.push(repoMove); }
                }
              }
            }
          }
        }
      }
    });
  }

  /* 3. Steps and Recursive Jumps */
  Object.keys(s.map).forEach(function (k) {
    var stack = s.map[k] || [];
    var coords = k.split(",");
    var px = parseInt(coords[0], 10);
    var py = parseInt(coords[1], 10);

    for (var p = 0; p < stack.length; p++) {
      var pod = stack[p];
      if (pod.owner !== player) continue;
      var cfgText = pod.cfg.text || "";

      // Single steps (distance 1)
      for (var d = 0; d < 8; d++) {
        if (!pod.cfg.prongs[d]) continue;
        var dx = DIR_DELTAS[d][0], dy = DIR_DELTAS[d][1];
        var sx = px + dx, sy = py + dy;
        if (s.edgeless) {
          sx = wrapCoord(sx, s.gw); sy = wrapCoord(sy, s.gh);
        } else {
          if (sx < 1 || sx > s.gw || sy < 1 || sy > s.gh) continue;
        }

        var stepK = sx + "," + sy;
        var stepStack = s.map[stepK];
        var canLandStep = false;
        if (!stepStack || !stepStack.length) {
          canLandStep = true;
        } else if (s.mode !== "OFK" && !squareHasEnemyOf(s, sx, sy, player)) {
          canLandStep = true;
        }

        if (canLandStep) {
          var isBaseEntry = oppBases.some(function (b) { return b.x === sx && b.y === sy; });
          if (!onlyTactical || isBaseEntry) {
            var stepMove = px + "" + py + cfgText + "-" + sx + "" + sy;
            if (!seenMap[stepMove]) { seenMap[stepMove] = true; moves.push(stepMove); }
          }
        }
      }

      // Multi-hop jumps
      dfsJumpChains(s, px, py, px, py, pod, [], "", moves, seenMap, onlyTactical, true);
    }
  });

  return moves;
}

function applySingleHopChain(s, move, player, deferCapturesList) {
  var pIdx = playerIndex(player);
  var res = s.reserves[pIdx];
  var hops = move.split("-");
  var origM = /^([1-9])([1-9])([a-hA-H!]*)/.exec(hops[0]);
  if (!origM) return;
  var curX = parseInt(origM[1], 10);
  var curY = parseInt(origM[2], 10);
  var fromK = curX + "," + curY;
  var rawCfgMatch = origM[3] || "";
  var normCfgMatch = rawCfgMatch ? formatProngConfig(parseProngConfig(rawCfgMatch).prongs) : "";

  if (!s.map[fromK] || !s.map[fromK].length) return;

  var podIdx = -1;
  for (var pi = 0; pi < s.map[fromK].length; pi++) {
    if (s.map[fromK][pi].owner === player) {
      var pText = formatProngConfig(s.map[fromK][pi].cfg.prongs);
      if (!rawCfgMatch || pText === normCfgMatch || s.map[fromK][pi].cfg.text === rawCfgMatch) {
        podIdx = pi;
        break;
      }
    }
  }
  if (podIdx === -1) {
    for (var pi2 = 0; pi2 < s.map[fromK].length; pi2++) {
      if (s.map[fromK][pi2].owner === player) { podIdx = pi2; break; }
    }
  }
  if (podIdx === -1) return;

  var movingPod = s.map[fromK].splice(podIdx, 1)[0];
  if (!s.map[fromK].length) delete s.map[fromK];

  for (var h = 1; h < hops.length; h++) {
    var hopM = /^([1-9])([1-9])(x?)/.exec(hops[h]);
    if (!hopM) break;
    var toX = parseInt(hopM[1], 10);
    var toY = parseInt(hopM[2], 10);
    var isCap = (hopM[3] === "x");

    var dx = toX - curX, dy = toY - curY;
    if (s.edgeless) {
      if (dx > s.gw / 2) dx -= s.gw;
      else if (dx < -s.gw / 2) dx += s.gw;
      if (dy > s.gh / 2) dy -= s.gh;
      else if (dy < -s.gh / 2) dy += s.gh;
    }

    if (isCap || Math.abs(dx) === 2 || Math.abs(dy) === 2) {
      var midX = curX + dx / 2, midY = curY + dy / 2;
      if (s.edgeless) {
        midX = wrapCoord(midX, s.gw);
        midY = wrapCoord(midY, s.gh);
      }
      var midK = midX + "," + midY;

      if (isCap) {
        if (deferCapturesList) {
          if (deferCapturesList.indexOf(midK) === -1) deferCapturesList.push(midK);
        } else if (s.map[midK] && s.map[midK].length) {
          var remaining = [];
          for (var v = 0; v < s.map[midK].length; v++) {
            var vPod = s.map[midK][v];
            if (!areAllied(s.mode, vPod.owner, player)) {
              var vIdx = playerIndex(vPod.owner);
              s.reserves[vIdx].capturedPods++;
              for (var d = 0; d < 8; d++) {
                if (vPod.cfg.prongs[d] === 1) {
                  res.freeProngs++;
                } else if (vPod.cfg.prongs[d] === 2) {
                  if (s.superProngs) res.freeSuper++;
                  else res.freeProngs++;
                }
              }
            } else {
              remaining.push(vPod);
            }
          }
          if (remaining.length) s.map[midK] = remaining;
          else delete s.map[midK];
        }
      }
    }
    curX = toX; curY = toY;
  }

  var toK = curX + "," + curY;
  if (!s.map[toK]) s.map[toK] = [];
  s.map[toK].push(movingPod);
}

function check4PlayerElimination(s, player) {
  if (s.mode !== "4" && s.mode !== "4P") return;
  var homeCoords = [{x:3,y:3}, {x:3,y:7}, {x:7,y:7}, {x:7,y:3}];
  var pIdx = playerIndex(player);
  var res = s.reserves[pIdx];

  for (var pl = 0; pl < 4; pl++) {
    if ((s.elimMask & (1 << pl)) === 0) {
      var hCoord = homeCoords[pl];
      var seatOwner = ["A", "B", "C", "D"][pl];
      if (squareHasEnemyOf(s, hCoord.x, hCoord.y, seatOwner)) {
        s.elimMask |= (1 << pl);
        Object.keys(s.map).forEach(function (mk) {
          var keep = [];
          (s.map[mk] || []).forEach(function (sqPod) {
            if (sqPod.owner === seatOwner) {
              s.reserves[pl].capturedPods++;
              for (var di = 0; di < 8; di++) {
                if (sqPod.cfg.prongs[di] === 1) res.freeProngs++;
                else if (sqPod.cfg.prongs[di] === 2) {
                  if (s.superProngs) res.freeSuper++;
                  else res.freeProngs++;
                }
              }
            } else {
              keep.push(sqPod);
            }
          });
          if (keep.length) s.map[mk] = keep;
          else delete s.map[mk];
        });
      }
    }
  }
}

function checkWinner(s) {
  if (s.mode === "4" || s.mode === "4P") {
    var acWinsBases = squareHasEnemyOf(s, 3, 7, "B") && squareHasEnemyOf(s, 7, 3, "D");
    if (acWinsBases) {
      s.status = "O";
      s.turn = "-";
      s.winner = "A";
      s.termination = "BASE_CAPTURE";
      return s;
    }
    var bdWinsBases = squareHasEnemyOf(s, 3, 3, "A") && squareHasEnemyOf(s, 7, 7, "C");
    if (bdWinsBases) {
      s.status = "O";
      s.turn = "-";
      s.winner = "B";
      s.termination = "BASE_CAPTURE";
      return s;
    }

    var podCounts = { A: 0, B: 0, C: 0, D: 0 };
    Object.keys(s.map).forEach(function (k) {
      (s.map[k] || []).forEach(function (p) {
        if (podCounts[p.owner] !== undefined) podCounts[p.owner]++;
      });
    });

    var bOut = (s.elimMask & 2) !== 0 || (podCounts.B === 0 && (!s.reserves[1] || s.reserves[1].freePods === 0));
    var dOut = (s.elimMask & 8) !== 0 || (podCounts.D === 0 && (!s.reserves[3] || s.reserves[3].freePods === 0));
    var aOut = (s.elimMask & 1) !== 0 || (podCounts.A === 0 && (!s.reserves[0] || s.reserves[0].freePods === 0));
    var cOut = (s.elimMask & 4) !== 0 || (podCounts.C === 0 && (!s.reserves[2] || s.reserves[2].freePods === 0));
    var bdElim = bOut && dOut;
    var acElim = aOut && cOut;

    if (bdElim) {
      s.status = "O";
      s.turn = "-";
      s.winner = "A";
      s.termination = "TOTAL_CAPTURE";
      return s;
    }
    if (acElim) {
      s.status = "O";
      s.turn = "-";
      s.winner = "B";
      s.termination = "TOTAL_CAPTURE";
      return s;
    }
    return s;
  }

  var oppBasesA = getOpponentBases(s.mode, "A", s.bases);
  var oppBasesB = getOpponentBases(s.mode, "B", s.bases);
  var needAll = (s.mode !== "OFK" && s.bases === 3);

  var aWon = needAll
    ? oppBasesA.every(function (b) { return isSquareOccupiedBy(s, b.x, b.y, "A"); })
    : oppBasesA.some(function (b) { return isSquareOccupiedBy(s, b.x, b.y, "A"); });
  if (aWon) {
    s.status = "O";
    s.turn = "-";
    s.winner = "A";
    s.termination = "BASE_CAPTURE";
    return s;
  }

  var bWon = needAll
    ? oppBasesB.every(function (b) { return isSquareOccupiedBy(s, b.x, b.y, "B"); })
    : oppBasesB.some(function (b) { return isSquareOccupiedBy(s, b.x, b.y, "B"); });
  if (bWon) {
    s.status = "O";
    s.turn = "-";
    s.winner = "B";
    s.termination = "BASE_CAPTURE";
    return s;
  }

  var podsA = 0, podsB = 0;
  Object.keys(s.map).forEach(function (k) {
    (s.map[k] || []).forEach(function (p) {
      if (p.owner === "A") podsA++;
      else if (p.owner === "B") podsB++;
    });
  });

  var freeA = (s.mode === "OFK" || !s.reserves[0]) ? 0 : s.reserves[0].freePods;
  var freeB = (s.mode === "OFK" || !s.reserves[1]) ? 0 : s.reserves[1].freePods;
  if (podsA === 0 && freeA === 0) {
    s.status = "O";
    s.turn = "-";
    s.winner = "B";
    s.termination = "TOTAL_CAPTURE";
    return s;
  }
  if (podsB === 0 && freeB === 0) {
    s.status = "O";
    s.turn = "-";
    s.winner = "A";
    s.termination = "TOTAL_CAPTURE";
    return s;
  }

  return s;
}

function hasAnyLegalMove(s, player) {
  var pIdx = playerIndex(player);
  var res = s.reserves[pIdx];
  var keys = Object.keys(s.map);
  for (var i = 0; i < keys.length; i++) {
    var stack = s.map[keys[i]] || [];
    for (var p = 0; p < stack.length; p++) {
      var pod = stack[p];
      if (pod.owner !== player) continue;
      var cnt = pod.cfg ? pod.cfg.count : 0;
      if (cnt < 8 && res && (res.freeProngs > 0 || (s.superProngs && res.freeSuper > 0))) {
        return true;
      }
      if (s.mode !== "OFK" && cnt > 0 && cnt < 8 && pod.cfg && pod.cfg.prongs) {
        for (var d = 0; d < 8; d++) {
          if (pod.cfg.prongs[d] === 1) return true;
        }
      }
    }
  }
  return generateMoves(s, player).length > 0;
}

function advanceTurn(s, player) {
  if (s.mode === "4" || s.mode === "4P") {
    var seats = ["A", "B", "C", "D"];
    var curIdx = seats.indexOf(player);
    var foundNext = false;
    for (var step = 1; step <= 4; step++) {
      var nextIdx = (curIdx + step) % 4;
      if ((s.elimMask & (1 << nextIdx)) !== 0) continue;
      var nextP = seats[nextIdx];
      if (hasAnyLegalMove(s, nextP)) {
        s.turn = nextP;
        foundNext = true;
        break;
      }
      var partIdx = partnerIndex("4P", nextIdx);
      if ((s.elimMask & (1 << partIdx)) === 0) {
        if (hasAnyLegalMove(s, seats[partIdx])) continue;
      }
    }
    if (!foundNext) {
      s.status = "O";
      s.turn = "-";
      s.winner = (player === "A" || player === "C") ? "A" : "B";
      s.termination = "STALEMATE";
    }
  } else {
    var nextPlayer = (player === "A") ? "B" : "A";
    if (!hasAnyLegalMove(s, nextPlayer)) {
      s.status = "O";
      s.turn = "-";
      s.winner = player;
      s.termination = "STALEMATE";
    } else {
      s.turn = nextPlayer;
    }
  }
}

function applyMoveToState(s, move, player) {
  var pIdx = playerIndex(player);
  var res = s.reserves[pIdx];

  // Super-prelude: e.g. "33a!-34;34a!+b"
  if (move.indexOf(";") !== -1) {
    var parts = move.split(";");
    for (var sp = 0; sp < parts.length - 1; sp++) {
      var preMove = parts[sp].trim();
      if (preMove) applySingleHopChain(s, preMove, player);
    }
    move = parts[parts.length - 1].trim();
  }

  // Composite move: e.g. "37df-46,37e-36"
  if (move.indexOf(",") !== -1 && move.indexOf("-") !== -1) {
    var subMoves = move.split(",");
    var deferredCaps = [];
    for (var sm = 0; sm < subMoves.length; sm++) {
      var singleMove = subMoves[sm].trim();
      if (singleMove) applySingleHopChain(s, singleMove, player, deferredCaps);
    }
    for (var dc = 0; dc < deferredCaps.length; dc++) {
      var midK = deferredCaps[dc];
      if (s.map[midK] && s.map[midK].length) {
        var remaining = [];
        for (var v = 0; v < s.map[midK].length; v++) {
          var vPod = s.map[midK][v];
          if (!areAllied(s.mode, vPod.owner, player)) {
            var vIdx = playerIndex(vPod.owner);
            s.reserves[vIdx].capturedPods++;
            for (var d = 0; d < 8; d++) {
              if (vPod.cfg.prongs[d] === 1) res.freeProngs++;
              else if (vPod.cfg.prongs[d] === 2) {
                if (s.superProngs) res.freeSuper++;
                else res.freeProngs++;
              }
            }
          } else {
            remaining.push(vPod);
          }
        }
        if (remaining.length) s.map[midK] = remaining;
        else delete s.map[midK];
      }
    }
    if (s.mode === "4" || s.mode === "4P") {
      check4PlayerElimination(s, player);
    }
    checkWinner(s);
    if (s.status === "O") return s;
    advanceTurn(s, player);
    return s;
  }

  // Regular Move:
  // 1. Pod placement: 33 or 33*
  var mPlace = /^\(?([1-9])([1-9])\)?(\*?)$/.exec(move);
  if (mPlace && move.indexOf("-") === -1 && move.indexOf("+") === -1) {
    var x = parseInt(mPlace[1], 10);
    var y = parseInt(mPlace[2], 10);
    var isLib = (mPlace[3] === "*");
    var k = x + "," + y;
    if (!s.map[k]) s.map[k] = [];

    var partnerIdx = partnerIndex(s.mode, pIdx);
    var isPartnerRevival = (s.mode === "4" || s.mode === "4P") &&
                           (s.elimMask & (1 << partnerIdx)) !== 0 &&
                           isPartnerHome(s.mode, partnerIdx, x, y);

    if (isPartnerRevival) {
      if (s.reserves[partnerIdx]) {
        s.reserves[partnerIdx].capturedPods = Math.max(0, s.reserves[partnerIdx].capturedPods - 1);
      }
      s.elimMask &= ~(1 << partnerIdx);
      s.map[k].push({
        owner: ["A", "B", "C", "D"][partnerIdx],
        cfg: { prongs: [0, 0, 0, 0, 0, 0, 0, 0], count: 0, text: "" }
      });
    } else if (isLib || (res && res.freePods === 0 && res.capturedPods > 0)) {
      if (res) res.capturedPods = Math.max(0, res.capturedPods - 1);
      s.map[k].push({
        owner: player,
        cfg: { prongs: [0, 0, 0, 0, 0, 0, 0, 0], count: 0, text: "" }
      });
    } else {
      if (res) res.freePods = Math.max(0, res.freePods - 1);
      s.map[k].push({
        owner: player,
        cfg: { prongs: [0, 0, 0, 0, 0, 0, 0, 0], count: 0, text: "" }
      });
    }
  }
  // 2. Prong repositioning
  else if (move.indexOf("+") !== -1 && move.indexOf("-") !== -1) {
    var mRepo = /^([1-9])([1-9])([a-hA-H!]*)\+([a-hA-H])-([a-hA-H])(!?)$/.exec(move) ||
                /^([1-9])([1-9])([a-hA-H!]*)-([a-hA-H])\+([a-hA-H])(!?)$/.exec(move);
    if (mRepo) {
      var mx = mRepo[1], my = mRepo[2], rawRepoCfg = mRepo[3] || "";
      var normRepoCfg = rawRepoCfg ? formatProngConfig(parseProngConfig(rawRepoCfg).prongs) : "";
      var addCh = (move.indexOf("+") < move.indexOf("-")) ? mRepo[4].toLowerCase() : mRepo[5].toLowerCase();
      var remCh = (move.indexOf("+") < move.indexOf("-")) ? mRepo[5].toLowerCase() : mRepo[4].toLowerCase();
      var addIdx = DIR_CHARS.indexOf(addCh), remIdx = DIR_CHARS.indexOf(remCh);
      var mk = mx + "," + my;
      if (s.map[mk] && s.map[mk].length && remIdx >= 0 && addIdx >= 0) {
        var foundPod = null;
        for (var p = 0; p < s.map[mk].length; p++) {
          var cand = s.map[mk][p];
          if (cand.owner === player && cand.cfg.prongs[remIdx] && !cand.cfg.prongs[addIdx]) {
            var candText = formatProngConfig(cand.cfg.prongs);
            if (!rawRepoCfg || candText === normRepoCfg || cand.cfg.text === rawRepoCfg) {
              foundPod = cand;
              break;
            }
          }
        }
        if (!foundPod) {
          for (var p2 = 0; p2 < s.map[mk].length; p2++) {
            var cand2 = s.map[mk][p2];
            if (cand2.owner === player && cand2.cfg.prongs[remIdx] && !cand2.cfg.prongs[addIdx]) {
              foundPod = cand2;
              break;
            }
          }
        }
        if (foundPod) {
          var pt = foundPod.cfg.prongs[remIdx];
          foundPod.cfg.prongs[remIdx] = 0;
          foundPod.cfg.prongs[addIdx] = pt;
          foundPod.cfg.text = formatProngConfig(foundPod.cfg.prongs);
        }
      }
    }
  }
  // 3. Prong addition
  else if (move.indexOf("+") !== -1 || /^([1-9])([1-9])([a-hA-H])$/.test(move)) {
    var mx, my, rawAddCfg = "", addCh = "", isSuper = false;
    var mAdd = /^([1-9])([1-9])([a-hA-H!]*)\+([a-hA-H])(!?)$/.exec(move);
    if (mAdd) {
      mx = mAdd[1]; my = mAdd[2]; rawAddCfg = mAdd[3] || ""; addCh = mAdd[4].toLowerCase();
      isSuper = (mAdd[5] === "!" || mAdd[4] === mAdd[4].toUpperCase());
    } else {
      var mAddShort = /^([1-9])([1-9])([a-hA-H])$/.exec(move);
      if (mAddShort) {
        mx = mAddShort[1]; my = mAddShort[2]; addCh = mAddShort[3].toLowerCase();
      }
    }
    var normAddCfg = rawAddCfg ? formatProngConfig(parseProngConfig(rawAddCfg).prongs) : "";
    var dIdx = DIR_CHARS.indexOf(addCh);
    var mk = mx + "," + my;
    if (mx && s.map[mk] && s.map[mk].length && dIdx >= 0) {
      var foundPod = null;
      for (var p = 0; p < s.map[mk].length; p++) {
        var cand = s.map[mk][p];
        if (cand.owner === player && !cand.cfg.prongs[dIdx]) {
          var candText = formatProngConfig(cand.cfg.prongs);
          if (!rawAddCfg || candText === normAddCfg || cand.cfg.text === rawAddCfg) {
            foundPod = cand;
            break;
          }
        }
      }
      if (!foundPod) {
        for (var p2 = 0; p2 < s.map[mk].length; p2++) {
          var cand2 = s.map[mk][p2];
          if (cand2.owner === player && !cand2.cfg.prongs[dIdx]) {
            foundPod = cand2;
            break;
          }
        }
      }
      if (foundPod) {
        foundPod.cfg.prongs[dIdx] = isSuper ? 2 : 1;
        foundPod.cfg.count++;
        foundPod.cfg.text = formatProngConfig(foundPod.cfg.prongs);
        if (res) {
          if (isSuper && s.superProngs && res.freeSuper > 0) res.freeSuper--;
          else res.freeProngs = Math.max(0, res.freeProngs - 1);
        }
      }
    }
  }
  // 4. Pod moves / jumps
  else if (move.indexOf("-") !== -1) {
    applySingleHopChain(s, move, player);
  }

  if (s.mode === "4" || s.mode === "4P") {
    check4PlayerElimination(s, player);
  }
  checkWinner(s);
  if (s.status === "O") return s;
  advanceTurn(s, player);
  return s;
}

var DEFAULT_BOT_TUNING = {
  timeBudgetMs: 8000,
  maxSearchDepth: 8,
  maxQuiesceDepth: 2,
  quiesceBranchCap: 6,
  rootBranchCap: 40,
  treeBranchCap: 24,
  lazySmpThreads: 1,
  compositeMoveMaxPly: 1,
  rootScoreJitter: 20,
  capturedEnemyBaseBonus: 28000,
  lostAlliedBasePenalty: 32000,
  neutralOctiSquareBonus: 4000,
  closestPodDistWeight: 320,
  secondClosestPodDistWeight: 150,
  oneStepBaseThreatBonus: 8500,
  oneJumpBaseThreatBonus: 12000,
  twoPlyBaseThreatWeight: 2400,
  podZeroProngVal: 320,
  podOneProngVal: 950,
  podTwoProngVal: 1450,
  podThreePlusProngVal: 1750,
  normalProngVal: 220,
  superProngVal: 350,
  orthDiagVersatilityBonus: 550,
  diagOnlyParityPenalty: 350,
  firstArmedPodBonus: 600,
  secondArmedPodBonus: 1100,
  thirdArmedPodBonus: 600,
  singlePodMonopolyPenalty: 1000,
  loneOneProngRushPenalty: 550,
  leapfrogBatteryWeight: 550,
  leapfrogForwardBonus: 400,
  jumpReachableSquareWeight: 130,
  jumpReachStepWeight: 450,
  jumpCapturablePodWeight: 1200,
  jumpCapturableProngWeight: 200,
  hangingPodBasePenalty: 2200,
  hangingProngWeight: 320,
  ofkProngSeizureWeight: 600,
  opponentTurnHangingPct: 140,
  soleArmedPodDangerPenalty: 3500,
  wipeoutDangerPenalty: 16000,
  moveOrderBaseEntryBonus: 16000,
  moveOrderCaptureBonus: 8500,
  moveOrderLeapfrogBonus: 1400,
  moveOrderBlunderPenalty: 7000
};

var BOT_PROFILES = {
  1: Object.assign({}, DEFAULT_BOT_TUNING, {
    timeBudgetMs: 2000,
    maxSearchDepth: 3,
    maxQuiesceDepth: 0,
    quiesceBranchCap: 0,
    rootBranchCap: 24,
    treeBranchCap: 16,
    rootScoreJitter: 160,
    closestPodDistWeight: 280,
    secondClosestPodDistWeight: 0,
    oneStepBaseThreatBonus: 7500,
    oneJumpBaseThreatBonus: 10000,
    twoPlyBaseThreatWeight: 0,
    firstArmedPodBonus: 500,
    secondArmedPodBonus: 400,
    thirdArmedPodBonus: 200,
    singlePodMonopolyPenalty: 300,
    leapfrogBatteryWeight: 200,
    leapfrogForwardBonus: 150,
    jumpReachableSquareWeight: 60,
    jumpReachStepWeight: 150,
    hangingPodBasePenalty: 1800,
    hangingProngWeight: 250,
    ofkProngSeizureWeight: 350,
    opponentTurnHangingPct: 100,
    soleArmedPodDangerPenalty: 1000,
    moveOrderBlunderPenalty: 2000
  }),
  2: Object.assign({}, DEFAULT_BOT_TUNING, {
    timeBudgetMs: 8000,
    maxSearchDepth: 8,
    maxQuiesceDepth: 2,
    quiesceBranchCap: 6,
    rootBranchCap: 40,
    treeBranchCap: 24,
    rootScoreJitter: 0
  }),
  3: Object.assign({}, DEFAULT_BOT_TUNING, {
    timeBudgetMs: 15000,
    maxSearchDepth: 10,
    maxQuiesceDepth: 4,
    quiesceBranchCap: 10,
    rootBranchCap: 56,
    treeBranchCap: 32,
    rootScoreJitter: 0,
    closestPodDistWeight: 350,
    secondClosestPodDistWeight: 220,
    oneStepBaseThreatBonus: 9500,
    oneJumpBaseThreatBonus: 14000,
    twoPlyBaseThreatWeight: 4200,
    firstArmedPodBonus: 700,
    secondArmedPodBonus: 1500,
    thirdArmedPodBonus: 900,
    singlePodMonopolyPenalty: 1400,
    leapfrogBatteryWeight: 800,
    leapfrogForwardBonus: 600,
    jumpReachableSquareWeight: 180,
    jumpReachStepWeight: 650,
    hangingPodBasePenalty: 2600,
    hangingProngWeight: 380,
    ofkProngSeizureWeight: 750,
    opponentTurnHangingPct: 180,
    soleArmedPodDangerPenalty: 6000,
    moveOrderBlunderPenalty: 11000
  })
};

function torusDist(a, b, dim, edgeless) {
  var d = Math.abs(a - b);
  if (edgeless && dim > 0) d = Math.min(d, dim - d);
  return d;
}

function chebyshevDist(x1, y1, x2, y2, gw, gh, edgeless) {
  return Math.max(torusDist(x1, x2, gw, edgeless), torusDist(y1, y2, gh, edgeless));
}

function minChebyshevToTargets(x, y, targets, gw, gh, edgeless) {
  if (!targets || !targets.length) return 0;
  var best = 999;
  for (var i = 0; i < targets.length; i++) {
    var d = chebyshevDist(x, y, targets[i].x, targets[i].y, gw, gh, edgeless);
    if (d < best) best = d;
  }
  return best;
}

function getUncapturedOpponentBases(s, player) {
  var allOpp = getOpponentBases(s.mode, player, s.bases);
  if (!allOpp || !allOpp.length) return [];
  var partner = (s.mode === "4" || s.mode === "4P") ? partnerOf(s.mode, player) : null;
  var uncap = [];
  for (var i = 0; i < allOpp.length; i++) {
    var b = allOpp[i];
    var occ = isSquareOccupiedBy(s, b.x, b.y, player) ||
              (partner && isSquareOccupiedBy(s, b.x, b.y, partner));
    if (!occ) uncap.push(b);
  }
  return uncap.length ? uncap : allOpp;
}

function evaluateJumpThreatDfs(s, curX, curY, startX, startY, depth, podOwner, pod, targets, visitedDest, jumpedMap, threat) {
  if (depth > 4) return;
  var prongs = (pod.cfg && pod.cfg.prongs) ? pod.cfg.prongs : null;
  if (!prongs) return;
  var startStackLen = (s.map[startX + "," + startY] || []).length;

  for (var d = 0; d < 8; d++) {
    if (!prongs[d]) continue;
    var dx = DIR_DELTAS[d][0], dy = DIR_DELTAS[d][1];
    var jx = curX + dx, jy = curY + dy;
    var destX = curX + 2 * dx, destY = curY + 2 * dy;
    if (s.edgeless) {
      jx = wrapCoord(jx, s.gw); jy = wrapCoord(jy, s.gh);
      destX = wrapCoord(destX, s.gw); destY = wrapCoord(destY, s.gh);
    } else {
      if (jx < 1 || jx > s.gw || jy < 1 || jy > s.gh) continue;
      if (destX < 1 || destX > s.gw || destY < 1 || destY > s.gh) continue;
    }

    var jKey = jx + "," + jy;
    if (jumpedMap[jKey]) continue;

    var midStack = s.map[jKey];
    var midCount = midStack ? ((jx === startX && jy === startY) ? (midStack.length - 1) : midStack.length) : 0;
    if (midCount <= 0) continue;

    var destKey = destX + "," + destY;
    var destStack = s.map[destKey];
    var canLand = (!destStack || !destStack.length) ||
                  (destX === startX && destY === startY && startStackLen === 1);
    if (!canLand) continue;

    if (visitedDest[destKey]) continue;
    visitedDest[destKey] = true;
    threat.reachableSquares++;
    var tDist = minChebyshevToTargets(destX, destY, targets, s.gw, s.gh, s.edgeless);
    if (tDist < threat.minJumpDistToTargets) threat.minJumpDistToTargets = tDist;

    if (squareHasEnemyOf(s, jx, jy, podOwner)) {
      if (!threat.capturedSqMap[jKey]) {
        threat.capturedSqMap[jKey] = true;
        for (var pi = 0; pi < midStack.length; pi++) {
          var mp = midStack[pi];
          if (!areAllied(s.mode, mp.owner, podOwner)) {
            threat.capturablePods++;
            threat.capturableProngs += (mp.cfg ? mp.cfg.count : 0);
          }
        }
      }
    }

    jumpedMap[jKey] = true;
    evaluateJumpThreatDfs(s, destX, destY, startX, startY, depth + 1, podOwner, pod, targets, visitedDest, jumpedMap, threat);
    jumpedMap[jKey] = false;
  }
}

function evaluatePodJumpThreat(s, x, y, pod, targets) {
  var threat = {
    reachableSquares: 0,
    capturablePods: 0,
    capturableProngs: 0,
    minJumpDistToTargets: 999,
    capturedSqMap: {}
  };
  evaluateJumpThreatDfs(s, x, y, x, y, 0, pod.owner, pod, targets, {}, {}, threat);
  return threat;
}

function filterDominatedMoves(moves) {
  if (!moves || moves.length <= 1) return moves || [];
  var hasCapVariant = {};
  var anyCapFound = false;
  for (var i = 0; i < moves.length; i++) {
    var m = moves[i];
    if (m.indexOf("-") !== -1 && m.indexOf("+") === -1 && m.indexOf("x") !== -1) {
      hasCapVariant[m.replace(/x/g, "")] = true;
      anyCapFound = true;
    }
  }
  if (!anyCapFound) return moves;
  var filtered = [];
  for (var j = 0; j < moves.length; j++) {
    var mv = moves[j];
    if (mv.indexOf("-") !== -1 && mv.indexOf("+") === -1 && mv.indexOf("x") === -1) {
      if (hasCapVariant[mv]) continue;
    }
    filtered.push(mv);
  }
  return filtered.length ? filtered : moves;
}

function evaluateState(s, botSide, prof) {
  if (!prof) prof = BOT_PROFILES[2];
  if (s.status === "O") {
    if (s.winner === botSide || areAllied(s.mode, s.winner, botSide)) return 1000000;
    if (s.winner && s.winner !== "-") return -1000000;
    return 0;
  }

  var score = 0;
  var is4p = (s.mode === "4" || s.mode === "4P");
  var sampleEnemy = (botSide === "A" || botSide === "C") ? "B" : "A";
  var allOppBases = getOpponentBases(s.mode, botSide, s.bases);
  var allAllyHomeSquares = getHomeSquares(s.mode, botSide, s.bases);
  if (is4p) {
    var partSide = partnerOf(s.mode, botSide);
    allAllyHomeSquares = allAllyHomeSquares.concat(getHomeSquares(s.mode, partSide, s.bases));
  }
  var allyTargets = getUncapturedOpponentBases(s, botSide);
  var enemyTargets = getUncapturedOpponentBases(s, sampleEnemy);

  // 1. Captured Enemy Bases & Lost Allied Bases
  for (var bi = 0; bi < allOppBases.length; bi++) {
    var b = allOppBases[bi];
    if (isSquareOccupiedBy(s, b.x, b.y, botSide) ||
        (is4p && isSquareOccupiedBy(s, b.x, b.y, partnerOf(s.mode, botSide)))) {
      score += prof.capturedEnemyBaseBonus;
    }
  }
  for (var hi = 0; hi < allAllyHomeSquares.length; hi++) {
    var ah = allAllyHomeSquares[hi];
    if (squareHasEnemyOf(s, ah.x, ah.y, botSide)) {
      score -= prof.lostAlliedBasePenalty;
    }
  }

  // Neutral OCTI squares in 4P
  if (is4p) {
    var neutrals = [{x:5, y:3}, {x:5, y:7}];
    for (var ni = 0; ni < neutrals.length; ni++) {
      var no = neutrals[ni];
      if (isSquareOccupiedBy(s, no.x, no.y, botSide) ||
          isSquareOccupiedBy(s, no.x, no.y, partnerOf(s.mode, botSide))) {
        score += prof.neutralOctiSquareBonus;
      } else if (squareHasEnemyOf(s, no.x, no.y, botSide)) {
        score -= prof.neutralOctiSquareBonus;
      }
    }
  }

  var totalAllyPodsOnBoard = 0;
  var totalEnemyPodsOnBoard = 0;
  var allyArmedPods = 0;
  var enemyArmedPods = 0;
  var allyUnarmedHomePods = 0;
  var enemyUnarmedHomePods = 0;
  var allyPodCoords = [];
  var enemyPodCoords = [];
  var allyJumpThreatMap = {};
  var enemyJumpThreatMap = {};

  var mapKeys = Object.keys(s.map);
  for (var ki = 0; ki < mapKeys.length; ki++) {
    var k = mapKeys[ki];
    var stack = s.map[k] || [];
    var numPods = stack.length;
    if (!numPods) continue;

    var coords = k.split(",");
    var px = parseInt(coords[0], 10);
    var py = parseInt(coords[1], 10);
    var sqOwner = stack[0].owner;
    var sqAlly = areAllied(s.mode, sqOwner, botSide);

    if (sqAlly) totalAllyPodsOnBoard += numPods;
    else totalEnemyPodsOnBoard += numPods;

    var sqOppBases = sqAlly ? allOppBases : allAllyHomeSquares;
    var sqHomeBases = sqAlly ? allAllyHomeSquares : allOppBases;
    var onCapturedEnemyBase = sqOppBases.some(function (eb) { return eb.x === px && eb.y === py; });
    var onAlliedHome = sqHomeBases.some(function (hb) { return hb.x === px && hb.y === py; });
    var isNeutralOcti = is4p && ((px === 5 && py === 3) || (px === 5 && py === 7));

    var unarmedCount = 0;
    for (var ui = 0; ui < numPods; ui++) {
      var uProngs = stack[ui].cfg ? stack[ui].cfg.count : 0;
      if (uProngs === 0) {
        unarmedCount++;
        if (onAlliedHome) {
          if (sqAlly) allyUnarmedHomePods++;
          else enemyUnarmedHomePods++;
        }
      }
    }

    if (numPods > 1 && !onCapturedEnemyBase) {
      var stackPen = (numPods - 1) * 450 + unarmedCount * 950;
      score += sqAlly ? -stackPen : stackPen;
    }

    for (var pi = 0; pi < numPods; pi++) {
      var pod = stack[pi];
      var owner = pod.owner;
      var isAlly = areAllied(s.mode, owner, botSide);
      var targets = isAlly ? allyTargets : enemyTargets;
      var numProngs = pod.cfg ? pod.cfg.count : 0;

      if (numProngs > 0) {
        if (isAlly) {
          allyPodCoords.push({ x: px, y: py });
          allyArmedPods++;
        } else {
          enemyPodCoords.push({ x: px, y: py });
          enemyArmedPods++;
        }
      }

      var dist = minChebyshevToTargets(px, py, targets, s.gw, s.gh, s.edgeless);
      var podVal = 0;
      if (numProngs === 0 && !onCapturedEnemyBase && !isNeutralOcti) {
        podVal = prof.podZeroProngVal;
      } else if (numProngs === 1) {
        podVal = prof.podOneProngVal + Math.max(0, 10 - dist) * 110;
        if (!onAlliedHome && dist <= 4 && dist >= 2) {
          podVal -= prof.loneOneProngRushPenalty;
        }
      } else if (numProngs === 2) {
        podVal = prof.podTwoProngVal + Math.max(0, 10 - dist) * 180;
      } else if (numProngs >= 3) {
        podVal = prof.podThreePlusProngVal + Math.max(0, 10 - dist) * 210;
      }

      var hasOrth = false, hasDiag = false;
      if (pod.cfg && pod.cfg.prongs) {
        for (var d = 0; d < 8; d++) {
          var pt = pod.cfg.prongs[d];
          if (!pt) continue;
          podVal += (pt === 2) ? prof.superProngVal : prof.normalProngVal;
          if (d % 2 === 0) hasOrth = true; else hasDiag = true;

          // Forward prong orientation bonus
          if (owner === "A" || owner === "C") {
            if (d === 0) podVal += 380;
            else if (d === 1 || d === 7) podVal += 260;
            else if (d === 2 || d === 6) podVal += 120;
          } else {
            if (d === 4) podVal += 380;
            else if (d === 3 || d === 5) podVal += 260;
            else if (d === 2 || d === 6) podVal += 120;
          }

          var dx = DIR_DELTAS[d][0], dy = DIR_DELTAS[d][1];
          var nx = px + dx, ny = py + dy;
          if (s.edgeless) {
            nx = wrapCoord(nx, s.gw); ny = wrapCoord(ny, s.gh);
          }
          if (nx >= 1 && nx <= s.gw && ny >= 1 && ny <= s.gh) {
            var ndist = minChebyshevToTargets(nx, ny, targets, s.gw, s.gh, s.edgeless);
            if (ndist < dist) podVal += 140;

            var nKey = nx + "," + ny;
            var nStack = s.map[nKey];
            var canStepBase = (s.mode === "OFK")
              ? (!nStack || !nStack.length)
              : !squareHasEnemyOf(s, nx, ny, owner);
            if (ndist === 0 && canStepBase) {
              podVal += prof.oneStepBaseThreatBonus;
            }

            // Leapfrog battery detection
            if (nStack && nStack.length && areAllied(s.mode, nStack[0].owner, owner)) {
              var jx = px + 2 * dx, jy = py + 2 * dy;
              if (s.edgeless) {
                jx = wrapCoord(jx, s.gw); jy = wrapCoord(jy, s.gh);
              }
              if (jx >= 1 && jx <= s.gw && jy >= 1 && jy <= s.gh) {
                var jStack = s.map[jx + "," + jy];
                var canJumpBase = (s.mode === "OFK")
                  ? (!jStack || !jStack.length)
                  : !squareHasEnemyOf(s, jx, jy, owner);
                if (!jStack || !jStack.length || canJumpBase) {
                  podVal += prof.leapfrogBatteryWeight;
                  var jdist = minChebyshevToTargets(jx, jy, targets, s.gw, s.gh, s.edgeless);
                  if (jdist < dist) podVal += prof.leapfrogForwardBonus;
                }
              }
            }

            // 1-jump base threat
            var jx2 = px + 2 * dx, jy2 = py + 2 * dy;
            if (s.edgeless) {
              jx2 = wrapCoord(jx2, s.gw); jy2 = wrapCoord(jy2, s.gh);
            }
            if (jx2 >= 1 && jx2 <= s.gw && jy2 >= 1 && jy2 <= s.gh) {
              var destStack2 = s.map[jx2 + "," + jy2];
              if (nStack && nStack.length && (!destStack2 || !destStack2.length)) {
                var jdist2 = minChebyshevToTargets(jx2, jy2, targets, s.gw, s.gh, s.edgeless);
                if (jdist2 === 0) podVal += prof.oneJumpBaseThreatBonus;
              }
            }
          }
        }
      }

      if (dist === 1 && prof.twoPlyBaseThreatWeight > 0) {
        podVal += prof.twoPlyBaseThreatWeight;
      }
      if (hasOrth && hasDiag) podVal += prof.orthDiagVersatilityBonus;
      else if (numProngs >= 2 && hasDiag && !hasOrth) podVal -= prof.diagOnlyParityPenalty;

      if (numProngs > 0) {
        var threat = evaluatePodJumpThreat(s, px, py, pod, targets);
        podVal += threat.reachableSquares * prof.jumpReachableSquareWeight;
        podVal += threat.capturablePods * prof.jumpCapturablePodWeight;
        podVal += threat.capturableProngs * prof.jumpCapturableProngWeight;
        if (threat.minJumpDistToTargets < dist) {
          podVal += (dist - threat.minJumpDistToTargets) * prof.jumpReachStepWeight;
        }
        var capKeys = Object.keys(threat.capturedSqMap);
        for (var cki = 0; cki < capKeys.length; cki++) {
          if (isAlly) allyJumpThreatMap[capKeys[cki]] = true;
          else enemyJumpThreatMap[capKeys[cki]] = true;
        }
      }

      score += isAlly ? podVal : -podVal;
    }
  }

  // Single-pod monopoly penalty (after counting all unarmed home pods across board)
  for (var mki = 0; mki < mapKeys.length; mki++) {
    var mStack = s.map[mapKeys[mki]] || [];
    for (var mpi = 0; mpi < mStack.length; mpi++) {
      var mPod = mStack[mpi];
      var mProngs = mPod.cfg ? mPod.cfg.count : 0;
      if (mProngs > 2) {
        var mAlly = areAllied(s.mode, mPod.owner, botSide);
        if (mAlly && allyUnarmedHomePods >= 2) {
          score -= (mProngs - 2) * prof.singlePodMonopolyPenalty;
        } else if (!mAlly && enemyUnarmedHomePods >= 2) {
          score += (mProngs - 2) * prof.singlePodMonopolyPenalty;
        }
      }
    }
  }

  // Army Development Bonuses
  if (allyArmedPods >= 1) score += prof.firstArmedPodBonus;
  if (allyArmedPods >= 2) score += prof.secondArmedPodBonus;
  if (allyArmedPods >= 3) score += prof.thirdArmedPodBonus;
  if (enemyArmedPods >= 1) score -= prof.firstArmedPodBonus;
  if (enemyArmedPods >= 2) score -= prof.secondArmedPodBonus;
  if (enemyArmedPods >= 3) score -= prof.thirdArmedPodBonus;

  // Hanging-Pod Vulnerability Penalty
  var opponentTurnNext = !areAllied(s.mode, s.turn, botSide);
  var prongWeight = (s.mode === "OFK") ? prof.ofkProngSeizureWeight : prof.hangingProngWeight;
  for (var hki = 0; hki < mapKeys.length; hki++) {
    var hk = mapKeys[hki];
    var hStack = s.map[hk] || [];
    if (!hStack.length) continue;
    var hAlly = areAllied(s.mode, hStack[0].owner, botSide);
    var prongsOnSq = 0;
    for (var hpi = 0; hpi < hStack.length; hpi++) {
      prongsOnSq += (hStack[hpi].cfg ? hStack[hpi].cfg.count : 0);
    }

    if (hAlly && enemyJumpThreatMap[hk]) {
      var vulnPen = hStack.length * prof.hangingPodBasePenalty + prongsOnSq * prongWeight;
      if (opponentTurnNext) vulnPen = Math.floor((vulnPen * prof.opponentTurnHangingPct) / 100);
      if (prongsOnSq > 0 && allyArmedPods === 1) vulnPen += prof.soleArmedPodDangerPenalty;
      if (hStack.length >= totalAllyPodsOnBoard) vulnPen += (prof.wipeoutDangerPenalty || 16000);
      score -= vulnPen;
    } else if (!hAlly && allyJumpThreatMap[hk]) {
      var vulnPen2 = hStack.length * prof.hangingPodBasePenalty + prongsOnSq * prongWeight;
      if (!opponentTurnNext) vulnPen2 = Math.floor((vulnPen2 * prof.opponentTurnHangingPct) / 100);
      if (prongsOnSq > 0 && enemyArmedPods === 1) vulnPen2 += prof.soleArmedPodDangerPenalty;
      if (hStack.length >= totalEnemyPodsOnBoard) vulnPen2 += (prof.wipeoutDangerPenalty || 16000);
      score += vulnPen2;
    }
  }

  if (totalAllyPodsOnBoard === 0) {
    score -= (prof.wipeoutDangerPenalty || 16000) + prof.soleArmedPodDangerPenalty;
  }
  if (totalEnemyPodsOnBoard === 0) {
    score += (prof.wipeoutDangerPenalty || 16000) + prof.soleArmedPodDangerPenalty;
  }

  // Multi-Base Minimum Chebyshev Distance (d_min1 and d_min2 per uncaptured target base)
  for (var ati = 0; ati < allyTargets.length; ati++) {
    var tb = allyTargets[ati];
    var dMin1 = 999, dMin2 = 999;
    for (var api = 0; api < allyPodCoords.length; api++) {
      var ap = allyPodCoords[api];
      var dA = chebyshevDist(ap.x, ap.y, tb.x, tb.y, s.gw, s.gh, s.edgeless);
      if (dA < dMin1) { dMin2 = dMin1; dMin1 = dA; }
      else if (dA < dMin2) { dMin2 = dA; }
    }
    if (dMin1 < 999) score += Math.max(0, 10 - dMin1) * prof.closestPodDistWeight;
    if (dMin2 < 999 && prof.secondClosestPodDistWeight > 0) {
      score += Math.max(0, 10 - dMin2) * prof.secondClosestPodDistWeight;
    }
  }

  for (var eti = 0; eti < enemyTargets.length; eti++) {
    var etb = enemyTargets[eti];
    var eMin1 = 999, eMin2 = 999;
    for (var epi = 0; epi < enemyPodCoords.length; epi++) {
      var ep = enemyPodCoords[epi];
      var dE = chebyshevDist(ep.x, ep.y, etb.x, etb.y, s.gw, s.gh, s.edgeless);
      if (dE < eMin1) { eMin2 = eMin1; eMin1 = dE; }
      else if (dE < eMin2) { eMin2 = dE; }
    }
    if (eMin1 < 999) score -= Math.max(0, 10 - eMin1) * prof.closestPodDistWeight;
    if (eMin2 < 999 && prof.secondClosestPodDistWeight > 0) {
      score -= Math.max(0, 10 - eMin2) * prof.secondClosestPodDistWeight;
    }
  }

  // Reserves and material bonuses (capturedPods is a penalty for that player!)
  var numP = is4p ? 4 : 2;
  for (var p = 0; p < numP; p++) {
    var pLetter = ["A", "B", "C", "D"][p];
    var isAlliedSeat = areAllied(s.mode, pLetter, botSide);
    var r = s.reserves[p];
    if (r) {
      var resVal = r.freePods * 450
                 - r.capturedPods * 380
                 + r.freeProngs * 50
                 + r.freeSuper * 150;
      var unarmedHome = isAlliedSeat ? allyUnarmedHomePods : enemyUnarmedHomePods;
      var podsOnBoardSeat = isAlliedSeat ? totalAllyPodsOnBoard : totalEnemyPodsOnBoard;
      if (s.mode !== "OFK" && r.freePods < 4 && unarmedHome >= 2 && podsOnBoardSeat >= 2 && r.freeProngs >= 6) {
        resVal -= (4 - r.freePods) * (1500 + unarmedHome * 1500);
      }
      if (is4p && (s.elimMask & (1 << p)) !== 0) {
        resVal -= 20000;
      }
      if (r.capturedPods > 0 && canLiberate(s, pLetter)) {
        resVal += 2500;
      }
      score += isAlliedSeat ? resVal : -resVal;
    }
  }

  return score;
}

function orderMoves(rawMoves, s, curPlayer, prof, ttBestMove) {
  if (!prof) prof = BOT_PROFILES[2];
  var moves = filterDominatedMoves(rawMoves);
  if (moves.length <= 1) return moves;

  var targets = getUncapturedOpponentBases(s, curPlayer);
  var homeSquares = getHomeSquares(s.mode, curPlayer, s.bases);
  var pIdx = playerIndex(curPlayer);
  var res = s.reserves[pIdx];
  var earlyDevelopment = !!(res && res.freeProngs >= 8);

  var hasUnarmedHomePod = false;
  for (var hi = 0; hi < homeSquares.length; hi++) {
    var hs = s.map[homeSquares[hi].x + "," + homeSquares[hi].y] || [];
    for (var hpi = 0; hpi < hs.length; hpi++) {
      if (hs[hpi].owner === curPlayer && (!hs[hpi].cfg || hs[hpi].cfg.count < 2)) {
        hasUnarmedHomePod = true;
        break;
      }
    }
    if (hasUnarmedHomePod) break;
  }

  // Compute enemy jump threat map + 1-hop empty-landing jump-capture lanes
  var enemyThreatMap = {};
  var mapKeys = Object.keys(s.map);
  for (var ki = 0; ki < mapKeys.length; ki++) {
    var k = mapKeys[ki];
    var stack = s.map[k] || [];
    if (!stack.length || !squareHasEnemyOf(s, parseInt(k.split(",")[0], 10), parseInt(k.split(",")[1], 10), curPlayer)) {
      continue;
    }
    var coords = k.split(",");
    var ex = parseInt(coords[0], 10), ey = parseInt(coords[1], 10);
    for (var pi = 0; pi < stack.length; pi++) {
      var ep = stack[pi];
      if (!areAllied(s.mode, ep.owner, curPlayer) && ep.cfg && ep.cfg.count > 0) {
        for (var d = 0; d < 8; d++) {
          if (ep.cfg.prongs[d]) {
            var dx = DIR_DELTAS[d][0], dy = DIR_DELTAS[d][1];
            var mx = ex + dx, my = ey + dy;
            var lx = ex + 2 * dx, ly = ey + 2 * dy;
            if (s.edgeless) {
              mx = wrapCoord(mx, s.gw); my = wrapCoord(my, s.gh);
              lx = wrapCoord(lx, s.gw); ly = wrapCoord(ly, s.gh);
            }
            if (mx >= 1 && mx <= s.gw && my >= 1 && my <= s.gh &&
                lx >= 1 && lx <= s.gw && ly >= 1 && ly <= s.gh) {
              var landStack = s.map[lx + "," + ly];
              if (!landStack || !landStack.length) {
                enemyThreatMap[mx + "," + my] = true;
              }
            }
          }
        }
      }
    }
  }

  var scored = moves.map(function (m) {
    if (ttBestMove && m === ttBestMove) {
      return { move: m, score: 200000 };
    }
    var sc = 0;
    var hasPlus = (m.indexOf("+") !== -1);
    var hasDash = (m.indexOf("-") !== -1);

    // 1. Prong reposition: e.g. 53a+b-a
    if (hasPlus && hasDash) {
      sc += 60;
      var repoM = /^([1-9])([1-9])[a-hA-H!]*\+([a-hA-H])-([a-hA-H])/.exec(m);
      if (repoM) {
        var rx = parseInt(repoM[1], 10), ry = parseInt(repoM[2], 10);
        var addD = DIR_CHARS.indexOf(repoM[3].toLowerCase());
        var rmD = DIR_CHARS.indexOf(repoM[4].toLowerCase());
        if (addD >= 0 && rmD >= 0) {
          var addDist = minChebyshevToTargets(rx + DIR_DELTAS[addD][0], ry + DIR_DELTAS[addD][1], targets, s.gw, s.gh, s.edgeless);
          var rmDist = minChebyshevToTargets(rx + DIR_DELTAS[rmD][0], ry + DIR_DELTAS[rmD][1], targets, s.gw, s.gh, s.edgeless);
          if (addDist < rmDist) sc += 200;
        }
      }
      return { move: m, score: sc };
    }

    // 2. Prong addition: e.g. 53+a or 53a+b!
    if (hasPlus && !hasDash) {
      sc += 650;
      if (m.indexOf("!") !== -1) sc += 200;
      var addM = /^([1-9])([1-9])([a-hA-H!]*)\+([a-hA-H])/.exec(m);
      if (addM) {
        var ax = parseInt(addM[1], 10), ay = parseInt(addM[2], 10);
        var cfgStr = addM[3] || "";
        var dirCh = addM[4].toLowerCase();
        var dirIdx = DIR_CHARS.indexOf(dirCh);
        if (dirIdx >= 0) {
          var curDist = minChebyshevToTargets(ax, ay, targets, s.gw, s.gh, s.edgeless);
          var nextDist = minChebyshevToTargets(ax + DIR_DELTAS[dirIdx][0], ay + DIR_DELTAS[dirIdx][1], targets, s.gw, s.gh, s.edgeless);
          if (nextDist < curDist) sc += 600;

          var parsedCfg = parseProngConfig(cfgStr);
          if (parsedCfg.count === 0) sc += 900;
          else if (parsedCfg.count < 3) sc += 450;

          var hasOrth = (dirIdx % 2 === 0);
          var hasDiag = (dirIdx % 2 === 1);
          for (var pd = 0; pd < 8; pd++) {
            if (parsedCfg.prongs[pd]) {
              if (pd % 2 === 0) hasOrth = true; else hasDiag = true;
            }
          }
          if (hasOrth && hasDiag) sc += 650;

          if (curPlayer === "A" || curPlayer === "C") {
            if (dirIdx === 0) sc += 1100;
            else if (dirIdx === 1 || dirIdx === 7) sc += 750;
            else if (dirIdx === 2 || dirIdx === 6) sc += 400;
          } else {
            if (dirIdx === 4) sc += 1100;
            else if (dirIdx === 3 || dirIdx === 5) sc += 750;
            else if (dirIdx === 2 || dirIdx === 6) sc += 400;
          }
        }
      }
      return { move: m, score: sc };
    }

    // 3. Pod move / jump: e.g. 53a-54 or 53a-55x-57
    if (hasDash && !hasPlus) {
      var hops = m.split("-");
      var origM = /^([1-9])([1-9])([a-hA-H!]*)/.exec(hops[0]);
      var lastHopM = /^([1-9])([1-9])/.exec(hops[hops.length - 1]);
      if (origM && lastHopM) {
        var sx = parseInt(origM[1], 10), sy = parseInt(origM[2], 10);
        var ex2 = parseInt(lastHopM[1], 10), ey2 = parseInt(lastHopM[2], 10);
        var podCfg = parseProngConfig(origM[3] || "");

        var startDist = minChebyshevToTargets(sx, sy, targets, s.gw, s.gh, s.edgeless);
        var endDist = minChebyshevToTargets(ex2, ey2, targets, s.gw, s.gh, s.edgeless);
        sc += (startDist - endDist) * 550;

        var entersTargetBase = targets.some(function (tb) { return tb.x === ex2 && tb.y === ey2; });
        if (entersTargetBase) {
          sc += (prof.moveOrderBaseEntryBonus || 16000);
        }
        if ((s.mode === "4" || s.mode === "4P") && ((ex2 === 5 && ey2 === 3) || (ex2 === 5 && ey2 === 7))) {
          var nSq = s.map[ex2 + "," + ey2];
          if (!nSq || !nSq.length) sc += 3000;
        }

        var prevX = sx, prevY = sy;
        var foundLeapfrog = false;
        var anyCapture = false;
        var isJump = (hops.length > 2);
        for (var hi2 = 1; hi2 < hops.length; hi2++) {
          var hm = /^([1-9])([1-9])(x?)/.exec(hops[hi2]);
          if (!hm) continue;
          var hx = parseInt(hm[1], 10), hy = parseInt(hm[2], 10);
          if (Math.abs(hx - prevX) > 1 || Math.abs(hy - prevY) > 1) {
            isJump = true;
          }
          if (hm[3] === "x") {
            anyCapture = true;
            sc += (prof.moveOrderCaptureBonus || 8500);
          } else {
            sc += 250;
          }
          if (!foundLeapfrog && (Math.abs(hx - prevX) === 2 || Math.abs(hy - prevY) === 2)) {
            var midX = (prevX + hx) / 2, midY = (prevY + hy) / 2;
            var midSq = s.map[midX + "," + midY];
            if (midSq && midSq.length && areAllied(s.mode, midSq[0].owner, curPlayer)) {
              sc += (prof.moveOrderLeapfrogBonus || 1400);
              foundLeapfrog = true;
            }
          }
          prevX = hx; prevY = hy;
        }

        if (!anyCapture && enemyThreatMap[ex2 + "," + ey2]) {
          sc -= prof.moveOrderBlunderPenalty;
        }

        var allOpp = getOpponentBases(s.mode, curPlayer, s.bases);
        var startOnOppBase = allOpp.some(function (ob) { return ob.x === sx && ob.y === sy; });
        var endOnOppBase = allOpp.some(function (ob) { return ob.x === ex2 && ob.y === ey2; });
        if (startOnOppBase && !endOnOppBase) {
          var startStack = s.map[sx + "," + sy] || [];
          if (startStack.length <= 1) sc -= 22000;
        }

        var destStack = s.map[ex2 + "," + ey2] || [];
        if (!endOnOppBase && destStack.length >= 1) {
          sc -= 1200 * destStack.length;
        }

        if (!anyCapture && !isJump && earlyDevelopment && podCfg.count <= 1 && endDist <= 3) {
          sc -= 3200;
        }
      }
      return { move: m, score: sc };
    }

    // 4. Liberation or reserve pod placement
    if (m.indexOf("*") !== -1) {
      sc += 6500;
    } else {
      var placeM = /^([1-9])([1-9])$/.exec(m);
      if (placeM) {
        var px2 = parseInt(placeM[1], 10), py2 = parseInt(placeM[2], 10);
        var pStack = s.map[px2 + "," + py2] || [];
        if (pStack.length > 0 || hasUnarmedHomePod) {
          sc -= 6000;
        } else {
          sc += 150;
        }
      }
    }
    return { move: m, score: sc };
  });

  scored.sort(function (a, b) { return b.score - a.score; });
  return scored.map(function (item) { return item.move; });
}

function quiesce(s, qDepth, ply, alpha, beta, botSide, curPlayer, ctx, prof) {
  ctx.nodes++;
  if ((ctx.nodes & 63) === 0 && ctx.deadlineMs && Date.now() >= ctx.deadlineMs) {
    ctx.timedOut = true;
    return evaluateState(s, botSide, prof);
  }
  if (s.status === "O") {
    if (s.winner === botSide || areAllied(s.mode, s.winner, botSide)) return 1000000 - ply;
    if (s.winner && s.winner !== "-") return -1000000 + ply;
    return 0;
  }
  var standPat = evaluateState(s, botSide, prof);
  var isMax = areAllied(s.mode, curPlayer, botSide);
  if (qDepth <= 0) return standPat;

  if (isMax) {
    if (standPat >= beta) return standPat;
    if (standPat > alpha) alpha = standPat;
  } else {
    if (standPat <= alpha) return standPat;
    if (standPat < beta) beta = standPat;
  }

  var tacticalMoves = generateMoves(s, curPlayer, true);
  if (!tacticalMoves.length) return standPat;

  var orderedTactical = orderMoves(tacticalMoves, s, curPlayer, prof, null);
  var capLimit = Math.min(orderedTactical.length, prof.quiesceBranchCap || 6);
  if (isMax) {
    for (var i = 0; i < capLimit; i++) {
      var nextS = applyMoveToState(cloneState(s), orderedTactical[i], curPlayer);
      var score = quiesce(nextS, qDepth - 1, ply + 1, alpha, beta, botSide, nextS.turn, ctx, prof);
      if (ctx.timedOut) break;
      if (score > alpha) alpha = score;
      if (alpha >= beta) break;
    }
    return alpha;
  } else {
    for (var j = 0; j < capLimit; j++) {
      var nextS2 = applyMoveToState(cloneState(s), orderedTactical[j], curPlayer);
      var score2 = quiesce(nextS2, qDepth - 1, ply + 1, alpha, beta, botSide, nextS2.turn, ctx, prof);
      if (ctx.timedOut) break;
      if (score2 < beta) beta = score2;
      if (beta <= alpha) break;
    }
    return beta;
  }
}

function alphaBeta(s, depth, ply, alpha, beta, botSide, curPlayer, ctx, prof, ttMap) {
  ctx.nodes++;
  if ((ctx.nodes & 63) === 0 && ctx.deadlineMs) {
    var nowMs = Date.now();
    if (nowMs >= ctx.deadlineMs) {
      ctx.timedOut = true;
      return evaluateState(s, botSide, prof);
    }
    if (ctx.progressCb && (ctx.nodes & 2047) === 0) {
      ctx.progressCb({
        depth: ctx.curDepth || 1,
        maxDepth: ctx.maxDepth || prof.maxSearchDepth,
        nodes: ctx.nodes,
        score: ctx.bestScore || 0,
        bestMove: ctx.bestMove || "",
        elapsedMs: nowMs - ctx.t0,
        budgetMs: ctx.budgetMs || prof.timeBudgetMs
      });
    }
  }
  if (s.status === "O") {
    if (s.winner === botSide || areAllied(s.mode, s.winner, botSide)) return 1000000 - ply;
    if (s.winner && s.winner !== "-") return -1000000 + ply;
    return 0;
  }
  if (depth <= 0) {
    if (prof.maxQuiesceDepth > 0) {
      return quiesce(s, prof.maxQuiesceDepth, ply, alpha, beta, botSide, curPlayer, ctx, prof);
    }
    return evaluateState(s, botSide, prof);
  }

  var fenKey = null;
  var ttBestMove = null;
  if (ttMap) {
    fenKey = serializeOctiFen(s);
    var entry = ttMap[fenKey];
    if (entry) {
      ttBestMove = entry.bestMove || null;
      if (entry.depth >= depth) {
        var ttScore = entry.score;
        if (ttScore >= 999000) ttScore -= ply;
        else if (ttScore <= -999000) ttScore += ply;
        if (entry.flag === "EXACT") return ttScore;
        if (entry.flag === "LOWER" && ttScore > alpha) alpha = ttScore;
        if (entry.flag === "UPPER" && ttScore < beta) beta = ttScore;
        if (alpha >= beta) return ttScore;
      }
    }
  }

  var isMax = areAllied(s.mode, curPlayer, botSide);
  var rawMoves = generateMoves(s, curPlayer);
  if (!rawMoves.length) {
    return isMax ? (-1000000 + ply) : (1000000 - ply);
  }
  var orderedMoves = orderMoves(rawMoves, s, curPlayer, prof, ttBestMove);
  var branchLimit = Math.min(orderedMoves.length, prof.treeBranchCap || 24);

  var bestScore = isMax ? -Infinity : Infinity;
  var bestMoveStr = orderedMoves[0];
  var origAlpha = alpha;
  var origBeta = beta;

  if (isMax) {
    for (var i = 0; i < branchLimit; i++) {
      var nextS = applyMoveToState(cloneState(s), orderedMoves[i], curPlayer);
      var val = alphaBeta(nextS, depth - 1, ply + 1, alpha, beta, botSide, nextS.turn, ctx, prof, ttMap);
      if (ctx.timedOut) break;
      if (val > bestScore) {
        bestScore = val;
        bestMoveStr = orderedMoves[i];
      }
      if (val > alpha) alpha = val;
      if (beta <= alpha) break;
    }
  } else {
    for (var j = 0; j < branchLimit; j++) {
      var nextS2 = applyMoveToState(cloneState(s), orderedMoves[j], curPlayer);
      var val2 = alphaBeta(nextS2, depth - 1, ply + 1, alpha, beta, botSide, nextS2.turn, ctx, prof, ttMap);
      if (ctx.timedOut) break;
      if (val2 < bestScore) {
        bestScore = val2;
        bestMoveStr = orderedMoves[j];
      }
      if (val2 < beta) beta = val2;
      if (beta <= alpha) break;
    }
  }

  if (!ctx.timedOut && bestScore !== -Infinity && bestScore !== Infinity &&
      ttMap && fenKey && Object.keys(ttMap).length < 65536) {
    var flag = "EXACT";
    if (bestScore <= origAlpha) flag = "UPPER";
    else if (bestScore >= origBeta) flag = "LOWER";
    var storeScore = bestScore;
    if (storeScore >= 999000) storeScore += ply;
    else if (storeScore <= -999000) storeScore -= ply;
    ttMap[fenKey] = { depth: depth, score: storeScore, flag: flag, bestMove: bestMoveStr };
  }

  return bestScore;
}

function botSearch(fen, difficulty, mode, botSide, progressCb, customBudgetMs) {
  var s = parseOctiFen(fen);
  if (!s) return { bestMove: null, score: 0, nodes: 0, depth: 0 };
  var diff = parseInt(difficulty, 10) || 2;
  var prof = BOT_PROFILES[diff] || BOT_PROFILES[2];

  var moves = generateMoves(s, botSide);
  if (!moves.length) return { bestMove: null, score: -1000000, nodes: 1, depth: 0 };

  var t0 = Date.now();
  var budgetMs = (customBudgetMs && customBudgetMs > 0) ? customBudgetMs : prof.timeBudgetMs;
  var deadlineMs = t0 + budgetMs;
  var maxDepth = prof.maxSearchDepth;
  var ctx = {
    nodes: 0,
    t0: t0,
    budgetMs: budgetMs,
    deadlineMs: deadlineMs,
    timedOut: false,
    progressCb: progressCb || null,
    curDepth: 1,
    maxDepth: maxDepth,
    bestScore: 0,
    bestMove: ""
  };

  // 1. Immediate 1-ply victory check across ALL legal moves before branch capping
  for (var wi = 0; wi < moves.length; wi++) {
    ctx.nodes++;
    var winS = applyMoveToState(cloneState(s), moves[wi], botSide);
    if (winS.status === "O" && (winS.winner === botSide || areAllied(s.mode, winS.winner, botSide))) {
      return {
        bestMove: moves[wi],
        score: 999999,
        nodes: ctx.nodes,
        depth: 1,
        elapsedMs: Date.now() - t0
      };
    }
  }

  var orderedMoves = orderMoves(moves, s, botSide, prof, null);
  var rootCap = Math.min(orderedMoves.length, prof.rootBranchCap || 40);
  var candidates = orderedMoves.slice(0, rootCap);

  var ttMap = {};
  var bestOverallMove = candidates[0];
  var bestOverallScore = -Infinity;
  var completedDepth = 1;

  for (var d = 1; d <= maxDepth; d++) {
    if (Date.now() >= deadlineMs && d > 1) break;
    ctx.timedOut = false;
    ctx.curDepth = d;
    ctx.bestMove = bestOverallMove;
    ctx.bestScore = bestOverallScore;

    if (bestOverallMove) {
      var idx = candidates.indexOf(bestOverallMove);
      if (idx > 0) {
        candidates.splice(idx, 1);
        candidates.unshift(bestOverallMove);
      }
    }

    var depthBestMove = candidates[0];
    var depthBestScore = -Infinity;
    var depthTimedOut = false;
    var alpha = -2000000;
    var beta = 2000000;
    var completedRootMoves = 0;

    for (var i = 0; i < candidates.length; i++) {
      if (Date.now() >= deadlineMs && d > 1) {
        depthTimedOut = true;
        break;
      }
      var mv = candidates[i];
      var nextS = applyMoveToState(cloneState(s), mv, botSide);
      ctx.nodes++;

      var sc;
      if (nextS.status === "O") {
        if (nextS.winner === botSide || areAllied(s.mode, nextS.winner, botSide)) {
          sc = 1000000 - 1;
        } else if (nextS.winner && nextS.winner !== "-") {
          sc = -1000000 + 1;
        } else {
          sc = 0;
        }
      } else if (d <= 1 && prof.maxQuiesceDepth === 0) {
        sc = evaluateState(nextS, botSide, prof);
      } else {
        sc = alphaBeta(nextS, d - 1, 1, alpha, beta, botSide, nextS.turn, ctx, prof, ttMap);
      }

      if (ctx.timedOut && d > 1) {
        depthTimedOut = true;
        break;
      }

      completedRootMoves++;

      if (sc > alpha) {
        if (prof.rootScoreJitter && d === 1 && Math.abs(sc) < 500000) {
          sc += Math.floor((Math.random() * 2 - 1) * prof.rootScoreJitter);
        }
        depthBestScore = sc;
        depthBestMove = mv;
        alpha = sc;
        ctx.bestMove = depthBestMove;
        ctx.bestScore = depthBestScore;
      }

      if (progressCb) {
        progressCb({
          depth: d,
          maxDepth: maxDepth,
          nodes: ctx.nodes,
          score: depthBestScore,
          bestMove: depthBestMove,
          elapsedMs: Date.now() - t0,
          budgetMs: budgetMs
        });
      }
    }

    if (!depthTimedOut || d === 1 || completedRootMoves >= 1) {
      bestOverallMove = depthBestMove;
      bestOverallScore = depthBestScore;
      if (!depthTimedOut || d === 1) {
        completedDepth = d;
      }
      if (progressCb) {
        progressCb({
          depth: completedDepth,
          maxDepth: maxDepth,
          nodes: ctx.nodes,
          score: bestOverallScore,
          bestMove: bestOverallMove,
          elapsedMs: Date.now() - t0,
          budgetMs: budgetMs
        });
      }
    }
    if (depthTimedOut || Date.now() >= deadlineMs || Math.abs(bestOverallScore) >= 999000) {
      break;
    }
  }

  return {
    bestMove: bestOverallMove,
    score: bestOverallScore,
    nodes: ctx.nodes,
    depth: completedDepth,
    elapsedMs: Date.now() - t0
  };
}

/* Inline Web Worker Initialization */
var botWorker = null;
try {
  var workerParts = [
    "self.onmessage = function (e) {",
    "  var d = e.data;",
    "  if (d.type === 'search') {",
    "    var t0 = Date.now();",
    "    var res = (" + botSearch.toString() + ")(d.fen, d.depth, d.mode, d.side, function (prog) {",
    "      self.postMessage(Object.assign({ type: 'progress' }, prog));",
    "    });",
    "    var elapsed = Date.now() - t0;",
    "    self.postMessage({",
    "      type: 'done',",
    "      bestMove: res.bestMove,",
    "      score: res.score,",
    "      nodes: res.nodes,",
    "      depth: res.depth,",
    "      elapsedMs: elapsed",
    "    });",
    "  }",
    "};",
    "var DIR_DELTAS = " + JSON.stringify(DIR_DELTAS) + ";",
    "var DIR_CHARS = " + JSON.stringify(DIR_CHARS) + ";",
    "var DEFAULT_BOT_TUNING = " + JSON.stringify(DEFAULT_BOT_TUNING) + ";",
    "var BOT_PROFILES = " + JSON.stringify(BOT_PROFILES) + ";",
    wrapCoord.toString(),
    playerIndex.toString(),
    partnerOf.toString(),
    partnerIndex.toString(),
    isPartnerHome.toString(),
    areAllied.toString(),
    isSquareOccupiedBy.toString(),
    squareHasEnemyOf.toString(),
    canLiberate.toString(),
    canPlaceReservePodOnSquare.toString(),
    canLiberateOnSquare.toString(),
    formatProngConfig.toString(),
    parseProngConfig.toString(),
    parseOctiFen.toString(),
    serializeOctiFen.toString(),
    cloneState.toString(),
    getHomeSquares.toString(),
    getOpponentBases.toString(),
    dfsJumpChains.toString(),
    generateMoves.toString(),
    applySingleHopChain.toString(),
    check4PlayerElimination.toString(),
    checkWinner.toString(),
    hasAnyLegalMove.toString(),
    advanceTurn.toString(),
    applyMoveToState.toString(),
    torusDist.toString(),
    chebyshevDist.toString(),
    minChebyshevToTargets.toString(),
    getUncapturedOpponentBases.toString(),
    evaluateJumpThreatDfs.toString(),
    evaluatePodJumpThreat.toString(),
    filterDominatedMoves.toString(),
    evaluateState.toString(),
    orderMoves.toString(),
    quiesce.toString(),
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
  replay: $("tab-replay"),
  account: $("tab-account")
};

function toggleAccountRobots(forceOpen) {
  var btn = $("btn-toggle-robots-section");
  var body = $("account-robots-expand-body");
  if (!btn || !body) return;
  var isExpanded = btn.getAttribute("aria-expanded") === "true";
  var willExpand = (forceOpen !== undefined) ? forceOpen : !isExpanded;
  btn.setAttribute("aria-expanded", willExpand ? "true" : "false");
  btn.innerHTML = willExpand ? "Hide Robot Registration &#9652;" : "Show Robot Registration &#9662;";
  body.style.display = willExpand ? "block" : "none";
  if (willExpand && currentUser) {
    loadMyRobots();
  }
}

if ($("btn-toggle-robots-section")) {
  $("btn-toggle-robots-section").addEventListener("click", function () {
    toggleAccountRobots();
  });
}

function switchTab(name) {
  if (name === "robots") {
    name = "account";
    toggleAccountRobots(true);
  }
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
  } else if (name === "account") {
    if (currentUser || localStorage.getItem("octi_jwt_token")) {
      loadMyProfile();
      loadMyRobots();
    }
  } else if (name === "play") {
    if (activePlayState) {
      renderPlayBoardCanvas();
      renderAllWebReserves();
      updatePlaySidebar();
    }
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
  } else if (h.indexOf("#replay/") === 0 || h.indexOf("#archive/") === 0) {
    var id = (h.indexOf("#replay/") === 0) ? decodeURIComponent(h.substring(8)) : decodeURIComponent(h.substring(9));
    switchTab("replay");
    if (id) { arOpen(id); }
  } else if (h === "#robots") {
    switchTab("account");
    toggleAccountRobots(true);
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

if ($("btn-robots-to-account")) {
  $("btn-robots-to-account").addEventListener("click", function () {
    switchTab("account");
  });
}

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
          window.open(window.location.pathname + "#replay/" + encodeURIComponent(gm.game_id), "_blank");
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
 * Tab 2: Web Player (Desktop & Android Parity Engine, Audio Synth & Gesture Board)
 * ========================================================================= */

var OctiWebSoundSynth = (function () {
  var audioCtx = null;
  var sfxEnabled = true;

  function getAudioContext() {
    if (!audioCtx && (typeof window !== "undefined") && (window.AudioContext || window.webkitAudioContext)) {
      audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    }
    if (audioCtx && audioCtx.state === "suspended") {
      audioCtx.resume().catch(function () {});
    }
    return audioCtx;
  }

  function setEnabled(val) {
    sfxEnabled = !!val;
    var btn = typeof document !== "undefined" ? document.getElementById("btn-sfx") : null;
    if (btn) {
      btn.textContent = sfxEnabled ? "SFX: On" : "SFX: Off";
      btn.className = sfxEnabled ? "btn" : "btn dim";
    }
  }

  function isEnabled() {
    return sfxEnabled;
  }

  function play(cue) {
    if (!sfxEnabled) return;
    var ctx = getAudioContext();
    if (!ctx) return;
    var sampleRate = 44100;
    var durationMs = 110;
    if (cue === "POD_PLACE") durationMs = 120;
    else if (cue === "PRONG_SNAP") durationMs = 75;
    else if (cue === "POD_STEP") durationMs = 95;
    else if (cue === "POD_JUMP") durationMs = 150;
    else if (cue === "POD_CAPTURE") durationMs = 210;
    else if (cue === "GAME_VICTORY") durationMs = 360;

    var numSamples = Math.floor((sampleRate * durationMs) / 1000);
    var buffer = ctx.createBuffer(1, numSamples, sampleRate);
    var ch = buffer.getChannelData(0);
    var twoPi = 2.0 * Math.PI;

    for (var i = 0; i < numSamples; i++) {
      var t = i / sampleRate;
      var progress = i / Math.max(1, numSamples);
      var attack = Math.min(1.0, i / 48.0);
      var val = 0.0;
      if (cue === "POD_PLACE") {
        var fPlace = 145.0 * Math.exp(-t * 14.0) + 68.0;
        var envPlace = attack * Math.exp(-t * 24.0);
        val = envPlace * (0.72 * Math.sin(twoPi * fPlace * t) + 0.28 * Math.sin(twoPi * (fPlace * 1.9) * t));
      } else if (cue === "PRONG_SNAP") {
        var envSnap = attack * Math.exp(-t * 48.0);
        val = envSnap * (0.50 * Math.sin(twoPi * 1420.0 * t) + 0.35 * Math.sin(twoPi * 2680.0 * t) + 0.15 * Math.sin(twoPi * 3920.0 * t));
      } else if (cue === "POD_STEP") {
        var fStep = 195.0 * Math.exp(-t * 16.0) + 85.0;
        var envStep = attack * Math.exp(-t * 30.0);
        val = envStep * (0.75 * Math.sin(twoPi * fStep * t) + 0.25 * Math.sin(twoPi * (fStep * 2.1) * t));
      } else if (cue === "POD_JUMP") {
        var fJump = (progress < 0.45) ? 440.0 : 587.33;
        var envJump = attack * Math.exp(-t * 14.0);
        val = envJump * (0.68 * Math.sin(twoPi * fJump * t) + 0.32 * Math.sin(twoPi * (fJump * 1.5) * t));
      } else if (cue === "POD_CAPTURE") {
        var fCap = (progress < 0.33) ? 440.0 : ((progress < 0.66) ? 554.37 : 659.25);
        var envCap = attack * Math.exp(-t * 10.0);
        val = envCap * (0.65 * Math.sin(twoPi * fCap * t) + 0.25 * Math.sin(twoPi * (fCap * 2.0) * t) + 0.10 * Math.sin(twoPi * (fCap * 3.0) * t));
      } else if (cue === "GAME_VICTORY") {
        var fVic = (progress < 0.22) ? 523.25 : ((progress < 0.44) ? 659.25 : ((progress < 0.68) ? 783.99 : 1046.50));
        var envVic = attack * Math.exp(-t * 5.5);
        val = envVic * (0.60 * Math.sin(twoPi * fVic * t) + 0.28 * Math.sin(twoPi * (fVic * 1.5) * t) + 0.12 * Math.sin(twoPi * (fVic * 2.0) * t));
      }
      ch[i] = Math.max(-1.0, Math.min(1.0, val));
    }

    var srcNode = ctx.createBufferSource();
    srcNode.buffer = buffer;
    srcNode.connect(ctx.destination);
    srcNode.start();
  }

  return {
    setEnabled: setEnabled,
    isEnabled: isEnabled,
    play: play,
    triggerCue: play
  };
})();

/* Rule & Sorting Utilities */
function sortedPodIndicesMostProngsFirst(stack) {
  if (!stack || !stack.length) return [];
  var indices = [];
  for (var i = 0; i < stack.length; i++) indices.push(i);
  indices.sort(function (a, b) {
    var ca = (stack[a] && stack[a].cfg) ? stack[a].cfg.count : 0;
    var cb = (stack[b] && stack[b].cfg) ? stack[b].cfg.count : 0;
    if (cb !== ca) return cb - ca; // descending: most prongs on top
    return b - a; // highest stack layer on top
  });
  return indices;
}


/* Global Board & Play State */
var boardZoomScale = 1.0;
var boardZoomFocus = { x: 0.5, y: 0.5 };
var liveRotationOffsetDeg = 0;
var activePlayState = null;
var localMatchConfig = {
  playMode: "bot", // "bot" or "pass"
  botDifficulty: 2, // 1, 2, 3
  humanSeat: "A", // "A" or "B"
  gameMode: "2P", // "2P", "4P", "OFK"
  bases: 3, // 3 or 1 (OFK = 1)
  superProngs: false,
  edgeless: false,
  timeControl: "untimed"
};
var matchStarted = false;
var activeOnlineGameId = "";
var activeOnlineTableRoomId = "";

var moveComposer = null;
var lastMoveHighlightSquares = [];
var lastMoveSetAtMs = 0;
var playMatchPlies = [];
var playMatchHistory = [];
var playVcrIndex = 0;

var clockTimers = { A: 0, B: 0, C: 0, D: 0 };
var clockInterval = null;



/* --- WebMoveComposer Implementation --- */
function WebMoveComposer(stateGetter, onCommitCb) {
  this.getState = stateGetter;
  this.onCommit = onCommitCb;
  this.stagedOps = [];
  this.stagedNotation = "";
  this.completedSubMoves = [];
  this.currentChainNotation = "";
  this.previewState = null;
  this.isAwaitingConfirmation = false;
  this.awaitingSuperProngBonusMove = false;
  this.bonusPodCoord = null;
  this.bonusPodIdx = -1;

  this.guidedMoveActive = false;
  this.guidedOriginSquare = null;
  this.guidedCurrentSquare = null;
  this.guidedPodIndex = -1;
  this.guidedPodConfig = "";
  this.guidedLegalDests = [];
  this.guidedHops = [];
  this.claimedPodsAtOrigin = {}; // map of podIdx -> true
  this.jumpedSquaresChain = [];
  this.captureChoices = []; // { jumpedX, jumpedY, capture, mandatory }
}

Object.defineProperty(WebMoveComposer.prototype, "isGuidedMoveActive", {
  get: function () { return this.guidedMoveActive; }
});

WebMoveComposer.prototype.cancel = function () {
  this.reset();
  renderPlayBoardCanvas();
  renderAllWebReserves();
};

WebMoveComposer.prototype.reset = function () {
  this.stagedOps = [];
  this.stagedNotation = "";
  this.completedSubMoves = [];
  this.currentChainNotation = "";
  this.previewState = null;
  this.isAwaitingConfirmation = false;
  this.awaitingSuperProngBonusMove = false;
  this.bonusPodCoord = null;
  this.bonusPodIdx = -1;
  this.guidedMoveActive = false;
  this.guidedOriginSquare = null;
  this.guidedCurrentSquare = null;
  this.guidedPodIndex = -1;
  this.guidedPodConfig = "";
  this.guidedLegalDests = [];
  this.guidedHops = [];
  this.claimedPodsAtOrigin = {};
  this.jumpedSquaresChain = [];
  this.captureChoices = [];
  var capOv = document.getElementById("web-capture-overlay");
  if (capOv) capOv.style.display = "none";
  this.updateUi();
};

WebMoveComposer.prototype.updateUi = function () {
  var bar = document.getElementById("web-ply-confirm-bar");
  var notEl = document.getElementById("web-staged-notation");
  if (!bar || !notEl) return;
  if (this.isAwaitingConfirmation && this.stagedNotation) {
    notEl.textContent = this.stagedNotation;
    bar.style.display = "flex";
  } else {
    bar.style.display = "none";
  }
};

WebMoveComposer.prototype.getWorkingState = function () {
  if (this.previewState) return this.previewState;
  var s = this.getState();
  this.previewState = cloneState(s);
  return this.previewState;
};

WebMoveComposer.prototype.hasOtherMovablePodAtOrigin = function () {
  if (!this.guidedOriginSquare) return false;
  var s = this.previewState || this.getState();
  if (!s || !s.map) return false;
  var ox = this.guidedOriginSquare.x, oy = this.guidedOriginSquare.y;
  var k = ox + "," + oy;
  var stack = s.map[k] || [];
  var turn = s.turn;
  for (var i = 0; i < stack.length; i++) {
    if (stack[i].owner === turn && !this.claimedPodsAtOrigin[i]) {
      var dests = this.computeLegalDestsForPod(s, ox, oy, stack[i], []);
      if (dests.length > 0) return true;
    }
  }
  return false;
};

WebMoveComposer.prototype.unclaimedMovablePodIndicesAtOrigin = function () {
  if (!this.guidedOriginSquare) return [];
  var s = this.previewState || this.getState();
  if (!s || !s.map) return [];
  var ox = this.guidedOriginSquare.x, oy = this.guidedOriginSquare.y;
  var k = ox + "," + oy;
  var stack = s.map[k] || [];
  var turn = s.turn;
  var out = [];
  for (var i = 0; i < stack.length; i++) {
    if (stack[i].owner === turn && !this.claimedPodsAtOrigin[i]) {
      var dests = this.computeLegalDestsForPod(s, ox, oy, stack[i], []);
      if (dests.length > 0) out.push(i);
    }
  }
  return out;
};

WebMoveComposer.prototype.hasDistinctUnclaimedPodsAtOrigin = function () {
  var unclaimed = this.unclaimedMovablePodIndicesAtOrigin();
  if (unclaimed.length <= 1) return false;
  var s = this.previewState || this.getState();
  var stack = s.map[this.guidedOriginSquare.x + "," + this.guidedOriginSquare.y] || [];
  var firstCfg = formatProngConfig(stack[unclaimed[0]].cfg.prongs);
  for (var i = 1; i < unclaimed.length; i++) {
    if (formatProngConfig(stack[unclaimed[i]].cfg.prongs) !== firstCfg) return true;
  }
  return false;
};

WebMoveComposer.prototype.stageAddPod = function (x, y, forceLiberate) {
  if (!this.awaitingSuperProngBonusMove) {
    this.reset();
  }
  var s = this.getWorkingState();
  var turn = s.turn;
  var pIdx = playerIndex(turn);
  var res = s.reserves[pIdx];
  if (!res) return false;

  var canPlace = canPlaceReservePodOnSquare(s, x, y, turn);
  var canLib = canLiberateOnSquare(s, x, y, turn);

  var doLiberate = false;
  if (forceLiberate) {
    if (!canLib) return false;
    doLiberate = true;
  } else if (canPlace) {
    doLiberate = false;
  } else if (canLib) {
    doLiberate = true;
  } else {
    return false;
  }

  var k = x + "," + y;
  var partnerIdx = partnerIndex(s.mode, pIdx);
  var isPartnerRevival = (s.mode === "4" || s.mode === "4P") &&
                         (s.elimMask & (1 << partnerIdx)) !== 0 &&
                         isPartnerHome(s.mode, partnerIdx, x, y);

  if (doLiberate) {
    if (isPartnerRevival) {
      if (s.reserves[partnerIdx]) s.reserves[partnerIdx].capturedPods--;
      s.elimMask &= ~(1 << partnerIdx);
      if (!s.map[k]) s.map[k] = [];
      s.map[k].push({
        owner: ["A", "B", "C", "D"][partnerIdx],
        cfg: { prongs: [0, 0, 0, 0, 0, 0, 0, 0], count: 0, text: "" }
      });
    } else {
      res.capturedPods--;
      if (!s.map[k]) s.map[k] = [];
      s.map[k].push({
        owner: turn,
        cfg: { prongs: [0, 0, 0, 0, 0, 0, 0, 0], count: 0, text: "" }
      });
    }
    this.stagedNotation = x + "" + y + "*";
  } else {
    res.freePods--;
    if (!s.map[k]) s.map[k] = [];
    s.map[k].push({
      owner: turn,
      cfg: { prongs: [0, 0, 0, 0, 0, 0, 0, 0], count: 0, text: "" }
    });
    this.stagedNotation = x + "" + y;
  }

  OctiWebSoundSynth.play("POD_PLACE");
  this.isAwaitingConfirmation = true;
  this.updateUi();
  renderPlayBoardCanvas();
  renderAllWebReserves();
  return true;
};

WebMoveComposer.prototype.stageAddProng = function (x, y, podIdx, dirIdx, isSuper) {
  if (!this.awaitingSuperProngBonusMove) {
    this.reset();
  }
  var s = this.getWorkingState();
  var turn = s.turn;
  var pIdx = (turn === "A") ? 0 : ((turn === "B") ? 1 : ((turn === "C") ? 2 : 3));
  var res = s.reserves[pIdx];
  var k = x + "," + y;
  var stack = s.map[k];
  if (!stack || podIdx < 0 || podIdx >= stack.length) return false;
  var pod = stack[podIdx];
  if (pod.owner !== turn || pod.cfg.prongs[dirIdx]) return false;

  var prevCfg = formatProngConfig(pod.cfg.prongs);

  if (isSuper) {
    if (!s.superProngs || !res || res.freeSuper <= 0) return false;
    res.freeSuper--;
    pod.cfg.prongs[dirIdx] = 2;
  } else {
    if (!res || res.freeProngs <= 0) return false;
    res.freeProngs--;
    pod.cfg.prongs[dirIdx] = 1;
  }
  pod.cfg.count++;
  var dirChar = DIR_CHARS[dirIdx];
  pod.cfg.text = formatProngConfig(pod.cfg.prongs);

  var not = x + "" + y + prevCfg + "+" + dirChar + (isSuper ? "!" : "");
  this.stagedNotation = not;

  OctiWebSoundSynth.play("PRONG_SNAP");
  this.isAwaitingConfirmation = true;

  this.updateUi();
  renderPlayBoardCanvas();
  renderAllWebReserves();
  return true;
};

WebMoveComposer.prototype.addProng = function (x, y, dir, isSuper, podIdx) {
  if (podIdx === undefined) podIdx = 0;
  var dIdx = (typeof dir === "number") ? dir : DIR_CHARS.indexOf(String(dir).toLowerCase());
  return this.stageAddProng(x, y, podIdx, dIdx, !!isSuper);
};

WebMoveComposer.addProng = function (x, y, dir, isSuper, podIdx) {
  if (moveComposer) return moveComposer.addProng(x, y, dir, isSuper, podIdx);
  return false;
};

WebMoveComposer.prototype.stageRepositionProng = function (x, y, podIdx, srcDirIdx, dstDirIdx) {
  if (!this.awaitingSuperProngBonusMove) {
    this.reset();
  }
  var s = this.getWorkingState();
  if (s.mode === "OFK") return false;
  var turn = s.turn;
  var k = x + "," + y;
  var stack = s.map[k];
  if (!stack || podIdx < 0 || podIdx >= stack.length) return false;
  var pod = stack[podIdx];
  if (pod.owner !== turn || pod.cfg.prongs[srcDirIdx] !== 1 || pod.cfg.prongs[dstDirIdx]) return false;

  var prevCfg = formatProngConfig(pod.cfg.prongs);
  pod.cfg.prongs[srcDirIdx] = 0;
  pod.cfg.prongs[dstDirIdx] = 1;
  pod.cfg.text = formatProngConfig(pod.cfg.prongs);

  var srcCh = DIR_CHARS[srcDirIdx];
  var dstCh = DIR_CHARS[dstDirIdx];
  this.stagedNotation = x + "" + y + prevCfg + "+" + dstCh + "-" + srcCh;

  OctiWebSoundSynth.play("PRONG_SNAP");
  this.isAwaitingConfirmation = true;
  this.updateUi();
  renderPlayBoardCanvas();
  renderAllWebReserves();
  return true;
};

WebMoveComposer.prototype.computeLegalDestsForPod = function (state, x, y, pod, chainJumped) {
  var dests = [];
  if (!pod || !pod.cfg) return dests;
  var gw = state.gw, gh = state.gh;
  var edgeless = !!state.edgeless;

  for (var d = 0; d < 8; d++) {
    var ptype = pod.cfg.prongs[d];
    if (!ptype) continue;
    var delta = DIR_DELTAS[d];
    var dx = delta[0], dy = delta[1];

    if (!chainJumped || !chainJumped.length) {
      // Step: distance 1
      var sx = x + dx, sy = y + dy;
      if (edgeless) {
        sx = wrapCoord(sx, gw);
        sy = wrapCoord(sy, gh);
      }
      if (sx >= 1 && sx <= gw && sy >= 1 && sy <= gh) {
        var stepK = sx + "," + sy;
        var stepStack = state.map[stepK];
        var canLandStep = (!stepStack || !stepStack.length) || (state.mode !== "OFK" && !squareHasEnemyOf(state, sx, sy, pod.owner));
        if (canLandStep) {
          dests.push({ x: sx, y: sy, dist: 1, isJump: false });
        }
      }
    }

    // Jumps: distance 2 over adjacent occupied square
    var jx = x + dx, jy = y + dy;
    var landX = x + dx * 2, landY = y + dy * 2;
    if (edgeless) {
      jx = wrapCoord(jx, gw); jy = wrapCoord(jy, gh);
      landX = wrapCoord(landX, gw); landY = wrapCoord(landY, gh);
    }
    if (jx >= 1 && jx <= gw && jy >= 1 && jy <= gh &&
        landX >= 1 && landX <= gw && landY >= 1 && landY <= gh) {
      var jumpedK = jx + "," + jy;
      var jumpedStack = state.map[jumpedK];
      var alreadyJumped = (chainJumped || []).some(function (p) { return p.x === jx && p.y === jy; });
      if (jumpedStack && jumpedStack.length && !alreadyJumped) {
        var landK = landX + "," + landY;
        var landStack = state.map[landK];
        var canLandJump = (!landStack || !landStack.length);
        if (this.guidedOriginSquare && landX === this.guidedOriginSquare.x && landY === this.guidedOriginSquare.y) {
          var origStack = state.map[landK];
          if (!origStack || origStack.length === 0) {
            canLandJump = true;
          }
        }
        if (canLandJump) {
          dests.push({ x: landX, y: landY, dist: 2, isJump: true, jumpedX: jx, jumpedY: jy });
        }
      }
    }
  }
  return dests;
};

WebMoveComposer.prototype.beginGuidedPodMove = function (x, y, podIdx) {
  var isContinuingComposite = this.guidedOriginSquare &&
    this.guidedOriginSquare.x === x && this.guidedOriginSquare.y === y &&
    this.completedSubMoves && this.completedSubMoves.length > 0;

  if (!isContinuingComposite && !this.awaitingSuperProngBonusMove) {
    this.reset();
  } else if (isContinuingComposite && this.currentChainNotation) {
    // Save previous chain if it wasn't pushed yet
    if (this.completedSubMoves.indexOf(this.currentChainNotation) === -1) {
      this.completedSubMoves.push(this.currentChainNotation);
    }
    this.currentChainNotation = "";
  }

  var s = this.getWorkingState();
  var k = x + "," + y;
  var stack = s.map[k] || [];
  if (podIdx < 0 || podIdx >= stack.length) {
    var sorted = sortedPodIndicesMostProngsFirst(stack);
    podIdx = -1;
    for (var si = 0; si < sorted.length; si++) {
      if (stack[sorted[si]].owner === s.turn) { podIdx = sorted[si]; break; }
    }
    if (podIdx === -1) podIdx = stack.length - 1;
  }
  var pod = stack[podIdx];
  if (!pod || pod.owner !== s.turn) return false;

  this.guidedMoveActive = true;
  this.guidedOriginSquare = { x: x, y: y };
  this.guidedCurrentSquare = { x: x, y: y };
  this.guidedPodIndex = podIdx;
  this.guidedPodConfig = formatProngConfig(pod.cfg.prongs);
  this.jumpedSquaresChain = [];
  this.guidedHops = [];
  this.currentChainNotation = "";
  this.guidedLegalDests = this.computeLegalDestsForPod(s, x, y, pod, this.jumpedSquaresChain);

  renderPlayBoardCanvas();
  return true;
};

WebMoveComposer.prototype.selectGuidedDestination = function (toX, toY) {
  if (!this.guidedMoveActive || !this.guidedCurrentSquare) return false;
  var matchDest = this.guidedLegalDests.find(function (d) { return d.x === toX && d.y === toY; });
  if (!matchDest) return false;

  var s = this.getWorkingState();
  var curX = this.guidedCurrentSquare.x, curY = this.guidedCurrentSquare.y;
  var curK = curX + "," + curY;
  var toK = toX + "," + toY;
  var stack = s.map[curK];
  if (!stack || this.guidedPodIndex < 0 || this.guidedPodIndex >= stack.length) return false;
  var pod = stack.splice(this.guidedPodIndex, 1)[0];
  if (!stack.length) delete s.map[curK];

  if (matchDest.isJump) {
    var jx = matchDest.jumpedX, jy = matchDest.jumpedY;
    var jK = jx + "," + jy;
    var jStack = s.map[jK];
    var isEnemy = squareHasEnemyOf(s, jx, jy, s.turn);
    var doCap = isEnemy;

    var capSnapshot = [];
    var capturerIdx = playerIndex(s.turn);
    var seizedNormal = 0;
    var seizedSuper = 0;
    var remainingPods = [];

    if (doCap && jStack && jStack.length) {
      for (var v = 0; v < jStack.length; v++) {
        var vPod = jStack[v];
        if (!areAllied(s.mode, vPod.owner, s.turn)) {
          capSnapshot.push(vPod);
          var vIdx = playerIndex(vPod.owner);
          s.reserves[vIdx].capturedPods++;
          for (var pi = 0; pi < 8; pi++) {
            if (vPod.cfg.prongs[pi] === 1) seizedNormal++;
            else if (vPod.cfg.prongs[pi] === 2) seizedSuper++;
          }
        } else {
          remainingPods.push(vPod);
        }
      }
      if (remainingPods.length > 0) {
        s.map[jK] = remainingPods;
      } else {
        delete s.map[jK];
      }
      s.reserves[capturerIdx].freeProngs += (seizedNormal + (s.superProngs ? 0 : seizedSuper));
      if (s.superProngs) s.reserves[capturerIdx].freeSuper += seizedSuper;
      OctiWebSoundSynth.play("POD_CAPTURE");
    } else {
      OctiWebSoundSynth.play("POD_JUMP");
    }

    this.jumpedSquaresChain.push({
      x: jx,
      y: jy,
      toX: toX,
      toY: toY,
      capturedPodsSnapshot: capSnapshot,
      capturerIdx: capturerIdx,
      seizedNormal: seizedNormal,
      seizedSuper: seizedSuper,
      capture: (capSnapshot.length > 0)
    });

    if (!this.currentChainNotation) {
      this.currentChainNotation = this.guidedOriginSquare.x + "" + this.guidedOriginSquare.y + this.guidedPodConfig + "-" + toX + "" + toY + (doCap ? "x" : "");
    } else {
      this.currentChainNotation += "-" + toX + "" + toY + (doCap ? "x" : "");
    }

    if (doCap && capSnapshot.length > 0) {
      var capPromptEl = document.getElementById("web-capture-prompt-text");
      if (capPromptEl) {
        capPromptEl.textContent = "Capture jumped enemy pod" + (capSnapshot.length > 1 ? "s" : "") + " at Square (" + jx + ", " + jy + ")?";
      }
      var capOverlay = document.getElementById("web-capture-overlay");
      if (capOverlay) capOverlay.style.display = "flex";
    }
  } else {
    // Step
    OctiWebSoundSynth.play("POD_STEP");
    if (!this.currentChainNotation) {
      this.currentChainNotation = this.guidedOriginSquare.x + "" + this.guidedOriginSquare.y + this.guidedPodConfig + "-" + toX + "" + toY;
    } else {
      this.currentChainNotation += "-" + toX + "" + toY;
    }
  }

  this.guidedHops.push({ toX: toX, toY: toY, isJump: !!matchDest.isJump });
  this.stagedNotation = this.completedSubMoves.concat([this.currentChainNotation]).join(",");

  if (!s.map[toK]) s.map[toK] = [];
  s.map[toK].push(pod);
  this.guidedPodIndex = s.map[toK].length - 1;
  this.guidedCurrentSquare = { x: toX, y: toY };

  if (matchDest.isJump) {
    var nextJumps = this.computeLegalDestsForPod(s, toX, toY, pod, this.jumpedSquaresChain).filter(function (d) { return d.isJump; });
    if (nextJumps.length > 0) {
      this.guidedLegalDests = nextJumps;
      this.isAwaitingConfirmation = true;
      this.updateUi();
      renderPlayBoardCanvas();
      renderAllWebReserves();
      return true;
    }
  }

  this.completedSubMoves.push(this.currentChainNotation);
  this.currentChainNotation = "";
  this.isAwaitingConfirmation = true;

  if (this.hasOtherMovablePodAtOrigin()) {
    this.guidedLegalDests = [];
  } else {
    this.guidedMoveActive = false;
    this.guidedLegalDests = [];
  }

  this.updateUi();
  renderPlayBoardCanvas();
  renderAllWebReserves();
  return true;
};

WebMoveComposer.prototype.toggleCaptureAtSquare = function (jx, jy) {
  var hop = this.jumpedSquaresChain.find(function (h) { return h.x === jx && h.y === jy; });
  if (!hop || !hop.capturedPodsSnapshot || !hop.capturedPodsSnapshot.length) return false;
  var s = this.getWorkingState();
  var jK = jx + "," + jy;

  if (hop.capture) {
    if (!s.map[jK]) s.map[jK] = [];
    for (var i = 0; i < hop.capturedPodsSnapshot.length; i++) {
      var pod = hop.capturedPodsSnapshot[i];
      s.map[jK].push(pod);
      var vIdx = playerIndex(pod.owner);
      s.reserves[vIdx].capturedPods = Math.max(0, s.reserves[vIdx].capturedPods - 1);
    }
    s.reserves[hop.capturerIdx].freeProngs -= (hop.seizedNormal + (s.superProngs ? 0 : hop.seizedSuper));
    if (s.superProngs) s.reserves[hop.capturerIdx].freeSuper -= hop.seizedSuper;
    hop.capture = false;
  } else {
    var rem = [];
    for (var k = 0; k < (s.map[jK] || []).length; k++) {
      var cp = s.map[jK][k];
      if (!areAllied(s.mode, cp.owner, s.turn)) {
        var vIdx2 = playerIndex(cp.owner);
        s.reserves[vIdx2].capturedPods++;
      } else {
        rem.push(cp);
      }
    }
    if (rem.length) s.map[jK] = rem; else delete s.map[jK];
    s.reserves[hop.capturerIdx].freeProngs += (hop.seizedNormal + (s.superProngs ? 0 : hop.seizedSuper));
    if (s.superProngs) s.reserves[hop.capturerIdx].freeSuper += hop.seizedSuper;
    hop.capture = true;
  }

  var not = this.guidedOriginSquare.x + "" + this.guidedOriginSquare.y + this.guidedPodConfig;
  for (var h = 0; h < this.jumpedSquaresChain.length; h++) {
    var item = this.jumpedSquaresChain[h];
    not += "-" + item.toX + "" + item.toY + (item.capture ? "x" : "");
  }
  if (this.currentChainNotation) {
    this.currentChainNotation = not;
    this.stagedNotation = this.completedSubMoves.concat([this.currentChainNotation]).join(",");
  } else if (this.completedSubMoves.length > 0) {
    this.completedSubMoves[this.completedSubMoves.length - 1] = not;
    this.stagedNotation = this.completedSubMoves.join(",");
  } else {
    this.stagedNotation = not;
  }

  if (this.guidedMoveActive && this.guidedCurrentSquare) {
    var curK2 = this.guidedCurrentSquare.x + "," + this.guidedCurrentSquare.y;
    var curStk = s.map[curK2] || [];
    var activePod = (this.guidedPodIndex >= 0 && this.guidedPodIndex < curStk.length) ? curStk[this.guidedPodIndex] : curStk[curStk.length - 1];
    if (activePod) {
      var nextJumps = this.computeLegalDestsForPod(s, this.guidedCurrentSquare.x, this.guidedCurrentSquare.y, activePod, this.jumpedSquaresChain).filter(function (d) { return d.isJump; });
      this.guidedLegalDests = nextJumps;
    }
  }

  this.updateUi();
  renderPlayBoardCanvas();
  renderAllWebReserves();
  return true;
};

WebMoveComposer.prototype.commitStagedMove = function () {
  if (!this.isAwaitingConfirmation || !this.stagedNotation) return false;
  var not = this.stagedNotation;
  var baseState = this.getState();
  var prevTurn = baseState.turn;
  var finalState = applyMoveToState(cloneState(baseState), not, prevTurn);
  this.reset();
  if (this.onCommit) {
    this.onCommit(not, finalState);
  }
  return true;
};

WebMoveComposer.prototype.cancelAndRollback = function () {
  this.reset();
  renderPlayBoardCanvas();
  renderAllWebReserves();
};



/* --- Vector Reserve HUD Rendering --- */
function renderWebReserveCard(player, cardEl, state, isTurn) {
  if (!cardEl) return;
  cardEl.innerHTML = "";
  if (!state) return;
  var pIdx = (player === "A") ? 0 : ((player === "B") ? 1 : ((player === "C") ? 2 : 3));
  var res = state.reserves ? state.reserves[pIdx] : { freePods: 0, freeProngs: 0, freeSuper: 0, capturedPods: 0 };
  var pal = PALETTES[currentTheme] || PALETTES.obsidian;
  var fp = pal.pods[player] || pal.pods.A;

  if (isTurn) {
    cardEl.className = "web-res-card active-turn";
  } else {
    cardEl.className = "web-res-card";
  }

  var header = document.createElement("div");
  header.className = "web-res-header";
  var titleSpan = document.createElement("span");
  var dotSpan = document.createElement("span");
  dotSpan.className = "pod-dot " + player;
  dotSpan.textContent = player;
  dotSpan.style.marginRight = "6px";
  titleSpan.appendChild(dotSpan);

  var nameText = "PLAYER " + player;
  if (localMatchConfig.playMode === "bot") {
    if (player === localMatchConfig.humanSeat) {
      nameText = (currentUser && currentUser.handle) ? ("@" + currentUser.handle + " (" + player + ")") : ("Human (" + player + ")");
    } else {
      nameText = "JS Robot " + player;
    }
  } else {
    nameText = "Player " + player;
  }
  titleSpan.appendChild(document.createTextNode(nameText));
  header.appendChild(titleSpan);

  if (isTurn) {
    var turnPill = document.createElement("span");
    turnPill.className = "badge";
    turnPill.style.background = fp.core;
    turnPill.style.color = "#fff";
    turnPill.textContent = "TURN";
    header.appendChild(turnPill);
  }
  cardEl.appendChild(header);

  var wells = document.createElement("div");
  wells.className = "web-res-wells";

  // 1. Pods well
  var canPlace = isTurn && res && (res.freePods > 0 || res.capturedPods > 0);
  var podWell = document.createElement("div");
  podWell.className = "web-res-well" + (canPlace ? " interactive" : "");
  podWell.title = canPlace ? "Click or drag to place onto board" : "Reserve Pods";

  var podSvg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  podSvg.setAttribute("width", "18");
  podSvg.setAttribute("height", "18");
  podSvg.setAttribute("viewBox", "0 0 24 24");
  var podPoly = document.createElementNS("http://www.w3.org/2000/svg", "polygon");
  podPoly.setAttribute("points", "7,2 17,2 22,7 22,17 17,22 7,22 2,17 2,7");
  podPoly.setAttribute("fill", fp.core);
  podPoly.setAttribute("stroke", fp.hi);
  podPoly.setAttribute("stroke-width", "1.5");
  podSvg.appendChild(podPoly);
  podWell.appendChild(podSvg);

  var podCount = document.createElement("span");
  podCount.textContent = "Pods: " + (res ? res.freePods : 0);
  podWell.appendChild(podCount);

  if (canPlace) {
    podWell.addEventListener("click", function () {
      if (!moveComposer) return;
      var homes = getHomeSquares(state.mode, player, state.bases);
      if (homes.length) {
        openZoomedSquare(homes[0].x, homes[0].y);
      }
    });
  }
  if (state.mode !== "OFK") {
    wells.appendChild(podWell);
  }

  // 2. Prongs well
  var prongWell = document.createElement("div");
  prongWell.setAttribute("data-well", "prongs");
  prongWell.className = "web-res-well" + (isTurn && res && res.freeProngs > 0 ? " interactive" : "");
  prongWell.title = isTurn ? "Standard Prongs (Click to add to friendly pod)" : "Standard Prongs";
  var prongSvg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  prongSvg.setAttribute("width", "14");
  prongSvg.setAttribute("height", "18");
  prongSvg.setAttribute("viewBox", "0 0 14 18");
  var prongP = document.createElementNS("http://www.w3.org/2000/svg", "path");
  prongP.setAttribute("d", "M7 1L12 14L7 17L2 14Z");
  prongP.setAttribute("fill", "#F8FAFC");
  prongP.setAttribute("stroke", "#0B0F17");
  prongP.setAttribute("stroke-width", "1");
  prongSvg.appendChild(prongP);
  prongWell.appendChild(prongSvg);
  var prongCount = document.createElement("span");
  prongCount.textContent = "Prongs: " + (res ? res.freeProngs : 0);
  prongWell.appendChild(prongCount);

  if (isTurn && res && res.freeProngs > 0) {
    prongWell.addEventListener("click", function () {
      if (!moveComposer) return;
      var targetSq = null;
      if (moveComposer.guidedOriginSquare) {
        targetSq = moveComposer.guidedOriginSquare;
      } else {
        for (var y = 1; y <= state.gh; y++) {
          for (var x = 1; x <= state.gw; x++) {
            var st = state.map[x + "," + y];
            if (st && st.length && st[st.length - 1].owner === player) {
              targetSq = { x: x, y: y };
              break;
            }
          }
          if (targetSq) break;
        }
      }
      if (targetSq) {
        openZoomedSquare(targetSq.x, targetSq.y);
      }
    });
  }
  wells.appendChild(prongWell);

  // 3. Superprongs well (strictly hidden if !superProngs)
  if (state.superProngs) {
    var superWell = document.createElement("div");
    superWell.className = "web-res-well";
    superWell.title = "Superprongs";
    var superSvg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    superSvg.setAttribute("width", "16");
    superSvg.setAttribute("height", "18");
    superSvg.setAttribute("viewBox", "0 0 16 18");
    var superP = document.createElementNS("http://www.w3.org/2000/svg", "path");
    superP.setAttribute("d", "M8 1L15 13L8 17L1 13Z");
    superP.setAttribute("fill", "#FDE047");
    superP.setAttribute("stroke", "#78350F");
    superP.setAttribute("stroke-width", "1");
    superSvg.appendChild(superP);
    superWell.appendChild(superSvg);
    var superCount = document.createElement("span");
    superCount.textContent = "Super: " + (res ? res.freeSuper : 0);
    superCount.style.color = "#FDE047";
    superWell.appendChild(superCount);
    wells.appendChild(superWell);
  }

  // 4. Captured Pods well: Displays opponent pods captured by this player (oppRes.capturedPods)
  var oppPlayer = (player === "A") ? "B" : "A";
  var oppIdx = (oppPlayer === "A") ? 0 : 1;
  var oppRes = state.reserves ? state.reserves[oppIdx] : null;
  var enemyPodsCapturedByPlayer = oppRes ? oppRes.capturedPods : 0;
  var ownCapturedPods = res ? res.capturedPods : 0;

  if (enemyPodsCapturedByPlayer > 0) {
    var capWell = document.createElement("div");
    capWell.className = "web-res-well";
    capWell.title = "Captured Pods (Enemy pods captured by Player " + player + ")";
    var capSvg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    capSvg.setAttribute("width", "18");
    capSvg.setAttribute("height", "18");
    capSvg.setAttribute("viewBox", "0 0 24 24");

    var oppFp = pal.pods[oppPlayer] || pal.pods.B;
    var oppPodPoly = document.createElementNS("http://www.w3.org/2000/svg", "polygon");
    oppPodPoly.setAttribute("points", "7,2 17,2 22,7 22,17 17,22 7,22 2,17 2,7");
    oppPodPoly.setAttribute("fill", oppFp.core);
    oppPodPoly.setAttribute("stroke", oppFp.hi);
    oppPodPoly.setAttribute("stroke-width", "1");
    capSvg.appendChild(oppPodPoly);

    var capCircle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
    capCircle.setAttribute("cx", "12");
    capCircle.setAttribute("cy", "12");
    capCircle.setAttribute("r", "10");
    capCircle.setAttribute("fill", "none");
    capCircle.setAttribute("stroke", "#E05252");
    capCircle.setAttribute("stroke-width", "2");
    capSvg.appendChild(capCircle);

    var capLine = document.createElementNS("http://www.w3.org/2000/svg", "line");
    capLine.setAttribute("x1", "5");
    capLine.setAttribute("y1", "5");
    capLine.setAttribute("x2", "19");
    capLine.setAttribute("y2", "19");
    capLine.setAttribute("stroke", "#E05252");
    capLine.setAttribute("stroke-width", "2");
    capSvg.appendChild(capLine);
    capWell.appendChild(capSvg);

    var capCount = document.createElement("span");
    capCount.textContent = "Captured: " + enemyPodsCapturedByPlayer;
    capCount.style.color = "#F43F5E";
    capWell.appendChild(capCount);
    wells.appendChild(capWell);
  }

  if (ownCapturedPods > 0) {
    var canLibAny = isTurn && canLiberate(state, player);
    var libWell = document.createElement("div");
    libWell.className = "web-res-well" + (canLibAny ? " interactive" : "");
    libWell.title = canLibAny ? "Click to liberate your captured pod (*)" : "Your pods held captive by opponent";
    var libSpan = document.createElement("span");
    libSpan.textContent = "Captive: " + ownCapturedPods + (canLibAny ? " (*)" : "");
    libSpan.style.color = canLibAny ? "#10B981" : "#94A3B8";
    libWell.appendChild(libSpan);
    if (canLibAny) {
      libWell.addEventListener("click", function () {
        var cands = [{x:3,y:3},{x:5,y:3},{x:7,y:3},{x:3,y:7},{x:5,y:7},{x:7,y:7}];
        for (var ci = 0; ci < cands.length; ci++) {
          if (canLiberateOnSquare(state, cands[ci].x, cands[ci].y, player)) {
            openZoomedSquare(cands[ci].x, cands[ci].y);
            break;
          }
        }
      });
    }
    wells.appendChild(libWell);
  }

  cardEl.appendChild(wells);
}

function renderAllWebReserves() {
  var s = (moveComposer && moveComposer.previewState) ? moveComposer.previewState : activePlayState;
  if (!s) return;
  var isA = (s.turn === "A");
  var isB = (s.turn === "B");
  renderWebReserveCard("A", document.getElementById("web-res-card-a"), s, isA);
  renderWebReserveCard("B", document.getElementById("web-res-card-b"), s, isB);

  var cardC = document.getElementById("web-res-card-c");
  var cardD = document.getElementById("web-res-card-d");
  if (s.mode === "4P" || s.mode === "4") {
    if (cardC) { cardC.style.display = "block"; renderWebReserveCard("C", cardC, s, s.turn === "C"); }
    if (cardD) { cardD.style.display = "block"; renderWebReserveCard("D", cardD, s, s.turn === "D"); }
  } else {
    if (cardC) cardC.style.display = "none";
    if (cardD) cardD.style.display = "none";
  }
}

/* --- Canvas Board Renderer --- */
var playBoardGeom = null;

function renderPlayBoardCanvas() {
  var canvas = document.getElementById("web-play-canvas");
  if (!canvas) return;
  var s = (moveComposer && moveComposer.previewState) ? moveComposer.previewState : activePlayState;
  if (!s) return;

  var pal = PALETTES[currentTheme] || PALETTES.obsidian;
  var cssSize = 760;
  var dpr = window.devicePixelRatio || 1;
  var pxSize = Math.round(cssSize * dpr);
  if (canvas.width !== pxSize || canvas.height !== pxSize) {
    canvas.width = pxSize;
    canvas.height = pxSize;
  }
  var ctx = canvas.getContext("2d");
  ctx.save();
  ctx.scale(dpr, dpr);

  // Apply board zoom & focus
  if (boardZoomScale > 1.001) {
    ctx.translate(cssSize * boardZoomFocus.x, cssSize * boardZoomFocus.y);
    ctx.scale(boardZoomScale, boardZoomScale);
    ctx.translate(-cssSize * boardZoomFocus.x, -cssSize * boardZoomFocus.y);
  }

  ctx.fillStyle = pal.windowBg;
  ctx.fillRect(0, 0, cssSize, cssSize);

  var effectiveRot = ((boardRotationDeg + liveRotationOffsetDeg) % 360 + 360) % 360;
  var geom = computeBoardGeometry(cssSize, cssSize, s.gw, s.gh, effectiveRot);
  playBoardGeom = geom;

  // 1. Luxury Outer Frame
  var frameGrad = ctx.createLinearGradient(geom.outer.x, geom.outer.y, geom.outer.x + geom.outer.w, geom.outer.y + geom.outer.h);
  frameGrad.addColorStop(0, pal.boardOuterTop);
  frameGrad.addColorStop(0.5, pal.boardOuterMid);
  frameGrad.addColorStop(1, pal.boardOuterBot);
  roundRectPath(ctx, geom.outer.x, geom.outer.y, geom.outer.w, geom.outer.h, 12);
  ctx.fillStyle = frameGrad;
  ctx.fill();
  ctx.lineWidth = 2.4;
  ctx.strokeStyle = pal.bevelLight;
  ctx.stroke();

  roundRectPath(ctx, geom.outer.x + 3.5, geom.outer.y + 3.5, geom.outer.w - 7, geom.outer.h - 7, 9);
  ctx.lineWidth = 1.6;
  ctx.strokeStyle = pal.bevelDark;
  ctx.stroke();

  // 2. Coordinate Gutters
  ctx.font = "bold " + Math.max(9, Math.round(geom.cell * 0.23)) + "px 'Segoe UI', sans-serif";
  ctx.fillStyle = pal.coordLabel;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  for (var x = 1; x <= s.gw; x++) {
    var rBot = geom.squareRect(x, 1);
    var rTop = geom.squareRect(x, s.gh);
    if (geom.rotDeg === 0 || geom.rotDeg === 180) {
      ctx.fillText(String(x), rBot.cx, geom.grid.y + geom.grid.h + geom.gutter * 0.52);
      ctx.fillText(String(x), rTop.cx, geom.outer.y + geom.gutter * 0.48);
    } else {
      ctx.fillText(String(x), geom.outer.x + geom.gutter * 0.48, rBot.ry);
      ctx.fillText(String(x), geom.grid.x + geom.grid.w + geom.gutter * 0.52, rTop.ry);
    }
  }
  for (var y = 1; y <= s.gh; y++) {
    var rLeft = geom.squareRect(1, y);
    var rRight = geom.squareRect(s.gw, y);
    if (geom.rotDeg === 0 || geom.rotDeg === 180) {
      ctx.fillText(String(y), geom.outer.x + geom.gutter * 0.48, rLeft.ry);
      ctx.fillText(String(y), geom.grid.x + geom.grid.w + geom.gutter * 0.52, rRight.ry);
    } else {
      ctx.fillText(String(y), rLeft.cx, geom.grid.y + geom.grid.h + geom.gutter * 0.52);
      ctx.fillText(String(y), rRight.cx, geom.outer.y + geom.gutter * 0.48);
    }
  }

  // 3. Edgeless Portal Glow
  if (s.edgeless) {
    ctx.save();
    ctx.setLineDash([6, 4]);
    ctx.strokeStyle = pal.portalGlow;
    ctx.lineWidth = 2.0;
    ctx.strokeRect(geom.grid.x - 3, geom.grid.y - 3, geom.grid.w + 6, geom.grid.h + 6);
    ctx.restore();
  }

  // 4. Grid Squares, Sockets, Home Bases
  for (var sy = 1; sy <= s.gh; sy++) {
    for (var sx = 1; sx <= s.gw; sx++) {
      var sqRect = geom.squareRect(sx, sy);
      var isDark = ((sx + sy) % 2 === 0);
      ctx.fillStyle = isDark ? pal.cellDark : pal.cellLight;
      ctx.fillRect(sqRect.x, sqRect.y, sqRect.w, sqRect.h);
      ctx.lineWidth = 1.0;
      ctx.strokeStyle = pal.gridLine;
      ctx.strokeRect(sqRect.x, sqRect.y, sqRect.w, sqRect.h);

      // Recessed socket ring
      ctx.beginPath();
      ctx.arc(sqRect.cx, sqRect.ry, sqRect.w * 0.34, 0, Math.PI * 2);
      ctx.lineWidth = 1.1;
      ctx.strokeStyle = pal.socketShadow;
      ctx.stroke();

      var st = squareTypeAt(s.mode, sx, sy);
      var stack = s.map[sx + "," + sy] || [];
      if (st === "A_HOME" || st === "B_HOME" || st === "C_HOME" || st === "D_HOME") {
        var hOwner = st.charAt(0);
        ctx.fillStyle = pal.homeOuter;
        ctx.fillRect(sqRect.x + 1, sqRect.y + 1, sqRect.w - 2, sqRect.h - 2);
        roundRectPath(ctx, sqRect.x + 3.5, sqRect.y + 3.5, sqRect.w - 7, sqRect.h - 7, 4);
        ctx.fillStyle = pal.homeInner;
        ctx.fill();
        var isCap = stack.some(function (p) { return p.owner !== hOwner; });
        ctx.lineWidth = isCap ? 2.2 : 1.2;
        ctx.strokeStyle = isCap ? pal.captureGlow : pal.bevelDark;
        ctx.stroke();

        if (!stack.length) {
          var seatRot = rotationForSeat(s.mode, hOwner);
          var txtAng = ((seatRot - effectiveRot) % 360 + 360) % 360;
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
          ctx.textAlign = "center";
          ctx.textBaseline = "middle";
          ctx.fillText("OCTI", sqRect.cx, sqRect.ry);
        }
        ctx.restore();
      }
    }
  }

  // 5. Board Pods: Sorted Fewest-Prongs-at-Bottom, Most-Prongs-on-Top
  for (var py = 1; py <= s.gh; py++) {
    for (var px = 1; px <= s.gw; px++) {
      var podStack = s.map[px + "," + py];
      if (!podStack || !podStack.length) continue;
      var baseR = geom.squareRect(px, py);

      // Sort ascending by prong count so top layer (maxL - 1) has the most prongs
      var sortedAsc = podStack.slice().sort(function (a, b) {
        return (a.cfg ? a.cfg.count : 0) - (b.cfg ? b.cfg.count : 0);
      });
      var maxL = Math.min(sortedAsc.length, 4);
      for (var l = 0; l < maxL; l++) {
        var pItem = (l === maxL - 1) ? sortedAsc[sortedAsc.length - 1] : sortedAsc[l];
        var off = (maxL - 1 - l) * 2.4;
        var layerR = {
          x: baseR.x - off,
          y: baseR.y + off,
          w: baseR.w,
          h: baseR.h
        };
        var isTop = (l === maxL - 1);
        var centerBadge = (isTop && sortedAsc.length >= 2) ? ("x" + sortedAsc.length) : "";
        drawOctagonalPod(ctx, layerR, pItem.owner, pItem.cfg, pal, effectiveRot, false, centerBadge);
      }
    }
  }

  var nowMs = Date.now();

  // 6. Clockwise Orange Move Indicator (#FF8C00, 5000ms duration, 2000ms period)
  if (lastMoveHighlightSquares && lastMoveHighlightSquares.length &&
      lastMoveSetAtMs && (nowMs - lastMoveSetAtMs) < LAST_MOVE_DISPLAY_DURATION_MS) {
    var elapsed = Math.max(0, nowMs - lastMoveSetAtMs);
    var phase = (elapsed % 2000) / 2000.0;
    var s0 = phase * 4.0;
    var s1 = s0 + 1.2;
    ctx.save();
    ctx.strokeStyle = "#FF8C00";
    ctx.lineWidth = 3.2;
    ctx.lineCap = "round";
    ctx.lineJoin = "round";
    lastMoveHighlightSquares.forEach(function (sq) {
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

  // 7. Pink (#FF69B4) Active-Player Board-Edge Turn Indicator
  if (s.status === "P" && s.turn && s.turn !== "-") {
    var edge = activeTurnScreenEdge(s.mode, s.turn, effectiveRot);
    var gr = geom.grid;
    var inset = 1.0;
    var startPt, endPt;
    if (edge === 1) { // Left
      startPt = [gr.x - inset, gr.y + gr.h];
      endPt = [gr.x - inset, gr.y];
    } else if (edge === 2) { // Top
      startPt = [gr.x, gr.y - inset];
      endPt = [gr.x + gr.w, gr.y - inset];
    } else if (edge === 3) { // Right
      startPt = [gr.x + gr.w + inset, gr.y];
      endPt = [gr.x + gr.w + inset, gr.y + gr.h];
    } else { // Bottom
      startPt = [gr.x + gr.w, gr.y + gr.h + inset];
      endPt = [gr.x, gr.y + gr.h + inset];
    }
    var pulse = (Math.sin(nowMs * 0.00314) + 1.0) * 0.5;
    ctx.save();
    ctx.strokeStyle = "#FF69B4";
    ctx.lineWidth = 3.5 + pulse * 1.5;
    ctx.lineCap = "round";
    ctx.beginPath();
    ctx.moveTo(startPt[0], startPt[1]);
    ctx.lineTo(endPt[0], endPt[1]);
    ctx.stroke();
    ctx.restore();
  }

  // 8. Dashed Origin Indicator Ring around guidedOriginSquare
  if (moveComposer && moveComposer.guidedOriginSquare && moveComposer.hasOtherMovablePodAtOrigin()) {
    var oSq = geom.squareRect(moveComposer.guidedOriginSquare.x, moveComposer.guidedOriginSquare.y);
    ctx.save();
    ctx.setLineDash([5, 4]);
    ctx.strokeStyle = "#38BDF8";
    ctx.lineWidth = 2.4;
    ctx.beginPath();
    ctx.arc(oSq.cx, oSq.ry, oSq.w * 0.44, 0, Math.PI * 2);
    ctx.stroke();
    ctx.restore();
  }

  // 9. Guided Move Destination Rings & Dots (Green #10B981)
  if (moveComposer && moveComposer.guidedLegalDests && moveComposer.guidedLegalDests.length) {
    ctx.save();
    moveComposer.guidedLegalDests.forEach(function (dst) {
      var dSq = geom.squareRect(dst.x, dst.y);
      ctx.beginPath();
      ctx.arc(dSq.cx, dSq.ry, dSq.w * 0.28, 0, Math.PI * 2);
      ctx.fillStyle = "rgba(16, 185, 129, 0.28)";
      ctx.fill();
      ctx.lineWidth = 2.0;
      ctx.strokeStyle = "#10B981";
      ctx.stroke();

      ctx.beginPath();
      ctx.arc(dSq.cx, dSq.ry, dSq.w * 0.08, 0, Math.PI * 2);
      ctx.fillStyle = "#10B981";
      ctx.fill();
    });
    ctx.restore();
  }

  // 10. Top Victory Banner on Game Over
  if (s.status === "O" || s.status === "R") {
    ctx.save();
    ctx.fillStyle = "rgba(16, 22, 34, 0.88)";
    ctx.fillRect(geom.grid.x, geom.grid.y + 12, geom.grid.w, 38);
    ctx.strokeStyle = "#C8A253";
    ctx.lineWidth = 1.5;
    ctx.strokeRect(geom.grid.x, geom.grid.y + 12, geom.grid.w, 38);

    ctx.font = "bold 15px 'Segoe UI', sans-serif";
    ctx.fillStyle = "#FDE047";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    var wLabel = "PLAYER " + (s.winner || "A") + " WINS";
    if (s.mode === "4" || s.mode === "4P") {
      wLabel = (s.winner === "A" || s.winner === "C" || s.winner === "TEAM_13_WIN")
        ? "TEAM A & C WINS" : "TEAM B & D WINS";
    }
    var reasonStr = (s.status === "R") ? "RESIGNATION" : (s.termination ? s.termination.replace("_", " ") : "BASE OCCUPATION");
    var winMsg = wLabel + " — VICTORY BY " + reasonStr;
    ctx.fillText(winMsg, geom.grid.x + geom.grid.w / 2, geom.grid.y + 31);
    ctx.restore();
  }

  ctx.restore();
}

/* --- ZoomedSquareDialog Implementation --- */
var zoomSq = null;
var zoomSelectedPodIdx = 0;
var zoomHoverDir = null;
var zoomSelectedRepositionDir = null;

function openZoomedSquare(x, y) {
  if (typeof x === "object" && x !== null) {
    y = x.y;
    x = x.x;
  }
  var s = (moveComposer && moveComposer.previewState) ? moveComposer.previewState : activePlayState;
  if (!s) return;
  zoomSq = { x: x, y: y };
  var k = x + "," + y;
  var stack = s.map[k] || [];
  var sortedIndices = sortedPodIndicesMostProngsFirst(stack);
  zoomSelectedPodIdx = sortedIndices.length ? sortedIndices[0] : 0;
  for (var si = 0; si < sortedIndices.length; si++) {
    if (stack[sortedIndices[si]].owner === s.turn) {
      zoomSelectedPodIdx = sortedIndices[si];
      break;
    }
  }
  zoomHoverDir = null;
  zoomSelectedRepositionDir = null;

  var overlay = document.getElementById("web-zoom-overlay");
  var titleEl = document.getElementById("web-zoom-sq-title");
  if (titleEl) titleEl.textContent = "Square (" + x + ", " + y + ")";

  updateZoomOverlayControls();
  renderZoomCanvas();
  if (overlay) overlay.style.display = "flex";
}

function closeZoomedSquare() {
  var overlay = document.getElementById("web-zoom-overlay");
  if (overlay) overlay.style.display = "none";
  zoomSq = null;
  zoomSelectedRepositionDir = null;
}

function updateZoomOverlayControls() {
  var s = (moveComposer && moveComposer.previewState) ? moveComposer.previewState : activePlayState;
  if (!s || !zoomSq) return;
  var k = zoomSq.x + "," + zoomSq.y;
  var stack = s.map[k] || [];
  var turn = s.turn;
  var isTurn = ((matchStarted || activeOnlineGameId) && s.status === "P" &&
    (localMatchConfig.playMode === "pass" || s.turn === localMatchConfig.humanSeat || (activeOnlineGameId && isMyOnlineTurn(s))));

  var tabsContainer = document.getElementById("web-zoom-pod-tabs");
  if (tabsContainer) {
    tabsContainer.innerHTML = "";
    var sortedIndices = sortedPodIndicesMostProngsFirst(stack);
    sortedIndices.forEach(function (pIdx, rank) {
      var btn = document.createElement("button");
      btn.type = "button";
      var isMoved = moveComposer && moveComposer.claimedPodsAtOrigin[pIdx];
      btn.className = "zoom-pod-tab-btn" + (pIdx === zoomSelectedPodIdx ? " active" : "");
      btn.textContent = "Pod " + (rank + 1) + " (" + stack[pIdx].owner + ")" + (isMoved ? " (Moved)" : "");
      btn.addEventListener("click", function () {
        zoomSelectedPodIdx = pIdx;
        zoomSelectedRepositionDir = null;
        updateZoomOverlayControls();
        renderZoomCanvas();
      });
      tabsContainer.appendChild(btn);
    });
  }

  // Update counts on tray wells
  var pIdx = (turn === "A") ? 0 : ((turn === "B") ? 1 : ((turn === "C") ? 2 : 3));
  var res = s.reserves[pIdx];
  var prongCountEl = document.getElementById("zoom-prong-count");
  if (prongCountEl) prongCountEl.textContent = res ? res.freeProngs : 0;
  var superCountEl = document.getElementById("zoom-super-count");
  if (superCountEl) superCountEl.textContent = res ? res.freeSuper : 0;
  var normWell = document.getElementById("zoom-well-normal");
  var superWell = document.getElementById("zoom-well-super");
  if (superWell) {
    superWell.style.display = s.superProngs ? "flex" : "none";
    if (!s.superProngs && superWell.classList.contains("active")) {
      superWell.classList.remove("active");
      if (normWell) normWell.classList.add("active");
    }
  }

  // Action Buttons
  var movePodBtn = document.getElementById("btn-zoom-move-pod");
  var placePodBtn = document.getElementById("btn-zoom-place-pod");
  var liberatePodBtn = document.getElementById("btn-zoom-liberate-pod");

  var curPod = (stack && stack[zoomSelectedPodIdx]) ? stack[zoomSelectedPodIdx] : null;
  var canMove = isTurn && curPod && curPod.owner === turn && !(moveComposer && moveComposer.claimedPodsAtOrigin[zoomSelectedPodIdx]);
  if (movePodBtn) {
    movePodBtn.style.display = canMove ? "inline-block" : "none";
  }

  var canPlaceReserve = isTurn && canPlaceReservePodOnSquare(s, zoomSq.x, zoomSq.y, turn);
  if (placePodBtn) {
    placePodBtn.style.display = canPlaceReserve ? "inline-block" : "none";
  }

  var canLiberateHere = isTurn && canLiberateOnSquare(s, zoomSq.x, zoomSq.y, turn);
  if (liberatePodBtn) {
    liberatePodBtn.style.display = canLiberateHere ? "inline-block" : "none";
  }
}

function renderZoomCanvas() {
  var canvas = document.getElementById("web-zoom-canvas");
  if (!canvas || !zoomSq) return;
  var s = (moveComposer && moveComposer.previewState) ? moveComposer.previewState : activePlayState;
  if (!s) return;
  var pal = PALETTES[currentTheme] || PALETTES.obsidian;

  var ctx = canvas.getContext("2d");
  ctx.clearRect(0, 0, 320, 320);

  ctx.fillStyle = pal.windowBg;
  ctx.fillRect(0, 0, 320, 320);

  var k = zoomSq.x + "," + zoomSq.y;
  var stack = s.map[k] || [];
  var sortedIndices = sortedPodIndicesMostProngsFirst(stack);
  var podR = 68;
  var cx = 160, cy = 160;
  var rotDeg = ((boardRotationDeg + liveRotationOffsetDeg) % 360 + 360) % 360;
  var isTurn = ((matchStarted || activeOnlineGameId) && s.status === "P" &&
    (localMatchConfig.playMode === "pass" || s.turn === localMatchConfig.humanSeat || (activeOnlineGameId && isMyOnlineTurn(s))));

  // 1. Draw non-selected pods underneath with stepped offsets at 0.28 alpha
  var stepCount = 0;
  for (var i = sortedIndices.length - 1; i >= 0; i--) {
    var pIdx = sortedIndices[i];
    if (pIdx === zoomSelectedPodIdx) continue;
    stepCount++;
    var offX = -stepCount * 12;
    var offY = stepCount * 12;
    var uPod = stack[pIdx];
    ctx.save();
    ctx.globalAlpha = 0.28;
    var uRect = { x: cx + offX - podR, y: cy + offY - podR, w: podR * 2, h: podR * 2 };
    drawOctagonalPod(ctx, uRect, uPod.owner, uPod.cfg, pal, rotDeg, false, "Pod " + (i + 1));
    ctx.restore();
  }

  // 2. Draw selected pod centered at 1.0 alpha
  if (stack.length && stack[zoomSelectedPodIdx]) {
    var activePod = stack[zoomSelectedPodIdx];
    ctx.save();
    ctx.globalAlpha = 1.0;
    var aRect = { x: cx - podR, y: cy - podR, w: podR * 2, h: podR * 2 };
    drawOctagonalPod(ctx, aRect, activePod.owner, activePod.cfg, pal, rotDeg, true, activePod.owner);
    ctx.restore();

    // Highlight selected reposition source prong if active
    if (zoomSelectedRepositionDir !== null && activePod.cfg.prongs[zoomSelectedRepositionDir] === 1) {
      var angRad = ((zoomSelectedRepositionDir * 45 + rotDeg - 90) * Math.PI) / 180;
      var hx = cx + Math.cos(angRad) * (podR * 1.18);
      var hy = cy + Math.sin(angRad) * (podR * 1.18);
      ctx.save();
      ctx.strokeStyle = "#38BDF8";
      ctx.lineWidth = 2.5;
      ctx.setLineDash([4, 3]);
      ctx.beginPath();
      ctx.arc(hx, hy, 14, 0, Math.PI * 2);
      ctx.stroke();
      ctx.restore();
    }

    // 3. Draw hover preview blade if hovering empty socket on own pod during turn
    if (isTurn && activePod.owner === s.turn && zoomHoverDir !== null && !activePod.cfg.prongs[zoomHoverDir]) {
      var pIdxTurn = playerIndex(s.turn);
      var resTurn = s.reserves[pIdxTurn];
      var isSuperSelected = s.superProngs && document.getElementById("zoom-well-super") &&
        document.getElementById("zoom-well-super").classList.contains("active");
      var canPreview = (zoomSelectedRepositionDir !== null) ||
        (isSuperSelected ? (resTurn && resTurn.freeSuper > 0) : (resTurn && resTurn.freeProngs > 0));
      if (canPreview) {
        var mockCfg = { prongs: [0, 0, 0, 0, 0, 0, 0, 0] };
        mockCfg.prongs[zoomHoverDir] = (zoomSelectedRepositionDir !== null) ? 1 : (isSuperSelected ? 2 : 1);
        ctx.save();
        ctx.globalAlpha = 0.72;
        drawProngs(ctx, cx, cy, podR, mockCfg, pal, rotDeg);
        ctx.restore();
      }
    }
  } else {
    // Empty square view
    ctx.font = "14px 'Segoe UI', sans-serif";
    ctx.fillStyle = "#94A3B8";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText("Empty Square", cx, cy);
  }
}

// Wire Zoom Card Action Buttons
var btnZoomMove = document.getElementById("btn-zoom-move-pod");
if (btnZoomMove) {
  btnZoomMove.addEventListener("click", function () {
    if (!zoomSq || !moveComposer) return;
    var x = zoomSq.x, y = zoomSq.y, idx = zoomSelectedPodIdx;
    closeZoomedSquare();
    moveComposer.beginGuidedPodMove(x, y, idx);
  });
}
var btnZoomPlace = document.getElementById("btn-zoom-place-pod");
if (btnZoomPlace) {
  btnZoomPlace.addEventListener("click", function () {
    if (!zoomSq || !moveComposer) return;
    var x = zoomSq.x, y = zoomSq.y;
    closeZoomedSquare();
    moveComposer.stageAddPod(x, y, false);
  });
}
var btnZoomLiberate = document.getElementById("btn-zoom-liberate-pod");
if (btnZoomLiberate) {
  btnZoomLiberate.addEventListener("click", function () {
    if (!zoomSq || !moveComposer) return;
    var x = zoomSq.x, y = zoomSq.y;
    closeZoomedSquare();
    moveComposer.stageAddPod(x, y, true);
  });
}

// Zoom Tray Wells Selection
var zoomWellNormal = document.getElementById("zoom-well-normal");
var zoomWellSuper = document.getElementById("zoom-well-super");
if (zoomWellNormal && zoomWellSuper) {
  zoomWellNormal.addEventListener("click", function () {
    zoomSelectedRepositionDir = null;
    zoomWellNormal.classList.add("active");
    zoomWellSuper.classList.remove("active");
    renderZoomCanvas();
  });
  zoomWellSuper.addEventListener("click", function () {
    zoomSelectedRepositionDir = null;
    zoomWellSuper.classList.add("active");
    zoomWellNormal.classList.remove("active");
    renderZoomCanvas();
  });
}

// Zoom Canvas Mouse & Touch Events
var zoomCanvas = document.getElementById("web-zoom-canvas");
if (zoomCanvas) {
  function getZoomDirFromPoint(px, py) {
    var cx = 160, cy = 160;
    var dx = px - cx, dy = py - cy;
    var dist = Math.hypot(dx, dy);
    if (dist < 40 || dist > 115) return null;
    var angle = Math.atan2(dy, dx) * 180 / Math.PI; // -180 to 180
    // Directions: 0: N (-90), 1: NE (-45), 2: E (0), 3: SE (45), 4: S (90), 5: SW (135), 6: W (180), 7: NW (-135)
    var rotDeg = ((boardRotationDeg + liveRotationOffsetDeg) % 360 + 360) % 360;
    var dNorm = ((angle - rotDeg + 90) % 360 + 360) % 360; // 0 is N, 45 is NE, etc.
    var sector = Math.round(dNorm / 45) % 8;
    return sector;
  }

  zoomCanvas.addEventListener("mousemove", function (e) {
    var rect = zoomCanvas.getBoundingClientRect();
    var scaleX = 320 / rect.width, scaleY = 320 / rect.height;
    var x = (e.clientX - rect.left) * scaleX;
    var y = (e.clientY - rect.top) * scaleY;
    var dir = getZoomDirFromPoint(x, y);
    if (dir !== zoomHoverDir) {
      zoomHoverDir = dir;
      renderZoomCanvas();
    }
  });

  zoomCanvas.addEventListener("mouseleave", function () {
    zoomHoverDir = null;
    renderZoomCanvas();
  });

  zoomCanvas.addEventListener("click", function (e) {
    var s = (moveComposer && moveComposer.previewState) ? moveComposer.previewState : activePlayState;
    if (!s || !zoomSq || !moveComposer) return;
    var isTurn = ((matchStarted || activeOnlineGameId) && s.status === "P" &&
      (localMatchConfig.playMode === "pass" || s.turn === localMatchConfig.humanSeat || (activeOnlineGameId && isMyOnlineTurn(s))));
    if (!isTurn) return;

    var rect = zoomCanvas.getBoundingClientRect();
    var x = (e.clientX - rect.left) * (320 / rect.width);
    var y = (e.clientY - rect.top) * (320 / rect.height);
    var dir = getZoomDirFromPoint(x, y);
    if (dir === null) return;

    var sqX = zoomSq.x, sqY = zoomSq.y, pIdx = zoomSelectedPodIdx;
    var stack = s.map[sqX + "," + sqY] || [];
    var pod = stack[pIdx];
    if (!pod || pod.owner !== s.turn) return;

    // 1. Clicking an existing normal prong in 2P/4P selects/deselects it for repositioning
    if (pod.cfg.prongs[dir] === 1 && s.mode !== "OFK") {
      if (zoomSelectedRepositionDir === dir) {
        zoomSelectedRepositionDir = null;
      } else {
        zoomSelectedRepositionDir = dir;
      }
      renderZoomCanvas();
      return;
    }

    // 2. Clicking an empty socket while a reposition source prong is selected repositions it
    if (!pod.cfg.prongs[dir] && zoomSelectedRepositionDir !== null) {
      var srcDir = zoomSelectedRepositionDir;
      zoomSelectedRepositionDir = null;
      var okRepo = moveComposer.stageRepositionProng(sqX, sqY, pIdx, srcDir, dir);
      if (okRepo) {
        closeZoomedSquare();
      }
      return;
    }

    // 3. Otherwise add a reserve prong to the empty socket
    if (!pod.cfg.prongs[dir]) {
      var isSuper = s.superProngs && zoomWellSuper && zoomWellSuper.classList.contains("active");
      var ok = moveComposer.stageAddProng(sqX, sqY, pIdx, dir, isSuper);
      if (ok) {
        closeZoomedSquare();
      }
    }
  });

  // Mouse wheel cycling over zoom canvas
  zoomCanvas.addEventListener("wheel", function (e) {
    e.preventDefault();
    if (!zoomSq) return;
    var s = (moveComposer && moveComposer.previewState) ? moveComposer.previewState : activePlayState;
    if (!s) return;
    var stack = s.map[zoomSq.x + "," + zoomSq.y] || [];
    var sorted = sortedPodIndicesMostProngsFirst(stack);
    if (sorted.length <= 1) return;
    var curRank = sorted.indexOf(zoomSelectedPodIdx);
    if (e.deltaY > 0) {
      curRank = (curRank + 1) % sorted.length;
    } else {
      curRank = ((curRank - 1) % sorted.length + sorted.length) % sorted.length;
    }
    zoomSelectedPodIdx = sorted[curRank];
    updateZoomOverlayControls();
    renderZoomCanvas();
  });
}

// Dismiss ZoomedSquareDialog on outside backdrop click or Esc
var zoomOverlay = document.getElementById("web-zoom-overlay");
if (zoomOverlay) {
  zoomOverlay.addEventListener("click", function (e) {
    if (e.target === zoomOverlay) {
      closeZoomedSquare();
    }
  });
}



/* --- Board Canvas Event & Gesture Handlers --- */
var playCanvas = document.getElementById("web-play-canvas");
var boardMouseHeld = false;
var boardPressInGrid = false;
var boardPressRawPos = { x: 0, y: 0 };
var boardLastPanRawPos = { x: 0, y: 0 };
var boardPanActive = false;
var handRotateActive = false;
var handRotateRefAngleDeg = 0;
var dragJumpedSquare = null;

if (playCanvas) {
  function getRawPos(e) {
    var rect = playCanvas.getBoundingClientRect();
    return {
      x: (e.clientX - rect.left) * (760 / rect.width),
      y: (e.clientY - rect.top) * (760 / rect.height)
    };
  }

  function getBoardPos(raw) {
    if (boardZoomScale <= 1.001) return { x: raw.x, y: raw.y };
    var cx = 760 * boardZoomFocus.x;
    var cy = 760 * boardZoomFocus.y;
    return {
      x: (raw.x - cx) / boardZoomScale + cx,
      y: (raw.y - cy) / boardZoomScale + cy
    };
  }

  // 1. Wheel Zoom (1.0x .. 3.0x) anchored at cursor
  playCanvas.addEventListener("wheel", function (e) {
    e.preventDefault();
    var raw = getRawPos(e);
    var oldScale = boardZoomScale;
    var newScale = (e.deltaY < 0) ? Math.min(3.0, oldScale * 1.15) : Math.max(1.0, oldScale / 1.15);
    if (newScale <= 1.001) {
      boardZoomScale = 1.0;
      boardZoomFocus = { x: 0.5, y: 0.5 };
    } else {
      var boardX = (oldScale > 1.001) ? ((raw.x - 760 * boardZoomFocus.x) / oldScale + 760 * boardZoomFocus.x) : raw.x;
      var boardY = (oldScale > 1.001) ? ((raw.y - 760 * boardZoomFocus.y) / oldScale + 760 * boardZoomFocus.y) : raw.y;
      boardZoomFocus.x = (boardX * newScale - raw.x) / ((newScale - 1.0) * 760);
      boardZoomFocus.y = (boardY * newScale - raw.y) / ((newScale - 1.0) * 760);
      boardZoomFocus.x = Math.max(0.0, Math.min(1.0, boardZoomFocus.x));
      boardZoomFocus.y = Math.max(0.0, Math.min(1.0, boardZoomFocus.y));
      boardZoomScale = newScale;
    }
    renderPlayBoardCanvas();
  });

  // 2. Mouse Press (Left / Right)
  playCanvas.addEventListener("mousedown", function (e) {
    if (lastMoveHighlightSquares.length) {
      lastMoveHighlightSquares = [];
      renderPlayBoardCanvas();
    }
    var raw = getRawPos(e);
    boardPressRawPos = raw;
    boardLastPanRawPos = raw;
    boardPanActive = false;

    // Right-click handling: open zoom on valid board square even when boardZoomScale > 1.001
    if (e.button === 2) {
      e.preventDefault();
      e.stopPropagation();
      if (playBoardGeom) {
        var bPos = getBoardPos(raw);
        var sq = playBoardGeom.screenToSquare(bPos.x, bPos.y);
        if (sq) {
          openZoomedSquare(sq.x, sq.y);
          return;
        }
      }
      if (boardZoomScale > 1.001) {
        boardZoomScale = 1.0;
        boardZoomFocus = { x: 0.5, y: 0.5 };
        renderPlayBoardCanvas();
      }
      return;
    }

    if (e.button === 0) {
      boardMouseHeld = true;
      handRotateActive = false;
      var bPos = getBoardPos(raw);
      boardPressInGrid = !!(playBoardGeom &&
        bPos.x >= playBoardGeom.grid.x && bPos.x <= playBoardGeom.grid.x + playBoardGeom.grid.w &&
        bPos.y >= playBoardGeom.grid.y && bPos.y <= playBoardGeom.grid.y + playBoardGeom.grid.h);
      handRotateRefAngleDeg = Math.atan2(raw.y - 380, raw.x - 380) * 180 / Math.PI;
    }
  });

  // Context menu prevention across all board and zoom overlays
  ["web-play-canvas", "web-zoom-overlay", "web-zoom-canvas", "web-zoom-card", "web-play-board-wrap", "play-local-panel"].forEach(function (id) {
    var el = document.getElementById(id);
    if (el) {
      el.addEventListener("contextmenu", function (e) {
        e.preventDefault();
        e.stopPropagation();
        return false;
      });
    }
  });

  // Double-click handling: open zoom on valid board square even when boardZoomScale > 1.001
  playCanvas.addEventListener("dblclick", function (e) {
    e.preventDefault();
    var raw = getRawPos(e);
    if (playBoardGeom) {
      var bPos = getBoardPos(raw);
      var sq = playBoardGeom.screenToSquare(bPos.x, bPos.y);
      if (sq) {
        openZoomedSquare(sq.x, sq.y);
        return;
      }
    }
    if (boardZoomScale > 1.001) {
      boardZoomScale = 1.0;
      boardZoomFocus = { x: 0.5, y: 0.5 };
      renderPlayBoardCanvas();
    }
  });

  // 3. Mouse Move: Drag-Pan when Zoomed, Continuous Rotation when Unzoomed outside grid
  playCanvas.addEventListener("mousemove", function (e) {
    var raw = getRawPos(e);

    if (boardMouseHeld && boardZoomScale > 1.001) {
      var dist = Math.hypot(raw.x - boardPressRawPos.x, raw.y - boardPressRawPos.y);
      if (dist > 6 || boardPanActive) {
        boardPanActive = true;
        var dx = raw.x - boardLastPanRawPos.x;
        var dy = raw.y - boardLastPanRawPos.y;
        boardLastPanRawPos = raw;
        boardZoomFocus.x -= dx / ((boardZoomScale - 1.0) * 760);
        boardZoomFocus.y -= dy / ((boardZoomScale - 1.0) * 760);
        boardZoomFocus.x = Math.max(0.0, Math.min(1.0, boardZoomFocus.x));
        boardZoomFocus.y = Math.max(0.0, Math.min(1.0, boardZoomFocus.y));
        renderPlayBoardCanvas();
        return;
      }
    }

    if (boardMouseHeld && boardZoomScale <= 1.001) {
      var mDist = Math.hypot(raw.x - boardPressRawPos.x, raw.y - boardPressRawPos.y);
      if (!boardPressInGrid && (mDist > 18 || handRotateActive)) {
        handRotateActive = true;
        var curAngle = Math.atan2(raw.y - 380, raw.x - 380) * 180 / Math.PI;
        var diff = curAngle - handRotateRefAngleDeg;
        while (diff > 180) diff -= 360;
        while (diff < -180) diff += 360;
        liveRotationOffsetDeg = diff;
        renderPlayBoardCanvas();
        return;
      }
    }
  });

  // 4. Mouse Up & Click: Snap 90deg, Off-Board Drag Capture, Drag-Move, or Square Click
  playCanvas.addEventListener("mouseup", function (e) {
    if (!boardMouseHeld) return;
    boardMouseHeld = false;
    var raw = getRawPos(e);

    if (handRotateActive) {
      handRotateActive = false;
      var rawSnapped = Math.round((boardRotationDeg + liveRotationOffsetDeg) / 90.0) * 90;
      boardRotationDeg = ((rawSnapped % 360) + 360) % 360;
      liveRotationOffsetDeg = 0;
      renderPlayBoardCanvas();
      return;
    }

    if (boardPanActive) {
      boardPanActive = false;
      return;
    }

    // Off-board drag to toggle capture check
    if (moveComposer && moveComposer.jumpedSquaresChain.length && playBoardGeom) {
      var bPos = getBoardPos(raw);
      if (bPos.x < playBoardGeom.grid.x || bPos.x > playBoardGeom.grid.x + playBoardGeom.grid.w ||
          bPos.y < playBoardGeom.grid.y || bPos.y > playBoardGeom.grid.y + playBoardGeom.grid.h) {
        var lastJ = moveComposer.jumpedSquaresChain[moveComposer.jumpedSquaresChain.length - 1];
        moveComposer.toggleCaptureAtSquare(lastJ.x, lastJ.y);
        return;
      }
    }

    // Drag-to-move support: if user pressed down on square A and released on legal destination square B
    var bPressPos = getBoardPos(boardPressRawPos);
    var bReleasePos = getBoardPos(raw);
    var pressSq = playBoardGeom ? playBoardGeom.screenToSquare(bPressPos.x, bPressPos.y) : null;
    var releaseSq = playBoardGeom ? playBoardGeom.screenToSquare(bReleasePos.x, bReleasePos.y) : null;
    var dragDist = Math.hypot(raw.x - boardPressRawPos.x, raw.y - boardPressRawPos.y);

    if (dragDist > 14 && pressSq && releaseSq && (pressSq.x !== releaseSq.x || pressSq.y !== releaseSq.y)) {
      if (moveComposer && moveComposer.isGuidedMoveActive) {
        var matchD = moveComposer.guidedLegalDests.find(function (d) { return d.x === releaseSq.x && d.y === releaseSq.y; });
        if (matchD) {
          moveComposer.selectGuidedDestination(releaseSq.x, releaseSq.y);
          return;
        }
      } else {
        var stCur = (moveComposer && moveComposer.previewState) ? moveComposer.previewState : activePlayState;
        if (stCur && stCur.status === "P") {
          var kSt = pressSq.x + "," + pressSq.y;
          var stackSt = stCur.map[kSt] || [];
          var sortedSt = sortedPodIndicesMostProngsFirst(stackSt);
          var bestIdxSt = -1;
          for (var si = 0; si < sortedSt.length; si++) {
            if (stackSt[sortedSt[si]].owner === stCur.turn) { bestIdxSt = sortedSt[si]; break; }
          }
          if (bestIdxSt !== -1) {
            moveComposer.beginGuidedPodMove(pressSq.x, pressSq.y, bestIdxSt);
            var matchD = moveComposer.guidedLegalDests.find(function (d) { return d.x === releaseSq.x && d.y === releaseSq.y; });
            if (matchD) {
              moveComposer.selectGuidedDestination(releaseSq.x, releaseSq.y);
              return;
            }
          }
        }
      }
    }

    // Board Square Click Handling
    if (!matchStarted && !activeOnlineGameId) return;
    var s = (moveComposer && moveComposer.previewState) ? moveComposer.previewState : activePlayState;
    if (!s || s.status === "O" || s.status === "R") return;

    var isMyTurn = (localMatchConfig.playMode === "pass" || s.turn === localMatchConfig.humanSeat || (activeOnlineGameId && isMyOnlineTurn(s)));
    if (!isMyTurn) return;

    if (playBoardGeom) {
      var bPt = getBoardPos(raw);
      var sq = playBoardGeom.screenToSquare(bPt.x, bPt.y);
      if (sq) {
        handlePlaySquareClick(sq.x, sq.y, bPt.x, bPt.y);
      }
    }
  });

  // Touch double-tap & long-press gestures
  var lastTouchTime = 0;
  var touchStartPt = { x: 0, y: 0 };
  var touchHoldTimer = null;

  playCanvas.addEventListener("touchstart", function (e) {
    if (e.touches.length === 1) {
      var t = e.touches[0];
      var rect = playCanvas.getBoundingClientRect();
      var raw = {
        x: (t.clientX - rect.left) * (760 / rect.width),
        y: (t.clientY - rect.top) * (760 / rect.height)
      };
      touchStartPt = raw;
      var now = Date.now();
      if (now - lastTouchTime < 280) {
        // Double tap -> Zoom View
        e.preventDefault();
        if (playBoardGeom) {
          var bPos = getBoardPos(raw);
          var sq = playBoardGeom.screenToSquare(bPos.x, bPos.y);
          if (sq) openZoomedSquare(sq.x, sq.y);
        }
      }
      lastTouchTime = now;

      // Long press (420ms) -> Zoom View
      touchHoldTimer = setTimeout(function () {
        if (playBoardGeom) {
          var bPos = getBoardPos(raw);
          var sq = playBoardGeom.screenToSquare(bPos.x, bPos.y);
          if (sq) openZoomedSquare(sq.x, sq.y);
        }
      }, 420);
    }
  }, { passive: false });

  playCanvas.addEventListener("touchmove", function () {
    if (touchHoldTimer) {
      clearTimeout(touchHoldTimer);
      touchHoldTimer = null;
    }
  }, { passive: true });

  playCanvas.addEventListener("touchend", function () {
    if (touchHoldTimer) {
      clearTimeout(touchHoldTimer);
      touchHoldTimer = null;
    }
  });
}

// Square Click Logic: Guided Move / Prong Addition / Zoom View
function handlePlaySquareClick(x, y, ptX, ptY) {
  if (!moveComposer) return;
  var s = (moveComposer.previewState) ? moveComposer.previewState : activePlayState;
  var k = x + "," + y;
  var stack = s.map[k] || [];
  var turn = s.turn;
  var pIdx = (turn === "A") ? 0 : ((turn === "B") ? 1 : ((turn === "C") ? 2 : 3));
  var res = s.reserves[pIdx];

  // 1. If guided move active and clicked a destination
  if (moveComposer.isGuidedMoveActive) {
    var matchDest = moveComposer.guidedLegalDests.find(function (d) { return d.x === x && d.y === y; });
    if (matchDest) {
      moveComposer.selectGuidedDestination(x, y);
      return;
    }
    // If clicked origin square while 0 hops have been made (2nd click on origin pod)
    var isOrigin = moveComposer.guidedOriginSquare && moveComposer.guidedOriginSquare.x === x && moveComposer.guidedOriginSquare.y === y;
    if (isOrigin && (!moveComposer.guidedHops || moveComposer.guidedHops.length === 0)) {
      moveComposer.cancel();
      openZoomedSquare(x, y);
      return;
    }
    if (isOrigin && moveComposer.hasOtherMovablePodAtOrigin()) {
      openZoomedSquare(x, y);
      return;
    }
  }

  // If clicked origin square after finishing first pod move in a multi-pod stack, allow composite move
  if (!moveComposer.isGuidedMoveActive && moveComposer.guidedOriginSquare &&
      moveComposer.guidedOriginSquare.x === x && moveComposer.guidedOriginSquare.y === y &&
      moveComposer.hasOtherMovablePodAtOrigin()) {
    openZoomedSquare(x, y);
    return;
  }

  // 2. Find friendly pod on square with the most prongs (top of visual stack)
  var sortedIndices = sortedPodIndicesMostProngsFirst(stack);
  var bestFriendlyIdx = -1;
  for (var si = 0; si < sortedIndices.length; si++) {
    if (stack[sortedIndices[si]].owner === turn) {
      bestFriendlyIdx = sortedIndices[si];
      break;
    }
  }

  if (bestFriendlyIdx !== -1) {
    var topPod = stack[bestFriendlyIdx];
    // Check if outer socket ring was clicked directly on board (only when no move is currently staged)
    if (playBoardGeom && !moveComposer.isAwaitingConfirmation && res && res.freeProngs > 0) {
      var sqR = playBoardGeom.squareRect(x, y);
      var dx = ptX - sqR.cx, dy = ptY - sqR.ry;
      var dist = Math.hypot(dx, dy);
      var cellR = sqR.w * 0.36;
      if (dist >= cellR * 0.92 && dist <= cellR * 1.32) {
        var effRot = ((boardRotationDeg + liveRotationOffsetDeg) % 360 + 360) % 360;
        var angle = Math.atan2(dy, dx) * 180 / Math.PI;
        var dNorm = ((angle - effRot + 90) % 360 + 360) % 360;
        var sector = Math.round(dNorm / 45) % 8;
        if (!topPod.cfg.prongs[sector]) {
          if (moveComposer.stageAddProng(x, y, bestFriendlyIdx, sector, false)) {
            return;
          }
        }
      }
    }

    // Begin guided move for top friendly pod
    moveComposer.beginGuidedPodMove(x, y, bestFriendlyIdx);
    if (!moveComposer.guidedLegalDests || moveComposer.guidedLegalDests.length === 0) {
      // Unarmed pod -> open zoom modal directly!
      moveComposer.cancel();
      openZoomedSquare(x, y);
      return;
    }
    return;
  }

  // 3. If friendly home square and freePods > 0, stage placement
  var homes = getHomeSquares(s.mode, turn);
  var isHome = homes.some(function (h) { return h.x === x && h.y === y; });
  if (isHome && res && res.freePods > 0 && !squareHasEnemyOf(s, x, y, turn)) {
    moveComposer.stageAddPod(x, y, false);
    return;
  }

  // 4. Otherwise if square has any pods (e.g. enemy pod inspection), open zoom view
  if (stack.length > 0) {
    openZoomedSquare(x, y);
  }
}

// Global Keydown Listeners (Enter to commit, Esc to cancel / close)
window.addEventListener("keydown", function (e) {
  var capOverlay = document.getElementById("web-capture-overlay");
  var capOpen = (capOverlay && capOverlay.style.display === "flex");

  if (e.key === "Escape") {
    if (capOpen) {
      var btnNo = document.getElementById("btn-capture-no");
      if (btnNo) { btnNo.click(); return; }
      capOverlay.style.display = "none";
      return;
    }
    var zoomEl = document.getElementById("web-zoom-overlay");
    if (zoomEl && zoomEl.style.display === "flex") {
      closeZoomedSquare();
      return;
    }
    if (boardZoomScale > 1.001) {
      boardZoomScale = 1.0;
      boardZoomFocus = { x: 0.5, y: 0.5 };
      renderPlayBoardCanvas();
      return;
    }
    if (moveComposer && moveComposer.isAwaitingConfirmation) {
      moveComposer.cancelAndRollback();
      return;
    }
  }

  if (e.key === "Enter") {
    if (capOpen) {
      var btnYes = document.getElementById("btn-capture-yes");
      if (btnYes) { btnYes.click(); return; }
      capOverlay.style.display = "none";
      return;
    }
    if (moveComposer && moveComposer.isAwaitingConfirmation) {
      moveComposer.commitStagedMove();
      return;
    }
  }
});

// Staged Confirmation Bar Buttons
var btnCommitPly = document.getElementById("btn-web-commit-ply");
if (btnCommitPly) {
  btnCommitPly.addEventListener("click", function () {
    if (moveComposer) moveComposer.commitStagedMove();
  });
}
var btnCancelPly = document.getElementById("btn-web-cancel-ply");
if (btnCancelPly) {
  btnCancelPly.addEventListener("click", function () {
    if (moveComposer) moveComposer.cancelAndRollback();
  });
}

// Capture Dialog Buttons
var btnCapYes = document.getElementById("btn-capture-yes");
var btnCapNo = document.getElementById("btn-capture-no");
if (btnCapYes && btnCapNo) {
  btnCapYes.addEventListener("click", function () {
    var capOverlay = document.getElementById("web-capture-overlay");
    if (capOverlay) capOverlay.style.display = "none";
  });
  btnCapNo.addEventListener("click", function () {
    var capOverlay = document.getElementById("web-capture-overlay");
    if (capOverlay) capOverlay.style.display = "none";
    if (moveComposer && moveComposer.jumpedSquaresChain.length) {
      var lastJ = moveComposer.jumpedSquaresChain[moveComposer.jumpedSquaresChain.length - 1];
      moveComposer.toggleCaptureAtSquare(lastJ.x, lastJ.y);
    }
  });
}

// Game Over Dialog Buttons
var btnGoNew = document.getElementById("btn-gameover-new");
var btnGoReview = document.getElementById("btn-gameover-review");
if (btnGoNew && btnGoReview) {
  btnGoNew.addEventListener("click", function () {
    var goOverlay = document.getElementById("web-gameover-overlay");
    if (goOverlay) goOverlay.style.display = "none";
    showMatchSetupModal();
  });
  btnGoReview.addEventListener("click", function () {
    var goOverlay = document.getElementById("web-gameover-overlay");
    if (goOverlay) goOverlay.style.display = "none";
  });
}

// Rotate 90 CW buttons
var btnWebRotate = document.getElementById("btn-web-rotate");
if (btnWebRotate) {
  btnWebRotate.addEventListener("click", function () {
    boardRotationDeg = (boardRotationDeg + 90) % 360;
    renderPlayBoardCanvas();
  });
}

// Theme Selectors
var webThemeSelect = document.getElementById("web-play-theme-select");
if (webThemeSelect) {
  webThemeSelect.addEventListener("change", function () {
    currentTheme = webThemeSelect.value || "obsidian";
    var globalThemeSelect = document.getElementById("theme-select");
    if (globalThemeSelect) globalThemeSelect.value = currentTheme;
    renderPlayBoardCanvas();
    renderAllWebReserves();
  });
}

// SFX Toggle Button
var btnSfx = document.getElementById("btn-sfx");
if (btnSfx) {
  btnSfx.addEventListener("click", function () {
    OctiWebSoundSynth.setEnabled(!OctiWebSoundSynth.isEnabled());
  });
}

/* --- Match Setup Modal & Non-Modal Start Match Overlay --- */
function syncSetupModalControls(gMode) {
  var isOfk = (gMode === "OFK");
  var is4p = (gMode === "4P" || gMode === "4");
  var optC = document.getElementById("setup-seat-opt-c");
  var optD = document.getElementById("setup-seat-opt-d");
  if (optC) optC.style.display = is4p ? "block" : "none";
  if (optD) optD.style.display = is4p ? "block" : "none";
  var seatSel = document.getElementById("setup-human-seat");
  if (seatSel && !is4p && (seatSel.value === "C" || seatSel.value === "D")) {
    seatSel.value = "A";
  }
  var basesSel = document.getElementById("setup-bases");
  if (basesSel) {
    if (isOfk) basesSel.value = "1";
    else if (is4p) basesSel.value = "3";
    basesSel.disabled = (isOfk || is4p);
  }
  var spCb = document.getElementById("setup-superprongs");
  if (spCb) {
    if (isOfk) spCb.checked = false;
    spCb.disabled = isOfk;
  }
  var edCb = document.getElementById("setup-edgeless");
  if (edCb) {
    if (isOfk) edCb.checked = false;
    edCb.disabled = isOfk;
  }
}

function updateStartMatchSummary() {
  var line1 = document.getElementById("web-start-summary-line1");
  var line2 = document.getElementById("web-start-summary-line2");
  if (!line1 || !line2) return;

  var is4p = (localMatchConfig.gameMode === "4P" || localMatchConfig.gameMode === "4");
  var playModeStr = localMatchConfig.playMode === "bot"
    ? ("vs AI Bot (Lv" + localMatchConfig.botDifficulty + ")")
    : ("Pass-and-Play (Local " + (is4p ? "4P" : "2P") + ")");
  var seatColors = { A: "Emerald", B: "Ruby", C: "Sapphire", D: "Amber" };
  var seatStr = "Seat " + localMatchConfig.humanSeat + " (" + (seatColors[localMatchConfig.humanSeat] || "Emerald") + ")";
  line1.textContent = playModeStr + " • " + seatStr;

  var modeStr = localMatchConfig.gameMode + " (" + (localMatchConfig.gameMode === "OFK" ? "6x7" : "9x9") + ")";
  var basesStr = is4p ? "2 Enemy Bases" : (localMatchConfig.bases + " " + (localMatchConfig.bases === 1 ? "Base" : "Bases"));
  var extras = [];
  if (localMatchConfig.superProngs) extras.push("Superprongs");
  if (localMatchConfig.edgeless) extras.push("Edgeless");
  if (localMatchConfig.timeControl !== "untimed") extras.push(localMatchConfig.timeControl);
  var extraStr = extras.length ? (" • " + extras.join(", ")) : " • Standard";
  line2.textContent = modeStr + " • " + basesStr + extraStr;
}

function showMatchSetupModal() {
  var modal = document.getElementById("web-match-setup-modal");
  if (!modal) return;
  modal.style.display = "flex";
  if (document.getElementById("setup-play-mode")) document.getElementById("setup-play-mode").value = localMatchConfig.playMode;
  if (document.getElementById("setup-bot-difficulty")) document.getElementById("setup-bot-difficulty").value = String(localMatchConfig.botDifficulty);
  if (document.getElementById("setup-game-mode")) document.getElementById("setup-game-mode").value = localMatchConfig.gameMode;
  if (document.getElementById("setup-bases")) document.getElementById("setup-bases").value = String(localMatchConfig.bases === 1 ? 1 : 3);
  if (document.getElementById("setup-superprongs")) document.getElementById("setup-superprongs").checked = localMatchConfig.superProngs;
  if (document.getElementById("setup-edgeless")) document.getElementById("setup-edgeless").checked = localMatchConfig.edgeless;
  if (document.getElementById("setup-time-control")) document.getElementById("setup-time-control").value = localMatchConfig.timeControl;
  syncSetupModalControls(localMatchConfig.gameMode);
  if (document.getElementById("setup-human-seat")) document.getElementById("setup-human-seat").value = localMatchConfig.humanSeat;
}

var setupGameModeSelect = document.getElementById("setup-game-mode");
if (setupGameModeSelect) {
  setupGameModeSelect.addEventListener("change", function () {
    syncSetupModalControls(setupGameModeSelect.value);
  });
}

var btnChangeType = document.getElementById("btn-web-change-type");
if (btnChangeType) {
  btnChangeType.addEventListener("click", showMatchSetupModal);
}
var btnNewGameSetup = document.getElementById("btn-web-new-game-setup");
if (btnNewGameSetup) {
  btnNewGameSetup.addEventListener("click", showMatchSetupModal);
}

var btnSetupCancel = document.getElementById("btn-setup-cancel");
if (btnSetupCancel) {
  btnSetupCancel.addEventListener("click", function () {
    var modal = document.getElementById("web-match-setup-modal");
    if (modal) modal.style.display = "none";
  });
}

var btnSetupApply = document.getElementById("btn-setup-apply");
if (btnSetupApply) {
  btnSetupApply.addEventListener("click", function () {
    var modal = document.getElementById("web-match-setup-modal");
    if (modal) modal.style.display = "none";

    localMatchConfig.playMode = document.getElementById("setup-play-mode").value;
    localMatchConfig.botDifficulty = parseInt(document.getElementById("setup-bot-difficulty").value, 10) || 2;
    localMatchConfig.gameMode = document.getElementById("setup-game-mode").value;
    localMatchConfig.humanSeat = document.getElementById("setup-human-seat").value || "A";
    if (localMatchConfig.gameMode !== "4P" && localMatchConfig.gameMode !== "4" &&
        (localMatchConfig.humanSeat === "C" || localMatchConfig.humanSeat === "D")) {
      localMatchConfig.humanSeat = "A";
    }
    var parsedBases = parseInt(document.getElementById("setup-bases").value, 10);
    localMatchConfig.bases = (parsedBases === 1) ? 1 : 3;
    if (localMatchConfig.gameMode === "OFK") {
      localMatchConfig.bases = 1;
      localMatchConfig.superProngs = false;
      localMatchConfig.edgeless = false;
    } else {
      if (localMatchConfig.gameMode === "4P" || localMatchConfig.gameMode === "4") {
        localMatchConfig.bases = 3;
      }
      localMatchConfig.superProngs = !!document.getElementById("setup-superprongs").checked;
      localMatchConfig.edgeless = !!document.getElementById("setup-edgeless").checked;
    }
    localMatchConfig.timeControl = document.getElementById("setup-time-control").value;

    updateStartMatchSummary();
    prepareLocalMatchInitialState();
  });
}

function prepareLocalMatchInitialState() {
  if (clockInterval) {
    clearInterval(clockInterval);
    clockInterval = null;
  }
  if (moveComposer) {
    moveComposer.reset();
  }
  var goOverlay = document.getElementById("web-gameover-overlay");
  if (goOverlay) goOverlay.style.display = "none";
  var thinkOverlay = document.getElementById("web-thinking-overlay");
  if (thinkOverlay) thinkOverlay.style.display = "none";
  var archSt = document.getElementById("web-play-archive-status");
  if (archSt) archSt.textContent = "";

  matchStarted = false;
  activeOnlineGameId = "";
  var mode = localMatchConfig.gameMode;
  var bases = (localMatchConfig.bases === 1) ? 1 : 3;
  var sp = localMatchConfig.superProngs ? "1" : "0";
  var ed = localMatchConfig.edgeless ? "1" : "0";

  var fen = "";
  if (mode === "OFK") {
    bases = 1;
    localMatchConfig.bases = 1;
    localMatchConfig.superProngs = false;
    localMatchConfig.edgeless = false;
    sp = "0";
    ed = "0";
    fen = "OFK:1:0:0:P:A:-:0:0,0,12,0:0,0,12,0:A22,A32,A42,A52,B26,B36,B46,B56";
  } else if (mode === "4" || mode === "4P") {
    bases = 3;
    localMatchConfig.bases = 3;
    var r4p = "3,0," + (localMatchConfig.superProngs ? "11,1" : "12,0");
    fen = "4:3:" + sp + ":" + ed + ":P:A:-:0:" + r4p + ":" + r4p + ":" + r4p + ":" + r4p + ":A33,D73,B37,C77";
  } else {
    var r2p = "4,0," + (localMatchConfig.superProngs ? "24,1" : "25,0");
    fen = "2:" + bases + ":" + sp + ":" + ed + ":P:A:-:0:" + r2p + ":" + r2p + ":A33,A53,A73,B37,B57,B77";
  }

  activePlayState = parseOctiFen(fen);
  playMatchHistory = [];
  playMatchPlies = [{
    plyIndex: 0,
    player: "-",
    move: "(Initial Setup)",
    fenAfter: fen
  }];
  playVcrIndex = 0;
  lastMoveHighlightSquares = [];

  var startOverlay = document.getElementById("web-start-match-overlay");
  if (startOverlay) startOverlay.style.display = "flex";

  updateStartMatchSummary();
  updatePlaySidebar();
  renderPlayBoardCanvas();
  renderAllWebReserves();
}

function updateClockDisplay() {
  function fmtClock(sec) {
    if (sec <= 0 && localMatchConfig.timeControl === "untimed") return "--:--";
    var m = Math.floor(sec / 60);
    var sc = sec % 60;
    return (m < 10 ? "0" : "") + m + ":" + (sc < 10 ? "0" : "") + sc;
  }
  var is4p = activePlayState && (activePlayState.mode === "4P" || activePlayState.mode === "4");
  if (document.getElementById("web-clock-a")) document.getElementById("web-clock-a").textContent = "A: " + fmtClock(clockTimers.A);
  if (document.getElementById("web-clock-b")) document.getElementById("web-clock-b").textContent = "B: " + fmtClock(clockTimers.B);
  if (document.getElementById("web-clock-c")) {
    document.getElementById("web-clock-c").style.display = is4p ? "block" : "none";
    document.getElementById("web-clock-c").textContent = "C: " + fmtClock(clockTimers.C);
  }
  if (document.getElementById("web-clock-d")) {
    document.getElementById("web-clock-d").style.display = is4p ? "block" : "none";
    document.getElementById("web-clock-d").textContent = "D: " + fmtClock(clockTimers.D);
  }
}

var btnStartMatch = document.getElementById("btn-web-start-match");
if (btnStartMatch) {
  btnStartMatch.addEventListener("click", function () {
    var startOverlay = document.getElementById("web-start-match-overlay");
    if (startOverlay) startOverlay.style.display = "none";
    matchStarted = true;

    // Reset clocks
    var initialSec = 0;
    if (localMatchConfig.timeControl === "300+3") initialSec = 300;
    else if (localMatchConfig.timeControl === "600+5") initialSec = 600;
    else if (localMatchConfig.timeControl === "900+10") initialSec = 900;
    clockTimers.A = initialSec;
    clockTimers.B = initialSec;
    clockTimers.C = initialSec;
    clockTimers.D = initialSec;

    if (clockInterval) {
      clearInterval(clockInterval);
      clockInterval = null;
    }
    if (localMatchConfig.timeControl !== "untimed") {
      clockInterval = setInterval(function () {
        if (!matchStarted || !activePlayState || activePlayState.status !== "P") {
          return;
        }
        var curTurn = activePlayState.turn;
        if (clockTimers[curTurn] !== undefined && clockTimers[curTurn] > 0) {
          clockTimers[curTurn]--;
          updateClockDisplay();
          if (clockTimers[curTurn] <= 0) {
            clearInterval(clockInterval);
            clockInterval = null;
            activePlayState.status = "O";
            activePlayState.turn = "-";
            if (activePlayState.mode === "4" || activePlayState.mode === "4P") {
              activePlayState.winner = (curTurn === "A" || curTurn === "C") ? "B" : "A";
            } else {
              activePlayState.winner = (curTurn === "A") ? "B" : "A";
            }
            activePlayState.termination = "TIMEOUT";
            OctiWebSoundSynth.play("GAME_VICTORY");
            var goOverlay = document.getElementById("web-gameover-overlay");
            var goTitle = document.getElementById("web-gameover-title");
            var goSub = document.getElementById("web-gameover-subtitle");
            if (goOverlay && goTitle && goSub) {
              if (activePlayState.mode === "4" || activePlayState.mode === "4P") {
                goTitle.textContent = (activePlayState.winner === "A" ? "TEAM A & C" : "TEAM B & D") + " VICTORIOUS!";
              } else {
                goTitle.textContent = "PLAYER " + activePlayState.winner + " WINS";
              }
              goSub.textContent = "VICTORY ON TIME";
              goOverlay.style.display = "flex";
            }
            updatePlaySidebar();
            renderPlayBoardCanvas();
            renderAllWebReserves();
            autoUploadCompletedLocalMatch();
          }
        }
      }, 1000);
    }

    updatePlaySidebar();
    renderPlayBoardCanvas();
    renderAllWebReserves();

    checkMatchTurn();
  });
}

function updatePlaySidebar() {
  if (!activePlayState) return;
  var s = activePlayState;
  var is4p = (s.mode === "4" || s.mode === "4P");
  var modeBadge = document.getElementById("web-play-mode-badge");
  if (modeBadge) {
    var baseDesc = is4p ? "2 Enemy Bases" : (s.bases + (s.bases === 1 ? " Base" : " Bases"));
    modeBadge.textContent = s.mode + " • " + baseDesc;
  }

  var turnBadge = document.getElementById("web-play-turn-badge");
  if (turnBadge) {
    var turnStyles = {
      A: { bg: "#064E3B", fg: "#6EE7B7" },
      B: { bg: "#881337", fg: "#FDA4AF" },
      C: { bg: "#1E3A8A", fg: "#93C5FD" },
      D: { bg: "#78350F", fg: "#FDE047" }
    };
    if (s.status === "O" || s.status === "R") {
      var winLabel = "Player " + s.winner;
      if (is4p) {
        winLabel = (s.winner === "A" || s.winner === "C" || s.winner === "TEAM_13_WIN")
          ? "Team A & C" : "Team B & D";
      }
      turnBadge.textContent = "Winner: " + winLabel;
      var wSt = turnStyles[s.winner === "A" || s.winner === "TEAM_13_WIN" ? "A" : "B"];
      turnBadge.style.background = wSt.bg;
      turnBadge.style.color = wSt.fg;
    } else {
      turnBadge.textContent = "Player " + s.turn + " Turn";
      var tSt = turnStyles[s.turn] || turnStyles.A;
      turnBadge.style.background = tSt.bg;
      turnBadge.style.color = tSt.fg;
    }
  }

  updateClockDisplay();

  // Update Move History List
  var movesBox = document.getElementById("web-play-moves");
  if (movesBox) {
    movesBox.innerHTML = "";
    if (playMatchPlies.length <= 1) {
      var d = document.createElement("div");
      d.className = "dim";
      d.style.padding = "4px";
      d.textContent = "No moves recorded yet.";
      movesBox.appendChild(d);
    } else {
      var pal = PALETTES[currentTheme] || PALETTES.obsidian;
      for (var i = 1; i < playMatchPlies.length; i++) {
        var pl = playMatchPlies[i];
        var row = document.createElement("div");
        var fp = pal.pods[pl.player] || pal.pods.A;
        row.style.background = fp.sh;
        row.style.color = fp.hi;
        row.style.borderLeft = "3px solid " + fp.rim;
        row.style.padding = "3px 8px";
        row.style.marginBottom = "4px";
        row.style.borderRadius = "4px";
        row.style.cursor = "pointer";
        row.textContent = "#" + pl.plyIndex + " [" + pl.player + "] " + pl.move;
        (function (targetIdx) {
          row.addEventListener("click", function () {
            showPlayVcrPly(targetIdx);
          });
        })(i);
        movesBox.appendChild(row);
      }
      movesBox.scrollTop = movesBox.scrollHeight;
    }
  }

  // Update VCR label
  var vcrLabel = document.getElementById("web-vcr-label");
  if (vcrLabel) {
    var tot = Math.max(0, playMatchPlies.length - 1);
    vcrLabel.textContent = "Ply " + playVcrIndex + "/" + tot;
  }
}

function showPlayVcrPly(idx) {
  if (idx < 0 || idx >= playMatchPlies.length) return;
  if (moveComposer) moveComposer.reset();
  playVcrIndex = idx;
  var pl = playMatchPlies[idx];
  activePlayState = parseOctiFen(pl.fenAfter);
  lastMoveHighlightSquares = extractModifiedSquaresFromMove(pl.move);
  lastMoveSetAtMs = Date.now();
  updatePlaySidebar();
  renderPlayBoardCanvas();
  renderAllWebReserves();
}

// VCR Bar Navigation
var vcrFirst = document.getElementById("web-vcr-first");
var vcrPrev = document.getElementById("web-vcr-prev");
var vcrNext = document.getElementById("web-vcr-next");
var vcrLast = document.getElementById("web-vcr-last");
if (vcrFirst) vcrFirst.addEventListener("click", function () { showPlayVcrPly(0); });
if (vcrPrev) vcrPrev.addEventListener("click", function () { showPlayVcrPly(Math.max(0, playVcrIndex - 1)); });
if (vcrNext) vcrNext.addEventListener("click", function () { showPlayVcrPly(Math.min(playMatchPlies.length - 1, playVcrIndex + 1)); });
if (vcrLast) vcrLast.addEventListener("click", function () { showPlayVcrPly(playMatchPlies.length - 1); });

// Export Move History Modal & Transcript
var btnExportMoves = document.getElementById("btn-web-export-moves");
if (btnExportMoves) {
  btnExportMoves.addEventListener("click", function () {
    var modal = document.getElementById("web-export-modal");
    var txtArea = document.getElementById("web-export-json-text");
    if (!modal || !txtArea) return;
    var transcript = generateOctiTranscriptV3();
    txtArea.value = JSON.stringify(transcript, null, 2);
    modal.style.display = "flex";
  });
}

var btnCloseExport = document.getElementById("btn-web-close-export");
if (btnCloseExport) {
  btnCloseExport.addEventListener("click", function () {
    var modal = document.getElementById("web-export-modal");
    if (modal) modal.style.display = "none";
  });
}

var btnCopyTranscript = document.getElementById("btn-web-copy-transcript");
if (btnCopyTranscript) {
  btnCopyTranscript.addEventListener("click", function () {
    var txtArea = document.getElementById("web-export-json-text");
    if (txtArea && navigator.clipboard) {
      navigator.clipboard.writeText(txtArea.value).then(function () {
        btnCopyTranscript.textContent = "Copied!";
        setTimeout(function () { btnCopyTranscript.textContent = "Copy to Clipboard"; }, 1500);
      });
    }
  });
}

function generateOctiTranscriptV3() {
  var s = activePlayState;
  var gId = "web-" + Date.now().toString(36) + "-" + Math.random().toString(36).substr(2, 6);
  var nowSec = Math.floor(Date.now() / 1000);
  var movesList = [];
  var pliesList = [];
  for (var i = 1; i < playMatchPlies.length; i++) {
    var pl = playMatchPlies[i];
    movesList.push(pl.move);
    var pObj = {
      ply: i,
      player: pl.player,
      move: pl.move
    };
    if (pl.ai) {
      pObj.ai = pl.ai;
    }
    if (pl.elapsedMs !== undefined) {
      pObj.elapsedMs = pl.elapsedMs;
    }
    pliesList.push(pObj);
  }

  var winnerStr = "NONE";
  if (s.winner && s.winner !== "-") {
    if (s.mode === "4" || s.mode === "4P") {
      winnerStr = (s.winner === "A" || s.winner === "C" || s.winner === "TEAM_13_WIN")
        ? "A" : "B";
    } else {
      winnerStr = s.winner;
    }
  }

  var isPass = (localMatchConfig.playMode === "pass");
  var diff = localMatchConfig.botDifficulty || 2;
  var prof = BOT_PROFILES[diff] || BOT_PROFILES[2];
  var budgetMs = prof.timeBudgetMs || 3000;
  var humanHandle = (currentUser && currentUser.handle) ? currentUser.handle : null;

  function makeSeatMeta(seatNum, seatLetter, defaultBotName) {
    var isHumanSeat = isPass || (localMatchConfig.humanSeat === seatLetter);
    if (isHumanSeat) {
      return {
        seat: seatNum,
        player: seatLetter,
        handle: (localMatchConfig.humanSeat === seatLetter && humanHandle) ? humanHandle : ("Player_" + seatLetter),
        type: "HUMAN"
      };
    }
    return {
      seat: seatNum,
      player: seatLetter,
      handle: defaultBotName,
      type: "AI",
      aiDifficulty: diff,
      aiTimeBudgetMs: budgetMs
    };
  }

  var playersArr = [];
  if (s.mode === "4" || s.mode === "4P") {
    playersArr = [
      makeSeatMeta(1, "A", "AI_Casual"),
      makeSeatMeta(2, "B", "AI_Tactical"),
      makeSeatMeta(3, "C", "AI_Master"),
      makeSeatMeta(4, "D", "AI_Tactical")
    ];
  } else {
    var botName2P = (diff === 1) ? "AI_Casual" : ((diff === 3) ? "AI_Master" : "AI_Tactical");
    playersArr = [
      makeSeatMeta(1, "A", botName2P),
      makeSeatMeta(2, "B", botName2P)
    ];
  }

  var outMode = s.mode;
  if (outMode === "2P" || outMode === "2") outMode = "2";
  else if (outMode === "4P" || outMode === "4") outMode = "4";

  var fenStr = serializeOctiFen(s);

  return {
    format: "octi.transcript.v3",
    schema: "octi.transcript.v3",
    version: 3,
    game_id: gId,
    mode: outMode,
    bases: s.bases,
    superProngs: !!s.superProngs,
    edgeless: !!s.edgeless,
    startPlayer: "A",
    status: (s.status === "R" ? "RESIGNED" : (s.status === "O" ? "GAME_OVER" : "IN_PROGRESS")),
    termination: s.termination || (s.status === "R" ? "RESIGN" : (s.status === "O" ? "BASE_CAPTURE" : "NONE")),
    winner: winnerStr,
    finalFen: fenStr,
    final_fen: fenStr,
    players: playersArr,
    moves: movesList,
    plies: pliesList,
    timestamp: nowSec
  };
}

function autoUploadCompletedLocalMatch() {
  if (activeOnlineGameId || !activePlayState || playMatchPlies.length <= 1) return;
  if (activePlayState.status !== "O" && activePlayState.status !== "R") return;
  var trObj = generateOctiTranscriptV3();
  var archSt = document.getElementById("web-play-archive-status");
  if (archSt) archSt.textContent = "Uploading transcript to archive...";
  api("/api/v1/archive/upload", {
    method: "POST",
    body: { transcript: JSON.stringify(trObj) }
  }).then(function (res) {
    if (archSt && res && res.game_id) {
      archSt.textContent = "Archived (#" + res.game_id.slice(0, 8) + ")";
    }
    loadArchiveStats();
  }).catch(function () {
    if (archSt) archSt.textContent = "";
  });
}

// Move Commit Callback: updates active state, history, checks victory, triggers bot
function onLocalMoveCommitted(notation, newState, aiMeta) {
  if (playVcrIndex < playMatchPlies.length - 1) {
    playMatchPlies.length = playVcrIndex + 1;
  }

  lastMoveHighlightSquares = extractModifiedSquaresFromMove(notation);
  lastMoveSetAtMs = Date.now();

  var prevTurn = activePlayState.turn;
  if (newState.turn === prevTurn && newState.status === "P") {
    newState = applyMoveToState(cloneState(activePlayState), notation, prevTurn);
  }
  activePlayState = newState;

  // Add clock increment to prevTurn
  var inc = 0;
  if (localMatchConfig.timeControl === "300+3") inc = 3;
  else if (localMatchConfig.timeControl === "600+5") inc = 5;
  else if (localMatchConfig.timeControl === "900+10") inc = 10;
  if (inc > 0 && clockTimers[prevTurn] !== undefined) {
    clockTimers[prevTurn] += inc;
    updateClockDisplay();
  }

  var plyIdx = playMatchPlies.length;
  var plyObj = {
    plyIndex: plyIdx,
    player: prevTurn,
    move: notation,
    fenAfter: serializeOctiFen(activePlayState)
  };
  if (aiMeta) {
    var diff = localMatchConfig.botDifficulty || 2;
    var prof = BOT_PROFILES[diff] || BOT_PROFILES[2];
    plyObj.ai = {
      difficulty: diff,
      budgetMs: prof.timeBudgetMs || 3000,
      depthReached: aiMeta.depth,
      nodesVisited: aiMeta.nodes,
      bestScore: aiMeta.score,
      score: aiMeta.score,
      depth: aiMeta.depth,
      nodes: aiMeta.nodes,
      elapsedMs: aiMeta.elapsedMs
    };
    plyObj.elapsedMs = aiMeta.elapsedMs;
  }
  playMatchPlies.push(plyObj);
  playVcrIndex = plyIdx;

  updatePlaySidebar();
  renderPlayBoardCanvas();
  renderAllWebReserves();

  if (activePlayState.status === "O") {
    if (clockInterval) {
      clearInterval(clockInterval);
      clockInterval = null;
    }
    OctiWebSoundSynth.play("GAME_VICTORY");
    var goOverlay = document.getElementById("web-gameover-overlay");
    var goTitle = document.getElementById("web-gameover-title");
    var goSub = document.getElementById("web-gameover-subtitle");
    if (goOverlay && goTitle && goSub) {
      if (activePlayState.mode === "4" || activePlayState.mode === "4P") {
        var w = activePlayState.winner;
        var teamStr = (w === "A" || w === "C" || w === "TEAM_13_WIN") ? "TEAM A & C" : "TEAM B & D";
        goTitle.textContent = teamStr + " VICTORIOUS!";
      } else {
        goTitle.textContent = "PLAYER " + activePlayState.winner + " WINS!";
      }
      goSub.textContent = (activePlayState.termination || "VICTORY BY BASE OCCUPATION");
      goOverlay.style.display = "flex";
    }
    autoUploadCompletedLocalMatch();
    return;
  }

  checkMatchTurn();
}

function checkMatchTurn() {
  if (!matchStarted || !activePlayState) return;
  var s = activePlayState;
  if (s.status === "O" || s.status === "R") return;

  if (localMatchConfig.playMode === "bot" && s.turn !== localMatchConfig.humanSeat) {
    triggerAiBotTurn();
  }
}

// AI Bot Search with 100% Opaque Thinking Progress Bar
function triggerAiBotTurn() {
  var s = activePlayState;
  var fen = serializeOctiFen(s);
  var diff = localMatchConfig.botDifficulty || 2;
  var side = s.turn;
  var mode = s.mode;

  var thinkOverlay = document.getElementById("web-thinking-overlay");
  var fillEl = document.getElementById("web-thinking-progress-fill");
  var txtEl = document.getElementById("web-thinking-progress-text");
  if (thinkOverlay) thinkOverlay.style.display = "flex";
  if (fillEl) fillEl.style.width = "10%";
  if (txtEl) txtEl.textContent = "Depth 1 • Searching root moves... • (10%)";

  function finishAiMove(res) {
    if (thinkOverlay) thinkOverlay.style.display = "none";
    if (!matchStarted || !activePlayState || activePlayState.status !== "P" || activePlayState.turn !== side) {
      return;
    }
    if (res && res.bestMove) {
      var nextState = applyMoveToState(cloneState(activePlayState), res.bestMove, side);
      var aiMeta = {
        score: res.score,
        depth: res.depth,
        nodes: res.nodes,
        elapsedMs: res.elapsedMs
      };
      onLocalMoveCommitted(res.bestMove, nextState, aiMeta);
    }
  }

  if (botWorker) {
    botWorker.onerror = function (err) {
      console.warn("botWorker error, falling back to main-thread botSearch:", err);
      var r = botSearch(fen, diff, mode, side);
      finishAiMove(r);
    };
    botWorker.onmessage = function (e) {
      if (!e.data) return;
      if (e.data.type === "progress") {
        var d = e.data;
        var maxD = d.maxDepth || 8;
        var depthPct = Math.round((d.depth / maxD) * 100);
        var timePct = (d.budgetMs > 0 && d.elapsedMs !== undefined)
          ? Math.round((d.elapsedMs / d.budgetMs) * 100)
          : 0;
        var pct = Math.min(99, Math.max(10, Math.max(depthPct, timePct)));
        var secStr = (d.elapsedMs !== undefined) ? (" • " + (d.elapsedMs / 1000).toFixed(1) + "s") : "";
        if (fillEl) fillEl.style.width = pct + "%";
        if (txtEl) txtEl.textContent = "Depth " + d.depth + "/" + maxD + " • " + (d.nodes || 0) + " nodes" + secStr + " • (" + pct + "%)";
      } else if (e.data.type === "done" || e.data.type === "result") {
        finishAiMove(e.data);
      }
    };
    botWorker.postMessage({ type: "search", fen: fen, depth: diff, mode: mode, side: side });
  } else {
    // Fallback search with live progress callback
    setTimeout(function () {
      var r = botSearch(fen, diff, mode, side, function (prog) {
        var maxD = prog.maxDepth || 8;
        var depthPct = Math.round((prog.depth / maxD) * 100);
        var timePct = (prog.budgetMs > 0 && prog.elapsedMs !== undefined)
          ? Math.round((prog.elapsedMs / prog.budgetMs) * 100)
          : 0;
        var pct = Math.min(99, Math.max(10, Math.max(depthPct, timePct)));
        var secStr = (prog.elapsedMs !== undefined) ? (" • " + (prog.elapsedMs / 1000).toFixed(1) + "s") : "";
        if (fillEl) fillEl.style.width = pct + "%";
        if (txtEl) txtEl.textContent = "Depth " + prog.depth + "/" + maxD + " • " + (prog.nodes || 0) + " nodes" + secStr + " • (" + pct + "%)";
      });
      finishAiMove(r);
    }, 50);
  }
}

// Undo Move
var btnUndo = document.getElementById("btn-web-undo");
if (btnUndo) {
  btnUndo.addEventListener("click", function () {
    if (moveComposer && (moveComposer.stagedNotation || moveComposer.isGuidedMoveActive)) {
      moveComposer.cancel();
      renderPlayBoardCanvas();
      return;
    }
    var goOverlay = document.getElementById("web-gameover-overlay");
    if (goOverlay) goOverlay.style.display = "none";
    var thinkOverlay = document.getElementById("web-thinking-overlay");
    if (thinkOverlay) thinkOverlay.style.display = "none";

    if (localMatchConfig.playMode === "bot") {
      while (playMatchPlies.length > 1) {
        playMatchPlies.pop();
        var topPl = playMatchPlies[playMatchPlies.length - 1];
        var topSt = parseOctiFen(topPl.fenAfter);
        if (topSt && topSt.turn === localMatchConfig.humanSeat) {
          activePlayState = topSt;
          break;
        }
      }
      if (playMatchPlies.length <= 1) {
        var firstPl = playMatchPlies[0];
        activePlayState = parseOctiFen(firstPl.fenAfter);
      }
      playVcrIndex = playMatchPlies.length - 1;
      var curPl = playMatchPlies[playVcrIndex];
      lastMoveHighlightSquares = curPl && playVcrIndex > 0 ? extractModifiedSquaresFromMove(curPl.move) : [];
      lastMoveSetAtMs = Date.now();
      if (moveComposer) moveComposer.cancel();
      updatePlaySidebar();
      renderPlayBoardCanvas();
      renderAllWebReserves();
      checkMatchTurn();
      return;
    }
    if (playMatchPlies.length <= 1) return;
    playMatchPlies.pop();
    var lastPl = playMatchPlies[playMatchPlies.length - 1];
    activePlayState = parseOctiFen(lastPl.fenAfter);
    playVcrIndex = playMatchPlies.length - 1;
    lastMoveHighlightSquares = lastPl && playVcrIndex > 0 ? extractModifiedSquaresFromMove(lastPl.move) : [];
    lastMoveSetAtMs = Date.now();
    if (moveComposer) moveComposer.cancel();
    updatePlaySidebar();
    renderPlayBoardCanvas();
    renderAllWebReserves();
  });
}

// Resign
var btnResign = document.getElementById("btn-web-resign");
if (btnResign) {
  btnResign.addEventListener("click", function () {
    if (!activePlayState || activePlayState.status === "O" || activePlayState.status === "R") return;
    if (!confirm("Are you sure you want to resign the game?")) return;
    var turn = activePlayState.turn;
    if (clockInterval) {
      clearInterval(clockInterval);
      clockInterval = null;
    }
    activePlayState.status = "R";
    activePlayState.turn = "-";
    if (activePlayState.mode === "4" || activePlayState.mode === "4P") {
      activePlayState.winner = (turn === "A" || turn === "C") ? "B" : "A";
    } else {
      activePlayState.winner = (turn === "A") ? "B" : "A";
    }
    activePlayState.termination = "RESIGN";
    OctiWebSoundSynth.play("GAME_VICTORY");

    var goOverlay = document.getElementById("web-gameover-overlay");
    var goTitle = document.getElementById("web-gameover-title");
    var goSub = document.getElementById("web-gameover-subtitle");
    if (goOverlay && goTitle && goSub) {
      if (activePlayState.mode === "4" || activePlayState.mode === "4P") {
        goTitle.textContent = (activePlayState.winner === "A" ? "TEAM A & C" : "TEAM B & D") + " VICTORIOUS!";
      } else {
        goTitle.textContent = "PLAYER " + activePlayState.winner + " WINS";
      }
      goSub.textContent = "VICTORY BY RESIGNATION";
      goOverlay.style.display = "flex";
    }

    updatePlaySidebar();
    renderPlayBoardCanvas();
    renderAllWebReserves();
    autoUploadCompletedLocalMatch();
  });
}

// Initialize MoveComposer
moveComposer = new WebMoveComposer(function () {
  return activePlayState;
}, onLocalMoveCommitted);

// Subtab Toggles
var subtabLocal = document.getElementById("subtab-play-local");
var subtabOnline = document.getElementById("subtab-play-online");
if (subtabLocal && subtabOnline) {
  subtabLocal.addEventListener("click", function () {
    subtabLocal.className = "btn btn-primary";
    subtabOnline.className = "btn";
    document.getElementById("play-local-panel").style.display = "block";
    document.getElementById("play-online-panel").style.display = "none";
  });
  subtabOnline.addEventListener("click", function () {
    subtabOnline.className = "btn btn-primary";
    subtabLocal.className = "btn";
    document.getElementById("play-online-panel").style.display = "block";
    document.getElementById("play-local-panel").style.display = "none";
    loadOnlineTables();
  });
}

// Online Table Board Play
function isMyOnlineTurn(state) {
  return true; // checked against table seats if authenticated
}

function openOnlineTableBoard(roomId) {
  if (!roomId) return;
  activeOnlineGameId = roomId;
  activeOnlineTableRoomId = roomId;
  subtabLocal.click();

  var bar = document.getElementById("web-play-online-status-bar");
  var titleEl = document.getElementById("web-play-online-table-title");
  if (bar && titleEl) {
    titleEl.textContent = "Online Table #" + roomId;
    bar.style.display = "flex";
  }

  // Fetch table state
  api("/api/v1/tables/" + encodeURIComponent(roomId)).then(function (res) {
    if (res && res.game) {
      activePlayState = parseOctiFen(res.game.current_fen || res.game.final_fen);
      matchStarted = (res.game.status === "IN_PROGRESS");
      updatePlaySidebar();
      renderPlayBoardCanvas();
      renderAllWebReserves();
    }
  }).catch(function () {});
}

// Wire initial start match summary on page load
prepareLocalMatchInitialState();


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


if ($("btn-web-open-my-table")) {
  $("btn-web-open-my-table").addEventListener("click", function () {
    if (currentMyTableRoomId) {
      openOnlineTableBoard(currentMyTableRoomId);
    }
  });
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
  var superProngs = $("web-create-super-prongs") ? !!$("web-create-super-prongs").checked : false;
  var edgeless = $("web-create-edgeless") ? !!$("web-create-edgeless").checked : false;
  if (mode === "OFK") {
    bases = 1;
    superProngs = false;
    edgeless = false;
  }
  if (mode === "4P" && bases === 4) bases = 1;
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

if ($("web-create-mode")) {
  $("web-create-mode").addEventListener("change", function () {
    var isOfk = $("web-create-mode").value === "OFK";
    if ($("web-create-bases")) {
      if (isOfk) $("web-create-bases").value = "1";
      $("web-create-bases").disabled = isOfk;
    }
    if ($("web-create-super-prongs")) {
      if (isOfk) $("web-create-super-prongs").checked = false;
      $("web-create-super-prongs").disabled = isOfk;
    }
    if ($("web-create-edgeless")) {
      if (isOfk) $("web-create-edgeless").checked = false;
      $("web-create-edgeless").disabled = isOfk;
    }
  });
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
      tr.addEventListener("click", function () { window.open(window.location.pathname + "#replay/" + encodeURIComponent(g.game_id), "_blank"); });
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
  location.hash = "#replay/" + encodeURIComponent(id);
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
          var sc = ply.ai.bestScore !== undefined ? ply.ai.bestScore : (ply.ai.score !== undefined ? ply.ai.score : (ply.ai.evalScore || 0));
          var nd = ply.ai.nodesVisited !== undefined ? ply.ai.nodesVisited : (ply.ai.nodes !== undefined ? ply.ai.nodes : (ply.ai.nodesEvaluated || 0));
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
  if (location.hash.indexOf("#replay/") === 0) {
    switchTab("archive");
  }
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

window.openZoomedSquare = openZoomedSquare;
window.closeZoomedSquare = closeZoomedSquare;
window.handlePlaySquareClick = handlePlaySquareClick;
window.prepareLocalMatchInitialState = prepareLocalMatchInitialState;
window.WebMoveComposer = WebMoveComposer;
window.moveComposer = moveComposer;
Object.defineProperty(window, "localMatchConfig", {
  get: function () { return localMatchConfig; },
  set: function (v) { localMatchConfig = v; }
});
Object.defineProperty(window, "playMatchPlies", {
  get: function () { return playMatchPlies; },
  set: function (v) { playMatchPlies = v; }
});
Object.defineProperty(window, "matchStarted", {
  get: function () { return matchStarted; },
  set: function (v) { matchStarted = v; }
});
Object.defineProperty(window, "zoomHoverDir", {
  get: function () { return zoomHoverDir; },
  set: function (v) { zoomHoverDir = v; }
});
Object.defineProperty(window, "activePlayState", {
  get: function () { return activePlayState; },
  set: function (v) { activePlayState = v; }
});
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
