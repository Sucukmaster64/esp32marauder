# ESP32-S3 Pwnagotchi Clone - Setup Guide

## Hardware Configuration

### Microcontroller
- **Board:** ESP32-S3 SuperMini (or compatible)
- **Variant:** ESP32-S3 with WiFi 802.11 b/g/n

### Display
- **Module:** SSD1306/SH1106 128x64 I2C OLED
- **Default I2C Address:** 0x3C
- **Connection:**
  - SDA → GPIO 6 (configurable in `PwnagotchiDisplay.h`)
  - SCL → GPIO 7 (configurable in `PwnagotchiDisplay.h`)
  - GND → GND
  - VCC → 3.3V

### Power
- **Battery:** LiPo 500mAh (3.7V)
- **Charging Module:** TP4056 (with protection)
- **Connection:**
  - BAT+ → TP4056 OUT+
  - BAT- → TP4056 OUT-
  - TP4056 IN+ → USB 5V (charging)
  - TP4056 OUT → ESP32-S3 Power (through diode/switch recommended)

## Arduino IDE Setup

### Board Selection
1. **Board:** XIAO_ESP32S3 (or esp32:esp32:esp32s3)
2. **Core:** esp32 by Espressif Systems (version 2.0.x or higher)

### Required Libraries
Add via Arduino IDE Library Manager:
- **Adafruit_SSD1306** (by Adafruit)
- **Adafruit-GFX-Library** (by Adafruit)
- **ArduinoJson** (by Benoit Blanchon)
- **ESP32Ping** (by Marian Craciunescu)

### Installation
```bash
# Via Arduino CLI:
arduino-cli lib install "Adafruit SSD1306"
arduino-cli lib install "Adafruit GFX Library"
arduino-cli lib install "ArduinoJson"
arduino-cli lib install "ESP32Ping"
```

## Compilation Steps

1. **Open Arduino IDE**
2. **Select Board:** Tools → Board → esp32 → XIAO_ESP32S3
3. **Set Flash Size:** Tools → Flash Size → 4MB
4. **Set Partition Scheme:** Tools → Partition Scheme → Minimal SPIFFS
5. **Select Serial Port:** Tools → Port → `/dev/ttyUSB0` (or your port)
6. **Open Sketch:** File → Open → `esp32_marauder/pwnagotchi_main.ino`
7. **Verify:** Sketch → Verify/Compile
8. **Upload:** Sketch → Upload

## Serial Monitor Usage

After uploading, open Serial Monitor (Tools → Serial Monitor):
- **Baud Rate:** 115200
- **Line Ending:** Newline (or CR+LF)

### Example Commands
```
help          # Show command list
stats         # Print WiFi statistics
channel 6     # Jump to Wi-Fi channel 6
hop           # Manual channel hop
display       # Show stats on OLED
mood 1        # Set mood to HAPPY
reboot        # Restart device
```

## Troubleshooting

### Display Not Showing

1. **Check I2C Connection:**
   - Use serial command to verify: no errors on startup
   - Monitor OLED power (should be steady 3.3V)

2. **Verify I2C Address:**
   - If using different module, scan I2C:
     ```cpp
     // Add to setup():
     Wire.begin(6, 7);
     for (addr = 0x01; addr < 127; addr++) {
         Wire.beginTransmission(addr);
         if (Wire.endTransmission() == 0) {
             Serial.printf("Found device at 0x%02X\n", addr);
         }
     }
     ```

3. **Check Pin Configuration:**
   - Edit `PwnagotchiDisplay.h`:
     ```cpp
     #define OLED_SDA 6  // Change if using different pins
     #define OLED_SCL 7
     ```

### WiFi Not Sniffing

1. **Verify Setup:**
   - Check serial output for "[WiFiCore] Initialization complete"
   - Watch for beacon/probe messages

2. **Channel Issue:**
   - Manual channel set: `channel 1` → `channel 13`
   - Verify ESP32-S3 antennas are properly attached

### Compilation Errors

1. **Missing Libraries:**
   - Install via Library Manager (see above)
   - Or copy libraries to `Arduino/libraries/`

2. **Pin Conflicts:**
   - Check `OLED_SDA` and `OLED_SCL` don't conflict with WiFi pins
   - Default ESP32-S3 WiFi uses internal pins (no conflicts expected)

## Development Notes

### Enabling Debug Logging

Edit `PwnagotchiWiFiCore.cpp` and `PwnagotchiDisplay.cpp`:
- Uncomment `Serial.println()` statements for verbose output
- Add custom logging via `#define DEBUG 1`

### Testing on Real Hardware

1. **Start sniffing:** `start`
2. **Observe OLED:** Mood and statistics should update
3. **Monitor Serial:** Watch beacon/probe detections
4. **Test channel hop:** `hop` command every 100ms
5. **Verify persistence:** Stats should accumulate over time

## Phase 1 vs Phase 2

**Phase 1** (WiFi Core only):
- No OLED display
- Serial-only output
- Simple beacon sniffing

**Phase 2** (Current):
- OLED display integration
- Mood states (text-based)
- Real-time statistics on screen
- Automatic mood updates based on WiFi activity

## Next Steps (Phase 3)

- Pixel-based face rendering
- More sophisticated mood animations
- Enhanced state machine transitions
