#include "WebServerManager.h"
#include "config.h"
#include "index_html.h"
#include <Update.h>
#include <WiFi.h>

void WebServerManager::begin(ConfigManager *cfg, HX711Manager *h, PumpManager *p, IrrigationController *ic, LogManager *lg, WeightHistoryManager *wh) {
  config = cfg; hx = h; pump = p; controller = ic; log = lg; history = wh; setupRoutes(); server.begin(); log->add("WEB", "HTTP server started");
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
  server.on("/api/weight-history", HTTP_GET, [this](){ handleWeightHistory(); });
  server.on("/wifi", HTTP_GET, [this](){ handleWiFiPage(); });
  server.on("/api/wifi/scan", HTTP_GET, [this](){ handleWiFiScan(); });
  server.on("/api/wifi/connect", HTTP_POST, [this](){ handleWiFiConnect(); });
  server.on("/api/wifi/status", HTTP_GET, [this](){ handleWiFiStatus(); });
  server.on("/firmware", HTTP_GET, [this](){ handleFirmwarePage(); });
  server.on("/api/firmware/update", HTTP_POST, [this](){ handleFirmwareUpload(); }, [this](){ 
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      // Reset update-in-progress flag on every new upload
      this->_updateInProgress = false;
      pump->setPump(false, "Firmware update");
      controller->setEmergencyStop(true);
      Serial.printf("Firmware upload start: %s (%u bytes)\n", upload.filename.c_str(), upload.totalSize);
      // Update.begin() validates size against flash partition
      if (!Update.begin(upload.totalSize)) {
        Update.printError(Serial);
      } else {
        this->_updateInProgress = true;
      }
    } else if (upload.status == UPLOAD_FILE_WRITE && this->_updateInProgress) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
        this->_updateInProgress = false;
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (this->_updateInProgress && Update.end(true)) {
        Serial.printf("Firmware upload success: %u bytes\n", upload.totalSize);
        log->add("OTA", "Web upload OK, rebooting...");
        server.send(200, "text/plain", "OK - Rebooting");
        delay(500);
        ESP.restart();
      } else {
        Update.printError(Serial);
        Update.end(); // clean up partial update
        log->add("OTA", "Web upload failed");
        server.send(500, "text/plain", "Update failed");
      }
    }
  });
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
void WebServerManager::handleWiFiPage(){
  String html = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>WiFi Setup</title>
<style>
body{font-family:Arial,sans-serif;background:#111;color:#eee;margin:0;padding:16px}
h1{font-size:1.4rem}
.card{background:#1d1d1d;border:1px solid #333;border-radius:12px;padding:16px;margin-bottom:12px}
.list{max-height:300px;overflow-y:auto;border:1px solid #333;border-radius:8px;background:#0a0a0a}
.network{padding:8px;border-bottom:1px solid #333;cursor:pointer}
.network:hover{background:#222}
.network.selected{background:#145c2a}
input,button{width:100%;padding:9px;border-radius:8px;border:1px solid #444;background:#222;color:#eee;box-sizing:border-box;margin-bottom:8px}
button{cursor:pointer}
.good{background:#145c2a}
.scanning{color:#ffd166}
</style>
</head><body>
<h1>WiFi Network Setup</h1>
<div class="card">
  <h2>Available Networks</h2>
  <button onclick="scanNetworks()">Scan for Networks</button>
  <div id="networks" class="list"></div>
  <p id="scanStatus"></p>
</div>
<div class="card">
  <h2>Connect to Network</h2>
  <label>Selected Network: <input id="selectedSsid" type="text" readonly></label>
  <label>Password: <input id="password" type="password"></label>
  <button class="good" onclick="connectNetwork()">Connect</button>
  <p id="connectStatus"></p>
</div>
<div class="card">
  <h2>Current Connection</h2>
  <p>SSID: <b id="currentSsid">--</b></p>
  <p>IP: <b id="currentIp">--</b></p>
  <button onclick="location.href='/'">Back to Dashboard</button>
</div>
<script>
async function api(u,o){let r=await fetch(u,o);let t=await r.text();try{return JSON.parse(t)}catch(e){return {ok:r.ok,text:t}}}
async function scanNetworks(){
  document.getElementById('scanStatus').textContent='Scanning...';
  document.getElementById('scanStatus').className='scanning';
  let result=await api('/api/wifi/scan');
  document.getElementById('scanStatus').textContent='';
  document.getElementById('scanStatus').className='';
  let list=document.getElementById('networks');
  list.innerHTML='';
  if(result.networks && Array.isArray(result.networks)){
    result.networks.forEach(net=>{
      let div=document.createElement('div');
      div.className='network';
      div.textContent=(net.ssid||'(Hidden)')+' ('+net.rssi+'dBm)';
      div.addEventListener('click',function(){selectNetwork(net.ssid,this)});
      list.appendChild(div);
    });
  }
  if(!list.children.length){list.innerHTML='<div class="network">No networks found</div>';}
}
function selectNetwork(ssid,el){
  document.getElementById('selectedSsid').value=ssid;
  document.querySelectorAll('.network').forEach(e=>e.classList.remove('selected'));
  el.classList.add('selected');
}
async function connectNetwork(){
  let ssid=document.getElementById('selectedSsid').value;
  let pwd=document.getElementById('password').value;
  if(!ssid){alert('Please select a network');return;}
  document.getElementById('connectStatus').textContent='Connecting...';
  document.getElementById('connectStatus').className='scanning';
  let result=await api('/api/wifi/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:ssid,password:pwd})});
  if(result.ok){
    document.getElementById('connectStatus').textContent='Connected! Reloading in 3 seconds...';
    document.getElementById('connectStatus').className='good';
    setTimeout(()=>{location.href='/';},3000);
  }else{
    document.getElementById('connectStatus').textContent='Failed: '+(result.error||'Unknown error');
    document.getElementById('connectStatus').className='bad';
  }
}
async function updateStatus(){
  let status=await api('/api/wifi/status');
  document.getElementById('currentSsid').textContent=status.ssid||'Not connected';
  document.getElementById('currentIp').textContent=status.ip||'--';
}
scanNetworks();
updateStatus();
setInterval(updateStatus,3000);
</script>
</body></html>)HTML";
  server.send(200,"text/html",html);
}
void WebServerManager::handleWiFiScan(){
  int n=WiFi.scanNetworks();
  JsonDocument doc;
  JsonArray networks=doc["networks"].to<JsonArray>();
  for(int i=0;i<n;i++){
    JsonObject net=networks.add<JsonObject>();
    net["ssid"]=WiFi.SSID(i);
    net["rssi"]=WiFi.RSSI(i);
    net["encryption"]=WiFi.encryptionType(i);
  }
  doc["total"]=n;
  String out;serializeJson(doc,out);
  server.send(200,"application/json",out);
}
void WebServerManager::handleWiFiConnect(){
  JsonDocument doc;
  DeserializationError e=deserializeJson(doc,server.arg("plain"));
  if(e){server.send(400,"application/json","{\"ok\":false,\"error\":\"Bad JSON\"}");return;}
  String ssid=doc["ssid"]|"";
  String pwd=doc["password"]|"";
  if(ssid.length()==0){server.send(400,"application/json","{\"ok\":false,\"error\":\"SSID required\"}");return;}
  config->get().wifiSsid[0]=0;
  strlcpy(config->get().wifiSsid,ssid.c_str(),sizeof(config->get().wifiSsid));
  config->get().wifiPassword[0]=0;
  strlcpy(config->get().wifiPassword,pwd.c_str(),sizeof(config->get().wifiPassword));
  config->save();
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(),pwd.c_str());
  log->add("WIFI","Connection attempt to: "+ssid);
  server.send(200,"application/json","{\"ok\":true}");
}
void WebServerManager::handleWiFiStatus(){
  JsonDocument doc;
  doc["connected"]=(WiFi.status()==WL_CONNECTED);
  doc["ssid"]=WiFi.SSID();
  doc["ip"]=WiFi.localIP().toString();
  String out;serializeJson(doc,out);
  server.send(200,"application/json",out);
}
bool WebServerManager::argBool(const String &name, bool fallback){ if(!server.hasArg(name)) return fallback; String v=server.arg(name); return v=="1"||v=="true"||v=="on"; }
uint16_t WebServerManager::parseMinutes(const String &hhmm, uint16_t fallback){ int p=hhmm.indexOf(':'); if(p<0)return fallback; int h=hhmm.substring(0,p).toInt(); int m=hhmm.substring(p+1).toInt(); if(h<0||h>23||m<0||m>59)return fallback; return h*60+m; }

void WebServerManager::handleFirmwarePage() {
  String html = R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Firmware Update</title>
<style>
body{font-family:Arial,sans-serif;background:#111;color:#eee;margin:0;padding:16px}
h1{font-size:1.4rem}
.card{background:#1d1d1d;border:1px solid #333;border-radius:12px;padding:16px;margin-bottom:12px}
input,button{width:100%;padding:9px;border-radius:8px;border:1px solid #444;background:#222;color:#eee;box-sizing:border-box;margin-bottom:8px}
button{cursor:pointer}
.good{background:#145c2a}
.danger{background:#6b1c1c}
#progressBar{width:100%;height:24px;background:#222;border-radius:8px;overflow:hidden;margin-top:8px}
#progressFill{height:100%;width:0%;background:#145c2a;transition:width .3s}
#status{text-align:center;margin-top:8px}
</style>
</head><body>
<h1>Firmware Update</h1>
<div class="card">
  <h2>Upload Firmware (.bin)</h2>
  <p>Current version: <b>)HTML" + String(FIRMWARE_VERSION) + R"HTML(</b></p>
  <input type="file" id="firmwareFile" accept=".bin">
  <button class="good" onclick="uploadFirmware()">Upload & Update</button>
  <div id="progressBar"><div id="progressFill"></div></div>
  <div id="status"></div>
</div>
<div class="card">
  <h2>ArduinoOTA (IDE)</h2>
  <p>Hostname: <b>irrigation.local</b></p>
  <p>Available as a network port in Arduino IDE or PlatformIO for wireless firmware upload.</p>
</div>
<div class="card">
  <button onclick="location.href='/'">Back to Dashboard</button>
</div>
<script>
function $(id){return document.getElementById(id)}
function setStatus(msg,cls){$('status').textContent=msg;$('status').className=cls||''}

function uploadFirmware(){
  let file=$('firmwareFile').files[0];
  if(!file){setStatus('Please select a .bin firmware file','bad');return;}
  if(!file.name.endsWith('.bin')){setStatus('Only .bin files are accepted','bad');return;}

  let xhr=new XMLHttpRequest();
  xhr.open('POST','/api/firmware/update');
  xhr.upload.onprogress=function(e){
    if(e.lengthComputable){
      let pct=Math.round(e.loaded/e.total*100);
      $('progressFill').style.width=pct+'%';
      setStatus('Uploading: '+pct+'%');
    }
  };
  xhr.onload=function(){
    if(xhr.status===200){
      $('progressFill').style.width='100%';
      setStatus('Update successful! Rebooting...','good');
      setTimeout(()=>{location.href='/'},5000);
    }else{
      setStatus('Update failed: '+xhr.responseText,'bad');
    }
  };
  xhr.onerror=function(){
    setStatus('Network error during upload','bad');
  };
  xhr.send(file);
  setStatus('Starting upload...');
}
</script>
</body></html>)HTML";
  server.send(200, "text/html", html);
}

void WebServerManager::handleFirmwareUpload() {
  // Upload handled entirely by the inline lambda in setupRoutes().
  // This method exists as a route target; the server.on third argument (upload handler) does the work.
  server.send(500, "text/plain", "Upload not received");
}

void WebServerManager::handleWeightHistory() {
  uint32_t rangeHours = 24;
  if (server.hasArg("range")) {
    long r = server.arg("range").toInt();
    if (r > 0 && r <= 168) rangeHours = (uint32_t)r;
  }

  uint32_t nowEpoch = history->currentEpoch();
  uint32_t sinceSec = 0;
  if (rangeHours < 168) {
    uint32_t subtractSec = rangeHours * 3600UL;
    sinceSec = (subtractSec < nowEpoch) ? (nowEpoch - subtractSec) : 0;
  }

  // Cap at 500 points for JSON size; chart canvas is only ~900px wide anyway
  const uint16_t MAX_JSON_POINTS = 500;
  // Each point ~45 bytes JSON, 500 * 45 = 22500, plus wrapper ~100 = 22600.
  // Use 32KB to be safe.
  DynamicJsonDocument doc(32768);
  JsonArray arr = doc["points"].to<JsonArray>();
  history->toJson(arr, sinceSec, MAX_JSON_POINTS);

  doc["rangeHours"] = rangeHours;
  doc["totalPoints"] = history->count();

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}
