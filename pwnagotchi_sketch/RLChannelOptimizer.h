#pragma once

#ifndef RLChannelOptimizer_h
#define RLChannelOptimizer_h

#include "PwnagotchiTypes.h"

#define RL_IN  4
#define RL_H   8
#define RL_OUT 13

enum ChannelPolicy { POLICY_ROUND_ROBIN = 0, POLICY_RL = 1 };

class RLChannelOptimizer {
  public:
    RLChannelOptimizer();

    void init();
    uint8_t getNextChannel(const PwnagotchiStats& stats);
    void recordReward(uint32_t new_aps, uint32_t new_handshakes);
    void setPolicy(ChannelPolicy p);
    ChannelPolicy getPolicy() const;

    float getAvgReward() const;
    uint32_t getHopsCount() const;

  private:
    float w1[RL_IN][RL_H];
    float b1[RL_H];
    float w2[RL_H][RL_OUT];
    float b2[RL_OUT];

    ChannelPolicy policy;
    uint8_t   last_channel;
    uint32_t  hops;
    float     total_reward;
    uint32_t  last_hop_ms;

    uint8_t rlNextChannel(const PwnagotchiStats& stats);
    uint8_t roundRobinNext();
    void    forwardPass(const float* in, float* out);
    void    updateWeights(float reward);
    void    softmax(float* v, int n);
    float   relu(float x);
    uint8_t argmax(const float* v, int n);
    void    normalise(const PwnagotchiStats& stats, float* in);
    void    resetWeights();
};

extern RLChannelOptimizer rl_optimizer;

#endif
