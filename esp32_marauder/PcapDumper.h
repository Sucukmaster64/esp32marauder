#pragma once

#ifndef PcapDumper_h
#define PcapDumper_h

#include <stdint.h>
#include <stdio.h>

#define PCAP_MAGIC 0xa1b2c3d4
#define PCAP_VERSION_MAJOR 2
#define PCAP_VERSION_MINOR 4
#define PCAP_MAX_PACKET 65535
#define MAX_HANDSHAKE_PACKETS 4
#define MAX_HANDSHAKES_STORED 20

typedef struct {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
} pcap_pkthdr;

typedef struct {
    uint8_t magic[4];
    uint16_t version_major;
    uint16_t version_minor;
    int32_t thiszone;
    uint32_t sigfigs;
    uint32_t snaplen;
    uint32_t network;
} pcap_file_hdr;

typedef struct {
    uint8_t bssid[6];
    uint8_t client_mac[6];
    uint32_t timestamp;
    uint16_t packet_count;
    uint32_t handshake_id;
} HandshakeEntry;

class PcapDumper {
  public:
    PcapDumper();
    ~PcapDumper();

    void init();
    void deinit();
    void addEAPOLPacket(const uint8_t* packet, uint16_t length,
                        const uint8_t* bssid, const uint8_t* client_mac);
    void saveHandshakeToPcap(const HandshakeEntry& entry);
    void exportPcapFile(const char* filename);

    uint32_t getHandshakeCount();
    void printHandshakes();
    void clearHandshakes();

  private:
    HandshakeEntry handshakes[MAX_HANDSHAKES_STORED];
    uint32_t handshake_count;
    uint32_t next_handshake_id;

    bool isValidEAPOLFrame(const uint8_t* packet, uint16_t length);
    void extractEAPOLInfo(const uint8_t* packet, uint16_t length,
                          uint8_t* bssid, uint8_t* client_mac);
};

extern PcapDumper pcap_dumper;

#endif
