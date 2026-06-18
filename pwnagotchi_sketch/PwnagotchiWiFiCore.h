#pragma once

#ifndef PwnagotchiWiFiCore_h
#define PwnagotchiWiFiCore_h

#include <stdint.h>
#include "PwnagotchiTypes.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

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

  private:
    uint8_t  current_channel;
    uint32_t packets_received;
    uint32_t aps_discovered;
    uint32_t clients_discovered;
    uint32_t handshakes_captured;
    uint32_t last_channel_hop;

    // Instance method — NOT static; called via global pointer
    void snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);

    void parseBeaconFrame(const uint8_t* frame, uint16_t length);
    void parseProbeRequestFrame(const uint8_t* frame, uint16_t length);

    // Free function registered with IDF; forwards to the singleton
    friend void _pwnCoreCallback(void* buf, wifi_promiscuous_pkt_type_t type);
};

extern PwnagotchiWiFiCore wifi_core;

#endif
