#include "RLChannelOptimizer.h"
#include <math.h>
#include <string.h>

RLChannelOptimizer rl_optimizer;

RLChannelOptimizer::RLChannelOptimizer()
    : current_policy(POLICY_ROUND_ROBIN), last_channel(1), last_hop_time(0),
      hops_count(0), cumulative_reward(0), aps_discovered(0),
      handshakes_captured(0), last_reward(0.0f) {
    memset(&hidden_layer, 0, sizeof(RL_HiddenLayer));
    memset(&output_layer, 0, sizeof(RL_OutputLayer));
    memset(&network_state, 0, sizeof(RL_NetworkState));
}

RLChannelOptimizer::~RLChannelOptimizer() {
    deinit();
}

void RLChannelOptimizer::init() {
    Serial.println(F("[RL] Initializing RL channel optimizer..."));
    resetNetworkWeights();
    loadWeights();  // Try to load saved weights from NVS
}

void RLChannelOptimizer::deinit() {
    saveWeights();  // Save weights to NVS before shutdown
}

uint8_t RLChannelOptimizer::getNextChannel(const PwnagotchiStats& wifi_stats) {
    unsigned long now = millis();
    if (now - last_hop_time < 100) {
        return last_channel;  // Don't hop more than once per 100ms
    }
    last_hop_time = now;

    uint8_t next_channel;
    if (current_policy == POLICY_RL_NETWORK) {
        next_channel = getNextChannelRL(wifi_stats);
    } else {
        next_channel = getNextChannelRoundRobin();
    }

    last_channel = next_channel;
    hops_count++;
    return next_channel;
}

uint8_t RLChannelOptimizer::getNextChannelRL(const PwnagotchiStats& wifi_stats) {
    // Normalize input state
    float input[RL_INPUT_SIZE];
    normalizeInput(wifi_stats, input);

    // Forward pass through network
    forwardPass(input);

    // Get channel with highest probability (argmax of softmax)
    uint8_t channel = argmax(network_state.output, RL_OUTPUT_SIZE) + 1;
    if (channel < 1 || channel > 13) {
        channel = wifi_stats.current_channel + 1;
        if (channel > 13) channel = 1;
    }

    return channel;
}

uint8_t RLChannelOptimizer::getNextChannelRoundRobin() {
    uint8_t next = last_channel + 1;
    if (next > 13) {
        next = 1;
    }
    return next;
}

void RLChannelOptimizer::forwardPass(const float* input) {
    // Copy input to state
    memcpy(network_state.input, input, RL_INPUT_SIZE * sizeof(float));

    // Hidden layer: Z = W*X + b
    for (int j = 0; j < RL_HIDDEN_SIZE; j++) {
        float z = hidden_layer.hidden_bias[j];
        for (int i = 0; i < RL_INPUT_SIZE; i++) {
            z += hidden_layer.weights[i][j] * input[i];
        }
        // ReLU activation
        network_state.hidden[j] = reluActivation(z);
    }

    // Output layer: Z = W*H + b
    for (int j = 0; j < RL_OUTPUT_SIZE; j++) {
        float z = output_layer.output_bias[j];
        for (int i = 0; i < RL_HIDDEN_SIZE; i++) {
            z += output_layer.weights[i][j] * network_state.hidden[i];
        }
        network_state.output[j] = z;
    }

    // Softmax on output
    softmaxActivation(network_state.output, RL_OUTPUT_SIZE);
}

void RLChannelOptimizer::updateWeights(float reward) {
    // Simple policy gradient update
    // dW = learning_rate * reward * dL/dW
    // For simplicity, we do a small random adjustment based on reward

    float learning_factor = RL_LEARNING_RATE * reward;

    // Update output layer weights
    for (int i = 0; i < RL_HIDDEN_SIZE; i++) {
        for (int j = 0; j < RL_OUTPUT_SIZE; j++) {
            // Increase weights that led to high reward
            float adjustment = learning_factor * (network_state.hidden[i] / 100.0f);
            output_layer.weights[i][j] += adjustment;
        }
    }

    // Update hidden layer weights
    for (int i = 0; i < RL_INPUT_SIZE; i++) {
        for (int j = 0; j < RL_HIDDEN_SIZE; j++) {
            float adjustment = learning_factor * (network_state.input[i] / 100.0f);
            hidden_layer.weights[i][j] += adjustment;
        }
    }
}

void RLChannelOptimizer::recordReward(uint32_t new_aps, uint32_t new_handshakes) {
    // Reward = new_aps + 2*new_handshakes (handshakes are more valuable)
    float reward = (float)(new_aps + (new_handshakes * 2));
    cumulative_reward += (uint32_t)reward;
    aps_discovered += new_aps;
    handshakes_captured += new_handshakes;

    last_reward = reward;

    // Update network weights if we have significant reward
    if (reward > 0.5f && hops_count % 10 == 0) {  // Every 10 hops
        updateWeights(reward);
    }
}

