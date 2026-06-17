#include "PwnagotchiWiFiCore.h"
#include "esp_random.h"
#include <string.h>

PwnagotchiWiFiCore wifi_core;

static PwnagotchiWiFiCore* g_wifi_core_instance = nullptr;

static void globalBeaconSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (g_wifi_core_instance) {
        g_wifi_core_instance->beaconSnifferCallbackStatic(buf, type);
    }
}

PwnagotchiWiFiCore::PwnagotchiWiFiCore()
    : current_channel(1), packets_received(0), aps_discovered(0),
      clients_discovered(0), handshakes_captured(0),
      last_channel_hop(0), last_ap_discovered(0) {
    g_wifi_core_instance = this;
}

PwnagotchiWiFiCore::~PwnagotchiWiFiCore() {
    deinit();
}

void PwnagotchiWiFiCore::init() {
    Serial.println(F("[WiFiCore] Initializing WiFi core..."));

    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();

    wifi_promiscuous_filter_t filt = {};
    filt.mask = WIFI_PROMISCUOUS_FILTER_MASK_MGMT;
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&globalBeaconSnifferCallback);

    setChannel(current_channel);
    Serial.println(F("[WiFiCore] Initialization complete"));
}

void PwnagotchiWiFiCore::deinit() {
    Serial.println(F("[WiFiCore] Shutting down WiFi core..."));
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
}

void PwnagotchiWiFiCore::startBeaconSniff() {
    Serial.println(F("[WiFiCore] Starting beacon sniffing..."));
    init();
}

void PwnagotchiWiFiCore::stopSniffing() {
    Serial.println(F("[WiFiCore] Stopping WiFi sniffing..."));
    deinit();
}

void PwnagotchiWiFiCore::hopChannel() {
    current_channel++;
    if (current_channel > 13) {
        current_channel = 1;
    }
    setChannel(current_channel);
    last_channel_hop = millis();
}

void PwnagotchiWiFiCore::setChannel(uint8_t channel) {
    if (channel < 1 || channel > 13) {
        Serial.printf("[WiFiCore] Invalid channel: %d\n", channel);
        return;
    }
    current_channel = channel;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

uint8_t PwnagotchiWiFiCore::getCurrentChannel() {
    return current_channel;
}

PwnagotchiStats PwnagotchiWiFiCore::getStats() {
    PwnagotchiStats stats = {
        .total_aps = aps_discovered,
        .total_clients = clients_discovered,
        .total_handshakes = handshakes_captured,
        .current_channel = current_channel,
        .packets_received = packets_received
    };
    return stats;
}

uint32_t PwnagotchiWiFiCore::getAPCount() {
    return aps_discovered;
}

uint32_t PwnagotchiWiFiCore::getClientCount() {
    return clients_discovered;
}

uint32_t PwnagotchiWiFiCore::getHandshakeCount() {
    return handshakes_captured;
}

void PwnagotchiWiFiCore::beaconSnifferCallbackStatic(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type == WIFI_PKT_MGMT) {
        uint8_t* frame = (uint8_t*)buf;
        uint16_t length = 0;

        // Extract frame length from the wifi_pkt_rx_ctrl_t structure
        // The frame data starts after the header
        wifi_pkt_rx_ctrl_t* rx_ctrl = (wifi_pkt_rx_ctrl_t*)frame;
        length = rx_ctrl->sig_len;

        // Frame data starts at offset 4 from buf
        uint8_t* frame_data = frame + 4;

        packets_received++;

        // Parse frame type
        uint8_t frame_control = frame_data[0];
        uint8_t frame_type = (frame_control >> 2) & 0x3;
        uint8_t frame_subtype = (frame_control >> 4) & 0xF;

        // Type 0 = Management frame
        if (frame_type == 0) {
            // Subtype 8 = Beacon frame
            if (frame_subtype == 8) {
                parseBeaconFrame(frame_data, length);
            }
            // Subtype 4 = Probe Request
            else if (frame_subtype == 4) {
                parseProbeRequestFrame(frame_data, length);
            }
        }
    }
}

void PwnagotchiWiFiCore::parseBeaconFrame(uint8_t* frame, uint16_t length) {
    if (length < 36) {
        return;
    }

    // Extract BSSID (address 3) from frame header
    uint8_t bssid[6];
    memcpy(bssid, frame + 16, 6);

    // Log beacon
    Serial.printf("[Beacon] BSSID: %02x:%02x:%02x:%02x:%02x:%02x ",
                  bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    Serial.printf("CH:%d PKT:%lu\n", current_channel, packets_received);

    aps_discovered++;
    last_ap_discovered = millis();
}

void PwnagotchiWiFiCore::parseProbeRequestFrame(uint8_t* frame, uint16_t length) {
    if (length < 24) {
        return;
    }

    // Extract source MAC (address 2) from frame header
    uint8_t source_mac[6];
    memcpy(source_mac, frame + 10, 6);

    Serial.printf("[Probe] Source: %02x:%02x:%02x:%02x:%02x:%02x ",
                  source_mac[0], source_mac[1], source_mac[2],
                  source_mac[3], source_mac[4], source_mac[5]);
    Serial.printf("CH:%d\n", current_channel);

    clients_discovered++;
}

void PwnagotchiWiFiCore::parseEAPOLFrame(uint8_t* frame, uint16_t length) {
    // Placeholder for EAPOL parsing (Phase 2+)
    // Will implement handshake detection later
    handshakes_captured++;
}
