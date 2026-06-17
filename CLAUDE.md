# ESP32-S3 Pwnagotchi Clone - Development Guide

## Project Overview

This is a fork of **ESP32Marauder** (base: `justcallmekoko/ESP32Marauder`) tasked with building a **lightweight Pwnagotchi-inspired Wi-Fi awareness tool** for the **ESP32-S3 SuperMini** with a **128x64 I2C OLED display** (SSD1306/SH1106).

### Core Goals

1. **Reuse Marauder's WiFi core** — Extract and adapt Wi-Fi sniffing, beacon/probe detection, EAPOL handshake capture, and injection capabilities
2. **Custom personality layer** — Replace Marauder's complex Display/Menu/Keyboard system with a simple mood-based visual interface
3. **Reinforcement Learning for channel hopping** — Implement a lightweight RL network in pure C (no TFLite) to optimize AP discovery
4. **Persistent storage** — Save handshakes (`.pcap`) and statistics via NVS for later analysis
5. **Security-first design** — Active attacks (deauth, probe spam) restricted to an allowlist; passive scanning unrestricted

### Target Hardware

- **Microcontroller:** ESP32-S3 SuperMini
- **Display:** 128x64 I2C OLED (SSD1306/SH1106)
- **Power:** LiPo 500mAh + TP4056 charging module
- **Connectivity:** Wi-Fi 802.11 b/g/n (via ESP32-S3)

---

## Repository Structure

```
esp32marauder/
├── esp32_marauder/              # Main application source
│   ├── WiFiScan.cpp/h           # Wi-Fi scanning, sniffing, injection (core to extract)
│   ├── Display.cpp/h            # [DO NOT REUSE] Marauder's display driver
│   ├── MenuFunctions.cpp/h      # [DO NOT REUSE] Complex menu system
│   ├── TouchKeyboard.cpp/h      # [DO NOT REUSE] Touch/keyboard input
│   ├── Keyboard.cpp/h           # [DO NOT REUSE] Keyboard support
│   ├── Settings.cpp/h           # Configuration and NVS storage
│   ├── configs.h                # Hardware configuration defines
│   ├── EvilPortal.cpp/h         # Portal/injection utilities (may reuse parts)
│   ├── SDInterface.cpp/h        # SD card I/O (reusable for pcap dumps)
│   ├── BatteryInterface.cpp/h   # Battery monitoring (optional)
│   ├── GpsInterface.cpp/h       # [DO NOT USE] GPS (not in scope)
│   ├── LEDInterface.cpp/h       # Status LED control (reusable)
│   ├── Buffer.cpp/h             # Ring buffer for packet storage
│   ├── Assets.h                 # Icons, strings, resources
│   ├── utils.h                  # Utility functions
│   └── (various device-specific files)
├── libraries/                   # External libraries
│   ├── ESPAsyncWebServer/       # Web server (not in initial scope)
│   └── (other ESP-IDF libraries)
├── User_Setup_*.h               # Device-specific configuration templates
├── README.md                    # Original Marauder documentation
└── FlashFiles/                  # Pre-built binaries, firmware files
```

### Key WiFiScan Functions to Extract

These functions in `WiFiScan.cpp/h` are the backbone of passive sniffing and active attacks:

```cpp
// Mode setup
void setWiFiMode(wifi_mode_t mode, wifi_promiscuous_cb_t cb);

// Sniffing callbacks (passive, no filtering needed)
void beaconSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
void apSnifferCallbackFull(void* buf, wifi_promiscuous_pkt_type_t type);
void eapolSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);
void wifiSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type);

// Packet injection (active—REQUIRES ALLOWLIST CHECK)
void sendDeauthFrame(uint8_t* bssid, uint8_t channel, uint8_t* mac);
void sendProbeAttack(unsigned long currentTime);
void sendProbeRequest(uint8_t* targetSSID, uint16_t ssidLen);

// Channel management
void channelHop(bool filtered = false, bool ranged = false);
void changeChannel(uint8_t channel);

// Pwnagotchi beacon detection (reference)
void processPwnagotchiBeacon(wifi_promiscuous_pkt_type_t type, uint8_t* frame, uint16_t length);
```

---

## Architecture & Design Patterns

### 1. **WiFi Core Layer** (Extracted from Marauder)

- **No Display dependencies:** All debug output → Serial (no `Display.print()` calls)
- **Callback-based design:** Use function pointers for packet callbacks instead of global state
- **Ring buffer:** Store packets temporarily (Buffer.cpp) before processing
- **Channel hopping:** Implement as a separate, testable state machine

