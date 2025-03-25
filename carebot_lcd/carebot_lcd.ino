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
#include "serial.h"
#include "menu.h"

// Task handles
TaskHandle_t displayTaskHandle = NULL;
TaskHandle_t lvglTaskHandle = NULL;
TaskHandle_t touchTaskHandle = NULL;
TaskHandle_t serialTaskHandle = NULL;
TaskHandle_t handleTaskHandle = NULL;

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
        touch_init(gfx->width(), gfx->height(), gfx->getRotation()); // 터치 초기화 함수가 정상 작동하는지 확인       
    } else {
        logMessage("Setup", LOG_LEVEL_INFO, "Failed to get GFX instance, touch initialization skipped");
    }
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
    Serial.begin(115200); //PC연결용 디버깅 포트 (USB)
	  Serial2.begin(115200, SERIAL_8N1, 44, 43); //to Main board 연결 케이블 시리얼 포트(4 Pin connector) 
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

	// 시리얼 통신을 위한 메시지 큐를 생성 
	  serialQueue = xQueueCreate(QUEUE_SIZE, sizeof(uint8_t[512]));  
	    if (serialQueue == NULL) {
	        Serial.println("Failed to create queue!");
	        return;
	    }

	// 메시지 큐 


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
        4096,             // Stack size in words
        NULL,             // Task input parameter
        5,                // Priority of the task
        &touchTaskHandle, // Task handle
        1                 // Core where the task should run
    );
    
    xTaskCreatePinnedToCore(
        TaskConsole,
        "Console",
         2*4096,
         NULL,
         1,
         NULL,
         0);
        // 시리얼  태스크 (Core  0  실행  )
    xTaskCreatePinnedToCore(
        serialTask,       // Function to implement the task
        "SerialTask",     // Name of the task
        4096,             // Stack size in words
        NULL,             // Task input parameter
        1,                // Priority of the task
        &serialTaskHandle,// Task handle
        0                 // Core where the task should run
    );
		// 핸들러 태스크 (Core 1  에서 실행)
	xTaskCreatePinnedToCore(
		handleTask,		 // Task 함수
		"HandleTask", 	 // Task 이름
		10000,			 // Stack 크기
		NULL, 			 // Task 파라미터
		2,				 // Task 우선순위
		&handleTaskHandle, // Task 핸들
		1 				 // Core 0에서 실행
	);
    
    logMessage("Setup", LOG_LEVEL_INFO, "All tasks created and started");
	// 마지막에 Free Heap 출력
  delay(500);  // ESP32의 시리얼 안정화를 위한 추가 대기
	Serial.print("Free heap: ");
	Serial.println(esp_get_free_heap_size());  // 남은 힙 메모리 출력
}

void loop() {
    // 태스크에게 실행 기회를 주기 위한 딜레이
    vTaskDelay(pdMS_TO_TICKS(5));
}