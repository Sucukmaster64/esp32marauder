#pragma once

#ifndef MoodEngine_h
#define MoodEngine_h

#include "PwnagotchiWiFiCore.h"

enum MoodState {
    MOOD_IDLE = 0,
    MOOD_HAPPY = 1,
    MOOD_EXCITED = 2,
    MOOD_SAD = 3,
    MOOD_SLEEPING = 4
};

typedef struct {
    uint32_t seenAPCount;
    uint32_t newAPCount;
    uint32_t clientCount;
    uint32_t handshakeCount;
    uint8_t currentChannel;
    uint32_t epochTimestamp;
} StateVector;

class MoodEngine {
  public:
    MoodEngine();
    ~MoodEngine();

    void update(const PwnagotchiStats& wifi_stats);
    MoodState getCurrentMood();
    StateVector getStateVector();
    void overrideMood(MoodState mood);

  private:
    MoodState current_mood;
    StateVector state_vector;
    uint32_t last_ap_count;
    uint32_t last_handshake_count;
    uint32_t last_client_count;
    uint32_t last_mood_update;
    uint32_t idle_timer_start;
    uint32_t last_activity_time;

    void updateMood(const PwnagotchiStats& wifi_stats);
    uint32_t getMoodTransitionDuration(MoodState from, MoodState to);
};

extern MoodEngine mood_engine;

#endif
