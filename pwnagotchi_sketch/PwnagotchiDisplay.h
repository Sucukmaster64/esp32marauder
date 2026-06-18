#pragma once

#ifndef PwnagotchiDisplay_h
#define PwnagotchiDisplay_h

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "PwnagotchiTypes.h"

#define OLED_WIDTH  128
#define OLED_HEIGHT  64
#define OLED_ADDR  0x3C

// I2C pins for ESP32-S3 SuperMini (XIAO)
#define OLED_SDA 6
#define OLED_SCL 7

#define DISPLAY_UPDATE_MS 250

class PwnagotchiDisplay {
  public:
    PwnagotchiDisplay(uint8_t sda = OLED_SDA, uint8_t scl = OLED_SCL);
    ~PwnagotchiDisplay();

    void init();
    void deinit();
    void update(const PwnagotchiStats& stats, MoodState mood);
    void displayWiFiStats(const PwnagotchiStats& stats);

  private:
    Adafruit_SSD1306* oled;
    uint8_t sda_pin;
    uint8_t scl_pin;

    void drawFace(MoodState mood);
    void drawMoodHappy();
    void drawMoodExcited();
    void drawMoodSad();
    void drawMoodSleeping();
    void drawMoodIdle();
};

extern PwnagotchiDisplay pwnagotchi_display;

#endif
