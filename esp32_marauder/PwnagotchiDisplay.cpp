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
    display->setTextColor(SSD1306_WHITE);

    // Draw pixel face (left side: columns 0-50)
    drawSimpleFace();

    // Draw statistics on right side (columns 55+)
    display->setTextSize(1);
    display->setCursor(55, 0);
    display->print("CH:");
    display->println(wifi_stats.current_channel);

    display->setCursor(55, 10);
    display->print("AP:");
    display->println(wifi_stats.total_aps);

    display->setCursor(55, 20);
    display->print("CL:");
    display->println(wifi_stats.total_clients);

    display->setCursor(55, 30);
    display->print("HS:");
    display->println(wifi_stats.total_handshakes);

    display->setCursor(55, 40);
    display->print("PKT:");
    display->println(wifi_stats.packets_received);

    // Draw mood label below face
    display->setTextSize(1);
    display->setCursor(0, 50);
    switch (current_mood) {
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
            display->println("SLEEP");
            break;
        case MOOD_IDLE:
        default:
            display->println("IDLE");
            break;
    }

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

// Pixel-based face drawing (Phase 3)
void PwnagotchiDisplay::drawMoodHappy() {
    // Happy face: :-)
    // Eyes at (20, 20) and (40, 20)
    // Smile arc from (15, 35) to (45, 35)
    if (!display) return;

    // Draw eyes
    display->fillRect(18, 18, 4, 4, SSD1306_WHITE);  // Left eye
    display->fillRect(38, 18, 4, 4, SSD1306_WHITE);  // Right eye

    // Draw smile (using horizontal line for simplicity)
    for (int x = 15; x < 45; x++) {
        int y = 25 + (x - 15) * (x - 45) / 900;  // Parabolic curve
        display->drawPixel(x, y + 8, SSD1306_WHITE);
    }
}

void PwnagotchiDisplay::drawMoodExcited() {
    // Excited face: o_o
    // Large wide eyes, mouth open (O shape)
    if (!display) return;

    // Draw wide eyes
    display->drawCircle(20, 20, 4, SSD1306_WHITE);   // Left eye (filled)
    display->fillCircle(20, 20, 2, SSD1306_WHITE);
    display->drawCircle(40, 20, 4, SSD1306_WHITE);   // Right eye (filled)
    display->fillCircle(40, 20, 2, SSD1306_WHITE);

    // Draw surprised mouth (circle)
    display->drawCircle(30, 40, 5, SSD1306_WHITE);
    display->fillCircle(30, 40, 3, SSD1306_WHITE);
}

void PwnagotchiDisplay::drawMoodSad() {
    // Sad face: :-(
    // Eyes at (20, 20) and (40, 20)
    // Sad mouth (inverted smile)
    if (!display) return;

    // Draw eyes
    display->fillRect(18, 18, 4, 4, SSD1306_WHITE);  // Left eye
    display->fillRect(38, 18, 4, 4, SSD1306_WHITE);  // Right eye

    // Draw sad mouth (inverted parabola)
    for (int x = 15; x < 45; x++) {
        int y = 32 - (x - 15) * (x - 45) / 900;  // Inverted parabolic curve
        display->drawPixel(x, y + 8, SSD1306_WHITE);
    }
}

void PwnagotchiDisplay::drawMoodSleeping() {
    // Sleeping face: -_-
    // Closed eyes (horizontal lines)
    // Peaceful mouth (small line)
    if (!display) return;

    // Draw closed eyes
    display->drawLine(16, 20, 24, 20, SSD1306_WHITE);  // Left eye (-)
    display->drawLine(36, 20, 44, 20, SSD1306_WHITE);  // Right eye (-)

    // Draw peaceful mouth
    display->drawLine(20, 40, 40, 40, SSD1306_WHITE);
}

void PwnagotchiDisplay::drawMoodIdle() {
    // Idle face: ...
    // Neutral eyes (dots)
    // Neutral mouth (straight line)
    if (!display) return;

    // Draw neutral eyes (dots)
    display->fillRect(19, 19, 2, 2, SSD1306_WHITE);   // Left eye
    display->fillRect(39, 19, 2, 2, SSD1306_WHITE);   // Right eye

    // Draw neutral mouth
    display->drawLine(25, 38, 35, 38, SSD1306_WHITE);
}

void PwnagotchiDisplay::drawSimpleFace() {
    // Draw face based on current mood
    switch (current_mood) {
        case MOOD_HAPPY:
            drawMoodHappy();
            break;
        case MOOD_EXCITED:
            drawMoodExcited();
            break;
        case MOOD_SAD:
            drawMoodSad();
            break;
        case MOOD_SLEEPING:
            drawMoodSleeping();
            break;
        case MOOD_IDLE:
        default:
            drawMoodIdle();
            break;
    }
}

void PwnagotchiDisplay::drawChannel(uint8_t channel) {
    if (!display) return;
    display->print("CH:");
    display->println(channel);
}

void PwnagotchiDisplay::drawAPCount(uint32_t count) {
    if (!display) return;
    display->print("APs:");
    display->println(count);
}

void PwnagotchiDisplay::drawClientCount(uint32_t count) {
    if (!display) return;
    display->print("Clients:");
    display->println(count);
}

void PwnagotchiDisplay::drawHandshakeCount(uint32_t count) {
    if (!display) return;
    display->print("HS:");
    display->println(count);
}
