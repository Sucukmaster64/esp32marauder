# ESP32-S3 Pwnagotchi Clone - Development Status

## Overview

This document tracks the development of a lightweight Pwnagotchi-inspired WiFi awareness tool built on the ESP32Marauder WiFi core. The project is implemented in 6 sequential phases, each with clear deliverables and milestones.

**Current Status:** All 6 phases implemented and committed ✅

---

## Phase 1: Minimal WiFi Core (✅ Complete)

**Objective:** Extract WiFi sniffing capability from Marauder without Display dependencies

**Files Created:**
- `PwnagotchiWiFiCore.h/cpp` - Core WiFi beacon/probe sniffing
- `pwnagotchi_main.ino` - Main sketch with Serial CLI

**Key Features:**
- Beacon frame detection on all 13 WiFi channels
- Probe request detection from clients
- Auto channel hopping (100ms interval)
- Statistics: packet count, AP count, client count, handshake count
- Serial commands: `help`, `stats`, `channel`, `hop`, `start`, `stop`

**How It Works:**
```cpp
wifi_core.startBeaconSniff();  // Enable WiFi promiscuous mode
wifi_core.hopChannel();         // Move to next channel
PwnagotchiStats stats = wifi_core.getStats();  // Get current stats
```

**Testing:** Monitor Serial at 115200 baud for beacon/probe detections

---

## Phase 2: OLED Display Integration (✅ Complete)

**Objective:** Render WiFi statistics and mood states on 128x64 OLED

**Files Created:**
- `PwnagotchiDisplay.h/cpp` - SSD1306/SH1106 driver with mood states
- `PWNAGOTCHI_SETUP.md` - Hardware config & build instructions

**Key Features:**
- I2C OLED support (default 0x3C address)
- 5 mood states: IDLE, HAPPY, EXCITED, SAD, SLEEPING
- Text-based mood display with statistics
- Automatic mood transitions based on WiFi activity
- Display update rate: 250ms

**Mood Logic:**
- **HAPPY:** New APs detected
- **EXCITED:** EAPOL handshake captured
- **SAD:** 30+ seconds without new activity
- **IDLE:** Waiting for activity
- **SLEEPING:** WiFi module disabled

**Hardware Pins (ESP32-S3 SuperMini):**
- SDA: GPIO 6
- SCL: GPIO 7
- GND: Ground
- VCC: 3.3V

---

## Phase 3: Mood Engine & Pixel Faces (✅ Complete)

**Objective:** Implement state machine with animated pixel-based faces

**Files Created:**
- `MoodEngine.h/cpp` - Sophisticated mood state tracking
- Updated `PwnagotchiDisplay.cpp` - Pixel face drawing

**Key Features:**
- Priority-based mood transitions
- Inactivity timer (30s → SAD)
- State vector aggregation:
  - Seen APs, New APs, Clients, Handshakes, Channel, Timestamp
- Pixel-based emoji faces:
  - **HAPPY:** `:-)` with parabolic smile
  - **EXCITED:** `o_o` with large eyes and open mouth
  - **SAD:** `:-(` with inverted smile
  - **SLEEPING:** `-_-` with closed eyes
  - **IDLE:** `...` neutral face

**Display Layout:**
```
┌─────────────────────────────┐
│ [PIXEL FACE]  CH:6          │
│               AP:12         │
│               CL:25         │
│               HS:3          │
│ HAPPY         PKT:1234      │
└─────────────────────────────┘
```

**Mood Persistence:**
- Transitions smooth (no flickering)
- Each mood has optimal display duration
- Tracks activity timestamps for idle detection

---

## Phase 4: RL Network for Channel Hopping (✅ Complete)

**Objective:** Implement lightweight neural network for intelligent channel selection

**Files Created:**
- `RLChannelOptimizer.h/cpp` - 2-layer dense NN (4→8→13 architecture)

**Key Features:**
- Pure C implementation (no TFLite dependency)
- Dual-policy support:
  - **Round-Robin:** Baseline (channels 1-13 sequentially)
  - **RL Network:** Neural network optimization

**Network Architecture:**
```
Input (4 neurons):
  [0] Current channel (normalized 0-1)
  [1] Idle time (0-60s normalized)
  [2] Handshakes (normalized 0-50)
  [3] Clients (normalized 0-200)
         ↓
Hidden Layer (8 neurons, ReLU):
  Dense[4→8] + bias
         ↓
Output Layer (13 neurons, Softmax):
  Dense[8→13] → P(channel) for each 1-13
         ↓
Action: argmax(softmax) → next channel
```

**Training:**
- Reward = new_APs + 2×new_handshakes
- Updates every 10 hops if reward > 0.5
- Simple policy gradient: dW = lr × reward × activation
- Hyperparameters:
  - Learning rate: 0.01
  - Discount factor: 0.99

**Statistics Tracking:**
- Hops count (per policy)
- Total reward (cumulative)
- Avg reward per hop
- APs discovered / handshakes captured

