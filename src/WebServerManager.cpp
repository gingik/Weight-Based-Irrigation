#include "WebServerManager.h"
#include "config.h"
#include <WiFi.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Weight Irrigation</title>
<style>
body{font-family:Arial,sans-serif;background:#111;color:#eee;margin:0;padding:16px}h1{font-size:1.4rem}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(210px,1fr));gap:12px}.card{background:#1d1d1d;border:1px solid #333;border-radius:12px;padding:14px}.value{font-size:1.8rem;font-weight:bold}.ok{color:#65d46e}.bad{color:#ff6464}.warn{color:#ffd166}label{display:block;margin-top:10px}input,select,button{width:100%;padding:9px;border-radius:8px;border:1px solid #444;background:#222;color:#eee;box-sizing:border-box}button{cursor:pointer;margin-top:8px}.row{display:flex;gap:8px}.row button{flex:1}.danger{background:#6b1c1c}.good{background:#145c2a}.log{font-family:monospace;font-size:.85rem;white-space:pre-wrap}.small{font-size:.8rem;color:#aaa}</style>
</head><body>
<h1 id="title">ESP32 Weight Irrigation</h1>
<div id="alert"></div>
<div class="grid">
 <div class="card"><div>Current Weight</div><div class="value" id="weight">--</div><div class="small">Raw: <span id="raw">--</span> | Stable: <span id="stable">--</span></div></div>
 <div class="card"><div>System</div><div class="value" id="state">--</div><div>Pump: <b id="pump">--</b></div></div>
 <div class="card"><div>Thresholds</div><div>Trigger: <b id="trigger">--</b> g</div><div>Stop: <b id="stop">--</b> g</div></div>
 <div class="card"><div>Safety</div><div>Tank empty: <b id="tank">--</b></div><div>Leak: <b id="leak">--</b></div><div>Wi-Fi: <b id="wifi">--</b></div></div>
</div>

<div class="grid" style="margin-top:12px">
 <div class="card"><h2>Manual Control</h2><div class="row"><button class="good" onclick="post('/api/pump/on')">Pump ON</button><button onclick="post('/api/pump/off')">Pump OFF</button></div><label>Run seconds<input id="runSec" type="number" value="10"></label><button onclick="post('/api/pump/run',{seconds:+runSec.value})">Run Timed</button><button class="danger" onclick="post('/api/emergency-stop')">Emergency Stop</button><button onclick="post('/api/clear-error')">Clear Error / E-Stop</button></div>
 <div class="card"><h2>Calibration</h2><button onclick="post('/api/calibration/tare')">Tare / Zero</button><label>Known weight grams<input id="knownWeight" type="number" value="1000"></label><button onclick="post('/api/calibration/known-weight',{knownWeightG:+knownWeight.value})">Calibrate Known Weight</button><button onclick="post('/api/calibration/reset')">Reset Calibration</button></div>
 <div class="card"><h2>Settings</h2><form id="settingsForm" onsubmit="saveSettings(event)">
  <label>Device name<input name="deviceName"></label><label>Trigger mode<select name="triggerMode"><option value="0">Absolute</option><option value="1">Dry-back %</option></select></label>
  <label>Trigger weight g<input name="triggerWeightG" type="number" step="0.1"></label><label>Stop weight g<input name="stopWeightG" type="number" step="0.1"></label>
  <label>Fully wet weight g<input name="fullyWetWeightG" type="number" step="0.1"></label><label>Trigger dry-back %<input name="dryBackTriggerPercent" type="number" step="0.1"></label><label>Stop dry-back %<input name="dryBackStopPercent" type="number" step="0.1"></label>
  <label>Max runtime sec<input name="maxRuntimeSec" type="number"></label><label>Min gap min<input name="minGapMin" type="number"></label><label>Stable duration sec<input name="stableDurationSec" type="number"></label><label>Filter samples<input name="filterSamples" type="number"></label>
  <label><input name="relayActiveHigh" type="checkbox"> Relay active HIGH</label><label><input name="tankSensorEnabled" type="checkbox"> Tank sensor enabled</label><label><input name="leakSensorEnabled" type="checkbox"> Leak sensor enabled</label><label><input name="stopOnWifiLoss" type="checkbox"> Stop pump on Wi-Fi loss</label>
  <button class="good" type="submit">Save Settings</button></form></div>
 <div class="card"><h2>Logs</h2><div class="log" id="logs">Loading...</div></div>
</div>
<script>
async function api(u,o){let r=await fetch(u,o);let t=await r.text();try{return JSON.parse(t)}catch(e){return {ok:r.ok,text:t}}}
async function post(u,b={}){let r=await api(u,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)}); alert(JSON.stringify(r)); refresh();}
function setText(id,v){document.getElementById(id).textContent=v}
async function refresh(){let s=await api('/api/status'); setText('title',s.deviceName); setText('weight',Number(s.weightGrams).toFixed(1)+' g'); setText('raw',s.rawReading); setText('stable',s.stable?'YES':'NO'); setText('state',s.state); setText('pump',s.pumpOn?'ON':'OFF'); setText('trigger',Number(s.triggerWeight).toFixed(1)); setText('stop',Number(s.stopWeight).toFixed(1)); setText('tank',s.tankEmpty?'YES':'NO'); setText('leak',s.leakDetected?'YES':'NO'); setText('wifi',s.wifiConnected?'OK':'DOWN'); document.getElementById('alert').innerHTML=(s.state.includes('ERROR')||s.state.includes('LEAK')||s.state.includes('TANK')||s.state.includes('EMERGENCY'))?'<div class="card bad"><b>'+s.state+'</b></div>':''; let l=await api('/api/logs'); document.getElementById('logs').textContent=l.logs.map(x=>x.uptimeSec+'s '+x.type+' - '+x.message).join('\n');}
async function loadSettings(){let c=await api('/api/settings'); let f=document.getElementById('settingsForm'); for(let k in c){ if(f[k]){ if(f[k].type==='checkbox')f[k].checked=!!c[k]; else f[k].value=c[k]; } }}
async function saveSettings(e){e.preventDefault();let f=e.target;let b={}; for(let el of f.elements){ if(!el.name)continue; b[el.name]=el.type==='checkbox'?el.checked:(el.type==='number'?+el.value:el.value);} let r=await api('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(b)}); alert(JSON.stringify(r)); refresh();}
loadSettings(); refresh(); setInterval(refresh,2000);
</script></body></html>
)HTML";

void WebServerManager::begin(ConfigManager *cfg, HX711Manager *h, PumpManager *p, IrrigationController *ic, LogManager *lg) {
  config = cfg; hx = h; pump = p; controller = ic; log = lg; setupRoutes(); server.begin(); log->add("WEB", "HTTP server started");
}
void WebServerManager::handle(){ server.handleClient(); }
void WebServerManager::setupRoutes(){
  server.on("/", HTTP_GET, [this](){ handleRoot(); });
  server.on("/api/status", HTTP_GET, [this](){ sendJsonStatus(); });
  server.on("/api/settings", HTTP_GET, [this](){ sendJsonSettings(); });
  server.on("/api/settings", HTTP_POST, [this](){ handleSaveSettings(); });
  server.on("/api/calibration/tare", HTTP_POST, [this](){ handleTare(); });
  server.on("/api/calibration/known-weight", HTTP_POST, [this](){ handleKnownWeight(); });
  server.on("/api/calibration/reset", HTTP_POST, [this](){ handleResetCalibration(); });
  server.on("/api/pump/on", HTTP_POST, [this](){ handlePumpOn(); });
  server.on("/api/pump/run", HTTP_POST, [this](){ handlePumpRun(); });
  server.on("/api/pump/off", HTTP_POST, [this](){ handlePumpOff(); });
  server.on("/api/emergency-stop", HTTP_POST, [this](){ handleEmergencyStop(); });
  server.on("/api/clear-error", HTTP_POST, [this](){ handleClearError(); });
  server.on("/api/logs", HTTP_GET, [this](){ handleLogs(); });
}
void WebServerManager::handleRoot(){ server.send_P(200, "text/html", INDEX_HTML); }

void WebServerManager::sendJsonStatus(){
  JsonDocument doc;
  doc["deviceName"] = config->get().deviceName; doc["firmwareVersion"] = FIRMWARE_VERSION; doc["state"] = controller->stateName(); doc["weightGrams"] = hx->weightG(); doc["rawReading"] = hx->raw(); doc["stable"] = hx->isStable(); doc["sensorValid"] = hx->isValid(); doc["sensorError"] = hx->error(); doc["pumpOn"] = pump->isOn(); doc["triggerWeight"] = controller->triggerWeight(); doc["stopWeight"] = controller->stopWeight(); doc["lastIrrigationDurationSec"] = pump->lastDurationSec(); doc["tankEmpty"] = controller->tankEmpty(); doc["leakDetected"] = controller->leakDetected(); doc["wifiConnected"] = WiFi.status() == WL_CONNECTED; doc["ip"] = WiFi.localIP().toString(); doc["uptimeSec"] = millis()/1000UL;
  String out; serializeJson(doc,out); server.send(200,"application/json",out);
}
void WebServerManager::sendJsonSettings(){
  const AppConfig &c=config->get(); JsonDocument doc; doc["deviceName"]=c.deviceName; doc["triggerMode"]=(uint8_t)c.triggerMode; doc["calibrationFactor"]=c.calibrationFactor; doc["tareOffset"]=c.tareOffset; doc["triggerWeightG"]=c.triggerWeightG; doc["stopWeightG"]=c.stopWeightG; doc["fullyWetWeightG"]=c.fullyWetWeightG; doc["dryBackTriggerPercent"]=c.dryBackTriggerPercent; doc["dryBackStopPercent"]=c.dryBackStopPercent; doc["maxRuntimeSec"]=c.maxRuntimeSec; doc["minGapMin"]=c.minGapMin; doc["stableDurationSec"]=c.stableDurationSec; doc["sampleIntervalMs"]=c.sampleIntervalMs; doc["filterSamples"]=c.filterSamples; doc["relayActiveHigh"]=c.relayActiveHigh; doc["tankSensorEnabled"]=c.tankSensorEnabled; doc["leakSensorEnabled"]=c.leakSensorEnabled; doc["stopOnWifiLoss"]=c.stopOnWifiLoss; String out; serializeJson(doc,out); server.send(200,"application/json",out);
}

void WebServerManager::handleSaveSettings(){
  JsonDocument doc; DeserializationError e = deserializeJson(doc, server.arg("plain")); if(e){server.send(400,"application/json","{\"ok\":false,\"error\":\"Bad JSON\"}"); return;} AppConfig old=config->get(); AppConfig &c=config->get();
  if(doc["deviceName"].is<const char*>()) strlcpy(c.deviceName, doc["deviceName"], sizeof(c.deviceName));
  if(doc["triggerMode"].is<int>()) c.triggerMode=(TriggerMode)((int)doc["triggerMode"]);
  if(doc["triggerWeightG"].is<float>()) c.triggerWeightG=doc["triggerWeightG"];
  if(doc["stopWeightG"].is<float>()) c.stopWeightG=doc["stopWeightG"];
  if(doc["fullyWetWeightG"].is<float>()) c.fullyWetWeightG=doc["fullyWetWeightG"];
  if(doc["dryBackTriggerPercent"].is<float>()) c.dryBackTriggerPercent=doc["dryBackTriggerPercent"];
  if(doc["dryBackStopPercent"].is<float>()) c.dryBackStopPercent=doc["dryBackStopPercent"];
  if(doc["maxRuntimeSec"].is<uint32_t>()) c.maxRuntimeSec=doc["maxRuntimeSec"];
  if(doc["minGapMin"].is<uint32_t>()) c.minGapMin=doc["minGapMin"];
  if(doc["stableDurationSec"].is<uint32_t>()) c.stableDurationSec=doc["stableDurationSec"];
  if(doc["filterSamples"].is<uint8_t>()) c.filterSamples=doc["filterSamples"];
  if(doc["relayActiveHigh"].is<bool>()) c.relayActiveHigh=doc["relayActiveHigh"];
  if(doc["tankSensorEnabled"].is<bool>()) c.tankSensorEnabled=doc["tankSensorEnabled"];
  if(doc["leakSensorEnabled"].is<bool>()) c.leakSensorEnabled=doc["leakSensorEnabled"];
  if(doc["stopOnWifiLoss"].is<bool>()) c.stopOnWifiLoss=doc["stopOnWifiLoss"];
  String err; if(!config->validate(c,err)){ config->get()=old; server.send(400,"application/json",String("{\"ok\":false,\"error\":\"")+err+"\"}"); return; }
  config->save(); hx->applyConfig(); log->add("SETTINGS", "Settings saved"); server.send(200,"application/json","{\"ok\":true}");
}
void WebServerManager::handleTare(){ pump->setPump(false,"Calibration tare"); bool ok=hx->tare(); log->add("CAL", ok?"Tare saved":"Tare failed"); server.send(ok?200:500,"application/json",ok?"{\"ok\":true}":"{\"ok\":false}"); }
void WebServerManager::handleKnownWeight(){ JsonDocument doc; deserializeJson(doc, server.arg("plain")); float g=doc["knownWeightG"]|0.0f; pump->setPump(false,"Calibration known weight"); bool ok=hx->calibrateWithKnownWeight(g); log->add("CAL", ok?"Calibration factor saved":"Calibration failed"); server.send(ok?200:400,"application/json",ok?"{\"ok\":true}":"{\"ok\":false,\"error\":\"Calibration failed\"}"); }
void WebServerManager::handleResetCalibration(){ config->get().tareOffset=0; config->get().calibrationFactor=1.0f; config->save(); hx->applyConfig(); log->add("CAL", "Calibration reset"); server.send(200,"application/json","{\"ok\":true}"); }
void WebServerManager::handlePumpOn(){ controller->manualOn(MAX_MANUAL_RUNTIME_SEC); server.send(200,"application/json","{\"ok\":true}"); }
void WebServerManager::handlePumpRun(){ JsonDocument doc; deserializeJson(doc, server.arg("plain")); uint32_t s=doc["seconds"]|10; controller->manualOn(s); server.send(200,"application/json","{\"ok\":true}"); }
void WebServerManager::handlePumpOff(){ controller->manualOff("Manual off from dashboard"); server.send(200,"application/json","{\"ok\":true}"); }
void WebServerManager::handleEmergencyStop(){ controller->setEmergencyStop(true); server.send(200,"application/json","{\"ok\":true}"); }
void WebServerManager::handleClearError(){ controller->setEmergencyStop(false); controller->clearError(); server.send(200,"application/json","{\"ok\":true}"); }
void WebServerManager::handleLogs(){ JsonDocument doc; JsonArray arr=doc["logs"].to<JsonArray>(); log->toJson(arr); String out; serializeJson(doc,out); server.send(200,"application/json",out); }
bool WebServerManager::argBool(const String &name, bool fallback){ if(!server.hasArg(name)) return fallback; String v=server.arg(name); return v=="1"||v=="true"||v=="on"; }
uint16_t WebServerManager::parseMinutes(const String &hhmm, uint16_t fallback){ int p=hhmm.indexOf(':'); if(p<0)return fallback; int h=hhmm.substring(0,p).toInt(); int m=hhmm.substring(p+1).toInt(); if(h<0||h>23||m<0||m>59)return fallback; return h*60+m; }
