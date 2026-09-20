#include "BtScanner.h"

BtScanner* BtScanner::instance = nullptr;

// 1. FIX: Custom Scan Callback to grab real names dynamically
void BtScanner::app_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    if (event == ESP_BT_GAP_DISC_RES_EVT) {
        esp_bd_addr_t bda;
        memcpy(bda, param->disc_res.bda, ESP_BD_ADDR_LEN);
        String deviceName = "";

        // 1. FIX: Initialize pointers to catch the EIR properties safely out of the array
        uint8_t *eir = nullptr;
        uint8_t eir_len = 0;

        // 2. Loop through discovered peripheral properties to isolate the EIR packet
        for (int i = 0; i < param->disc_res.num_prop; i++) {
            if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_EIR) {
                eir = (uint8_t *)param->disc_res.prop[i].val;
                eir_len = (uint8_t)param->disc_res.prop[i].len;
                break; // EIR payload located, exit the descriptor filter loop
            }
        }

        // 3. Resolve the strings using the isolated EIR reference pointer data
        uint8_t rlen = 0;
        uint8_t *rdata = nullptr;
        
        if (eir != nullptr) {
            rdata = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rlen);
            if (!rdata) {
                rdata = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rlen);
            }
        }

        if (rdata) {
            char nameBuf[32] = {0};
            int nameLen = (rlen > 31) ? 31 : rlen;
            memcpy(nameBuf, rdata, nameLen);
            deviceName = String(nameBuf);
        } else {
            deviceName = "[Unknown Device]";
        }

        // Keep your existing duplicate registration check filtering logic below this line...

        // Check if we already registered this MAC to prevent duplicate prints
        bool exists = false;
        for (int i = 0; i < instance->deviceCount; i++) {
            if (memcmp(instance->foundDevices[i].bda, bda, ESP_BD_ADDR_LEN) == 0) {
                // Update name if it was previously unknown but now resolved
                if (instance->foundDevices[i].name == "[Unknown Device]" && deviceName != "[Unknown Device]") {
                    instance->foundDevices[i].name = deviceName;
                    Serial.printf("[Scan] Resolved identity for index %d: %s\n", i, deviceName.c_str());
                }
                exists = true;
                break;
            }
        }

        if (!exists && instance->deviceCount < MAX_DEVICES) {
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);
            
            memcpy(instance->foundDevices[instance->deviceCount].bda, bda, ESP_BD_ADDR_LEN);
            instance->foundDevices[instance->deviceCount].name = deviceName;
            
            Serial.printf("[%d] Found Target -> MAC: %s | Name: %s\n", instance->deviceCount, macStr, deviceName.c_str());
            instance->deviceCount++;
        }
    }
}

void BtScanner::startScanning() {
    Serial.println("\n[Scan] Cleaning device cache and launching discovery pipeline...");
    deviceCount = 0;
    currentState = STATE_SCANNING;
    
    // Register the custom callback to ensure names drop into our array accurately
    esp_bt_gap_register_callback(app_gap_callback);
    
    // FIX: Change to ESP_BT_INQ_MODE_GENERAL_INQUIRY (Removed _GAP_)
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
}

void BtScanner::connectToDevice(int index) {
    if (index >= deviceCount) {
        Serial.println("Invalid index choice.");
        return;
    }

    Serial.printf("[Linker] Manually targeting: %s\n", foundDevices[index].name.c_str());
    
    // Command the low-level stack to pair
    a2dpSource.source().connect_to(foundDevices[index].bda);
    
    Serial.println("[Linker] Connecting... checking link status shortly.");
    delay(2000); 
}