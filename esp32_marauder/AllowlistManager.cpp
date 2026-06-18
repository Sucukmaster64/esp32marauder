#include "AllowlistManager.h"

AllowlistManager allowlist_manager;

AllowlistManager::AllowlistManager()
    : entry_count(0) {
    memset(entries, 0, sizeof(entries));
}

AllowlistManager::~AllowlistManager() {
    deinit();
}

void AllowlistManager::init() {
    Serial.println(F("[Allowlist] Initializing allowlist manager..."));
    entry_count = 0;
    loadFromNVS();  // Try to load saved allowlist from NVS
    Serial.printf("[Allowlist] Loaded %lu entries from storage\n", entry_count);
}

void AllowlistManager::deinit() {
    saveToNVS();  // Save allowlist before shutdown
    Serial.println(F("[Allowlist] Allowlist saved"));
}

bool AllowlistManager::isAllowlisted(const uint8_t* bssid, const char* ssid) {
    if (!bssid) {
        return false;
    }

    AllowlistEntry* entry = findEntry(bssid);
    if (!entry) {
        return false;
    }

    // Check if entry is enabled
    if (!entry->enabled) {
        return false;
    }

    // If SSID provided, also check SSID match (extra validation)
    if (ssid && strlen(entry->ssid) > 0) {
        if (strcmp(entry->ssid, ssid) != 0) {
            return false;
        }
    }

    return true;
}

bool AllowlistManager::addEntry(const uint8_t* bssid, const char* ssid) {
    if (!bssid) {
        return false;
    }

    // Check if already exists
    if (findEntry(bssid)) {
        Serial.println(F("[Allowlist] Entry already exists"));
        return false;
    }

    // Check capacity
    if (entry_count >= ALLOWLIST_MAX_ENTRIES) {
        Serial.println(F("[Allowlist] Allowlist is full"));
        return false;
    }

    // Add new entry
    AllowlistEntry* new_entry = &entries[entry_count];
    memcpy(new_entry->bssid, bssid, 6);
    new_entry->enabled = true;
    new_entry->added_timestamp = millis();

    if (ssid) {
        strncpy(new_entry->ssid, ssid, MAX_SSID_LEN - 1);
        new_entry->ssid[MAX_SSID_LEN - 1] = '\0';
    } else {
        new_entry->ssid[0] = '\0';
    }

    entry_count++;

    Serial.printf("[Allowlist] Added entry: %02x:%02x:%02x:%02x:%02x:%02x",
                  bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    if (ssid && strlen(ssid) > 0) {
        Serial.printf(" (SSID: %s)", ssid);
    }
    Serial.println();

    return true;
}

bool AllowlistManager::removeEntry(const uint8_t* bssid) {
    if (!bssid) {
        return false;
    }

    for (uint32_t i = 0; i < entry_count; i++) {
        if (memcmp(entries[i].bssid, bssid, 6) == 0) {
            // Shift remaining entries
            for (uint32_t j = i; j < entry_count - 1; j++) {
                memcpy(&entries[j], &entries[j + 1], sizeof(AllowlistEntry));
            }
            entry_count--;

            Serial.printf("[Allowlist] Removed entry: %02x:%02x:%02x:%02x:%02x:%02x\n",
                          bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
            return true;
        }
    }

    Serial.println(F("[Allowlist] Entry not found"));
    return false;
}

bool AllowlistManager::removeBySSID(const char* ssid) {
    AllowlistEntry* entry = findEntryBySSID(ssid);
    if (entry) {
        return removeEntry(entry->bssid);
    }
    return false;
}

bool AllowlistManager::enableEntry(const uint8_t* bssid) {
    AllowlistEntry* entry = findEntry(bssid);
    if (entry) {
        entry->enabled = true;
        Serial.println(F("[Allowlist] Entry enabled"));
        return true;
    }
    return false;
}

bool AllowlistManager::disableEntry(const uint8_t* bssid) {
    AllowlistEntry* entry = findEntry(bssid);
    if (entry) {
        entry->enabled = false;
        Serial.println(F("[Allowlist] Entry disabled"));
        return true;
    }
    return false;
}

uint32_t AllowlistManager::getEntryCount() {
    return entry_count;
}

void AllowlistManager::listEntries() {
    Serial.println(F("[Allowlist] Current allowlist entries:"));
    if (entry_count == 0) {
        Serial.println(F("  (empty)"));
        return;
    }

    for (uint32_t i = 0; i < entry_count; i++) {
        Serial.printf("  %lu. %02x:%02x:%02x:%02x:%02x:%02x",
                      i + 1,
                      entries[i].bssid[0], entries[i].bssid[1],
                      entries[i].bssid[2], entries[i].bssid[3],
                      entries[i].bssid[4], entries[i].bssid[5]);

        if (entries[i].ssid[0] != '\0') {
            Serial.printf(" [%s]", entries[i].ssid);
        }

        Serial.printf(" %s\n", entries[i].enabled ? "(enabled)" : "(disabled)");
    }
}

void AllowlistManager::clearAllowlist() {
    entry_count = 0;
    memset(entries, 0, sizeof(entries));
    Serial.println(F("[Allowlist] Allowlist cleared"));
}

AllowlistEntry* AllowlistManager::findEntry(const uint8_t* bssid) {
    if (!bssid) {
        return nullptr;
    }

    for (uint32_t i = 0; i < entry_count; i++) {
        if (memcmp(entries[i].bssid, bssid, 6) == 0) {
            return &entries[i];
        }
    }
    return nullptr;
}

AllowlistEntry* AllowlistManager::findEntryBySSID(const char* ssid) {
    if (!ssid) {
        return nullptr;
    }

    for (uint32_t i = 0; i < entry_count; i++) {
        if (strcmp(entries[i].ssid, ssid) == 0) {
            return &entries[i];
        }
    }
    return nullptr;
}

void AllowlistManager::loadFromNVS() {
    // Placeholder: Would load from preferences_obj.getSetting("allowlist/*")
    Serial.println(F("[Allowlist] NVS load not yet implemented"));
    // For now, add a demo entry for testing
    uint8_t test_bssid[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    addEntry(test_bssid, "HomeNetwork");
}

void AllowlistManager::saveToNVS() {
    // Placeholder: Would save to preferences_obj.setSetting("allowlist/*")
    Serial.printf("[Allowlist] Would save %lu entries to NVS\n", entry_count);
}