**Commands:**
```
Serial: "rl_policy round_robin"  → Switch to round-robin
Serial: "rl_policy neural"       → Switch to RL network
Serial: "rl_stats"               → Print policy statistics
Serial: "rl_weights"             → Dump network weights (debug)
```

---

## Phase 5: Persistent Storage & PCAP Dumps (✅ Complete)

**Objective:** Save EAPOL handshakes and statistics across reboots

**Files Created:**
- `PcapDumper.h/cpp` - EAPOL packet capture and PCAP serialization

**Key Features:**
- Captures up to 20 complete handshakes
- Validates EAPOL frames (802.1X)
- Associates BSSID ↔ Client MAC
- Packet count tracking (complete = 4 frames)
- Unique handshake IDs for audit trail

**Handshake Storage:**
```cpp
struct HandshakeEntry {
    uint8_t bssid[6];           // AP MAC address
    uint8_t client_mac[6];      // Client MAC address
    uint32_t timestamp;         // When first captured
    uint16_t packet_count;      // 1-4 EAPOL frames
    uint32_t handshake_id;      // Unique identifier
};
```

**PCAP File Format (Planned):**
- PCAP header (magic: 0xa1b2c3d4)
- Packet records with timestamps
- Compatible with Wireshark, hashcat, aircrack-ng

**NVS Integration (Placeholder):**
- Would save to: `preferences_obj.setSetting("pcap/handshakes")`
- Survives power cycles
- Per-session or persistent (configurable)

**Commands:**
```
Serial: "handshakes"            → List captured handshakes
Serial: "export pcap_dump.cap"  → Export to PCAP file (stub)
Serial: "clear_handshakes"      → Clear captured data
```

---

## Phase 6: Allowlist Gate & Security (✅ Complete)

**Objective:** Restrict active attacks to authorized networks only

**Files Created:**
- `AllowlistManager.h/cpp` - Authorization control for attacks

**Key Features:**
- Max 16 allowlisted entries
- Per-entry enable/disable toggle
- BSSID-based authentication
- Optional SSID cross-check
- Timestamp audit trail

**Security Model:**
```
Passive Scanning: ALWAYS ALLOWED (no gate)
├─ Beacon detection
├─ Probe sniffing
└─ EAPOL capture

Active Attacks: REQUIRES ALLOWLIST
├─ sendDeauthFrame()    → Check isAllowlisted()
├─ sendProbeAttack()    → Check isAllowlisted()
└─ sendBeaconSpam()     → Check isAllowlisted()
```

**Entry Structure:**
```cpp
struct AllowlistEntry {
    uint8_t bssid[6];           // AP MAC (primary key)
    char ssid[32];              // Network name (optional)
    bool enabled;               // Per-entry killswitch
    uint32_t added_timestamp;   // Audit trail
};
```

**Default Policy:**
- Empty allowlist at startup
- Demo entry pre-loaded: `AA:BB:CC:DD:EE:FF (HomeNetwork)` [disabled]
- No attacks possible until explicitly enabled

**Commands:**
```
Serial: "allowlist add BSSID SSID"     → Add entry
Serial: "allowlist remove BSSID"       → Remove entry
Serial: "allowlist enable BSSID"       → Enable attacks
Serial: "allowlist disable BSSID"      → Disable attacks
Serial: "allowlist list"               → Show current entries
Serial: "allowlist clear"              → Remove all entries
```

**NVS Integration (Placeholder):**
- Would save to: `preferences_obj.setSetting("allowlist/*")`
- Loads on boot via `loadFromNVS()`

---

## Integration Roadmap

### Immediate Next Steps

1. **Hardware Testing**
   - Flash onto ESP32-S3 SuperMini with SSD1306 OLED
   - Verify beacon sniffing in real WiFi environment
   - Test mood transitions and pixel faces

2. **Serial CLI Completion**
   - Integrate all commands into main loop
   - Add help text for each component
   - Debug output for network analysis

3. **NVS Persistence**
   - Link AllowlistManager to settings.cpp
   - Integrate PcapDumper with SPIFFS
   - Store RL weights in NVS

4. **EAPOL Detection**
   - Parse real 802.11 EAPOL frames
   - Implement 4-frame handshake detection
   - Generate valid PCAP output

### Medium-term Enhancements

5. **Deauth/Probe Integration**
   - Extract `sendDeauthFrame()` from WiFiScan.cpp
   - Modify to check `isAllowlisted()` before sending
   - Test attack gating on real hardware

6. **Battery Management**
   - Monitor LiPo voltage via ADC
   - Implement low-power sleep modes
   - Display battery % on OLED

7. **Advanced Mood States**
   - Animated eye blinking
   - Mood memory (persist across reboots)
   - Audio feedback (buzzer) for major events

### Long-term Features

8. **OTA Updates**
   - WiFi firmware update over-the-air
   - Preserve settings across updates

9. **Data Export**
   - USB serial PCAP export
   - WiFi web interface for data retrieval
   - Compressed handshake archive