### 2. **Mood Engine** (State Machine)

Simple finite-state machine (no ML) that drives the face/animation on the OLED:

```cpp
enum MoodState {
    MOOD_HAPPY,      // New APs detected
    MOOD_EXCITED,    // Handshake captured
    MOOD_SAD,        // No new activity (idle timer)
    MOOD_SLEEPING,   // WiFi module off (low power)
    MOOD_SCANNING    // Active channel hop
};

struct StateVector {
    uint32_t seenAPCount;      // Total unique APs this session
    uint32_t newAPCount;       // New APs since last epoch
    uint32_t clientCount;      // Detected clients
    uint32_t handshakeCount;   // EAPOL handshakes captured
    uint8_t  currentChannel;   // Current Wi-Fi channel
    uint32_t epochTimestamp;   // Seconds since last state update
};
```

Rules:
- **HAPPY** if `newAPCount > 0` in the last epoch
- **EXCITED** if handshake detected in the last second
- **SAD** if no new APs for > 30 seconds
- **SLEEPING** if WiFi module disabled
- **SCANNING** during active channel hop

### 3. **Reinforcement Learning for Channel Hopping** (Lightweight)

A simple 2-layer dense network in pure C:

```
Input (4 neurons):
  - Current channel (0-13)
  - Time since last AP discovery (seconds, normalized)
  - Handshakes in buffer (normalized)
  - Client count (normalized)

Hidden layer (8 neurons, ReLU)
Output layer (13 neurons, softmax) → next channel
```

**Training:**
- Reward = new APs detected + (2× new handshakes) since last hop
- Simple gradient descent (no backprop framework)
- Weights stored in NVS, updated every 100 hops
- Baseline: Round-robin channel hopping (for comparison)

### 4. **Allowlist Gate** (Security Hardpoint)

```cpp
// In configs.h
#define ALLOWLIST_MAX_ENTRIES 16
struct AllowlistEntry {
    uint8_t bssid[6];           // MAC address
    char ssid[32];              // SSID string
    bool active;                // Enable/disable per entry
};

// In WiFiCore
bool isAllowlisted(uint8_t* bssid, const char* ssid);
// Called by sendDeauthFrame(), sendProbeAttack(), etc.
// Returns false → abort attack
```

### 5. **Persistent Storage** (NVS)

```cpp
namespace Preferences {
    "stats/handshakes"       // uint32_t count
    "stats/aps"              // uint32_t count
    "stats/runtime"          // uint32_t seconds
    "rl/weights"             // Serialized NN weights
    "allowlist/*"            // Per-entry allowlist data
}
```

---

## Development Workflow & Conventions

### Coding Standards

- **Language:** C/C++ (Arduino/ESP-IDF)
- **Formatting:** 4-space indentation, `camelCase` for variables/functions, `UPPER_CASE` for macros
- **Comments:** Minimal—only explain WHY non-obvious behavior happens, not WHAT the code does
- **Error handling:** Trust internal guarantees; validate only at system boundaries (user input, external APIs)
- **No premature abstraction:** Three duplicate lines are better than a leaky abstraction

### Compilation & Build

Targets Arduino IDE + ESP-IDF. Configuration via `User_Setup_*.h` or `configs.h`:

```cpp
#define PWNAGOTCHI_CLONE          // Enable pwnagotchi mode (new define)
#define OLED_TYPE_SSD1306         // or SH1106
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_I2C_ADDR 0x3C        // Default; may vary
#define OLED_SDA 6                // GPIO6 for XIAO ESP32-S3
#define OLED_SCL 7                // GPIO7 for XIAO ESP32-S3
```

### Version Control

- Work exclusively on branch: `claude/claude-md-docs-am4eq3`
- Commit message format: `<type>: <brief description>`
  - `feat: Add mood engine state machine`
  - `refactor: Extract WiFi core from Display dependencies`
  - `fix: Allowlist gate in deauth function`
  - `docs: Update CLAUDE.md with RL architecture`
- Meaningful commits between milestones, not every line change

---

## Implementation Phases

### Phase 1: Minimal Launchable Build (WiFi Core)

**Goal:** Compile and run WiFi sniffing with **no display dependency**.

**Tasks:**
1. Create `PwnagotchiWiFiCore.cpp/h` wrapping WiFiScan functions
   - Remove all `Display.print()`, `Display.update()` calls
   - Replace with `Serial.printf()` debug output
   - Expose sniffing callbacks via function pointers
