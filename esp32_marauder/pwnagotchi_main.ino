/*
 * ESP32-S3 Pwnagotchi Clone - Main Sketch
 *
 * Phase 2: WiFi Core + OLED Display Integration
 *
 * WiFi sniffing with real-time OLED display
 * Mood states based on WiFi activity
 */

#include "PwnagotchiWiFiCore.h"
#include "PwnagotchiDisplay.h"

// Timing constants (milliseconds)
#define CHANNEL_HOP_INTERVAL 100
#define STATS_PRINT_INTERVAL 5000
#define DISPLAY_UPDATE_INTERVAL 250

unsigned long last_channel_hop = 0;
unsigned long last_stats_print = 0;
unsigned long last_display_update = 0;
unsigned long startup_time = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n================================");
    Serial.println("  ESP32-S3 Pwnagotchi Clone");
    Serial.println("  Phase 2: WiFi + Display");
    Serial.println("================================\n");

    startup_time = millis();

    // Initialize display
    display.init();
    delay(500);

    // Initialize WiFi core
    wifi_core.startBeaconSniff();

    Serial.println("Ready for WiFi scanning!");
    Serial.println("Commands: 'help' for usage");
}

void loop() {
    unsigned long now = millis();

    // Channel hopping
    if (now - last_channel_hop >= CHANNEL_HOP_INTERVAL) {
        wifi_core.hopChannel();
        last_channel_hop = now;
    }

    // Update display
    if (now - last_display_update >= DISPLAY_UPDATE_INTERVAL) {
        PwnagotchiStats stats = wifi_core.getStats();
        display.update(stats);
        last_display_update = now;
    }

    // Print statistics periodically to Serial
    if (now - last_stats_print >= STATS_PRINT_INTERVAL) {
        printStats();
        last_stats_print = now;
    }

    // Check for serial commands
    if (Serial.available()) {
        handleSerialCommand();
    }

    delay(10);
}

void printStats() {
    unsigned long uptime = (millis() - startup_time) / 1000;
    PwnagotchiStats stats = wifi_core.getStats();

    Serial.println("\n=== WiFi Statistics ===");
    Serial.printf("Uptime: %lu seconds\n", uptime);
    Serial.printf("Channel: %d\n", stats.current_channel);
    Serial.printf("Packets: %lu\n", stats.packets_received);
    Serial.printf("Unique APs: %lu\n", stats.total_aps);
    Serial.printf("Unique Clients: %lu\n", stats.total_clients);
    Serial.printf("Handshakes: %lu\n", stats.total_handshakes);
    Serial.println("=======================\n");
}

void handleSerialCommand() {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "help") {
        printHelp();
    }
    else if (cmd == "stats") {
        printStats();
    }
    else if (cmd.startsWith("channel ")) {
        int ch = cmd.substring(8).toInt();
        if (ch >= 1 && ch <= 13) {
            wifi_core.setChannel(ch);
            Serial.printf("Channel set to %d\n", ch);
        } else {
            Serial.println("Invalid channel (1-13)");
        }
    }
    else if (cmd == "hop") {
        wifi_core.hopChannel();
        Serial.printf("Hopped to channel %d\n", wifi_core.getCurrentChannel());
    }
    else if (cmd == "display") {
        PwnagotchiStats stats = wifi_core.getStats();
        display.displayWiFiStats(stats);
        Serial.println("Displaying WiFi stats on OLED");
    }
    else if (cmd.startsWith("mood ")) {
        int mood = cmd.substring(5).toInt();
        if (mood >= 0 && mood <= 4) {
            display.setMood((MoodState)mood);
            Serial.printf("Mood set to %d\n", mood);
        } else {
            Serial.println("Invalid mood (0-4)");
        }
    }
    else if (cmd == "stop") {
        wifi_core.stopSniffing();
        display.deinit();
        Serial.println("WiFi sniffing and display stopped");
    }
    else if (cmd == "start") {
        wifi_core.startBeaconSniff();
        display.init();
        Serial.println("WiFi sniffing and display started");
    }
    else if (cmd == "reboot") {
        Serial.println("Rebooting...");
        delay(100);
        ESP.restart();
    }
    else if (cmd.length() > 0) {
        Serial.println("Unknown command. Type 'help' for usage.");
    }
}

void printHelp() {
    Serial.println("\n=== Pwnagotchi Commands ===");
    Serial.println("help              - Show this message");
    Serial.println("stats             - Print WiFi statistics");
    Serial.println("channel <1-13>    - Jump to specific channel");
    Serial.println("hop               - Manual channel hop");
    Serial.println("display           - Show stats on OLED");
    Serial.println("mood <0-4>        - Set mood (0=IDLE, 1=HAPPY, 2=EXCITED, 3=SAD, 4=SLEEPING)");
    Serial.println("start             - Start WiFi sniffing");
    Serial.println("stop              - Stop WiFi sniffing");
    Serial.println("reboot            - Restart device");
    Serial.println("=============================\n");
}
