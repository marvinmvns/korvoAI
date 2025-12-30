// Test PSRAM allocation - compile and upload to verify PSRAM works
#include <Arduino.h>
#include <esp_heap_caps.h>

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("       PSRAM TEST - ESP32-KORVO");
    Serial.println("========================================\n");

    // Basic info
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("Chip Cores: %d\n", ESP.getChipCores());
    Serial.printf("CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());

    Serial.println("\n--- MEMORY INFO ---");
    Serial.printf("Total Heap: %d bytes\n", ESP.getHeapSize());
    Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Min Free Heap: %d bytes\n", ESP.getMinFreeHeap());

    Serial.println("\n--- PSRAM INFO ---");
    Serial.printf("PSRAM Size: %d bytes\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());

    // Check if PSRAM is available
    if (ESP.getPsramSize() == 0) {
        Serial.println("\n*** PSRAM NOT DETECTED! ***");
        Serial.println("Possible causes:");
        Serial.println("1. PSRAM not enabled in build config");
        Serial.println("2. Wrong board definition");
        Serial.println("3. Hardware issue");

        // Try heap_caps anyway
        Serial.println("\n--- Testing heap_caps_malloc ---");
        size_t spiram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        size_t spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        Serial.printf("SPIRAM Total (heap_caps): %d bytes\n", spiram_size);
        Serial.printf("SPIRAM Free (heap_caps): %d bytes\n", spiram_free);
    } else {
        Serial.println("\n*** PSRAM DETECTED! ***");

        // Test allocation
        Serial.println("\n--- Testing PSRAM Allocation ---");

        size_t testSizes[] = {1024*1024, 2*1024*1024, 4*1024*1024, 6*1024*1024};

        for (int i = 0; i < 4; i++) {
            size_t size = testSizes[i];
            Serial.printf("Trying to allocate %d MB... ", size / (1024*1024));

            void* ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
            if (ptr) {
                Serial.println("SUCCESS!");

                // Write test
                memset(ptr, 0xAA, size);
                uint8_t* test = (uint8_t*)ptr;
                bool ok = (test[0] == 0xAA && test[size-1] == 0xAA);
                Serial.printf("  Write/Read test: %s\n", ok ? "PASSED" : "FAILED");

                heap_caps_free(ptr);
                Serial.printf("  Free PSRAM after free: %d bytes\n", ESP.getFreePsram());
            } else {
                Serial.println("FAILED");
            }
        }
    }

    // Also try ps_malloc
    Serial.println("\n--- Testing ps_malloc ---");
    void* ps = ps_malloc(1024*1024);
    if (ps) {
        Serial.println("ps_malloc(1MB): SUCCESS");
        free(ps);
    } else {
        Serial.println("ps_malloc(1MB): FAILED");
    }

    Serial.println("\n========================================");
    Serial.println("              TEST COMPLETE");
    Serial.println("========================================");
}

void loop() {
    delay(10000);
}
