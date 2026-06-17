#pragma once

#ifndef AllowlistManager_h
#define AllowlistManager_h

#include <stdint.h>
#include <string.h>

#define ALLOWLIST_MAX_ENTRIES 16
#define MAX_SSID_LEN 32

typedef struct {
    uint8_t bssid[6];
    char ssid[MAX_SSID_LEN];
    bool enabled;
    uint32_t added_timestamp;
} AllowlistEntry;

class AllowlistManager {
  public:
    AllowlistManager();
    ~AllowlistManager();

    void init();
    void deinit();

    // Core functionality
    bool isAllowlisted(const uint8_t* bssid, const char* ssid = nullptr);
    bool addEntry(const uint8_t* bssid, const char* ssid);
    bool removeEntry(const uint8_t* bssid);
    bool removeBySSID(const char* ssid);
    bool enableEntry(const uint8_t* bssid);
    bool disableEntry(const uint8_t* bssid);

    // Management
    uint32_t getEntryCount();
    void listEntries();
    void clearAllowlist();

    // Persistence (Phase 5)
    void loadFromNVS();
    void saveToNVS();

  private:
    AllowlistEntry entries[ALLOWLIST_MAX_ENTRIES];
    uint32_t entry_count;

    AllowlistEntry* findEntry(const uint8_t* bssid);
    AllowlistEntry* findEntryBySSID(const char* ssid);
};

extern AllowlistManager allowlist_manager;

#endif
