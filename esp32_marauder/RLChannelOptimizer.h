#pragma once

#ifndef RLChannelOptimizer_h
#define RLChannelOptimizer_h

#include <stdint.h>
#include <float.h>
#include "PwnagotchiWiFiCore.h"

#define RL_INPUT_SIZE 4
#define RL_HIDDEN_SIZE 8
#define RL_OUTPUT_SIZE 13
#define RL_LEARNING_RATE 0.01f
#define RL_DISCOUNT_FACTOR 0.99f

enum ChannelPolicy {
    POLICY_ROUND_ROBIN = 0,
    POLICY_RL_NETWORK = 1
};

typedef struct {
    float weights[RL_INPUT_SIZE][RL_HIDDEN_SIZE];
    float hidden_bias[RL_HIDDEN_SIZE];
} RL_HiddenLayer;

typedef struct {
    float weights[RL_HIDDEN_SIZE][RL_OUTPUT_SIZE];
    float output_bias[RL_OUTPUT_SIZE];
} RL_OutputLayer;

typedef struct {
    float input[RL_INPUT_SIZE];
    float hidden[RL_HIDDEN_SIZE];
    float output[RL_OUTPUT_SIZE];
} RL_NetworkState;

typedef struct {
    uint32_t hops_count;
    uint32_t total_reward;
    float avg_reward_per_hop;
    uint32_t aps_discovered;
    uint32_t handshakes_captured;
} RL_PolicyStats;

class RLChannelOptimizer {
  public:
    RLChannelOptimizer();
    ~RLChannelOptimizer();

    void init();
    void deinit();
    uint8_t getNextChannel(const PwnagotchiStats& wifi_stats);
    void recordReward(uint32_t new_aps, uint32_t new_handshakes);
    void setPolicy(ChannelPolicy policy);
    ChannelPolicy getPolicy();

    RL_PolicyStats getStats();
    void printNetworkWeights();
    void resetNetworkWeights();

  private:
    RL_HiddenLayer hidden_layer;
    RL_OutputLayer output_layer;
    RL_NetworkState network_state;
    ChannelPolicy current_policy;
    uint8_t last_channel;
    uint32_t last_hop_time;

    uint32_t hops_count;
    uint32_t cumulative_reward;
    uint32_t aps_discovered;
    uint32_t handshakes_captured;
    float last_reward;

    uint8_t getNextChannelRL(const PwnagotchiStats& wifi_stats);
    uint8_t getNextChannelRoundRobin();

    void forwardPass(const float* input);
    void updateWeights(float reward);
    void normalizeInput(const PwnagotchiStats& wifi_stats, float* input);
    float reluActivation(float x);
    float softmaxActivation(float* output, int size);
    uint8_t argmax(const float* array, int size);

    void loadWeights();
    void saveWeights();
};

extern RLChannelOptimizer rl_optimizer;

#endif
