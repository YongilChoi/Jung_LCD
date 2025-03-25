#include "lvgl_controller.h"
#include "display.h"
#include "common.h"
#include "touch.h"
#include "event.h"
#include "menu.h"

// Define display variables
uint32_t screenWidth;
uint32_t screenHeight;
uint32_t bufSize;
lv_disp_draw_buf_t draw_buf;
lv_color_t *disp_draw_buf;
lv_disp_drv_t disp_drv;
SemaphoreHandle_t displayMutex = NULL;

// Internal GFX instance
Arduino_GFX *gfx = NULL;
bool backlightInitialized = false;

// Get GFX instance
Arduino_GFX* get_gfx_instance() {
    if (gfx == NULL) {
        // ESP32 QSPI setup for NV3041A display
        Arduino_DataBus *bus = new Arduino_ESP32QSPI(
            45 /* cs */, 47 /* sck */, 21 /* d0 */, 48 /* d1 */, 
            40 /* d2 */, 39 /* d3 */);
        Arduino_GFX *g = new Arduino_NV3041A(
            bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);
        gfx = new Arduino_Canvas(480 /* width */, 272 /* height */, g);
    }
    return gfx;
}

// Initialize display
void display_init() {
    logMessage("DISPLAY", LOG_LEVEL_INFO, "Starting display initialization");
    
    // Create display mutex if not already created
    if (displayMutex == NULL) {
        displayMutex = xSemaphoreCreateMutex();
        if (displayMutex == NULL) {
            logMessage("DISPLAY", LOG_LEVEL_ERROR, "Failed to create display mutex!");
            return;
        }
    }
    
    // Initialize GFX
    gfx = get_gfx_instance();
    
    // Begin display operation
    if (!gfx->begin()) {
        logMessage("DISPLAY", LOG_LEVEL_ERROR, "gfx->begin() failed!");
        return;
    }
    
    // 초기 화면을 검은색으로 채우기
    gfx->fillScreen(BLACK);
    
    // 중요: CANVAS 모드에서는 반드시 flush 호출
    #ifdef CANVAS
        gfx->flush();
        logMessage("DISPLAY", LOG_LEVEL_INFO, "Initial flush called");
    #endif
    
    #if 0
    // Configure backlight
    #ifdef GFX_BL
        pinMode(GFX_BL, OUTPUT);
        digitalWrite(GFX_BL, HIGH);
        backlightInitialized = true;
        logMessage("DISPLAY", LOG_LEVEL_INFO, "Backlight initialized");
    #endif
    #endif
    
    // Set screen dimensions
    screenWidth = gfx->width();
    screenHeight = gfx->height();
    
    // Configure buffer size
    #ifdef DIRECT_MODE
        bufSize = screenWidth * screenHeight; // Full frame buffer
    #else
        //bufSize = screenWidth * 40; // Partial buffer
        bufSize = screenWidth * 80; // Partial buffer
    #endif
    
    // Allocate memory for display buffer
    #ifdef ESP32
        disp_draw_buf = (lv_color_t *)heap_caps_malloc(
            sizeof(lv_color_t) * bufSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!disp_draw_buf) {
            // Try again without MALLOC_CAP_INTERNAL
            disp_draw_buf = (lv_color_t *)heap_caps_malloc(
                sizeof(lv_color_t) * bufSize, MALLOC_CAP_8BIT);
        }
    #else
        disp_draw_buf = (lv_color_t *)malloc(sizeof(lv_color_t) * bufSize);
    #endif
    
    if (!disp_draw_buf) {
        logMessage("DISPLAY", LOG_LEVEL_ERROR, "LVGL disp_draw_buf allocation failed!");
        return;
    }
    
    // Initialize LVGL display buffer
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, bufSize);
    
    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    
    #ifdef DIRECT_MODE
        disp_drv.direct_mode = true;
    #endif
    
    lv_disp_drv_register(&disp_drv);
    
    logMessage("DISPLAY", LOG_LEVEL_INFO, "Display initialized successfully");
}

// Display flush callback
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    #if (LV_COLOR_16_SWAP != 0)
        gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
    #else
        gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
    #endif
    
    // CANVAS 모드에서는 flush 호출이 필요할 수 있음
    #ifdef CANVAS
        gfx->flush();
    #endif
    
    lv_disp_flush_ready(disp);
}

// Display task for flushing buffer to screen
void display_task(void *pvParameters) {
    logMessage("DISPLAY", LOG_LEVEL_INFO, "Display task started");
    
    for (;;) {
        // Try to take mutex with timeout
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            // CANVAS 모드에서는 반드시 flush 호출
            #ifdef CANVAS
                gfx->flush();
            #endif
            
            // Release mutex
            xSemaphoreGive(displayMutex);
        }
        
        // Small delay to avoid hogging CPU
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// LVGL timer task
void lv_timer_task(void *pvParameters) {
    logMessage("DISPLAY", LOG_LEVEL_INFO, "LVGL timer task started");
    
    for (;;) {
        // Take mutex before handling LVGL tasks
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            lv_timer_handler();

            // 애니메이션 상태 업데이트
            updateAnimations();

            // UI 상태 업데이트
            updateUI();

            //log_ui_timer();

            xSemaphoreGive(displayMutex);
        }
        
        // LVGL recommends a 5ms delay between timer calls
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
