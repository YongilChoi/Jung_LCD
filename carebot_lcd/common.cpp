// common.cpp
#include <Arduino.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <time.h>
#include "FS.h"
#include "SD.h"
#include <nvs.h>
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "common.h"

SystemSettings settings;
char g_serialNumber[MAX_SERIAL_NUMBER_LENGTH] = "UNSET";
char g_current_version[] = "202503211400";

uint8_t debug_level = LOG_LEVEL_INFO;
//uint8_t debug_level = LOG_LEVEL_DEBUG;
int diag_mode = false;

// 로그 레벨을 문자열로 변환하는 함수
const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        default:              return "UNKNOWN";
    }
}

// 향상된 로그 메시지 함수
void logMessage(const char* taskName, LogLevel level, const char* format, ...) {
    if (level < debug_level) return;

    static char buffer[256]; // 로그 메시지를 저장할 버퍼
    
    // 현재 시간 가져오기
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    // 시간 문자열 생성
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    // 로그 메시지의 앞부분 생성 (시간, 태스크 이름, 로그 레벨)
    int prefixLen = snprintf(buffer, sizeof(buffer), "[%s] [%s] [%s] ", 
                             timeStr, taskName, logLevelToString(level));
    
    // 가변 인자를 사용하여 나머지 메시지 생성
    va_list args;
    va_start(args, format);
    vsnprintf(buffer + prefixLen, sizeof(buffer) - prefixLen, format, args);
    va_end(args);
    
    // 로그 출력 (여기서는 시리얼로 출력)
    Serial.println(buffer);
    
    // 필요한 경우 여기에 파일 로깅 또는 원격 로깅 로직을 추가할 수 있습니다.
}

/**
 * 특정 메모리 타입의 상태를 확인하는 함수
 * cap: MALLOC_CAP_INTERNAL, MALLOC_CAP_SPIRAM 등
 * name: 메모리 타입 이름 문자열
 */
void check_memory(uint32_t cap, const char* name) 
{
    uint32_t free_mem = heap_caps_get_free_size(cap);
    uint32_t largest_block = heap_caps_get_largest_free_block(cap);
    
    Serial.printf("%s 메모리: 가용 %d 바이트, 최대 블럭 %d 바이트\n", 
           name, free_mem, largest_block);
           
    // 단편화 수준 계산 (값이 낮을수록 단편화가 심함)
    float frag_metric = (float)largest_block / (float)free_mem * 100.0f;
    Serial.printf("%s 단편화 수준: %.1f%% (값이 100%%에 가까울수록 양호)\n", 
           name, frag_metric);
}
/**
 * ESP32 시스템 메모리 상태를 출력하는 함수
 */
void print_memory_info(void)
{
    // 힙 메모리 정보
    uint32_t free_heap = esp_get_free_heap_size();
    uint32_t min_free_heap = esp_get_minimum_free_heap_size();
    
    // IRAM (내부 RAM) 메모리 정보
    uint32_t free_iram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    uint32_t largest_iram_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    
    // SPIRAM (외부 RAM, PSRAM) 정보 - PSRAM 있는 경우만
    uint32_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    uint32_t largest_spiram_block = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    
    Serial.printf("\n--- 메모리 상태 ---\n");
    Serial.printf("Free Heap: %d 바이트\n", free_heap);
    Serial.printf("Min Free Heap: %d 바이트\n", min_free_heap);
    Serial.printf("Free IRAM: %d 바이트\n", free_iram);
    Serial.printf("Largest IRAM Block: %d 바이트\n", largest_iram_block);
    
    if (free_spiram > 0) {
        Serial.printf("Free SPIRAM: %d 바이트\n", free_spiram);
        Serial.printf("Largest SPIRAM Block: %d 바이트\n", largest_spiram_block);
    } else {
        Serial.printf("SPIRAM 사용 불가 또는 미장착\n");
    }
    
    // 힙 메모리 사용률 계산 (기본 힙 크기 약 320KB로 가정, 보드마다 다를 수 있음)
    float heap_usage = (1.0f - ((float)free_heap / 320000.0f)) * 100.0f;
    Serial.printf("힙 메모리 사용률: %.1f%%\n", heap_usage);

    // 각 메모리 유형별 상세 정보
  check_memory(MALLOC_CAP_INTERNAL, "내부 RAM");
  check_memory(MALLOC_CAP_SPIRAM, "외부 PSRAM");
  check_memory(MALLOC_CAP_DMA, "DMA 가능 RAM");

    Serial.printf("--------------------\n");
}

/**
 * ESP32 메모리 상태를 더 명확하게 출력하는 함수
 */
