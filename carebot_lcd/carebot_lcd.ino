#include <Arduino.h>
#include <lvgl.h>
#include <SPI.h>
#include <Wire.h>
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

// Include our modular components
#include "common.h"
#include "display.h"
#include "touch.h"
#include "lvgl_controller.h"
#include "audio.h"
#include "console.h"
#include "serial_lcd.h"
#include "menu.h"
#include "event.h"

// Task handles
TaskHandle_t displayTaskHandle = NULL;
TaskHandle_t lvglTaskHandle = NULL;
TaskHandle_t touchTaskHandle = NULL;
TaskHandle_t serialTaskHandle = NULL;

void initializeSystem();

// System initialization function
void initializeSystem() {
    // Initialize logger first for debugging
    logMessage("Setup", LOG_LEVEL_INFO, "System initialization started");
          
    // Initialize LVGL
    lvgl_init();

    // Initialize display
    display_init();
    
    // Initialize touch
    Arduino_GFX* gfx = get_gfx_instance();
    if (gfx) {
        touch_init(gfx->width(), gfx->height(), gfx->getRotation());
    } else {
        logMessage("Setup", LOG_LEVEL_INFO, "Failed to get GFX instance, touch initialization skipped");
    }

    current_ui_state.initialized = false;
    // Create main UI
    lvgl_create_app_ui();
    
    // 모든 초기화가 끝난 후 강제 렌더링
    lv_refr_now(NULL);

    #if 1
    // Configure backlight
    #ifdef GFX_BL
        pinMode(GFX_BL, OUTPUT);
        digitalWrite(GFX_BL, HIGH);
        backlightInitialized = true;
        logMessage("DISPLAY", LOG_LEVEL_INFO, "Backlight initialized");
    #endif
    
    #endif
    logMessage("Setup", LOG_LEVEL_INFO, "System initialization completed");
}

void setup() {
    // Start serial communication
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n+++ HygeraApplication Starting +++");
    
    // Disable watchdog timer
    esp_task_wdt_deinit();
    
    loadSettings();  // 설정 로드

#if 1
    // 오디오 파일 저장 방법, 0: sd_card, 1: RAM, 2: SPIFFS
    uint8_t storage_type = settings.storage_type;
    if (storage_type < 0 || storage_type >= 3) storage_type = currentStorageType;
    // Initialize audio system
    if (!audio_init((AudioStorageType)storage_type)) {
        Serial.println("Failed to initialize audio system");
    }
#endif

    // Initialize the system
    initializeSystem();
    
#if 0
    // Turn on backlight (from original code)
    #ifdef GFX_BL
        pinMode(GFX_BL, OUTPUT);
        digitalWrite(GFX_BL, HIGH);
    #endif
#endif
    // Create tasks for different modules
    xTaskCreatePinnedToCore(
        display_task,     // Function to implement the task
        "DisplayTask",    // Name of the task
        4096,             // Stack size in words
        NULL,             // Task input parameter
        2,                // Priority of the task
        &displayTaskHandle,  // Task handle
        1                 // Core where the task should run
    );
    
    xTaskCreatePinnedToCore(
        lv_timer_task,    // Function to implement the task
        "LvglTimerTask",  // Name of the task
        4096,             // Stack size in words
        NULL,             // Task input parameter
        4,                // Priority of the task
        &lvglTaskHandle,  // Task handle
        1                 // Core where the task should run
    );
    
    xTaskCreatePinnedToCore(
        touch_task,       // Function to implement the task
        "TouchTask",      // Name of the task
        2048,             // Stack size in words
        NULL,             // Task input parameter
        2,                // Priority of the task
        &touchTaskHandle, // Task handle
        1                 // Core where the task should run
    );
    
    // UI anumation용
    setupAnimationTask();

    xTaskCreatePinnedToCore(
        TaskConsole,
        "Console",
         2*4096,
         NULL,
         1,
         NULL,
         0);

    //vTaskDelay(pdMS_TO_TICKS(1000));

    #if 1
    serial_lcd_init();
    #else
    xTaskCreatePinnedToCore(
        serialTask,       // Function to implement the task
        "SerialTask",     // Name of the task
        4096,             // Stack size in words
        NULL,             // Task input parameter
        1,                // Priority of the task
        &serialTaskHandle,// Task handle
        0                 // Core where the task should run
    );
    #endif
    vTaskDelay(pdMS_TO_TICKS(2000));
    current_ui_state.initialized = true;
    
    logMessage("Setup", LOG_LEVEL_INFO, "All tasks created and started");
}

void loop() {
    // 태스크에게 실행 기회를 주기 위한 딜레이
    vTaskDelay(pdMS_TO_TICKS(5));
}