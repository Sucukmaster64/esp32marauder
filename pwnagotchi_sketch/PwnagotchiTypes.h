#pragma once

#ifndef PwnagotchiTypes_h
#define PwnagotchiTypes_h

#include <stdint.h>

enum MoodState {
    MOOD_IDLE     = 0,
    MOOD_HAPPY    = 1,
    MOOD_EXCITED  = 2,
    MOOD_SAD      = 3,
    MOOD_SLEEPING = 4
};

typedef struct {
    uint32_t seenAPCount;
    uint32_t newAPCount;
    uint32_t clientCount;
    uint32_t handshakeCount;
    uint8_t  currentChannel;
    uint32_t epochTimestamp;
} StateVector;

typedef struct {
    uint32_t total_aps;
    uint32_t total_clients;
    uint32_t total_handshakes;
    uint32_t current_channel;
    uint32_t packets_received;
} PwnagotchiStats;

#endif
