void telnetLoop() {
  if (server.hasClient()) {
    //find free/disconnected spot
    int i;
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
      if (!serverClients[i]) { // equivalent to !serverClients[i].connected()
        Serial.printf("New client: index %u", i);
        serverClients[i] = server.available();
        serverClients[i].printf("Hello %u\n", i);
        break;
      }

    //no free/disconnected spot so reject
    if (i == MAX_SRV_CLIENTS) {
      server.available().println("busy");
      // hints: server.available() is a WiFiClient with short-term scope
      // when out of scope, a WiFiClient will
      // - flush() - all data will be sent
      // - stop() - automatically too
      Serial.printf("server is busy with %d active connections\n", MAX_SRV_CLIENTS);
    }
  }

}

void telnetSend(String message) {
  for (int i = 0; i < MAX_SRV_CLIENTS; i++){
    if (serverClients[i]) { // equivalent to serverClients[i].connected()
      serverClients[i].println(message);
    }
  }  
}
