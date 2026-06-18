#pragma once

#ifndef MoodEngine_h
#define MoodEngine_h

#include "PwnagotchiTypes.h"

class MoodEngine {
  public:
    MoodEngine();

    void update(const PwnagotchiStats& stats);
    MoodState getCurrentMood() const;
    StateVector getStateVector() const;
    void overrideMood(MoodState mood);

  private:
    MoodState   current_mood;
    StateVector state_vector;
    uint32_t    last_ap_count;
    uint32_t    last_handshake_count;
    uint32_t    last_client_count;
    uint32_t    last_update_ms;
    uint32_t    last_activity_ms;
};

extern MoodEngine mood_engine;

#endif
