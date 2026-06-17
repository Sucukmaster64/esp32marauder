#pragma once

#ifndef PwnagotchiWiFiCore_h
#define PwnagotchiWiFiCore_h

#include <stdint.h>
#include <stdbool.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"

#define PWNAGOTCHI_MAX_APS 100
#define PWNAGOTCHI_MAX_CLIENTS 200
#define PWNAGOTCHI_MAX_HANDSHAKES 50

typedef struct {
    uint8_t bssid[6];
    char ssid[33];
    uint8_t channel;
    int8_t rssi;
    uint32_t first_seen;
    uint32_t last_seen;
    uint8_t auth_mode;
} PwnagotchiAP;

typedef struct {
    uint8_t mac[6];
    uint8_t bssid[6];
    int8_t rssi;
    uint32_t first_seen;
    uint32_t last_seen;
} PwnagotchiClient;

typedef struct {
    uint8_t bssid[6];
    uint8_t client_mac[6];
    uint32_t timestamp;
    uint16_t packet_num;
    uint8_t packet_type;
} PwnagotchiHandshake;

typedef struct {
    uint32_t total_aps;
    uint32_t total_clients;
    uint32_t total_handshakes;
    uint32_t current_channel;
    uint32_t packets_received;
} PwnagotchiStats;

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
    uint8_t current_channel;
    uint32_t packets_received;
    uint32_t aps_discovered;
    uint32_t clients_discovered;
    uint32_t handshakes_captured;

    uint32_t last_channel_hop;
    uint32_t last_ap_discovered;

    static void beaconSnifferCallbackStatic(void* buf, wifi_promiscuous_pkt_type_t type);
    void beaconSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);

    void parseBeaconFrame(uint8_t* frame, uint16_t length);
    void parseProbeRequestFrame(uint8_t* frame, uint16_t length);
    void parseEAPOLFrame(uint8_t* frame, uint16_t length);
};

extern PwnagotchiWiFiCore wifi_core;

#endif