2. Build skeleton `main.ino` (or `esp32_marauder.ino`)
   - Initialize WiFi core in promiscuous mode
   - Log detected APs, clients, handshakes to Serial
3. Test on hardware: Verify packets are captured

**Deliverable:** Compiling firmware that logs WiFi activity to Serial; no crashes on channel hop.

---

### Phase 2: OLED Display & Mood Placeholder

**Goal:** Render mood states as text on OLED; integrate Adafruit_SSD1306 library.

**Tasks:**
1. Add OLED library (Adafruit_SSD1306 + Adafruit_GFX)
2. Create `PwnagotchiDisplay.cpp/h`
   - `displayMood(MoodState mood, const StateVector& state)` → text/placeholder icons
   - `displayWiFiStats(const StateVector& state)` → AP count, clients, channel
3. Main loop: Update mood every 2–5 seconds based on StateVector
4. Test: OLED shows text like "HAPPY [AP:12 CH:6]" and updates

**Deliverable:** Firmware boots, OLED shows live WiFi stats and mood text; WiFi sniffing still works in background.

---

### Phase 3: Mood Engine & Pixel Faces

**Goal:** Implement state machine; render simple pixel faces (3×5 grid of pixels per "emotion").

**Tasks:**
1. Implement `MoodEngine.cpp/h`
   - StateVector aggregation every 2 seconds
   - Transition logic (HAPPY → EXCITED on handshake, etc.)
   - Timeout counters (SAD after 30s idle)
2. Create pixel face patterns
   - HAPPY: `:-)` smile
   - SAD: `:-(` frown
   - EXCITED: `o_o` wide eyes
   - SLEEPING: `-_-` closed eyes
3. `PwnagotchiDisplay.cpp` updates to draw faces instead of text
4. Test: Manually trigger APs and handshakes; watch mood change

**Deliverable:** Device displays appropriate face in response to WiFi activity; mood transitions are smooth and intuitive.

---

### Phase 4: Reinforcement Learning for Channel Hopping

**Goal:** Implement lightweight 2-layer NN; compare against round-robin.

**Tasks:**
1. Create `RLChannelOptimizer.cpp/h`
   - State → (4 floats): current_channel, time_idle, handshakes, clients
   - Forward pass: Dense[4→8]ReLU + Dense[8→13]Softmax → action (next channel)
   - Weight storage in NVS (binary serialization)
   - Update rule: simple SGD on reward (new APs + 2× new handshakes)
2. Implement basic training loop
   - Accumulate reward over 10 hops
   - Update weights every 100 hops (or manually via serial command)
3. Logging for comparison: track hops-per-handshake for RL vs. round-robin
4. Test: Run both policies; log statistics to SD card or NVS

**Deliverable:** Firmware can switch between RL and round-robin policies; logging shows RL finds handshakes faster (or equivalent).

---

### Phase 5: Persistent Storage & Pcap Dumps

**Goal:** Save EAPOL handshakes as `.pcap` files; persist stats across reboots.

**Tasks:**
1. Create `PcapDumper.cpp/h`
   - Format EAPOL packets as PCAP file header + packet records
   - Write to LittleFS/SPIFFS with timestamps
   - Limit to 10–20 handshakes per session (storage constraints)
2. NVS persistence
   - Save total AP count, handshake count, session duration
   - Restore on boot; display "Lifetime: X APs, Y handshakes"
3. USB or OTA export (optional for first version)
   - Serial command to dump pcap over USB
   - Or expose via simple HTTP endpoint (optional)

**Deliverable:** Device retains stats across power cycles; handshakes are saved in valid PCAP format for external password cracking.

---

### Phase 6: Allowlist Gate & Final Security Hardening

**Goal:** Lock active attacks behind a configuration allowlist.

**Tasks:**
1. Create `AllowlistManager.cpp/h`
   - NVS-backed list of allowed SSIDs/BSSIDs
   - `isAllowlisted(bssid, ssid)` function
2. Gate all active attacks
   - `sendDeauthFrame()` returns error if not allowlisted
   - `sendProbeAttack()` skips non-allowlisted targets
   - Passive sniffing always allowed
3. Serial command interface to manage allowlist
   - `allowlist add <SSID> <BSSID>` → add entry
   - `allowlist remove <SSID>` → remove entry
   - `allowlist list` → show current entries
4. Test: Attempt deauth on non-allowlisted AP; verify no frame sent

**Deliverable:** Device will not perform active attacks on unlisted networks; passive scans continue unrestricted.

---

## Key Files to Create/Modify

### New Files (Pwnagotchi-specific)

