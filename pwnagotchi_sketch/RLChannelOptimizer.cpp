#include "RLChannelOptimizer.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

RLChannelOptimizer rl_optimizer;

RLChannelOptimizer::RLChannelOptimizer()
    : policy(POLICY_ROUND_ROBIN), last_channel(1),
      hops(0), total_reward(0.0f), last_hop_ms(0) {
    resetWeights();
}

void RLChannelOptimizer::init() {
    Serial.println(F("[RL] Channel optimizer ready"));
}

uint8_t RLChannelOptimizer::getNextChannel(const PwnagotchiStats& stats) {
    uint32_t now = millis();
    if (now - last_hop_ms < 100) return last_channel;
    last_hop_ms = now;
    hops++;
    last_channel = (policy == POLICY_RL) ? rlNextChannel(stats) : roundRobinNext();
    return last_channel;
}

void RLChannelOptimizer::recordReward(uint32_t new_aps, uint32_t new_handshakes) {
    float r = (float)(new_aps + new_handshakes * 2);
    total_reward += r;
    if (r > 0.5f && hops % 10 == 0) updateWeights(r);
}

void RLChannelOptimizer::setPolicy(ChannelPolicy p) {
    policy = p;
    Serial.printf("[RL] Policy: %s\n", p == POLICY_RL ? "neural" : "round-robin");
}

ChannelPolicy RLChannelOptimizer::getPolicy() const { return policy; }
float         RLChannelOptimizer::getAvgReward()   const { return hops ? total_reward / hops : 0; }
uint32_t      RLChannelOptimizer::getHopsCount()   const { return hops; }

uint8_t RLChannelOptimizer::rlNextChannel(const PwnagotchiStats& stats) {
    float in[RL_IN], out[RL_OUT];
    normalise(stats, in);
    forwardPass(in, out);
    uint8_t ch = argmax(out, RL_OUT) + 1;
    return (ch < 1 || ch > 13) ? roundRobinNext() : ch;
}

uint8_t RLChannelOptimizer::roundRobinNext() {
    return (last_channel >= 13) ? 1 : last_channel + 1;
}

void RLChannelOptimizer::forwardPass(const float* in, float* out) {
    float h[RL_H];
    for (int j = 0; j < RL_H; j++) {
        float z = b1[j];
        for (int i = 0; i < RL_IN; i++) z += w1[i][j] * in[i];
        h[j] = relu(z);
    }
    for (int j = 0; j < RL_OUT; j++) {
        float z = b2[j];
        for (int i = 0; i < RL_H; i++) z += w2[i][j] * h[i];
        out[j] = z;
    }
    softmax(out, RL_OUT);
}

void RLChannelOptimizer::updateWeights(float reward) {
    const float lr = 0.01f;
    float scale = lr * reward;
    for (int i = 0; i < RL_H;  i++)
        for (int j = 0; j < RL_OUT; j++)
            w2[i][j] += scale * 0.01f;
    for (int i = 0; i < RL_IN; i++)
        for (int j = 0; j < RL_H; j++)
            w1[i][j] += scale * 0.01f;
}

void RLChannelOptimizer::softmax(float* v, int n) {
    float mx = v[0];
    for (int i = 1; i < n; i++) if (v[i] > mx) mx = v[i];
    float sum = 0;
    for (int i = 0; i < n; i++) { v[i] = expf(v[i] - mx); sum += v[i]; }
    for (int i = 0; i < n; i++) v[i] /= sum;
}

float   RLChannelOptimizer::relu(float x)           { return x > 0 ? x : 0; }
uint8_t RLChannelOptimizer::argmax(const float* v, int n) {
    uint8_t idx = 0;
    for (int i = 1; i < n; i++) if (v[i] > v[idx]) idx = i;
    return idx;
}

void RLChannelOptimizer::normalise(const PwnagotchiStats& stats, float* in) {
    in[0] = stats.current_channel / 13.0f;
    in[1] = 0.5f;
    in[2] = stats.total_handshakes > 50 ? 1.0f : stats.total_handshakes / 50.0f;
    in[3] = stats.total_clients    > 200 ? 1.0f : stats.total_clients    / 200.0f;
}

void RLChannelOptimizer::resetWeights() {
    srand(42);
    for (int i = 0; i < RL_IN; i++)
        for (int j = 0; j < RL_H; j++)
            w1[i][j] = (rand() % 100 - 50) * 0.002f;
    for (int j = 0; j < RL_H; j++) b1[j] = 0.01f;
    for (int i = 0; i < RL_H; i++)
        for (int j = 0; j < RL_OUT; j++)
            w2[i][j] = (rand() % 100 - 50) * 0.002f;
    for (int j = 0; j < RL_OUT; j++) b2[j] = 0.01f;
}
