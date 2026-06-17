#include "PwnagotchiDisplay.h"

PwnagotchiDisplay display;

PwnagotchiDisplay::PwnagotchiDisplay(uint8_t sda, uint8_t scl)
    : sda_pin(sda), scl_pin(scl), display(nullptr),
      current_mood(MOOD_IDLE), last_ap_count(0), last_handshake_count(0),
      last_mood_update(0), idle_timer_start(millis()) {
}

PwnagotchiDisplay::~PwnagotchiDisplay() {
    deinit();
}

void PwnagotchiDisplay::init() {
    Serial.println(F("[Display] Initializing OLED display..."));

    // Initialize I2C on specified pins
    Wire.begin(sda_pin, scl_pin);
    delay(100);

    // Create display object with address 0x3C (adjust if needed)
    display = new Adafruit_SSD1306(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

    if (!display->begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println(F("[Display] ERROR: Failed to initialize SSD1306!"));
        delete display;
        display = nullptr;
        return;
    }

    // Clear display and set text properties
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);

    // Show startup message
    display->println("Pwnagotchi Init...");
    display->display();

    Serial.println(F("[Display] OLED initialized successfully"));
    delay(500);
}

void PwnagotchiDisplay::deinit() {
    if (display) {
        display->clearDisplay();
        display->display();
        delete display;
        display = nullptr;
    }
    Serial.println(F("[Display] Display shut down"));
}

void PwnagotchiDisplay::update(const PwnagotchiStats& wifi_stats) {
    if (!display) return;

    unsigned long now = millis();
    if (now - last_mood_update < DISPLAY_UPDATE_INTERVAL) {
        return;
    }
    last_mood_update = now;

    // Update mood based on WiFi stats
    updateMood(wifi_stats);

    // Clear display
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);

    // Top line: Current mood
    switch (current_mood) {
        case MOOD_HAPPY:
            display->println("[:-)  HAPPY");
            break;
        case MOOD_EXCITED:
            display->println("[o_o  EXCITED");
            break;
        case MOOD_SAD:
            display->println("[:-( SAD");
            break;
        case MOOD_SLEEPING:
            display->println("[-_- SLEEPING");
            break;
        case MOOD_IDLE:
        default:
            display->println("[... IDLE");
            break;
    }

    display->println("");

    // Display statistics
    display->print("CH:");
    display->println(wifi_stats.current_channel);

    display->print("APs:");
    display->println(wifi_stats.total_aps);

    display->print("Clients:");
    display->println(wifi_stats.total_clients);

    display->print("Handshakes:");
    display->println(wifi_stats.total_handshakes);

    display->print("Packets:");
    display->println(wifi_stats.packets_received);

    // Render to display
    display->display();
}

void PwnagotchiDisplay::updateMood(const PwnagotchiStats& wifi_stats) {
    unsigned long now = millis();

    // Check for recent AP discovery
    if (wifi_stats.total_aps > last_ap_count) {
        current_mood = MOOD_HAPPY;
        idle_timer_start = now;
        last_ap_count = wifi_stats.total_aps;
        return;
    }

    // Check for recent handshake
    if (wifi_stats.total_handshakes > last_handshake_count) {
        current_mood = MOOD_EXCITED;
        idle_timer_start = now;
        last_handshake_count = wifi_stats.total_handshakes;
        return;
    }

    // Check for idle timeout (30 seconds)
    if (now - idle_timer_start > 30000) {
        current_mood = MOOD_SAD;
    } else {
        current_mood = MOOD_IDLE;
    }
}

void PwnagotchiDisplay::displayMood(MoodState mood, const StateVector& state) {
    if (!display) return;

    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);

    switch (mood) {
        case MOOD_HAPPY:
            display->println("HAPPY");
            break;
        case MOOD_EXCITED:
            display->println("EXCITED");
            break;
        case MOOD_SAD:
            display->println("SAD");
            break;
        case MOOD_SLEEPING:
            display->println("SLEEPING");
            break;
        default:
            display->println("IDLE");
    }

    display->println("");
    display->print("APs: ");
    display->println(state.seenAPCount);
    display->print("New: ");
    display->println(state.newAPCount);

    display->display();
}

void PwnagotchiDisplay::displayWiFiStats(const PwnagotchiStats& wifi_stats) {
    if (!display) return;

    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);

    display->println("=== WiFi Stats ===");
    display->print("Ch: ");
    display->println(wifi_stats.current_channel);
    display->print("APs: ");
    display->println(wifi_stats.total_aps);
    display->print("Clients: ");
    display->println(wifi_stats.total_clients);
    display->print("Handshakes: ");
    display->println(wifi_stats.total_handshakes);
    display->print("Packets: ");
    display->println(wifi_stats.packets_received);

    display->display();
}

MoodState PwnagotchiDisplay::getCurrentMood() {
    return current_mood;
}

void PwnagotchiDisplay::setMood(MoodState mood) {
    current_mood = mood;
}

// Placeholder functions for pixel-based face drawing (Phase 3)
void PwnagotchiDisplay::drawMoodHappy() {
    // Will implement pixel faces in Phase 3
}

void PwnagotchiDisplay::drawMoodExcited() {
    // Will implement pixel faces in Phase 3
}

void PwnagotchiDisplay::drawMoodSad() {
    // Will implement pixel faces in Phase 3
}

void PwnagotchiDisplay::drawMoodSleeping() {
    // Will implement pixel faces in Phase 3
}

void PwnagotchiDisplay::drawMoodIdle() {
    // Will implement pixel faces in Phase 3
}

void PwnagotchiDisplay::drawSimpleFace() {
    // Will implement in Phase 3
}

void PwnagotchiDisplay::drawChannel(uint8_t channel) {
    // Will implement in Phase 3
}

void PwnagotchiDisplay::drawAPCount(uint32_t count) {
    // Will implement in Phase 3
}

void PwnagotchiDisplay::drawClientCount(uint32_t count) {
    // Will implement in Phase 3
}

void PwnagotchiDisplay::drawHandshakeCount(uint32_t count) {
    // Will implement in Phase 3
}