```
esp32_marauder/
├── PwnagotchiWiFiCore.cpp/h         # Extracted WiFi core (Phase 1)
├── MoodEngine.cpp/h                 # State machine (Phase 3)
├── PwnagotchiDisplay.cpp/h          # OLED rendering (Phase 2+)
├── RLChannelOptimizer.cpp/h         # RL network (Phase 4)
├── PcapDumper.cpp/h                 # Handshake storage (Phase 5)
├── AllowlistManager.cpp/h           # Security gate (Phase 6)
└── pwnagotchi_main.ino              # Main sketch (all phases)
```

### Existing Files to Adapt

```
esp32_marauder/
├── WiFiScan.cpp/h                   # Extract core functions; stub Display calls
├── configs.h                        # Add PWNAGOTCHI_CLONE mode, OLED pins
├── settings.cpp/h                   # Extend NVS for RL weights, allowlist
├── Buffer.cpp/h                     # Ensure packet storage works standalone
└── [Skip] Display.cpp, MenuFunctions.cpp, TouchKeyboard.cpp, Keyboard.cpp
```

---

## Security & Safety Reminders

### Non-Negotiable Rules

1. **Passive scanning is always allowed** — no allowlist gate
2. **Active attacks require allowlist entry** — deauth, probe spam, beacon spam, etc.
3. **Allowlist check is mandatory** in every `sendDeauthFrame()`, `sendProbeAttack()` call
4. **No credential storage** on device (handshakes are captures for external cracking, not local password testing)
5. **No persistent logging of SSIDs/clients** to external APIs (local-only storage)

### Testing Checklist Before Each Phase

- [ ] Code compiles without warnings
- [ ] No global state dependencies on removed modules (Display, Menu, etc.)
- [ ] Serial output shows expected debug info
- [ ] No buffer overflows (use bounds checks)
- [ ] WiFi core survives 1-hour continuous operation (no memory leaks)

---

## Debugging & Development Tips

### Serial Monitor Commands (Phase 3+)

Implement a simple CLI for testing:

```
> help
  help         - Show this message
  wifi on      - Start WiFi sniffing
  wifi off     - Stop WiFi sniffing
  hop <ch>     - Jump to channel <ch> (1-13)
  mood <mode>  - Override mood (0=happy, 1=excited, etc.)
  allowlist    - Show current allowlist
  stats        - Print current statistics
  reboot       - Restart device
```

### Logging Levels

Use compile-time macros:

```cpp
#define PWNAGOTCHI_DEBUG 1   // Verbose logging
#define PWNAGOTCHI_INFO  1   // State changes, attacks
#define PWNAGOTCHI_WARN  1   // Errors, allowlist violations

#if PWNAGOTCHI_DEBUG
  #define LOG_D(...) Serial.printf("[DEBUG] " __VA_ARGS__)
#endif
```

### Memory Profiling

ESP32-S3 has ~320 KB SRAM available. Monitor with:

```cpp
void printMemoryStats() {
    Serial.printf("Free SRAM: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
}
```

Call periodically in main loop to detect leaks.

---

## External Dependencies

### Libraries to Add (Arduino IDE)

- **Adafruit_SSD1306** — OLED driver
- **Adafruit-GFX-Library** — Graphics primitives
- **ArduinoJson** — JSON (already in Marauder; used for settings)
- **ESP32Ping** — Network utilities (already present)
- **ESP-IDF** — Core framework (built in)

### Do NOT Add

- TensorFlow Lite Micro (RL network is hand-coded C)
- Any cloud/WiFi connectivity library beyond ESP-IDF
- Unnecessary abstraction layers

---

## Next Steps for AI Assistants

1. **Understand the mission:** Lightweight Pwnagotchi clone, security-first, Wi-Fi sniffing focus
2. **Phased approach:** Build one phase at a time; ensure each compiles and runs
3. **Extract, don't rewrite:** Reuse WiFiScan callbacks and packet handling; discard Marauder UI
4. **Test on hardware:** Development on ESP32-S3 SuperMini + OLED
5. **Security gate first:** Allowlist is hard requirement, not optional
6. **Commit after each phase:** Clear, descriptive commit messages

---

## References

- **Marauder docs:** `/esp32marauder/README.md`
- **Original Pwnagotchi:** https://github.com/evilsocket/pwnagotchi
- **ESP32-S3 XIAO dev board:** https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
- **Adafruit SSD1306 library:** https://github.com/adafruit/Adafruit_SSD1306

---

**Last Updated:** 2026-06-17  
**Status:** Ready for Phase 1 development
