#pragma once

#ifndef PwnagotchiDisplay_h
#define PwnagotchiDisplay_h

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "PwnagotchiWiFiCore.h"

// OLED display parameters (128x64 typical for SSD1306/SH1106)
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C

// I2C pins for ESP32-S3 SuperMini
#define OLED_SDA 6
#define OLED_SCL 7

// Display update interval (milliseconds)
#define DISPLAY_UPDATE_INTERVAL 250

enum MoodState {
    MOOD_IDLE = 0,       // Waiting for activity
    MOOD_HAPPY = 1,      // New APs detected
    MOOD_EXCITED = 2,    // Handshake captured
    MOOD_SAD = 3,        // No activity (idle timeout)
    MOOD_SLEEPING = 4    // WiFi off
};

typedef struct {
    uint32_t seenAPCount;      // Total unique APs this session
    uint32_t newAPCount;       // New APs since last epoch
    uint32_t clientCount;      // Detected clients
    uint32_t handshakeCount;   // EAPOL handshakes captured
    uint8_t currentChannel;    // Current Wi-Fi channel
    uint32_t epochTimestamp;   // Seconds since last state update
} StateVector;

class PwnagotchiDisplay {
  public:
    PwnagotchiDisplay(uint8_t sda = OLED_SDA, uint8_t scl = OLED_SCL);
    ~PwnagotchiDisplay();

    void init();
    void deinit();
    void update(const PwnagotchiStats& wifi_stats);
    void displayWiFiStats(const PwnagotchiStats& wifi_stats);
    void displayMood(MoodState mood, const StateVector& state);

    MoodState getCurrentMood();
    void setMood(MoodState mood);

  private:
    Adafruit_SSD1306* display;
    uint8_t sda_pin;
    uint8_t scl_pin;
    MoodState current_mood;
    uint32_t last_ap_count;
    uint32_t last_handshake_count;
    uint32_t last_mood_update;
    uint32_t idle_timer_start;

    void drawSimpleFace();
    void updateMood(const PwnagotchiStats& wifi_stats);
    void drawMoodHappy();
    void drawMoodExcited();
    void drawMoodSad();
    void drawMoodSleeping();
    void drawMoodIdle();
    void drawChannel(uint8_t channel);
    void drawAPCount(uint32_t count);
    void drawClientCount(uint32_t count);
    void drawHandshakeCount(uint32_t count);
};

extern PwnagotchiDisplay display;

#endif
