void handleRoot() {
  webserver.send(200, "text/html", "<html><body><script>var wsurl = 'ws://' + window.location.hostname + ':81'; var socket = new WebSocket(wsurl); socket.onmessage = function (event) { console.log(event.data);  var msg = JSON.parse(event.data); document.getElementById('temp').innerHTML = msg.data.temp }</script>Temp:&nbsp;<span id=\"temp\"></span>C</body></html>");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        String json = getStatusJSON();
        Serial.println(json);
        webSocket.sendTXT(num, json);
      }
      break;
    case WStype_TEXT:
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
  }

}
