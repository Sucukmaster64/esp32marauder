#include "PcapDumper.h"
#include <string.h>
#include <esp_timer.h>

PcapDumper pcap_dumper;

PcapDumper::PcapDumper()
    : handshake_count(0), next_handshake_id(1) {
    memset(handshakes, 0, sizeof(handshakes));
}

PcapDumper::~PcapDumper() {
    deinit();
}

void PcapDumper::init() {
    Serial.println(F("[PcapDumper] Initializing PCAP dumper..."));
    handshake_count = 0;
    next_handshake_id = 1;
}

void PcapDumper::deinit() {
    Serial.println(F("[PcapDumper] Shutting down PCAP dumper..."));
    // Placeholder: Would save to NVS/SPIFFS here
}

void PcapDumper::addEAPOLPacket(const uint8_t* packet, uint16_t length,
                                 const uint8_t* bssid, const uint8_t* client_mac) {
    if (!isValidEAPOLFrame(packet, length)) {
        return;
    }

    // Check if this is a new handshake or part of existing
    uint32_t handshake_id = 0;
    bool found_existing = false;

    for (uint32_t i = 0; i < handshake_count; i++) {
        if (memcmp(handshakes[i].bssid, bssid, 6) == 0 &&
            memcmp(handshakes[i].client_mac, client_mac, 6) == 0) {
            handshake_id = handshakes[i].handshake_id;
            handshakes[i].packet_count++;
            found_existing = true;
            break;
        }
    }

    // New handshake
    if (!found_existing && handshake_count < MAX_HANDSHAKES_STORED) {
        handshake_id = next_handshake_id++;
        memcpy(handshakes[handshake_count].bssid, bssid, 6);
        memcpy(handshakes[handshake_count].client_mac, client_mac, 6);
        handshakes[handshake_count].timestamp = millis();
        handshakes[handshake_count].packet_count = 1;
        handshakes[handshake_count].handshake_id = handshake_id;
        handshake_count++;

        Serial.printf("[PcapDumper] Captured handshake #%lu: ", handshake_id);
        Serial.printf("BSSID %02x:%02x:%02x:%02x:%02x:%02x <-> ",
                      bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        Serial.printf("Client %02x:%02x:%02x:%02x:%02x:%02x\n",
                      client_mac[0], client_mac[1], client_mac[2],
                      client_mac[3], client_mac[4], client_mac[5]);
    }

    // Complete handshake when we have 4 EAPOL frames
    if (found_existing && handshakes[handshake_count - 1].packet_count >= 4) {
        Serial.printf("[PcapDumper] Handshake #%lu complete (4/4 packets)\n", handshake_id);
    }
}

bool PcapDumper::isValidEAPOLFrame(const uint8_t* packet, uint16_t length) {
    // Minimal EAPOL validation
    // EAPOL frames have specific structure:
    // - Frame type: 0x88 0xB7 (802.1X Authentication)
    // - Protocol version in frame
    // This is a simplified check; full validation in production

    if (length < 20) {
        return false;
    }

    // Check for 802.1X (EAPOL) type: 0x88B7
    // Would need full frame parsing here; placeholder
    return true;
}

void PcapDumper::extractEAPOLInfo(const uint8_t* packet, uint16_t length,
                                   uint8_t* bssid, uint8_t* client_mac) {
    // Placeholder: Extract BSSID and client MAC from frame
    // In real implementation, would parse 802.11 header
    if (bssid && length >= 16) {
        memcpy(bssid, packet + 10, 6);
    }
    if (client_mac && length >= 22) {
        memcpy(client_mac, packet + 16, 6);
    }
}

void PcapDumper::saveHandshakeToPcap(const HandshakeEntry& entry) {
    // Placeholder for SPIFFS/LittleFS PCAP file writing
    // Would serialize to PCAP format and write to Flash
    Serial.printf("[PcapDumper] Would save handshake #%lu to PCAP file\n",
                  entry.handshake_id);
}

void PcapDumper::exportPcapFile(const char* filename) {
    Serial.printf("[PcapDumper] Would export to %s (not yet implemented)\n", filename);
    // Placeholder for USB/WiFi export
}

uint32_t PcapDumper::getHandshakeCount() {
    return handshake_count;
}

void PcapDumper::printHandshakes() {
    Serial.println(F("[PcapDumper] Captured handshakes:"));
    for (uint32_t i = 0; i < handshake_count; i++) {
        Serial.printf("  #%lu: ", handshakes[i].handshake_id);
        Serial.printf("BSSID %02x:%02x:%02x:%02x:%02x:%02x <-> ",
                      handshakes[i].bssid[0], handshakes[i].bssid[1],
                      handshakes[i].bssid[2], handshakes[i].bssid[3],
                      handshakes[i].bssid[4], handshakes[i].bssid[5]);
        Serial.printf("Client %02x:%02x:%02x:%02x:%02x:%02x ",
                      handshakes[i].client_mac[0], handshakes[i].client_mac[1],
                      handshakes[i].client_mac[2], handshakes[i].client_mac[3],
                      handshakes[i].client_mac[4], handshakes[i].client_mac[5]);
        Serial.printf("(%d packets)\n", handshakes[i].packet_count);
    }
}

void PcapDumper::clearHandshakes() {
    handshake_count = 0;
    next_handshake_id = 1;
    Serial.println(F("[PcapDumper] Handshakes cleared"));
}
