#include "WebUI.h"

#ifdef ENABLE_WEBUI

WebUI web_ui;

// Inline HTML served at GET /
// JS fetches /api/status every 2 s and updates face + stats without page reload.
static const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Pwnagotchi</title>
<style>
  body { background:#111; color:#0f0; font-family:monospace; text-align:center; margin:0; padding:20px; }
  h1   { font-size:1.2em; letter-spacing:.15em; margin-bottom:4px; }
  #face { font-size:4em; line-height:1; margin:16px 0; }
  table { margin:0 auto; border-collapse:collapse; }
  td    { padding:3px 12px; }
  td:first-child { text-align:right; color:#888; }
  td:last-child  { text-align:left;  color:#0f0; }
  #mood-label { font-size:0.85em; color:#555; margin-top:8px; }
</style>
</head>
<body>
<h1>&#x1F4F6; PWNAGOTCHI</h1>
<div id="face">( &#xB7;_&#xB7;)</div>
<div id="mood-label">IDLE</div>
<br>
<table>
  <tr><td>Channel</td><td id="ch">-</td></tr>
  <tr><td>APs</td><td id="aps">-</td></tr>
  <tr><td>Clients</td><td id="cl">-</td></tr>
  <tr><td>Handshakes</td><td id="hs">-</td></tr>
</table>
<script>
const FACES = {
  happy:    "( ^_^)",
  excited:  "(*O*)",
  sad:      "( ;_;)",
  sleeping: "(-_-) zZz",
  idle:     "( ._.) "
};
async function refresh() {
  try {
    const r = await fetch("/api/status");
    if (!r.ok) return;
    const d = await r.json();
    document.getElementById("face").textContent       = FACES[d.mood] || FACES.idle;
    document.getElementById("mood-label").textContent = d.mood.toUpperCase();
    document.getElementById("ch").textContent  = d.channel;
    document.getElementById("aps").textContent = d.aps;
    document.getElementById("cl").textContent  = d.clients;
    document.getElementById("hs").textContent  = d.handshakes;
  } catch(e) { /* AP may be on a different channel — just retry */ }
}
refresh();
setInterval(refresh, 2000);
</script>
</body>
</html>
)rawliteral";

WebUI::WebUI() : server(WEBUI_PORT), running(false) {}

void WebUI::begin() {
    if (running) return;

    // Start soft-AP so the device is reachable without a home network.
    // NOTE: ESP32 has a single radio — the AP will follow channel hops
    // when the sniffer changes channels. HTTP clients use 2-second retry
    // (JS fetch loop) which handles brief disconnects gracefully.
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WEBUI_SSID);

    Serial.printf("[WebUI] AP started  SSID: %s  IP: %s\n",
                  WEBUI_SSID, WiFi.softAPIP().toString().c_str());

    setupRoutes();
    server.begin();
    running = true;
    Serial.println(F("[WebUI] HTTP server running on port 80"));
}

void WebUI::end() {
    if (!running) return;
    server.end();
    WiFi.softAPdisconnect(true);
    running = false;
    Serial.println(F("[WebUI] Stopped"));
}

const char* WebUI::moodToString(MoodState mood) {
    switch (mood) {
        case MOOD_HAPPY:    return "happy";
        case MOOD_EXCITED:  return "excited";
        case MOOD_SAD:      return "sad";
        case MOOD_SLEEPING: return "sleeping";
        default:            return "idle";
    }
}

void WebUI::setupRoutes() {
    // GET / — single-page UI with live face + stats
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", HTML_PAGE);
    });

    // GET /api/status — JSON snapshot consumed by the JS fetch loop
    server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest* req) {
        PwnagotchiStats s   = wifi_core.getStats();
        MoodState       m   = mood_engine.getCurrentMood();
        const char*     ms  = WebUI::moodToString(m);

        char buf[128];
        snprintf(buf, sizeof(buf),
                 "{\"mood\":\"%s\",\"aps\":%lu,\"clients\":%lu,"
                 "\"handshakes\":%lu,\"channel\":%lu}",
                 ms,
                 (unsigned long)s.total_aps,
                 (unsigned long)s.total_clients,
                 (unsigned long)s.total_handshakes,
                 (unsigned long)s.current_channel);

        req->send(200, "application/json", buf);
    });

    // 404 fallback
    server.onNotFound([](AsyncWebServerRequest* req) {
        req->send(404, "text/plain", "Not found");
    });
}

#endif  // ENABLE_WEBUI