void print_memory_status(void)
{
    // 내부 RAM (IRAM)
    uint32_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    uint32_t internal_alloc = heap_caps_get_total_size(MALLOC_CAP_INTERNAL) - free_internal;
    uint32_t largest_internal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    
    // 외부 RAM (PSRAM)
    uint32_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    uint32_t total_spiram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    uint32_t largest_spiram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    
    // 전체 메모리
    uint32_t total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    uint32_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    uint32_t used_heap = total_heap - free_heap;
    
    // 메모리 사용률 계산
    float internal_usage = 0;
    float spiram_usage = 0;
    float total_usage = 0;
    
    if (heap_caps_get_total_size(MALLOC_CAP_INTERNAL) > 0) {
        internal_usage = 100.0f * internal_alloc / heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    }
    
    if (total_spiram > 0) {
        spiram_usage = 100.0f * (total_spiram - free_spiram) / total_spiram;
    }
    
    if (total_heap > 0) {
        total_usage = 100.0f * used_heap / total_heap;
    }
    
    // 형식화된 출력
    Serial.println("\n===== ESP32 메모리 상태 =====");
    
    Serial.println("\n----- 내부 RAM (IRAM) -----");
    Serial.printf("총 용량: %d KB\n", heap_caps_get_total_size(MALLOC_CAP_INTERNAL) / 1024);
    Serial.printf("사용 중: %d KB (%.1f%%)\n", internal_alloc / 1024, internal_usage);
    Serial.printf("가용 공간: %d KB\n", free_internal / 1024);
    Serial.printf("최대 연속 블록: %d KB\n", largest_internal / 1024);
    Serial.printf("단편화 수준: %.1f%%\n", 100.0f * largest_internal / free_internal);
    
    if (total_spiram > 0) {
        Serial.println("\n----- 외부 RAM (PSRAM) -----");
        Serial.printf("총 용량: %d KB (%d MB)\n", total_spiram / 1024, total_spiram / (1024*1024));
        Serial.printf("사용 중: %d KB (%.1f%%)\n", (total_spiram - free_spiram) / 1024, spiram_usage);
        Serial.printf("가용 공간: %d KB (%d MB)\n", free_spiram / 1024, free_spiram / (1024*1024));
        Serial.printf("최대 연속 블록: %d KB (%d MB)\n", largest_spiram / 1024, largest_spiram / (1024*1024));
        Serial.printf("단편화 수준: %.1f%%\n", 100.0f * largest_spiram / free_spiram);
    }
    
    Serial.println("\n----- 전체 메모리 요약 -----");
    Serial.printf("총 메모리: %d KB (%d MB)\n", total_heap / 1024, total_heap / (1024*1024));
    Serial.printf("사용 중인 메모리: %d KB (%.1f%%)\n", used_heap / 1024, total_usage);
    Serial.printf("가용 메모리: %d KB (%d MB)\n", free_heap / 1024, free_heap / (1024*1024));
    Serial.println("===========================\n");
}
// Function to initialize file systems
bool init_filesystem() {
    // Initialize SD card
    if (!SD.begin(10, SPI, 4000000)) {
        Serial.println("SD Card Mount Failed in init_filesystem");
        return false;
    }
    
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    
    return true;
}

void initializeNVS() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    logMessage("INIT-NVS", LOG_LEVEL_INFO, "NVS initialized");
}


void readNVS(const char* key, void* value, size_t length) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
           logMessage("NVS", LOG_LEVEL_INFO, "Error opening NVS handle");
    } else {
        err = nvs_get_blob(my_handle, key, value, &length);
        if (err != ESP_OK) {
               logMessage("NVS", LOG_LEVEL_INFO, "Error reading from NVS");
        }
        nvs_close(my_handle);
    }
}

void writeNVS(const char* key, const void* value, size_t length) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
           logMessage("NVS", LOG_LEVEL_INFO, "Error opening NVS handle");
    } else {
        err = nvs_set_blob(my_handle, key, value, length);
        if (err != ESP_OK) {
               logMessage("NVS", LOG_LEVEL_INFO, "Error writing to NVS");
        }
        err = nvs_commit(my_handle);
        if (err != ESP_OK) {
               logMessage("NVS", LOG_LEVEL_INFO, "Error committing NVS changes");
        }
        nvs_close(my_handle);
    }
}


#if 0
// 전체 시스템 초기화 함수
void initializeSystem() {

    loadSettings();  // 설정 로드

}
#endif

void loadSerialNumber() {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        logMessage("Settings", LOG_LEVEL_ERROR, "Error opening NVS handle");
        return;
    }

    size_t required_size = sizeof(g_serialNumber);
    err = nvs_get_str(my_handle, SERIAL_NUMBER_KEY, g_serialNumber, &required_size);
    if (err != ESP_OK) {
        logMessage("Settings", LOG_LEVEL_WARN, "Serial number not found in NVS");
        strcpy(g_serialNumber, "UNSET");
    }

    nvs_close(my_handle);
}

