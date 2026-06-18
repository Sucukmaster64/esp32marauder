#include <Arduino.h>
#include "MoodEngine.h"
#include <string.h>

MoodEngine mood_engine;

MoodEngine::MoodEngine()
    : current_mood(MOOD_IDLE),
      last_ap_count(0), last_handshake_count(0), last_client_count(0),
      last_update_ms(0), last_activity_ms(0) {
    memset(&state_vector, 0, sizeof(StateVector));
}

void MoodEngine::update(const PwnagotchiStats& stats) {
    uint32_t now = millis();
    if (now - last_update_ms < 500) return;
    last_update_ms = now;

    state_vector.seenAPCount    = stats.total_aps;
    state_vector.newAPCount     = stats.total_aps - last_ap_count;
    state_vector.clientCount    = stats.total_clients;
    state_vector.handshakeCount = stats.total_handshakes;
    state_vector.currentChannel = (uint8_t)stats.current_channel;
    state_vector.epochTimestamp = now / 1000;

    bool activity = (stats.total_aps        > last_ap_count) ||
                    (stats.total_handshakes  > last_handshake_count) ||
                    (stats.total_clients     > last_client_count);
    if (activity) last_activity_ms = now;

    // Priority order: EXCITED > HAPPY > SAD > IDLE
    if (stats.total_handshakes > last_handshake_count) {
        current_mood = MOOD_EXCITED;
    } else if (stats.total_aps > last_ap_count) {
        current_mood = MOOD_HAPPY;
    } else if ((now - last_activity_ms) >= 30000) {
        current_mood = MOOD_SAD;
    } else {
        // Cool down from EXCITED/HAPPY back to IDLE after 3s
        if (current_mood == MOOD_EXCITED || current_mood == MOOD_HAPPY) {
            if ((now - last_activity_ms) >= 3000) current_mood = MOOD_IDLE;
        } else {
            current_mood = MOOD_IDLE;
        }
    }

    last_ap_count        = stats.total_aps;
    last_handshake_count = stats.total_handshakes;
    last_client_count    = stats.total_clients;
}

MoodState MoodEngine::getCurrentMood() const { return current_mood; }
StateVector MoodEngine::getStateVector() const { return state_vector; }

void MoodEngine::overrideMood(MoodState mood) {
    current_mood = mood;
    Serial.printf("[Mood] Override → %d\n", mood);
}
