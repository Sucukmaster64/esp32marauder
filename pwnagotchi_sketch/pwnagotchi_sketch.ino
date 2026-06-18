/*
 * ESP32-S3 Pwnagotchi Clone
 * Target: XIAO ESP32-S3 + SSD1306 128x64 OLED (I2C: SDA=12, SCL=13 hardwired)
 *
 * Optional WebUI: add -DENABLE_WEBUI to compiler flags and install AsyncTCP lib.
 * Connect to AP "pwnagotchi-clone" then open http://192.168.4.1
 */

#include "PwnagotchiTypes.h"
#include "PwnagotchiWiFiCore.h"
#include "PwnagotchiDisplay.h"
#include "MoodEngine.h"
#include "RLChannelOptimizer.h"
#include "PcapDumper.h"
#include "AllowlistManager.h"
#include "WebUI.h"

#define CHANNEL_HOP_MS  100
#define DISPLAY_UPDATE_MS 250
#define STATS_PRINT_MS  5000

static unsigned long t_hop     = 0;
static unsigned long t_display = 0;
static unsigned long t_stats   = 0;
static unsigned long t_start   = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println(F("\n================================"));
    Serial.println(F("  ESP32-S3 Pwnagotchi Clone"));
    Serial.println(F("================================"));

    t_start = millis();

    pwnagotchi_display.init();
    wifi_core.startBeaconSniff();
    rl_optimizer.init();

#ifdef ENABLE_WEBUI
    web_ui.begin();
#endif

    Serial.println(F("Ready. Type 'help' for commands."));
}

void loop() {
    unsigned long now = millis();

    if (now - t_hop >= CHANNEL_HOP_MS) {
        PwnagotchiStats s = wifi_core.getStats();
        uint8_t ch = rl_optimizer.getNextChannel(s);
        wifi_core.setChannel(ch);
        t_hop = now;
    }

    if (now - t_display >= DISPLAY_UPDATE_MS) {
        PwnagotchiStats s = wifi_core.getStats();
        mood_engine.update(s);
        pwnagotchi_display.update(s, mood_engine.getCurrentMood());
        t_display = now;
    }

    if (now - t_stats >= STATS_PRINT_MS) {
        printStats();
        t_stats = now;
    }

    if (Serial.available()) handleCommand();

    delay(5);
}

void printStats() {
    PwnagotchiStats s = wifi_core.getStats();
    Serial.printf("\n[Stats] up=%lus ch=%lu ap=%lu cl=%lu hs=%lu pkt=%lu mood=%d\n",
                  (millis() - t_start) / 1000,
                  s.current_channel, s.total_aps, s.total_clients,
                  s.total_handshakes, s.packets_received,
                  mood_engine.getCurrentMood());
}

void handleCommand() {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "help") {
        Serial.println(F("Commands:"));
        Serial.println(F("  stats               - print statistics"));
        Serial.println(F("  ch <1-13>           - set channel"));
        Serial.println(F("  mood <0-4>          - override mood"));
        Serial.println(F("  policy rr|rl        - set channel policy"));
        Serial.println(F("  allowlist list      - show allowlist"));
        Serial.println(F("  handshakes          - list captured handshakes"));
        Serial.println(F("  reboot              - restart"));
    }
    else if (cmd == "stats") {
        printStats();
    }
    else if (cmd.startsWith("ch ")) {
        int ch = cmd.substring(3).toInt();
        if (ch >= 1 && ch <= 13) { wifi_core.setChannel(ch); Serial.printf("CH→%d\n", ch); }
        else Serial.println(F("Invalid channel (1-13)"));
    }
    else if (cmd.startsWith("mood ")) {
        int m = cmd.substring(5).toInt();
        if (m >= 0 && m <= 4) { mood_engine.overrideMood((MoodState)m); Serial.printf("Mood→%d\n", m); }
        else Serial.println(F("Invalid mood (0-4)"));
    }
    else if (cmd == "policy rr") {
        rl_optimizer.setPolicy(POLICY_ROUND_ROBIN);
    }
    else if (cmd == "policy rl") {
        rl_optimizer.setPolicy(POLICY_RL);
    }
    else if (cmd == "allowlist list") {
        allowlist.listEntries();
    }
    else if (cmd == "handshakes") {
        pcap_dumper.printHandshakes();
    }
    else if (cmd == "reboot") {
        Serial.println(F("Rebooting..."));
        delay(100);
        ESP.restart();
    }
    else if (cmd.length() > 0) {
        Serial.println(F("Unknown command. Type 'help'."));
    }
}
