#pragma once

#ifndef PwnagotchiWiFiCore_h
#define PwnagotchiWiFiCore_h

#include <stdint.h>
#include "PwnagotchiTypes.h"
#include "AllowlistManager.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

#define PWN_CHANNEL_MAX 13

class PwnagotchiWiFiCore {
  public:
    PwnagotchiWiFiCore();
    ~PwnagotchiWiFiCore();

    void init();
    void deinit();
    void startBeaconSniff();
    void stopSniffing();
    void hopChannel();
    void setChannel(uint8_t channel);
    uint8_t getCurrentChannel();

    PwnagotchiStats getStats();
    uint32_t getAPCount();
    uint32_t getClientCount();
    uint32_t getHandshakeCount();

    // Active attack methods — allowlist-gated (blocks if BSSID not listed)
    void sendDeauth(const uint8_t* bssid, uint8_t channel, const uint8_t* mac);
    void sendProbeAttack(const uint8_t* target_bssid, const char* ssid, uint8_t channel);

  private:
    uint8_t  current_channel;
    uint32_t packets_received;
    uint32_t aps_discovered;
    uint32_t clients_discovered;
    uint32_t handshakes_captured;
    uint32_t last_channel_hop;

    // Per-channel activity counters for weighted hop (index 0 = CH1)
    uint8_t  channel_activity[PWN_CHANNEL_MAX];
    uint32_t last_activity_reset;

    // Deauth frame template (matches Marauder's deauth_frame_default layout)
    uint8_t deauth_frame[26];

    void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void parseBeaconFrame(const uint8_t* frame, uint16_t length);
    void parseProbeRequestFrame(const uint8_t* frame, uint16_t length);
    bool detectEapol(const uint8_t* frame, uint16_t length);

    friend void _pwnCoreCallback(void* buf, wifi_promiscuous_pkt_type_t type);
};

extern PwnagotchiWiFiCore wifi_core;

#endif
