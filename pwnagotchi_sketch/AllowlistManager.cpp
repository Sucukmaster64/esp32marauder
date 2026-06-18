#include "AllowlistManager.h"

AllowlistManager allowlist;

AllowlistManager::AllowlistManager() : entry_count(0) {
    memset(entries, 0, sizeof(entries));
}

bool AllowlistManager::isAllowlisted(const uint8_t* bssid) const {
    int idx = findIndex(bssid);
    return (idx >= 0) && entries[idx].enabled;
}

bool AllowlistManager::addEntry(const uint8_t* bssid, const char* ssid) {
    if (!bssid || entry_count >= ALLOWLIST_MAX) return false;
    if (findIndex(bssid) >= 0) return false;  // already exists

    AllowlistEntry& e = entries[entry_count++];
    memcpy(e.bssid, bssid, 6);
    e.enabled = true;
    if (ssid) {
        strncpy(e.ssid, ssid, 32);
        e.ssid[32] = '\0';
    } else {
        e.ssid[0] = '\0';
    }

    Serial.printf("[Allowlist] Added %02x:%02x:%02x:%02x:%02x:%02x",
                  bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    if (ssid) Serial.printf(" (%s)", ssid);
    Serial.println();
    return true;
}

bool AllowlistManager::removeEntry(const uint8_t* bssid) {
    int idx = findIndex(bssid);
    if (idx < 0) return false;
    // Shift remaining entries left
    for (int i = idx; i < (int)entry_count - 1; i++) entries[i] = entries[i + 1];
    entry_count--;
    return true;
}

void AllowlistManager::listEntries() const {
    if (entry_count == 0) { Serial.println(F("[Allowlist] Empty")); return; }
    for (uint8_t i = 0; i < entry_count; i++) {
        const AllowlistEntry& e = entries[i];
        Serial.printf("[Allowlist] %d. %02x:%02x:%02x:%02x:%02x:%02x [%s] %s\n",
                      i + 1,
                      e.bssid[0], e.bssid[1], e.bssid[2],
                      e.bssid[3], e.bssid[4], e.bssid[5],
                      e.ssid,
                      e.enabled ? "ON" : "OFF");
    }
}

void AllowlistManager::clear() {
    entry_count = 0;
    memset(entries, 0, sizeof(entries));
}

uint8_t AllowlistManager::count() const { return entry_count; }

int AllowlistManager::findIndex(const uint8_t* bssid) const {
    for (int i = 0; i < (int)entry_count; i++)
        if (memcmp(entries[i].bssid, bssid, 6) == 0) return i;
    return -1;
}
