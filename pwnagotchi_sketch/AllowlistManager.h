#pragma once

#ifndef AllowlistManager_h
#define AllowlistManager_h

#include <stdint.h>
#include <string.h>

#define ALLOWLIST_MAX 16

typedef struct {
    uint8_t bssid[6];
    char    ssid[33];
    bool    enabled;
} AllowlistEntry;

class AllowlistManager {
  public:
    AllowlistManager();

    bool isAllowlisted(const uint8_t* bssid) const;
    bool addEntry(const uint8_t* bssid, const char* ssid = nullptr);
    bool removeEntry(const uint8_t* bssid);
    void listEntries() const;
    void clear();
    uint8_t count() const;

  private:
    AllowlistEntry entries[ALLOWLIST_MAX];
    uint8_t        entry_count;

    int findIndex(const uint8_t* bssid) const;
};

extern AllowlistManager allowlist;

#endif
