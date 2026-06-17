#pragma once

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>Weight Irrigation Controller</title>
  <style>
    :root {
      --bg: #07120e;
      --panel: #0d1f18;
      --panel-2: #102820;
      --panel-3: #16352a;
      --text: #ecfff5;
      --muted: #94b8a7;
      --line: rgba(166, 255, 205, 0.12);
      --green: #38f28f;
      --green-2: #17c974;
      --blue: #54b7ff;
      --amber: #ffcb66;
      --red: #ff5d5d;
      --shadow: 0 18px 50px rgba(0, 0, 0, 0.38);
      --radius: 18px;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background:
        radial-gradient(circle at top left, rgba(56, 242, 143, 0.18), transparent 34rem),
        radial-gradient(circle at bottom right, rgba(84, 183, 255, 0.13), transparent 32rem),
        var(--bg);
      color: var(--text);
      min-height: 100vh;
    }

    .app {
      width: min(1180px, 100%);
      margin: 0 auto;
      padding: 20px;
    }

    .topbar {
      display: flex;
      justify-content: space-between;
      gap: 16px;
      align-items: center;
      margin-bottom: 18px;
    }

    .brand {
      display: flex;
      align-items: center;
      gap: 12px;
    }

    .logo {
      width: 46px;
      height: 46px;
      display: grid;
      place-items: center;
      border-radius: 14px;
      background: linear-gradient(135deg, rgba(56, 242, 143, 0.95), rgba(84, 183, 255, 0.88));
      color: #04100b;
      font-size: 24px;
      box-shadow: 0 14px 35px rgba(56, 242, 143, 0.18);
    }

    h1 {
      margin: 0;
      font-size: clamp(21px, 3vw, 32px);
      letter-spacing: -0.04em;
    }

    .sub {
      margin-top: 3px;
      color: var(--muted);
      font-size: 14px;
    }

    .status-pill {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 10px 13px;
      border: 1px solid var(--line);
      border-radius: 999px;
      background: rgba(13, 31, 24, 0.82);
      color: var(--muted);
      white-space: nowrap;
    }

    .dot {
      width: 9px;
      height: 9px;
      border-radius: 999px;
      background: var(--green);
      box-shadow: 0 0 16px var(--green);
    }

    .nav {
      display: flex;
      gap: 8px;
      overflow-x: auto;
      padding: 8px;
      margin-bottom: 18px;
      background: rgba(13, 31, 24, 0.62);
      border: 1px solid var(--line);
      border-radius: 18px;
      position: sticky;
      top: 8px;
      z-index: 10;
      backdrop-filter: blur(18px);
    }

    .tab {
      border: 0;
      color: var(--muted);
      background: transparent;
      padding: 11px 14px;
      border-radius: 13px;
      font-weight: 700;
      cursor: pointer;
      white-space: nowrap;
      text-decoration: none;
      display: inline-block;
      font-size: inherit;
      font-family: inherit;
      line-height: 1;
    }

    .tab.active {
      color: #06120d;
      background: var(--green);
    }

    .chart-range {
      border: 1px solid var(--line);
      color: var(--muted);
      background: rgba(255,255,255,0.05);
      padding: 8px 16px;
      border-radius: 10px;
      font-weight: 700;
      cursor: pointer;
      font-size: 13px;
    }
    .chart-range.active {
      color: #06120d;
      background: var(--green);
      border-color: var(--green);
    }

    .grid {
      display: grid;
      grid-template-columns: 1.15fr 0.85fr;
      gap: 18px;
    }

    .section { display: none; }
    .section.active { display: block; }

    .card {
      border: 1px solid var(--line);
      background: linear-gradient(180deg, rgba(16, 40, 32, 0.94), rgba(9, 23, 17, 0.94));
      border-radius: var(--radius);
      box-shadow: var(--shadow);
      padding: 18px;
      margin-bottom: 18px;
    }

    .hero {
      min-height: 330px;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      overflow: hidden;
      position: relative;
    }

    .hero::after {
      content: "";
      position: absolute;
      width: 270px;
      height: 270px;
      right: -90px;
      top: -70px;
      border-radius: 999px;
      background: radial-gradient(circle, rgba(56, 242, 143, 0.23), transparent 65%);
      pointer-events: none;
    }

    .label {
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: 0.12em;
      font-size: 12px;
      font-weight: 800;
    }

    .weight {
      margin: 10px 0 0;
      font-size: clamp(58px, 10vw, 108px);
      letter-spacing: -0.08em;
      line-height: 0.92;
    }

    .unit {
      font-size: 28px;
      letter-spacing: -0.03em;
      color: var(--muted);
      margin-left: 8px;
    }

    .mini-grid {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 10px;
      margin-top: 18px;
    }

    .metric {
      padding: 13px;
      border-radius: 15px;
      background: rgba(255, 255, 255, 0.045);
      border: 1px solid var(--line);
    }

    .metric b {
      display: block;
      margin-top: 7px;
      font-size: 20px;
    }

    .pump-card {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 14px;
    }

    .pump-state {
      font-size: 34px;
      font-weight: 900;
      letter-spacing: -0.05em;
    }

    .pump-state.off { color: var(--muted); }
    .pump-state.on { color: var(--green); text-shadow: 0 0 20px rgba(56, 242, 143, 0.35); }

    .btn-row {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
    }

    button, .btn {
      border: 0;
      border-radius: 14px;
      padding: 13px 15px;
      font-weight: 900;
      cursor: pointer;
      color: #06120d;
      background: var(--green);
      box-shadow: 0 12px 28px rgba(56, 242, 143, 0.13);
    }

    button.secondary {
      color: var(--text);
      background: rgba(255, 255, 255, 0.075);
      border: 1px solid var(--line);
      box-shadow: none;
    }

    button.danger {
      background: var(--red);
      color: #210404;
    }

    button.warn {
      background: var(--amber);
      color: #201300;
    }

    .alert {
      display: none;
      padding: 14px 15px;
      border-radius: 15px;
      margin-bottom: 18px;
      font-weight: 850;
      border: 1px solid transparent;
    }

    .alert.show { display: block; }
    .alert.error { background: rgba(255, 93, 93, 0.13); border-color: rgba(255, 93, 93, 0.45); color: #ffdada; }
    .alert.warn { background: rgba(255, 203, 102, 0.13); border-color: rgba(255, 203, 102, 0.45); color: #ffe9ba; }

    .form-grid {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 14px;
    }

    .field {
      display: flex;
      flex-direction: column;
      gap: 7px;
    }

    .field label {
      font-size: 13px;
      color: var(--muted);
      font-weight: 800;
    }

    input, select {
      width: 100%;
      border: 1px solid var(--line);
      color: var(--text);
      background: rgba(255, 255, 255, 0.065);
      padding: 13px 12px;
      border-radius: 13px;
      outline: none;
      font-size: 16px;
    }

    input:focus, select:focus {
      border-color: rgba(56, 242, 143, 0.7);
      box-shadow: 0 0 0 4px rgba(56, 242, 143, 0.09);
    }

    .list {
      display: grid;
      gap: 10px;
    }

    .log-item, .safety-item {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      padding: 13px;
      border: 1px solid var(--line);
      background: rgba(255, 255, 255, 0.045);
      border-radius: 14px;
      color: var(--muted);
    }

    .ok { color: var(--green); font-weight: 900; }
    .bad { color: var(--red); font-weight: 900; }
    .muted { color: var(--muted); }

    .footer-note {
      color: var(--muted);
      font-size: 13px;
      line-height: 1.5;
      margin-top: 20px;
      padding-bottom: 28px;
    }

    @media (max-width: 820px) {
      .app { padding: 14px; }
      .topbar { align-items: flex-start; flex-direction: column; }
      .grid { grid-template-columns: 1fr; }
      .mini-grid { grid-template-columns: 1fr; }
      .form-grid { grid-template-columns: 1fr; }
      .pump-card { align-items: flex-start; flex-direction: column; }
      .nav { bottom: 8px; top: auto; position: sticky; }
    }
  </style>
</head>
<body>
  <div class="app">
    <header class="topbar">
      <div class="brand">
        <div class="logo">💧</div>
        <div>
          <h1>Weight Irrigation</h1>
          <div class="sub" id="deviceName">ESP32 HX711 Controller</div>
        </div>
      </div>
      <div class="status-pill"><span class="dot" id="connDot"></span><span id="wifiText">Connecting...</span></div>
    </header>

    <nav class="nav">
      <button class="tab active" data-tab="status">Status</button>
      <button class="tab" data-tab="calibration">Calibration</button>
      <button class="tab" data-tab="settings">Settings</button>
      <button class="tab" data-tab="manual">Manual</button>
      <button class="tab" data-tab="logs">Logs</button>
      <a class="tab" href="/wifi">Wi-Fi</a>
      <a class="tab" href="/firmware">Firmware</a>
      <button class="tab" data-tab="chart">Chart</button>
    </nav>

    <div id="alert" class="alert"></div>

    <section id="status" class="section active">
      <div class="grid">
        <div>
          <div class="card hero">
            <div>
              <div class="label">Current stable weight</div>
              <div class="weight"><span id="weightGrams">--</span><span class="unit">g</span></div>
            </div>
            <div class="mini-grid">
              <div class="metric"><span class="muted">Trigger</span><b id="triggerWeight">--g</b></div>
              <div class="metric"><span class="muted">Stop</span><b id="stopWeight">--g</b></div>
              <div class="metric"><span class="muted">State</span><b id="state">--</b></div>
            </div>
          </div>

          <div class="card pump-card">
            <div>
              <div class="label">Pump output</div>
              <div class="pump-state off" id="pumpState">OFF</div>
              <div class="muted">Last irrigation: <span id="lastIrrigation">--</span></div>
            </div>
            <div class="btn-row">
              <button onclick="apiPost('/api/pump/run',{seconds:10})">Run 10s</button>
              <button class="secondary" onclick="apiPost('/api/pump/off')">Pump Off</button>
              <button class="danger" onclick="apiPost('/api/emergency-stop')">Emergency Stop</button>
            </div>
          </div>
        </div>

        <aside>
          <div class="card">
            <div class="label">Safety inputs</div>
            <div class="list" style="margin-top:12px">
              <div class="safety-item"><span>HX711 sensor</span><span id="sensorValid" class="ok">--</span></div>
              <div class="safety-item"><span>Reading stable</span><span id="stable" class="ok">--</span></div>
              <div class="safety-item"><span>Tank empty</span><span id="tankEmpty" class="ok">--</span></div>
              <div class="safety-item"><span>Leak detected</span><span id="leakDetected" class="ok">--</span></div>
            </div>
          </div>
          <div class="card">
            <div class="label">System</div>
            <div class="mini-grid" style="grid-template-columns:1fr 1fr">
              <div class="metric"><span class="muted">Raw</span><b id="rawReading">--</b></div>
              <div class="metric"><span class="muted">Uptime</span><b id="uptime">--</b></div>
            </div>
          </div>
        </aside>
      </div>
    </section>

    <section id="calibration" class="section">
      <div class="card">
        <div class="label">Scale calibration</div>
        <h2>Tare and known weight</h2>
        <p class="muted">Tare with the empty platform/pot. Then add a known weight and calibrate.</p>
        <div class="form-grid">
          <div class="field"><label>Known weight in grams</label><input id="knownWeight" type="number" value="1000"></div>
          <div class="field"><label>Current raw reading</label><input id="calRaw" readonly value="--"></div>
        </div>
        <div class="btn-row" style="margin-top:16px">
          <button onclick="apiPost('/api/calibration/tare')">Tare / Zero</button>
          <button class="secondary" onclick="calibrateKnown()">Calibrate Known Weight</button>
          <button class="warn" onclick="apiPost('/api/calibration/reset')">Reset Calibration</button>
        </div>
      </div>
    </section>

    <section id="settings" class="section">
      <div class="card">
        <div class="label">Irrigation settings</div>
        <h2>Thresholds and safety limits</h2>
        <div class="form-grid">
          <div class="field"><label>Trigger mode</label><select id="triggerMode"><option value="0">Absolute weight</option><option value="1">Dry-back %</option></select></div>
          <div class="field"><label>Device name</label><input id="setDeviceName" value="ESP32 Weight Irrigation"></div>
          <div class="field"><label>Start below weight (g)</label><input id="setTriggerWeight" type="number" value="4300"></div>
          <div class="field"><label>Stop at weight (g)</label><input id="setStopWeight" type="number" value="4800"></div>
          <div class="field"><label>Fully wet weight (g)</label><input id="fullyWetWeight" type="number" value="5000"></div>
          <div class="field"><label>Trigger dry-back (%)</label><input id="triggerDryBack" type="number" value="10"></div>
          <div class="field"><label>Stop dry-back (%)</label><input id="stopDryBack" type="number" value="2"></div>
          <div class="field"><label>Max pump runtime (sec)</label><input id="maxRuntime" type="number" value="60"></div>
          <div class="field"><label>Min gap (min)</label><input id="minGap" type="number" value="20"></div>
          <div class="field"><label>Stable duration (sec)</label><input id="stableDuration" type="number" value="30"></div>
          <div class="field"><label>Filter sample count</label><input id="setFilterSamples" type="number" value="10" min="1" max="50"></div>
        </div>
        <div style="margin-top:16px;display:grid;gap:12px">
          <label style="display:flex;align-items:center;gap:10px"><input id="setRelayActiveHigh" type="checkbox"> Relay active HIGH</label>
          <label style="display:flex;align-items:center;gap:10px"><input id="setTankSensorEnabled" type="checkbox"> Tank sensor enabled</label>
          <label style="display:flex;align-items:center;gap:10px"><input id="setLeakSensorEnabled" type="checkbox"> Leak sensor enabled</label>
          <label style="display:flex;align-items:center;gap:10px"><input id="setStopOnWifiLoss" type="checkbox"> Stop pump on Wi-Fi loss</label>
        </div>
        <div class="btn-row" style="margin-top:16px">
          <button onclick="saveSettings()">Save Settings</button>
          <button class="secondary" onclick="loadSettings()">Reload</button>
        </div>
      </div>
    </section>

    <section id="manual" class="section">
      <div class="card">
        <div class="label">Manual control</div>
        <h2>Pump control</h2>
        <p class="muted">Manual actions should still respect safety limits: tank empty, leak detected, max runtime and emergency stop.</p>
        <div class="form-grid">
          <div class="field"><label>Timed run seconds</label><input id="manualSeconds" type="number" value="10"></div>
        </div>
        <div class="btn-row" style="margin-top:16px">
          <button onclick="apiPost('/api/pump/on')">Pump On</button>
          <button class="secondary" onclick="apiPost('/api/pump/off')">Pump Off</button>
          <button class="warn" onclick="runManual()">Run Timed</button>
          <button class="danger" onclick="apiPost('/api/emergency-stop')">Emergency Stop</button>
          <button class="secondary" onclick="apiPost('/api/clear-error')">Clear Error</button>
        </div>
      </div>
    </section>

    <section id="logs" class="section">
      <div class="card">
        <div class="label">Event log</div>
        <h2>Recent events</h2>
        <div class="list" id="logList"><div class="log-item"><span>Loading...</span><span></span></div></div>
      </div>
    </section>

    <section id="chart" class="section">
      <div class="card">
        <div class="label">Weight history</div>
        <h2>Weight over time</h2>
        <p class="muted">Readings are sampled every minute. Data stored for 7 days and persisted across reboots.</p>
        <div style="display:flex;flex-wrap:wrap;gap:8px;margin:12px 0">
          <button class="chart-range active" data-range="6">6h</button>
          <button class="chart-range" data-range="12">12h</button>
          <button class="chart-range" data-range="24">24h</button>
          <button class="chart-range" data-range="48">48h</button>
          <button class="chart-range" data-range="168">7d</button>
        </div>
        <div style="position:relative;width:100%">
          <canvas id="weightChart" style="width:100%;height:320px;display:block"></canvas>
        </div>
        <div id="chartSummary" class="muted" style="margin-top:8px"></div>
      </div>
    </section>

    <div class="footer-note">
      Safety note: wire the pump relay normally-open where possible. Software is not a substitute for a float switch, leak sensor, fuse, manual cutoff, and max-runtime limit.
    </div>
  </div>

  <script>
    const $ = id => document.getElementById(id);
    const api = path => fetch(path).then(r => r.json());
    const esc = s => String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;').replace(/'/g,'&#39;');

    document.querySelectorAll('button.tab').forEach(tab => {
      tab.addEventListener('click', () => {
        document.querySelectorAll('button.tab').forEach(t => t.classList.remove('active'));
        document.querySelectorAll('.section').forEach(s => s.classList.remove('active'));
        tab.classList.add('active');
        $(tab.dataset.tab).classList.add('active');
        if (tab.dataset.tab === 'logs') loadLogs();
        if (tab.dataset.tab === 'settings') loadSettings();
        if (tab.dataset.tab === 'chart') loadChart();
      });
    });

    function showAlert(message, type = 'warn') {
      const a = $('alert');
      if (!message) { a.className = 'alert'; a.textContent = ''; return; }
      a.className = 'alert show ' + type;
      a.textContent = message;
    }

    async function apiPost(path, body = {}) {
      try {
        const res = await fetch(path, {
          method: 'POST',
          headers: {'Content-Type':'application/json'},
          body: JSON.stringify(body)
        });
        const txt = await res.text();
        if (!res.ok) throw new Error(txt || 'Request failed');
        showAlert('Command sent: ' + path, 'warn');
        setTimeout(showAlert, 1800);
        refreshStatus();
      } catch (e) {
        showAlert(e.message, 'error');
      }
    }

    function fmtUptime(sec) {
      sec = Number(sec || 0);
      const h = Math.floor(sec / 3600);
      const m = Math.floor((sec % 3600) / 60);
      return h ? `${h}h ${m}m` : `${m}m`;
    }

    async function refreshStatus() {
      try {
        const s = await api('/api/status');
        $('deviceName').textContent = s.deviceName || 'ESP32 HX711 Controller';
        $('wifiText').textContent = s.wifiConnected ? 'Wi-Fi connected' : 'Wi-Fi disconnected';
        $('connDot').style.background = s.wifiConnected ? 'var(--green)' : 'var(--red)';
        $('weightGrams').textContent = Number(s.weightGrams || 0).toFixed(1);
        $('rawReading').textContent = s.rawReading ?? '--';
        $('calRaw').value = s.rawReading ?? '--';
        $('state').textContent = s.state || '--';
        $('triggerWeight').textContent = `${s.triggerWeight ?? '--'}g`;
        $('stopWeight').textContent = `${s.stopWeight ?? '--'}g`;
        $('lastIrrigation').textContent = s.lastIrrigationDurationSec ? `${s.lastIrrigationDurationSec}s` : '--';
        $('uptime').textContent = fmtUptime(s.uptimeSec);
        $('pumpState').textContent = s.pumpOn ? 'ON' : 'OFF';
        $('pumpState').className = 'pump-state ' + (s.pumpOn ? 'on' : 'off');
        setBool('sensorValid', s.sensorValid !== false, 'OK', 'ERROR');
        setBool('stable', !!s.stable, 'YES', 'NO');
        setBool('tankEmpty', !s.tankEmpty, 'OK', 'EMPTY');
        setBool('leakDetected', !s.leakDetected, 'OK', 'LEAK');

        const state = String(s.state || '');
        if (['SENSOR_ERROR','LEAK_DETECTED','TANK_EMPTY','EMERGENCY_STOP','CONFIG_ERROR'].includes(state)) {
          showAlert('Safety state active: ' + state, 'error');
        }
      } catch (e) {
        $('wifiText').textContent = 'Dashboard offline';
        showAlert('Could not read /api/status', 'error');
      }
    }

    function setBool(id, good, yes, no) {
      const el = $(id);
      el.textContent = good ? yes : no;
      el.className = good ? 'ok' : 'bad';
    }

    async function loadSettings() {
      try {
        const s = await api('/api/settings');
        $('triggerMode').value = String(s.triggerMode ?? '0');
        $('setDeviceName').value = s.deviceName || '';
        $('setTriggerWeight').value = s.triggerWeightG ?? '';
        $('setStopWeight').value = s.stopWeightG ?? '';
        $('fullyWetWeight').value = s.fullyWetWeightG ?? '';
        $('triggerDryBack').value = s.dryBackTriggerPercent ?? '';
        $('stopDryBack').value = s.dryBackStopPercent ?? '';
        $('maxRuntime').value = s.maxRuntimeSec ?? '';
        $('minGap').value = s.minGapMin ?? '';
        $('stableDuration').value = s.stableDurationSec ?? '';
        $('setFilterSamples').value = s.filterSamples ?? '';
        $('setRelayActiveHigh').checked = !!s.relayActiveHigh;
        $('setTankSensorEnabled').checked = !!s.tankSensorEnabled;
        $('setLeakSensorEnabled').checked = !!s.leakSensorEnabled;
        $('setStopOnWifiLoss').checked = !!s.stopOnWifiLoss;
      } catch (e) {
        showAlert('Could not load settings', 'error');
      }
    }

    function saveSettings() {
      apiPost('/api/settings', {
        triggerMode: Number($('triggerMode').value),
        deviceName: $('setDeviceName').value,
        triggerWeightG: Number($('setTriggerWeight').value),
        stopWeightG: Number($('setStopWeight').value),
        fullyWetWeightG: Number($('fullyWetWeight').value),
        dryBackTriggerPercent: Number($('triggerDryBack').value),
        dryBackStopPercent: Number($('stopDryBack').value),
        maxRuntimeSec: Number($('maxRuntime').value),
        minGapMin: Number($('minGap').value),
        stableDurationSec: Number($('stableDuration').value),
        filterSamples: Number($('setFilterSamples').value),
        relayActiveHigh: $('setRelayActiveHigh').checked,
        tankSensorEnabled: $('setTankSensorEnabled').checked,
        leakSensorEnabled: $('setLeakSensorEnabled').checked,
        stopOnWifiLoss: $('setStopOnWifiLoss').checked
      });
    }

    function calibrateKnown() {
      apiPost('/api/calibration/known-weight', { knownWeightG: Number($('knownWeight').value) });
    }

    function runManual() {
      apiPost('/api/pump/run', { seconds: Number($('manualSeconds').value) });
    }

    async function loadLogs() {
      try {
        const data = await api('/api/logs');
        const logs = Array.isArray(data) ? data : (data.logs || []);
        $('logList').innerHTML = logs.slice(-50).reverse().map(l => {
          const msg = [l.message, l.event, l.type, 'Event'].find(v => v) || 'Event';
          const ts = l.uptimeSec != null ? l.uptimeSec : (l.time || '');
          return `<div class="log-item"><span>${esc(msg)}</span><span>${esc(String(ts))}</span></div>`;
        }).join('') || '<div class="log-item"><span>No logs yet</span><span></span></div>';
      } catch (e) {
        $('logList').innerHTML = '<div class="log-item"><span>Could not load logs</span><span></span></div>';
      }
    }

    function drawChart(canvas, points) {
      const dpr = window.devicePixelRatio || 1;
      const rect = canvas.getBoundingClientRect();
      canvas.width = rect.width * dpr;
      canvas.height = rect.height * dpr;
      const ctx = canvas.getContext('2d');
      ctx.scale(dpr, dpr);

      const W = rect.width, H = rect.height;
      const pad = { top: 18, right: 16, bottom: 42, left: 62 };
      const plotW = W - pad.left - pad.right;
      const plotH = H - pad.top - pad.bottom;

      ctx.clearRect(0, 0, W, H);

      if (!points || points.length < 2) {
        ctx.fillStyle = '#94b8a7';
        ctx.font = '15px system-ui';
        ctx.textAlign = 'center';
        ctx.fillText(points && points.length === 1 ? 'Only 1 data point \u2014 need more' : 'No data yet', W / 2, H / 2);
        return;
      }

      let yMin = Infinity, yMax = -Infinity;
      for (const p of points) {
        if (p.weightG < yMin) yMin = p.weightG;
        if (p.weightG > yMax) yMax = p.weightG;
      }
      const yPad = (yMax - yMin) * 0.12 || 20;
      yMin -= yPad; yMax += yPad;

      const t0 = points[0].epoch;
      const t1 = points[points.length - 1].epoch;
      const tRange = (t1 - t0) || 3600;
      const showDates = tRange > 86400;  // > 24h, show day labels

      const x = t => pad.left + (t - t0) / tRange * plotW;
      const y = v => pad.top + plotH - (v - yMin) / (yMax - yMin) * plotH;

      // Grid lines + Y labels
      ctx.strokeStyle = 'rgba(166,255,205,0.08)';
      ctx.lineWidth = 1;
      ctx.fillStyle = '#94b8a7';
      ctx.font = '11px system-ui';
      ctx.textAlign = 'right';
      for (let i = 0; i <= 5; i++) {
        const val = yMin + (yMax - yMin) * i / 5;
        const yy = y(val);
        ctx.beginPath(); ctx.moveTo(pad.left, yy); ctx.lineTo(W - pad.right, yy); ctx.stroke();
        ctx.fillText(val.toFixed(0), pad.left - 8, yy + 4);
      }

      // X labels
      ctx.textAlign = 'center';
      const xSteps = Math.min(6, points.length);
      const MONTHS = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
      for (let i = 0; i <= xSteps; i++) {
        const idx = Math.round(i / xSteps * (points.length - 1));
        const pt = points[idx];
        const d = new Date(pt.epoch * 1000);
        let label;
        if (showDates) {
          label = d.getDate() + ' ' + MONTHS[d.getMonth()] + ' ' +
            String(d.getHours()).padStart(2,'0') + ':' + String(d.getMinutes()).padStart(2,'0');
        } else {
          const relSec = pt.epoch - t0;
          const hh = Math.floor(relSec / 3600);
          const mm = Math.floor((relSec % 3600) / 60);
          label = hh + 'h' + String(mm).padStart(2, '0') + 'm';
        }
        ctx.fillText(label, x(pt.epoch), H - pad.bottom + 17);
      }

      // Fill area
      ctx.beginPath();
      ctx.moveTo(x(t0), y(yMin));
      for (const pt of points) ctx.lineTo(x(pt.epoch), y(pt.weightG));
      ctx.lineTo(x(t1), y(yMin));
      ctx.closePath();
      const grad = ctx.createLinearGradient(0, pad.top, 0, H - pad.bottom);
      grad.addColorStop(0, 'rgba(56,242,143,0.20)');
      grad.addColorStop(1, 'rgba(56,242,143,0.02)');
      ctx.fillStyle = grad;
      ctx.fill();

      // Line
      ctx.strokeStyle = '#38f28f';
      ctx.lineWidth = 2.5;
      ctx.lineJoin = 'round';
      ctx.lineCap = 'round';
      ctx.beginPath();
      for (const pt of points) ctx.lineTo(x(pt.epoch), y(pt.weightG));
      ctx.stroke();

      // Dots (skip if too many)
      if (points.length <= 60) {
        for (const pt of points) {
          ctx.beginPath();
          ctx.arc(x(pt.epoch), y(pt.weightG), 3.5, 0, Math.PI * 2);
          ctx.fillStyle = '#06120d';
          ctx.fill();
          ctx.beginPath();
          ctx.arc(x(pt.epoch), y(pt.weightG), 2, 0, Math.PI * 2);
          ctx.fillStyle = '#38f28f';
          ctx.fill();
        }
      }
    }

    let _chartRangeHours = 6;

    async function loadChart() {
      try {
        const data = await api('/api/weight-history?range=' + _chartRangeHours);
        const points = data.points || [];
        await new Promise(function(r) { requestAnimationFrame(r); });
        drawChart($('weightChart'), points);
        $('chartSummary').textContent = points.length
          ? points.length + ' data points over ' + (_chartRangeHours <= 48 ? _chartRangeHours + 'h' : '7d')
          : 'No data yet \u2014 readings are recorded every minute';
      } catch (e) {
        drawChart($('weightChart'), []);
        $('chartSummary').textContent = 'Could not load chart data';
      }
    }

    document.querySelectorAll('.chart-range').forEach(btn => {
      btn.addEventListener('click', function() {
        document.querySelectorAll('.chart-range').forEach(b => b.classList.remove('active'));
        this.classList.add('active');
        _chartRangeHours = Number(this.dataset.range);
        loadChart();
      });
    });

    refreshStatus();
    setInterval(refreshStatus, 2000);
  </script>
</body>
</html>

)rawliteral";
