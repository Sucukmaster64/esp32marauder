#include "PcapDumper.h"

PcapDumper pcap_dumper;

PcapDumper::PcapDumper() : count(0), next_id(1) {
    memset(records, 0, sizeof(records));
}

void PcapDumper::addEAPOLPacket(const uint8_t* bssid, const uint8_t* client_mac) {
    int idx = findRecord(bssid, client_mac);

    if (idx >= 0) {
        records[idx].packet_count++;
        if (records[idx].packet_count == 4) {
            Serial.printf("[PCAP] Handshake #%lu complete (4/4)\n", records[idx].id);
        }
        return;
    }

    if (count >= PCAP_MAX_HANDSHAKES) return;

    HandshakeRecord& r = records[count++];
    memcpy(r.bssid,      bssid,      6);
    memcpy(r.client_mac, client_mac, 6);
    r.timestamp_ms = millis();
    r.packet_count = 1;
    r.id           = next_id++;

    Serial.printf("[PCAP] New handshake #%lu: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  r.id, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
}

uint32_t PcapDumper::getHandshakeCount() const { return count; }

void PcapDumper::printHandshakes() const {
    if (count == 0) { Serial.println(F("[PCAP] No handshakes captured")); return; }
    for (uint32_t i = 0; i < count; i++) {
        const HandshakeRecord& r = records[i];
        Serial.printf("[PCAP] #%lu AP=%02x:%02x:%02x:%02x:%02x:%02x "
                      "CL=%02x:%02x:%02x:%02x:%02x:%02x pkts=%d\n",
                      r.id,
                      r.bssid[0], r.bssid[1], r.bssid[2],
                      r.bssid[3], r.bssid[4], r.bssid[5],
                      r.client_mac[0], r.client_mac[1], r.client_mac[2],
                      r.client_mac[3], r.client_mac[4], r.client_mac[5],
                      r.packet_count);
    }
}

void PcapDumper::clear() {
    count = 0;
    next_id = 1;
    memset(records, 0, sizeof(records));
}

int PcapDumper::findRecord(const uint8_t* bssid, const uint8_t* client_mac) const {
    for (int i = 0; i < (int)count; i++)
        if (memcmp(records[i].bssid, bssid, 6) == 0 &&
            memcmp(records[i].client_mac, client_mac, 6) == 0)
            return i;
    return -1;
}
