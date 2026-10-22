#include <Arduino.h>
#ifdef UI_MIRROR
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "webmirror.h"

static WebServer server(80);
static WebSocketsServer webSocket(81);

#define VIRTUAL_KEY_BUFFER_SIZE 16
static char virtualKeyBuffer[VIRTUAL_KEY_BUFFER_SIZE];
static int vkHead = 0;
static int vkTail = 0;

static void pushVirtualKey(char k) {
    int next = (vkHead + 1) % VIRTUAL_KEY_BUFFER_SIZE;
    if (next != vkTail) {
        virtualKeyBuffer[vkHead] = k;
        vkHead = next;
    }
}

char webMirrorGetKey() {
    if (vkHead == vkTail) return '\0';
    char k = virtualKeyBuffer[vkTail];
    vkTail = (vkTail + 1) % VIRTUAL_KEY_BUFFER_SIZE;
    return k;
}

static const char* indexHtml = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>HRT Monitor - Virtual UI</title>
<style>
  body { background: #111; color: #eee; font-family: sans-serif; text-align: center; margin-top: 20px; }
  canvas { border: 4px solid #333; background: #000; box-shadow: 0 0 20px rgba(0,255,255,0.2); margin-bottom: 15px; }
  #status { margin: 10px; font-weight: bold; }
  .keypad { display: inline-grid; grid-template-columns: repeat(4, 60px); gap: 8px; }
  .key { background: #222; color: #fff; border: 2px solid #444; font-size: 20px; padding: 15px; cursor: pointer; border-radius: 8px; user-select: none; }
  .key:hover { background: #333; border-color: #0ff; }
  .key:active { background: #0ff; color: #000; }
</style>
</head>
<body>
  <h2>HRT Monitor - Virtual Display & Keypad</h2>
  <div id="status">Connecting...</div>
  <canvas id="canvas" width="320" height="240"></canvas>
  <br>
  <div class="keypad">
    <button class="key" onclick="sendKey('1')">1</button>
    <button class="key" onclick="sendKey('2')">2</button>
    <button class="key" onclick="sendKey('3')">3</button>
    <button class="key" onclick="sendKey('A')">A</button>
    <button class="key" onclick="sendKey('4')">4</button>
    <button class="key" onclick="sendKey('5')">5</button>
    <button class="key" onclick="sendKey('6')">6</button>
    <button class="key" onclick="sendKey('B')">B</button>
    <button class="key" onclick="sendKey('7')">7</button>
    <button class="key" onclick="sendKey('8')">8</button>
    <button class="key" onclick="sendKey('9')">9</button>
    <button class="key" onclick="sendKey('C')">C</button>
    <button class="key" onclick="sendKey('*')">*</button>
    <button class="key" onclick="sendKey('0')">0</button>
    <button class="key" onclick="sendKey('#')">#</button>
    <button class="key" onclick="sendKey('D')">D</button>
  </div>
  <script>
    const canvas = document.getElementById('canvas');
    const ctx = canvas.getContext('2d');
    const status = document.getElementById('status');
    
    canvas.style.width = '640px';
    canvas.style.height = '480px';

    const ws = new WebSocket('ws://' + window.location.hostname + ':81/');
    ws.onopen = () => { status.innerText = 'Connected'; status.style.color = '#0f0'; };
    ws.onclose = () => { status.innerText = 'Disconnected'; status.style.color = '#f00'; };
    
    function sendKey(k) {
      if (ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({cmd: 'key', key: k}));
      }
    }

    ws.onmessage = (event) => {
      try {
        const msg = JSON.parse(event.data);
        if (msg.cmd === 'fill') {
          ctx.fillStyle = msg.color || '#000000';
          ctx.fillRect(0, 0, 320, 240);
        } else if (msg.cmd === 'text') {
          ctx.font = (msg.size === 3 ? '24px' : msg.size === 2 ? '16px' : '12px') + ' monospace';
          ctx.fillStyle = msg.color || '#ffffff';
          ctx.fillText(msg.text, msg.x, msg.y + (msg.size === 3 ? 20 : msg.size === 2 ? 14 : 10));
        } else if (msg.cmd === 'line') {
          ctx.strokeStyle = msg.color || '#ffffff';
          ctx.beginPath();
          ctx.moveTo(msg.x1, msg.y1);
          ctx.lineTo(msg.x2, msg.y2);
          ctx.stroke();
        } else if (msg.cmd === 'hline') {
          ctx.strokeStyle = msg.color || '#ffffff';
          ctx.beginPath();
          ctx.moveTo(msg.x, msg.y);
          ctx.lineTo(msg.x + msg.w, msg.y);
          ctx.stroke();
        }
      } catch(e) { console.error(e); }
    };
  </script>
</body>
</html>
)rawliteral";

static String colorToHex(uint16_t c) {
    uint8_t r = ((c >> 11) & 0x1F) * 255 / 31;
    uint8_t g = ((c >> 5) & 0x3F) * 255 / 63;
    uint8_t b = (c & 0x1F) * 255 / 31;
    char buf[16];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
    return String(buf);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_CONNECTED) {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[WebMirror] Client #%u connected from %s\n", num, ip.toString().c_str());
    } else if (type == WStype_TEXT) {
        String msg = String((char*)payload);
        if (msg.indexOf("\"cmd\":\"key\"") >= 0) {
            int idx = msg.indexOf("\"key\":\"");
            if (idx >= 0) {
                char k = msg.charAt(idx + 7);
                pushVirtualKey(k);
                Serial.printf("[WebMirror] Virtual key received: %c\n", k);
            }
        }
    }
}

void webMirrorInit() {
    Serial.println("[WebMirror] Starting WiFi SoftAP (HRT-Monitor-Debug)...");
    WiFi.softAP("HRT-Monitor-Debug", "12345678");
    IPAddress IP = WiFi.softAPIP();
    Serial.print("[WebMirror] AP IP address: ");
    Serial.println(IP);

    server.on("/", []() {
        server.send(200, "text/html", indexHtml);
    });
    server.begin();

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    Serial.println("[WebMirror] HTTP server & WebSockets started.");
}

void webMirrorLoop() {
    server.handleClient();
    webSocket.loop();
}

void mirrorFillScreen(uint16_t color) {
    String hex = colorToHex(color);
    String json = "{\"cmd\":\"fill\",\"color\":\"" + hex + "\"}";
    webSocket.broadcastTXT(json);
}

void mirrorDrawString(const char* text, int x, int y, uint8_t size, uint16_t color) {
    String hex = colorToHex(color);
    String t(text);
    t.replace("\"", "\\\"");
    String json = "{\"cmd\":\"text\",\"text\":\"" + t + "\",\"x\":" + String(x) + ",\"y\":" + String(y) + ",\"size\":" + String(size) + ",\"color\":\"" + hex + "\"}";
    webSocket.broadcastTXT(json);
}

void mirrorDrawLine(int x1, int y1, int x2, int y2, uint16_t color) {
    String hex = colorToHex(color);
    String json = "{\"cmd\":\"line\",\"x1\":" + String(x1) + ",\"y1\":" + String(y1) + ",\"x2\":" + String(x2) + ",\"y2\":" + String(y2) + ",\"color\":\"" + hex + "\"}";
    webSocket.broadcastTXT(json);
}

void mirrorDrawHLine(int x, int y, int w, uint16_t color) {
    String hex = colorToHex(color);
    String json = "{\"cmd\":\"hline\",\"x\":" + String(x) + ",\"y\":" + String(y) + ",\"w\":" + String(w) + ",\"color\":\"" + hex + "\"}";
    webSocket.broadcastTXT(json);
}

#endif