void saveSerialNumber() {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        logMessage("Settings", LOG_LEVEL_ERROR, "Error opening NVS handle");
        return;
    }

    err = nvs_set_str(my_handle, SERIAL_NUMBER_KEY, g_serialNumber);
    if (err != ESP_OK) {
        logMessage("Settings", LOG_LEVEL_ERROR, "Error saving serial number to NVS");
    } else {
        nvs_commit(my_handle);
        logMessage("Settings", LOG_LEVEL_INFO, "Serial number saved successfully");
    }

    nvs_close(my_handle);
}
#if 0
void setWiFiCredentials(const char* ssid, const char* password) {
    WiFi.disconnect();
    strncpy(settings.wifi_ssid, ssid, sizeof(settings.wifi_ssid) - 1);
    settings.wifi_ssid[sizeof(settings.wifi_ssid) - 1] = '\0';
    strncpy(settings.wifi_password, password, sizeof(settings.wifi_password) - 1);
    settings.wifi_password[sizeof(settings.wifi_password) - 1] = '\0';
    saveSettings();
    WiFi.begin(settings.wifi_ssid, settings.wifi_password);
}
#endif
void setSerialNumber(const char* newSerialNumber) {
    strncpy(g_serialNumber, newSerialNumber, sizeof(g_serialNumber) - 1);
    g_serialNumber[sizeof(g_serialNumber) - 1] = '\0';  // Ensure null-termination
    saveSerialNumber();
    logMessage("Settings", LOG_LEVEL_INFO, "Serial number set to: %s", g_serialNumber);
}

const char* getSerialNumber() {
    //return settings.serial_number;
    return g_serialNumber;
}

void PrintAllSettings() {
    Serial.println("All System Settings:");
    Serial.println("--------------------");
    #if 0
    // WiFi 설정
    Serial.print("WiFi SSID: ");
    Serial.println(settings.wifi_ssid);
    Serial.print("WiFi Password: ");
    Serial.println(settings.wifi_password);
    // Serial.println("WiFi Password: [HIDDEN]");  // 비밀번호 숨김 옵션 (주석 처리)
    
    // 디바이스 정보
    Serial.print("Device ID: ");
    Serial.println(settings.device_id);
    #endif
    Serial.print("Current Version: ");
    Serial.println(settings.current_version);
    Serial.println(g_current_version);

    Serial.print("Storage Type: ");
    Serial.println(settings.storage_type);
    Serial.print("Audio Volume: ");
    Serial.println(settings.volume_value);
  
    Serial.println("--------------------");
}

void defaultSetSettings() {
#if 0
    // WiFi 설정
    strncpy(settings.wifi_ssid, "sorynory", sizeof(settings.wifi_ssid) - 1);
    strncpy(settings.wifi_password, "0312954876", sizeof(settings.wifi_password) - 1);

    // 디바이스 정보
    // MAC 주소를 device_id로 설정
    uint64_t chipId = ESP.getEfuseMac();
    char defaultDeviceId[32];
    snprintf(defaultDeviceId, sizeof(defaultDeviceId), "%04X%08X", 
             (uint16_t)(chipId >> 32), (uint32_t)chipId);
    strncpy(settings.device_id, defaultDeviceId, sizeof(settings.device_id) - 1);
    //strncpy(settings.current_version, "1.0.0", sizeof(settings.current_version) - 1);
    //Serial.print("Storage Type: ");
#endif
    settings.storage_type = 1; // 오디오 파일 저장 방법, 0: sd_card, 1: RAM, 2: SPIFFS
    //Serial.print("Audio Volume: ");
    settings.volume_value = 5;
    // 설정 저장
    saveSettings();

    // 시리얼 번호 설정 (만약 설정되지 않았다면)
    #if 0
    if (strcmp(g_serialNumber, "UNSET") == 0) {
        char defaultSerial[32];
        uint64_t chipId = ESP.getEfuseMac();
        snprintf(defaultSerial, sizeof(defaultSerial), "ESP32-%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
        setSerialNumber(defaultSerial);
    }
    #endif

    logMessage("Settings", LOG_LEVEL_INFO, "Default settings applied and saved.");
}

bool loadSettings() {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        logMessage("Settings", LOG_LEVEL_ERROR, "Error opening NVS handle");
        defaultSetSettings();
        return false;
    }

    size_t required_size = sizeof(SystemSettings);
    err = nvs_get_blob(my_handle, "settings", &settings, &required_size);

    nvs_close(my_handle);

    if (err != ESP_OK || required_size != sizeof(SystemSettings)) {
        logMessage("Settings", LOG_LEVEL_WARN, "Settings not found or size mismatch");
        defaultSetSettings();
        return false;
    }

    return true;
}

bool saveSettings() {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        logMessage("Settings", LOG_LEVEL_ERROR, "Error opening NVS handle");
        return false;
    }

    err = nvs_set_blob(my_handle, "settings", &settings, sizeof(SystemSettings));
    if (err != ESP_OK) {
        logMessage("Settings", LOG_LEVEL_ERROR, "Error saving settings to NVS");
        nvs_close(my_handle);
        return false;
    }

    err = nvs_commit(my_handle);
    nvs_close(my_handle);
    
    return (err == ESP_OK);
}
