/*
  Simple net radio
  Board : ESP32
  Amp   : MAX98357 I2S  (LRC=GPIO22, BCLK=GPIO19, DIN=GPIO23)
  Lib   : ESP32-audioI2S (by schreibfaul1)  -> Library Manager: "ESP32-audioI2S"

  Web UI: http://netradio.local  (or device IP)
    - station list (click to play)
    - play/pause button
    - volume slider
*/

#include <WiFi.h>
#include <Audio.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define I2S_LRC   22
#define I2S_BCLK  19
#define I2S_DIN   23

const char* WIFI_SSID     = "DRBOM";
const char* WIFI_PASSWORD = "55555555";

struct Station { const char* name; const char* url; };
const Station stations[] = {
  {"coolfh",          "https://coolism-web.cdn.byteark.com/;stream/1"},
  {"greenwave",       "https://atimeonline3.smartclick.co.th/green"},
  {"efm",             "https://atimeonline3.smartclick.co.th/efm"},
  {"looktungnetwork", "https://cdn-th2.livestreaming.in.th/shoutcast/8200"},
  {"munforward",      "https://cdn-th2.livestreaming.in.th/shoutcast/8730"},
  {"great93",         "https://radio.vpsthai.net/proxy/qndrbfic/stream2"},
  {"yesterdayradio",  "http://radio4.thzhost.com:4122/;"},
  {"newsfamily",      "https://media.onair.one:8110/106"},
  {"xfm",             "https://radio12.plathong.net/7360/;stream.mp3"},
  {"hotwave",         "https://atimeonline3.smartclick.co.th/hotwave"},
  {"lovefm",          "http://radio10.plathong.net:7296/;"},
  {"fat_chiangrai",   "https://hp1.hostpleng.cloud/8272/stream"},
  {"love_chiangmai",  "https://radio11.plathong.net/7142/;stream.mp3"}
};
const int STATION_COUNT = sizeof(stations) / sizeof(stations[0]);

const int VOLUME = 8;  // 0..21 (~40%)

Audio audio;
WebServer server(80);
int currentStation = 0;
bool isPaused = false;

void playStation(int index) {
  if (index < 0 || index >= STATION_COUNT) return;
  currentStation = index;
  isPaused = false;
  Serial.printf("Station %d: %s -> %s\n", currentStation, stations[currentStation].name, stations[currentStation].url);
  audio.connecttohost(stations[currentStation].url);
}

void handleRoot() {
  String html;
  html.reserve(4096);
  html += F("<!DOCTYPE html><html><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Net Radio</title><style>");
  html += F("body{font-family:sans-serif;margin:20px;background:#222;color:#eee}");
  html += F("h1{text-align:center}");
  html += F("ul{list-style:none;padding:0;max-width:420px;margin:auto}");
  html += F("li{margin:6px 0}");
  html += F("button.station{width:100%;padding:10px;font-size:16px;border:none;border-radius:6px;background:#333;color:#eee;cursor:pointer;text-align:left}");
  html += F("button.station:hover{background:#0a84ff}");
  html += F("button.station.active{background:#0a84ff}");
  html += F(".controls{text-align:center;margin-top:20px}");
  html += F("button.ctrl{font-size:20px;padding:10px 20px;margin:5px;border:none;border-radius:8px;background:#0a84ff;color:#fff;cursor:pointer}");
  html += F("input[type=range]{width:80%;margin:10px}");
  html += F("</style></head><body><h1>ESP32 Net Radio</h1>");
  html += F("<p style='text-align:center'>Now playing: <b id='now'>");
  html += stations[currentStation].name;
  html += F("</b></p><ul>");
  for (int i = 0; i < STATION_COUNT; i++) {
    html += "<li><button class='station' onclick=\"play(" + String(i) + ")\">" + String(stations[i].name) + "</button></li>";
  }
  html += F("</ul><div class='controls'>");
  html += F("<button class='ctrl' onclick='toggle()'>Play/Pause</button><br>");
  html += F("Volume: <span id='vol'>40</span>%<br>");
  html += F("<input type='range' min='0' max='21' value='8' id='slider' oninput='setVol(this.value)'>");
  html += F("</div><script>");
  html += "var currentStation = " + String(currentStation) + ";";
  html += F("function updateActive(){document.querySelectorAll('button.station').forEach((b,idx)=>{b.classList.toggle('active', idx===currentStation);});}");
  html += F("function play(i){currentStation=i;updateActive();document.getElementById('now').textContent=document.querySelectorAll('button.station')[i].textContent;fetch('/play?i='+i);}");

  html += F("function toggle(){fetch('/playpause');}");
  html += F("function setVol(v){document.getElementById('vol').textContent=Math.round(v/21*100);fetch('/volume?val='+v);}");
  html += F("window.onload=updateActive;");
  html += F("</script></body></html>");
  server.send(200, "text/html", html);
}

void handlePlay() {
  if (server.hasArg("i")) {
    playStation(server.arg("i").toInt());
  }
  server.send(200, "text/plain", "ok");
}

void handlePlayPause() {
  audio.pauseResume();
  isPaused = !isPaused;
  server.send(200, "text/plain", isPaused ? "paused" : "playing");
}

void handleVolume() {
  if (server.hasArg("val")) {
    int v = server.arg("val").toInt();
    if (v < 0) v = 0;
    if (v > 21) v = 21;
    audio.setVolume(v);
  }
  server.send(200, "text/plain", "ok");
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(500);

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DIN);
  audio.setVolume(VOLUME);

  connectWiFi();

  if (MDNS.begin("netradio")) {
    Serial.println("mDNS responder started: http://netradio.local");
  }

  playStation(currentStation);

  server.on("/", handleRoot);
  server.on("/play", handlePlay);
  server.on("/playpause", handlePlayPause);
  server.on("/volume", handleVolume);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  audio.loop();
  server.handleClient();

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'n') {
      playStation((currentStation + 1) % STATION_COUNT);
    } else if (c == 'p') {
      playStation((currentStation + STATION_COUNT - 1) % STATION_COUNT);
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    playStation(currentStation);
  }
}

void audio_info(const char* info) {
  Serial.print("info: ");
  Serial.println(info);
}

void audio_eof_stream(const char* info) {
  Serial.print("stream ended: ");
  Serial.println(info);
  playStation(currentStation);
}
