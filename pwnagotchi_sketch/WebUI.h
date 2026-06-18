#pragma once

#ifndef WebUI_h
#define WebUI_h

// Guard: compile WebUI only when explicitly enabled.
// To enable: add #define ENABLE_WEBUI before including this header,
// or pass -DENABLE_WEBUI to the compiler.
// Requires AsyncTCP library in addition to ESPAsyncWebServer.
#ifdef ENABLE_WEBUI

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "PwnagotchiTypes.h"
#include "PwnagotchiWiFiCore.h"
#include "MoodEngine.h"

#define WEBUI_SSID     "pwnagotchi-clone"
#define WEBUI_CHANNEL  1   // Soft-AP fixed channel (sniffing will follow this channel)
#define WEBUI_PORT     80

class WebUI {
  public:
    WebUI();
    void begin();
    void end();
    bool isRunning() const { return running; }

  private:
    AsyncWebServer server;
    bool running;

    void setupRoutes();
    static const char* moodToString(MoodState mood);
};

extern WebUI web_ui;

#endif  // ENABLE_WEBUI
#endif  // WebUI_h