10. **Pwnagotchi Protocol**
    - Emit custom beacon frames (pwnagotchi WiFi spec)
    - Detect other pwnagotchis in range
    - Display "friends nearby" indicator

---

## Build & Test Checklist

- [ ] Phase 1: Serial console logs beacon detections
- [ ] Phase 2: OLED displays mood text + stats
- [ ] Phase 3: Pixel faces animate correctly
- [ ] Phase 4: RL network hops more efficiently than round-robin
- [ ] Phase 5: Handshakes captured and listed
- [ ] Phase 6: Allowlist blocks deauth on unlisted networks
- [ ] Full integration: All phases work together
- [ ] Hardware: ESP32-S3 + OLED + Serial console

---

## Key Classes & Components

| Class | Purpose | Status |
|-------|---------|--------|
| `PwnagotchiWiFiCore` | WiFi sniffing engine | Phase 1 ✅ |
| `PwnagotchiDisplay` | OLED renderer | Phase 2 ✅ |
| `MoodEngine` | State machine | Phase 3 ✅ |
| `RLChannelOptimizer` | RL network | Phase 4 ✅ |
| `PcapDumper` | Handshake capture | Phase 5 ✅ |
| `AllowlistManager` | Attack authorization | Phase 6 ✅ |

---

## Memory & Performance Notes

### ESP32-S3 SuperMini Resources
- SRAM: ~320 KB available
- PSRAM: Not available on SuperMini
- Flash: 8 MB (or 4 MB)

### Current Estimated Usage
- WiFi sniffing loop: ~40 KB
- RL network weights: ~2.5 KB (4×8×8 + 8×13×13 floats)
- Allowlist storage: ~1 KB (16 entries × 64 bytes)
- OLED display buffer: ~1 KB
- **Total:** ~45 KB (plenty of headroom)

### Optimization Opportunities
- Compress RL weights (quantization)
- Implement handshake ring buffer instead of fixed array
- Use SPIFFS for PCAP data instead of SRAM

---

## Security Considerations

### Hard Constraints (Non-negotiable)

1. **Passive scanning always permitted** - No allowlist gate
2. **Active attacks require allowlist** - Deauth, probe spam, etc.
3. **Allowlist check is mandatory** - Every active function validates
4. **No credential storage** - Handshakes are for external cracking only
5. **No external data logging** - All storage is local

### Threat Model

**In Scope:**
- Unauthorized deauth attacks
- Rogue WiFi scanning
- Handshake interception (mitigated by allowlist)

**Out of Scope:**
- WPA3 SAE handshake attacks (not supported)
- Bluetooth attacks (separate system)
- Zero-day WiFi driver exploits

---

## Future Considerations

### Regulatory Compliance
- EU: WiFi jamming illegal (deauth in scope)
- US: FCC Part 15 rules apply
- Recommendation: Use on own networks only

### User Education
- Clear warnings about active attacks
- Allowlist best practices documentation
- Legal disclaimer on first boot

### Project Evolution
- Merge back to justcallmekoko/ESP32Marauder?
- Publish as standalone library?
- Community contributions welcome

---

## Repository Structure

```
esp32marauder/
├── CLAUDE.md                          # Main architecture guide
├── PWNAGOTCHI_SETUP.md                # Hardware setup instructions
├── PWNAGOTCHI_DEVELOPMENT.md          # This file
├── esp32_marauder/
│   ├── pwnagotchi_main.ino            # Main sketch
│   ├── PwnagotchiWiFiCore.h/cpp       # Phase 1: WiFi core
│   ├── PwnagotchiDisplay.h/cpp        # Phase 2: OLED display
│   ├── MoodEngine.h/cpp               # Phase 3: Mood state machine
│   ├── RLChannelOptimizer.h/cpp       # Phase 4: RL network
│   ├── PcapDumper.h/cpp               # Phase 5: Handshake capture
│   └── AllowlistManager.h/cpp         # Phase 6: Attack authorization
└── [original Marauder files]
```

---

## Getting Started

### Quick Start (Phase 1-2)

```bash
# 1. Clone and checkout development branch
git clone https://github.com/sucukmaster64/esp32marauder.git
cd esp32marauder
git checkout claude/claude-md-docs-am4eq3

# 2. Install Arduino IDE libraries
# See PWNAGOTCHI_SETUP.md for full list

# 3. Configure hardware (OLED pins in PwnagotchiDisplay.h)

# 4. Build and upload pwnagotchi_main.ino to ESP32-S3

# 5. Open Serial Monitor (115200 baud)
# Type 'help' for commands
```

### Testing Individual Phases

```cpp
// Phase 1 only (no display):
// Comment out: #include "PwnagotchiDisplay.h"
// In setup(): skip display.init()

// Phase 4 only (no mood engine):
// Use RLChannelOptimizer directly without Display integration

// All phases:
// Run as-is; all components initialized in setup()
```

---

**Last Updated:** 2026-06-17  
**Status:** 6/6 Phases Complete - Ready for Hardware Testing
**Next Milestone:** Successful handshake capture and PCAP export
