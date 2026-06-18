#pragma once

#ifndef PcapDumper_h
#define PcapDumper_h

#include <stdint.h>
#include <string.h>

#define PCAP_MAX_HANDSHAKES 20

typedef struct {
    uint8_t  bssid[6];
    uint8_t  client_mac[6];
    uint32_t timestamp_ms;
    uint8_t  packet_count;
    uint32_t id;
} HandshakeRecord;

class PcapDumper {
  public:
    PcapDumper();

    void addEAPOLPacket(const uint8_t* bssid, const uint8_t* client_mac);
    uint32_t getHandshakeCount() const;
    void printHandshakes() const;
    void clear();

  private:
    HandshakeRecord records[PCAP_MAX_HANDSHAKES];
    uint32_t        count;
    uint32_t        next_id;

    int findRecord(const uint8_t* bssid, const uint8_t* client_mac) const;
};

extern PcapDumper pcap_dumper;

#endif
