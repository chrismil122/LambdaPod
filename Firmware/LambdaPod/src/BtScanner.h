#ifndef BT_SCANNER_H
#define BT_SCANNER_H

#include <Arduino.h>
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "AudioTools.h"
#include "AudioTools/Communication/A2DPStream.h"
#include "BluetoothA2DPSource.h"

#define MAX_DEVICES 10

extern A2DPStream a2dpSource;
enum SystemState { STATE_AUTOCONNECT_WAIT, STATE_PLAYING, STATE_IDLE, STATE_SCANNING };

// Struct to hold discovered devices
struct DiscoveredDevice {
    esp_bd_addr_t bda;
    String name;
};

class BtScanner {
    public:
        static void app_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
        void startScanning();
        void connectToDevice(int index);
        SystemState currentState = STATE_AUTOCONNECT_WAIT;
        unsigned long stateTimer = 0;
        unsigned long autoconnectTimeout = 10000; // 10 seconds to look for autoconnect at boot

        BtScanner() {
        // 2. Point it to this specific instance when created
        instance = this; 
        }
    private:
        int deviceCount;
        uint8_t scanDuration;
        DiscoveredDevice foundDevices[MAX_DEVICES];
        static BtScanner* instance;
};

#endif