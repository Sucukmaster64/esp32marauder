#include "PwnagotchiDisplay.h"

PwnagotchiDisplay pwnagotchi_display;

PwnagotchiDisplay::PwnagotchiDisplay(uint8_t sda, uint8_t scl)
    : oled(nullptr), sda_pin(sda), scl_pin(scl) {}

PwnagotchiDisplay::~PwnagotchiDisplay() { deinit(); }

void PwnagotchiDisplay::init() {
    Wire.begin(sda_pin, scl_pin);  // SDA=12, SCL=13 — hardwired, not ESP32-S3 defaults
    delay(50);

    oled = new Adafruit_SSD1306(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
    if (!oled->begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println(F("[Display] SSD1306 not found — continuing without display"));
        delete oled;
        oled = nullptr;
        return;
    }

    oled->clearDisplay();
    oled->setTextColor(SSD1306_WHITE);
    oled->setTextSize(1);
    oled->setCursor(0, 0);
    oled->println("Pwnagotchi init...");
    oled->display();
    Serial.println(F("[Display] OLED ready"));
}

void PwnagotchiDisplay::deinit() {
    if (oled) {
        oled->clearDisplay();
        oled->display();
        delete oled;
        oled = nullptr;
    }
}

void PwnagotchiDisplay::update(const PwnagotchiStats& stats, MoodState mood) {
    if (!oled) return;

    oled->clearDisplay();

    // Left half: pixel face
    drawFace(mood);

    // Right half: stats
    oled->setTextSize(1);
    oled->setCursor(68, 0);  oled->print("CH:"); oled->println(stats.current_channel);
    oled->setCursor(68, 10); oled->print("AP:"); oled->println(stats.total_aps);
    oled->setCursor(68, 20); oled->print("CL:"); oled->println(stats.total_clients);
    oled->setCursor(68, 30); oled->print("HS:"); oled->println(stats.total_handshakes);
    oled->setCursor(68, 40); oled->print("PK:"); oled->println(stats.packets_received);

    // Mood label bottom-left
    oled->setCursor(0, 55);
    const char* labels[] = {"IDLE", "HAPPY", "EXCITED", "SAD", "SLEEP"};
    oled->println(labels[mood]);

    oled->display();
}

void PwnagotchiDisplay::displayWiFiStats(const PwnagotchiStats& stats) {
    if (!oled) return;
    oled->clearDisplay();
    oled->setTextSize(1);
    oled->setCursor(0, 0);
    oled->println("=== WiFi Stats ===");
    oled->print("Ch:  "); oled->println(stats.current_channel);
    oled->print("APs: "); oled->println(stats.total_aps);
    oled->print("Cls: "); oled->println(stats.total_clients);
    oled->print("HS:  "); oled->println(stats.total_handshakes);
    oled->print("Pkt: "); oled->println(stats.packets_received);
    oled->display();
}

void PwnagotchiDisplay::drawFace(MoodState mood) {
    switch (mood) {
        case MOOD_HAPPY:    drawMoodHappy();    break;
        case MOOD_EXCITED:  drawMoodExcited();  break;
        case MOOD_SAD:      drawMoodSad();      break;
        case MOOD_SLEEPING: drawMoodSleeping(); break;
        default:            drawMoodIdle();     break;
    }
}

void PwnagotchiDisplay::drawMoodHappy() {
    // Eyes
    oled->fillRect(14, 14, 5, 5, SSD1306_WHITE);
    oled->fillRect(32, 14, 5, 5, SSD1306_WHITE);
    // Smile: parabola opening downward from (10,32) to (42,32)
    for (int x = 10; x <= 42; x++) {
        int y = 38 - (x - 10) * (42 - x) / 64;
        oled->drawPixel(x, y, SSD1306_WHITE);
    }
}

void PwnagotchiDisplay::drawMoodExcited() {
    // Wide eyes
    oled->drawCircle(16, 16, 5, SSD1306_WHITE);
    oled->fillCircle(16, 16, 2, SSD1306_WHITE);
    oled->drawCircle(35, 16, 5, SSD1306_WHITE);
    oled->fillCircle(35, 16, 2, SSD1306_WHITE);
    // Open mouth (circle)
    oled->drawCircle(25, 38, 6, SSD1306_WHITE);
}

void PwnagotchiDisplay::drawMoodSad() {
    // Eyes
    oled->fillRect(14, 14, 5, 5, SSD1306_WHITE);
    oled->fillRect(32, 14, 5, 5, SSD1306_WHITE);
    // Inverted smile (frown)
    for (int x = 10; x <= 42; x++) {
        int y = 32 + (x - 10) * (42 - x) / 64;
        oled->drawPixel(x, y, SSD1306_WHITE);
    }
}

void PwnagotchiDisplay::drawMoodSleeping() {
    // Closed eyes (lines)
    oled->drawLine(12, 17, 20, 17, SSD1306_WHITE);
    oled->drawLine(30, 17, 38, 17, SSD1306_WHITE);
    // Flat mouth
    oled->drawLine(16, 37, 34, 37, SSD1306_WHITE);
    // ZZZ
    oled->setCursor(40, 5);
    oled->print("z");
    oled->setCursor(46, 0);
    oled->print("Z");
}

void PwnagotchiDisplay::drawMoodIdle() {
    // Dot eyes
    oled->fillRect(15, 16, 3, 3, SSD1306_WHITE);
    oled->fillRect(33, 16, 3, 3, SSD1306_WHITE);
    // Flat mouth
    oled->drawLine(18, 35, 32, 35, SSD1306_WHITE);
}