void RLChannelOptimizer::normalizeInput(const PwnagotchiStats& wifi_stats, float* input) {
    // Input: [current_channel (0-13), idle_time (0-60s), handshakes (0-50), clients (0-200)]
    input[0] = wifi_stats.current_channel / 13.0f;          // Normalized to 0-1
    input[1] = 0.5f;  // Placeholder for idle time (would need state tracking)
    input[2] = (float)wifi_stats.total_handshakes / 50.0f;  // Normalized
    input[3] = (float)wifi_stats.total_clients / 200.0f;    // Normalized
}

float RLChannelOptimizer::reluActivation(float x) {
    return (x > 0) ? x : 0.0f;
}

float RLChannelOptimizer::softmaxActivation(float* output, int size) {
    // Find max for numerical stability
    float max_val = output[0];
    for (int i = 1; i < size; i++) {
        if (output[i] > max_val) {
            max_val = output[i];
        }
    }

    // Compute exp(x - max) and sum
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        output[i] = expf(output[i] - max_val);
        sum += output[i];
    }

    // Normalize
    for (int i = 0; i < size; i++) {
        output[i] /= sum;
    }

    return sum;
}

uint8_t RLChannelOptimizer::argmax(const float* array, int size) {
    uint8_t max_idx = 0;
    float max_val = array[0];
    for (int i = 1; i < size; i++) {
        if (array[i] > max_val) {
            max_val = array[i];
            max_idx = i;
        }
    }
    return max_idx;
}

void RLChannelOptimizer::setPolicy(ChannelPolicy policy) {
    current_policy = policy;
    if (policy == POLICY_RL_NETWORK) {
        Serial.println(F("[RL] Switched to RL network policy"));
    } else {
        Serial.println(F("[RL] Switched to round-robin policy"));
    }
}

ChannelPolicy RLChannelOptimizer::getPolicy() {
    return current_policy;
}

RL_PolicyStats RLChannelOptimizer::getStats() {
    RL_PolicyStats stats = {
        .hops_count = hops_count,
        .total_reward = cumulative_reward,
        .avg_reward_per_hop = (hops_count > 0) ? (float)cumulative_reward / hops_count : 0.0f,
        .aps_discovered = aps_discovered,
        .handshakes_captured = handshakes_captured
    };
    return stats;
}

void RLChannelOptimizer::resetNetworkWeights() {
    // Initialize with small random values (simplified: use deterministic pattern)
    for (int i = 0; i < RL_INPUT_SIZE; i++) {
        for (int j = 0; j < RL_HIDDEN_SIZE; j++) {
            hidden_layer.weights[i][j] = 0.1f * (float)(rand() % 10) / 10.0f;
        }
    }

    for (int i = 0; i < RL_HIDDEN_SIZE; i++) {
        hidden_layer.hidden_bias[i] = 0.01f;
        for (int j = 0; j < RL_OUTPUT_SIZE; j++) {
            output_layer.weights[i][j] = 0.1f * (float)(rand() % 10) / 10.0f;
        }
    }

    for (int j = 0; j < RL_OUTPUT_SIZE; j++) {
        output_layer.output_bias[j] = 0.01f;
    }

    Serial.println(F("[RL] Network weights reset to initial values"));
}

void RLChannelOptimizer::printNetworkWeights() {
    Serial.println(F("[RL] Network weights:"));
    Serial.printf("  Hidden layer (input→hidden): %dx%d\n", RL_INPUT_SIZE, RL_HIDDEN_SIZE);
    for (int i = 0; i < RL_INPUT_SIZE; i++) {
        Serial.printf("    Input %d: ", i);
        for (int j = 0; j < RL_HIDDEN_SIZE; j++) {
            Serial.printf("%.3f ", hidden_layer.weights[i][j]);
        }
        Serial.println();
    }

    Serial.printf("  Output layer (hidden→output): %dx%d\n", RL_HIDDEN_SIZE, RL_OUTPUT_SIZE);
    // Only print first few for brevity
    Serial.println("    (Output layer too large to display)");
    Serial.printf("  Policy stats: %lu hops, %.2f avg reward\n",
                  hops_count, getStats().avg_reward_per_hop);
}

void RLChannelOptimizer::loadWeights() {
    // Placeholder for NVS loading (Phase 5)
    // Would load from preferences_obj.loadSetting("rl/weights")
    Serial.println(F("[RL] Weights load from NVS not yet implemented"));
}

void RLChannelOptimizer::saveWeights() {
    // Placeholder for NVS saving (Phase 5)
    // Would save to preferences_obj.setSetting("rl/weights")
    Serial.println(F("[RL] Weights save to NVS not yet implemented"));
}
