#include "MoodEngine.h"

MoodEngine mood_engine;

MoodEngine::MoodEngine()
    : current_mood(MOOD_IDLE), last_ap_count(0), last_handshake_count(0),
      last_client_count(0), last_mood_update(0), idle_timer_start(millis()),
      last_activity_time(millis()) {
    memset(&state_vector, 0, sizeof(StateVector));
}

MoodEngine::~MoodEngine() {
}

void MoodEngine::update(const PwnagotchiStats& wifi_stats) {
    unsigned long now = millis();

    // Only update mood every 500ms
    if (now - last_mood_update < 500) {
        return;
    }
    last_mood_update = now;

    // Update state vector
    state_vector.seenAPCount = wifi_stats.total_aps;
    state_vector.newAPCount = wifi_stats.total_aps - last_ap_count;
    state_vector.clientCount = wifi_stats.total_clients;
    state_vector.handshakeCount = wifi_stats.total_handshakes;
    state_vector.currentChannel = wifi_stats.current_channel;
    state_vector.epochTimestamp = now / 1000;

    // Track activity
    if (wifi_stats.total_aps > last_ap_count ||
        wifi_stats.total_handshakes > last_handshake_count ||
        wifi_stats.total_clients > last_client_count) {
        last_activity_time = now;
    }

    // Update mood based on activity
    updateMood(wifi_stats);

    // Track counters
    last_ap_count = wifi_stats.total_aps;
    last_handshake_count = wifi_stats.total_handshakes;
    last_client_count = wifi_stats.total_clients;
}

void MoodEngine::updateMood(const PwnagotchiStats& wifi_stats) {
    unsigned long now = millis();
    unsigned long inactivity_time = now - last_activity_time;

    // Priority 1: Handshake captured -> EXCITED (150ms)
    if (wifi_stats.total_handshakes > last_handshake_count) {
        current_mood = MOOD_EXCITED;
        idle_timer_start = now;
        return;
    }

    // Priority 2: New APs discovered -> HAPPY (2 seconds)
    if (wifi_stats.total_aps > last_ap_count) {
        current_mood = MOOD_HAPPY;
        idle_timer_start = now;
        return;
    }

    // Priority 3: Recent activity (within 30 seconds) -> IDLE
    if (inactivity_time < 30000) {
        if (current_mood == MOOD_SAD) {
            current_mood = MOOD_IDLE;
        }
    }
    // Priority 4: No activity for > 30 seconds -> SAD
    else if (inactivity_time >= 30000) {
        current_mood = MOOD_SAD;
    }
}

MoodState MoodEngine::getCurrentMood() {
    return current_mood;
}

StateVector MoodEngine::getStateVector() {
    return state_vector;
}

void MoodEngine::overrideMood(MoodState mood) {
    current_mood = mood;
    Serial.printf("[MoodEngine] Mood override: %d\n", mood);
}

uint32_t MoodEngine::getMoodTransitionDuration(MoodState from, MoodState to) {
    // Defines how long a mood should persist before transitioning
    // EXCITED: 150ms (quick spike)
    // HAPPY: 2000ms (longer to enjoy the discovery)
    // SAD: indefinite until activity
    // IDLE: depends on context

    if (to == MOOD_EXCITED) {
        return 150;
    } else if (to == MOOD_HAPPY) {
        return 2000;
    }
    return 0;  // No transition timeout for SAD/IDLE
}
