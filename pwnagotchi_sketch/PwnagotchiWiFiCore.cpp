#include "PwnagotchiWiFiCore.h"
#include <WiFi.h>
#include <string.h>

PwnagotchiWiFiCore wifi_core;

static PwnagotchiWiFiCore* g_instance = nullptr;

void _pwnCoreCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (g_instance) g_instance->snifferCallback(buf, type);
}

PwnagotchiWiFiCore::PwnagotchiWiFiCore()
    : current_channel(1), packets_received(0), aps_discovered(0),
      clients_discovered(0), handshakes_captured(0), last_channel_hop(0) {
    g_instance = this;
}

PwnagotchiWiFiCore::~PwnagotchiWiFiCore() {
    deinit();
}

void PwnagotchiWiFiCore::init() {
    Serial.println(F("[WiFiCore] Initializing..."));

    // Use Arduino WiFi API — handles esp_wifi_init() / nvs_flash_init() internally
    WiFi.mode(WIFI_MODE_NULL);

    wifi_promiscuous_filter_t filt = {};
    // Capture management (beacons/probes) and data (EAPOL)
    filt.mask = WIFI_PROMISCUOUS_FILTER_MASK_MGMT | WIFI_PROMISCUOUS_FILTER_MASK_DATA;
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&_pwnCoreCallback);

    setChannel(current_channel);
    Serial.println(F("[WiFiCore] Ready"));
}

void PwnagotchiWiFiCore::deinit() {
    esp_wifi_set_promiscuous(false);
    WiFi.mode(WIFI_OFF);
}

void PwnagotchiWiFiCore::startBeaconSniff() { init(); }
void PwnagotchiWiFiCore::stopSniffing()     { deinit(); }

void PwnagotchiWiFiCore::hopChannel() {
    current_channel = (current_channel >= 13) ? 1 : current_channel + 1;
    setChannel(current_channel);
    last_channel_hop = millis();
}

void PwnagotchiWiFiCore::setChannel(uint8_t channel) {
    if (channel < 1 || channel > 13) return;
    current_channel = channel;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

uint8_t PwnagotchiWiFiCore::getCurrentChannel() { return current_channel; }

PwnagotchiStats PwnagotchiWiFiCore::getStats() {
    PwnagotchiStats s;
    s.total_aps        = aps_discovered;
    s.total_clients    = clients_discovered;
    s.total_handshakes = handshakes_captured;
    s.current_channel  = current_channel;
    s.packets_received = packets_received;
    return s;
}

uint32_t PwnagotchiWiFiCore::getAPCount()        { return aps_discovered; }
uint32_t PwnagotchiWiFiCore::getClientCount()    { return clients_discovered; }
uint32_t PwnagotchiWiFiCore::getHandshakeCount() { return handshakes_captured; }

void PwnagotchiWiFiCore::snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    // Cast to the correct IDF struct — payload[] holds the 802.11 frame
    const wifi_promiscuous_pkt_t* pkt = (const wifi_promiscuous_pkt_t*)buf;
    uint16_t length = pkt->rx_ctrl.sig_len;
    const uint8_t* frame = pkt->payload;

    if (length < 4) return;
    packets_received++;

    uint8_t frame_type    = (frame[0] >> 2) & 0x3;
    uint8_t frame_subtype = (frame[0] >> 4) & 0xF;

    if (type == WIFI_PKT_MGMT && frame_type == 0) {
        if (frame_subtype == 8)      parseBeaconFrame(frame, length);
        else if (frame_subtype == 4) parseProbeRequestFrame(frame, length);
    }
}

void PwnagotchiWiFiCore::parseBeaconFrame(const uint8_t* frame, uint16_t length) {
    if (length < 36) return;

    // BSSID is address-3 in beacon frames (offset 16 in 802.11 MAC header)
    Serial.printf("[Beacon] BSSID: %02x:%02x:%02x:%02x:%02x:%02x CH:%d\n",
                  frame[16], frame[17], frame[18],
                  frame[19], frame[20], frame[21],
                  current_channel);
    aps_discovered++;
}

void PwnagotchiWiFiCore::parseProbeRequestFrame(const uint8_t* frame, uint16_t length) {
    if (length < 24) return;

    // Source MAC is address-2 (offset 10 in 802.11 MAC header)
    Serial.printf("[Probe]  SRC: %02x:%02x:%02x:%02x:%02x:%02x CH:%d\n",
                  frame[10], frame[11], frame[12],
                  frame[13], frame[14], frame[15],
                  current_channel);
    clients_discovered++;
}
