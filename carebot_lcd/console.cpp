#include <string.h>
#include <Arduino.h>
#include <nvs_flash.h>
#include <esp_system.h>
#include <WiFi.h>
#include "FS.h"
#include "LittleFS.h"

#include "common.h"
#include "lvgl_controller.h"
#include "console.h"
#include "audio.h"
//#include "serial_protocol.h"
#include "serial_lcd.h"
//#include "sd_utils.h"
#include "menu.h"

#define MAX_COMMAND_LENGTH 128
#define MAX_HISTORY_SIZE 10  // 저장할 명령어 히스토리 개수
#define ESC_CHAR '\x1B'      // 이스케이프 문자

//#define CONSOLE_PROMPT "Console> "
#define CONSOLE_PROMPT "> "

// 명령어 목록
const ConsoleCommand commands[] = {
    {"help", "Show available commands", printHelp},
//    {"diag_mode", "Enter/Exit diagnostic mode (diag_mode 1 or 0)", handleDiagMode},
//    {"debug_data", "Debug Data Processing (debug_data 1 or 0)", handleDebugData},
    {"debug_serial", "Debug Serial protocol (debug_serial 1 or 0)", handleDebugSerial},
    {"set_storage_type", "Set Storage Type (set_storage_type 0(sd-card),1(RAM),2(SPIFFS))", handleSetStorageType},
    {"set_volume", "Set Audio Volume (set_volume 0 ~ 10)", handleSetVolume},
    {"get_volume", "Get Audio Volume ", handleGetVolume},
    {"audio_water", "Play Water Sound (audio_water)", handleAudioWater},
    {"audio_urine", "Play Urine Sound (audio_urine)", handleAudioUrine},
    {"audio_stop", "Stop Sound (audio_stop)", handleAudioStop},
    {"audio_status", "Get Sound Status (audio_status)", handleAudioStatus},
    {"ui_status", "Get All UI Status Flags (ui_status)", handleUiStatus},
    {"ui_motor_on", "Play UI Motor On (ui_motor_on)", handleUiMotorOn},
    {"ui_motor_off", "Play UI Motor Off (ui_motor_off)", handleUiMotorOff},
    
//    {"test_serial", "Test Serial protocol (test_serial 1 or 0)", handleTestSerial},
//    {"debug_level", "Set debug level (0:DEBUG, 1:INFO, 2:WARN, 3:ERROR)", handleDebugLevel},
//    {"ls", "List saved MQTT files and storage info", handleListFiles},
//    {"stats", "Show MQTT storage statistics", handleStorageStats},
//    {"rm", "Remove a file (rm <filename>)", handleRemoveFile},
//    {"backup", "Backup MQTT data to LCD board", handleBackup},
//    {"restore", "Restore MQTT data from LCD board", handleRestore},
    {"uptime", "Show system's Uptime status", handleUptime},
    {"system_status", "Show system status", handleSystemStatus},
    {"mem_status", "Show Memory status", handleMemoryStatus},
    {"get_all_settings", "Show all system settings", handleGetAllSettings},
    {"reset_default", "Reset all settings to default values", handleResetToDefault},
//    {"debug_nvs", "Debug NVS and Error Handling", debugNVS},
    {"reset", "Reset the system", handleResetSystem},
//    {"set_wifi", "Set WiFi SSID and password (set_wifi <SSID> <password>)", handleSetWiFi},
//    {"get_wifi", "Get current WiFi settings", handleGetWiFi},
//    {"scan_wifi", "Scan WiFi Network", handleScanWiFi},
//    {"get_network_status", "Show all wifi status", handleGetWiFiStatus},
//    {"set_serial", "Set system serial number (set_serial <number>)", handleSetSerialNumber},
//    {"get_serial", "Get system serial number", handleGetSerialNumber},
//    {"set_version", "Set system version (set_version <version>)", handleSetVersion},
//    {"get_version", "Get system version", handleGetVersion},

    {NULL, NULL, NULL}  // 리스트의 끝을 표시
};

// 현재 입력 라인을 지우는 헬퍼 함수
void clearCurrentLine(char* buffer, int cursorPos, int length) {
    // 커서를 라인의 시작 부분으로 이동
    for (int i = 0; i < cursorPos; i++) {
        Serial.write(8); // 백스페이스
    }
    
    // 라인을 공백으로 덮어쓰기
    for (int i = 0; i < length; i++) {
        Serial.write(' ');
    }
    
    // 커서를 다시 라인의 시작 부분으로 이동
    for (int i = 0; i < length; i++) {
        Serial.write(8);
    }
    
    // 버퍼 초기화
    memset(buffer, 0, MAX_COMMAND_LENGTH);
}

// 히스토리에 명령어 추가하는 헬퍼 함수
void addToHistory(char history[][MAX_COMMAND_LENGTH], const char* cmd, int* count) {
    if (*count < MAX_HISTORY_SIZE) {
        strcpy(history[*count], cmd);
        (*count)++;
    } else {
        // 히스토리가 가득 찬 경우, 오래된 항목을 제거하고 새 항목 추가
        for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++) {
            strcpy(history[i], history[i + 1]);
        }
        strcpy(history[MAX_HISTORY_SIZE - 1], cmd);
    }
}

void TaskConsole(void *pvParameters) {
    (void)pvParameters;
    char cmdBuffer[MAX_COMMAND_LENGTH];
    int cmdIndex = 0;        // 현재 커서 위치
    int cmdLength = 0;       // 현재 명령어 길이
    
    // 명령어 히스토리 관리
    char cmdHistory[MAX_HISTORY_SIZE][MAX_COMMAND_LENGTH];
    int historyCount = 0;     // 총 히스토리 개수
    int historyIndex = -1;    // 현재 탐색 중인 히스토리 위치
    
    // 이스케이프 시퀀스 처리를 위한 상태 변수
    bool escapeSequence = false;
    bool bracketFound = false;
    
    logMessage("CONSOLE", LOG_LEVEL_INFO, "Task started");
    
    initializeConsole();
    
    for (;;) {
        if (Serial.available()) {
            char c = Serial.read();
            
            // 이스케이프 시퀀스 처리
            if (escapeSequence) {
                if (!bracketFound && c == '[') {
                    bracketFound = true;
                } 
                else if (bracketFound) {
                    escapeSequence = false;
                    bracketFound = false;
                    
                    // 위 방향키: ESC [ A
                    if (c == 'A') {
                        if (historyCount > 0 && historyIndex < historyCount - 1) {
                            // 현재 입력줄 지우기
                            clearCurrentLine(cmdBuffer, cmdIndex, cmdLength);
                            
                            // 히스토리 인덱스 증가 및 해당 명령어 가져오기
                            historyIndex++;
                            strcpy(cmdBuffer, cmdHistory[historyCount - 1 - historyIndex]);
                            cmdLength = strlen(cmdBuffer);
                            cmdIndex = cmdLength;
                            
                            // 가져온 명령어 출력
                            Serial.write(cmdBuffer);
                        }
                    }
                    // 아래 방향키: ESC [ B
                    else if (c == 'B') {
                        if (historyIndex > 0) {
                            clearCurrentLine(cmdBuffer, cmdIndex, cmdLength);
                            
                            historyIndex--;
                            strcpy(cmdBuffer, cmdHistory[historyCount - 1 - historyIndex]);
                            cmdLength = strlen(cmdBuffer);
                            cmdIndex = cmdLength;
                            
                            Serial.write(cmdBuffer);
                        } 
                        else if (historyIndex == 0) {
                            clearCurrentLine(cmdBuffer, cmdIndex, cmdLength);
                            historyIndex = -1;
                            cmdLength = 0;
                            cmdIndex = 0;
                        }
                    }
                    // 왼쪽 방향키: ESC [ D
                    else if (c == 'D') {
                        if (cmdIndex > 0) {
                            cmdIndex--;
                            Serial.write(8); // 백스페이스(커서 왼쪽으로)
                        }
                    }
                    // 오른쪽 방향키: ESC [ C
                    else if (c == 'C') {
                        if (cmdIndex < cmdLength) {
                            Serial.write(cmdBuffer[cmdIndex]);
                            cmdIndex++;
                        }
                    }
                } 
                else {
                    escapeSequence = false;
                    bracketFound = false;
                }
            }
            // 이스케이프 문자 감지
            else if (c == ESC_CHAR) {
                escapeSequence = true;
            }
            // 백스페이스 처리
            else if (c == 8 || c == 127) {
                if (cmdIndex > 0) {
                    // 커서가 라인 끝이 아닌 경우, 문자를 지우고 나머지 부분을 왼쪽으로 이동
                    if (cmdIndex < cmdLength) {
                        // 현재 위치의 문자 지우기
                        for (int i = cmdIndex - 1; i < cmdLength - 1; i++) {
                            cmdBuffer[i] = cmdBuffer[i + 1];
                        }
                        cmdLength--;
                        cmdIndex--;
                        
                        // 현재 커서 위치부터 라인 끝까지 다시 출력
                        Serial.write(8); // 백스페이스
                        for (int i = cmdIndex; i < cmdLength; i++) {
                            Serial.write(cmdBuffer[i]);
                        }
                        Serial.write(' '); // 마지막 문자 지우기
                        
                        // 커서를 원래 위치로 이동
                        for (int i = cmdLength + 1; i > cmdIndex; i--) {
                            Serial.write(8);
                        }
                    } 
                    // 커서가 라인 끝인 경우, 간단히 마지막 문자 지우기
                    else {
                        cmdIndex--;
                        cmdLength--;
                        Serial.write(8);    // 커서 왼쪽으로 이동
                        Serial.write(' ');  // 공백으로 덮어쓰기
                        Serial.write(8);    // 다시 커서 왼쪽으로 이동
                    }
                }
            }
            // 엔터키 처리
            else if (c == '\n' || c == '\r') {
                cmdBuffer[cmdLength] = '\0';
                Serial.println(); // 줄바꿈
                
                if (cmdLength > 0) {
                    // 명령어 히스토리에 추가
                    addToHistory(cmdHistory, cmdBuffer, &historyCount);
                    
                    // 명령어 처리
                    #if 0
                    if (strncmp(cmdBuffer, "ads_", 4) == 0) {
                        char* params = strchr(cmdBuffer, ' ');
                        if (params) {
                            *params = '\0';
                            params++;
                        } else {
                            params = "";
                        }
                        handleWeightConsoleCommands(cmdBuffer, params);
                    } else {
                        processCommand(cmdBuffer);
                    }
                    #else
                        processCommand(cmdBuffer);
                    #endif
                }
                
                // 히스토리 인덱스 재설정 및 버퍼 초기화
                historyIndex = -1;
                cmdIndex = 0;
                cmdLength = 0;
                Serial.print(CONSOLE_PROMPT);
            } 
            // 일반 문자 처리
            else if (cmdLength < MAX_COMMAND_LENGTH - 1) {
                // 커서가 라인 끝이 아닌 경우, 삽입 모드로 동작
                if (cmdIndex < cmdLength) {
                    // 현재 위치부터 뒤의 문자들을 한 칸씩 이동
                    for (int i = cmdLength; i > cmdIndex; i--) {
                        cmdBuffer[i] = cmdBuffer[i - 1];
                    }
                    cmdBuffer[cmdIndex] = c;
                    cmdLength++;
                    
                    // 현재 위치부터 라인 끝까지 다시 출력
                    for (int i = cmdIndex; i < cmdLength; i++) {
                        Serial.write(cmdBuffer[i]);
                    }
                    
                    // 커서를 삽입 위치 다음으로 이동
                    cmdIndex++;
                    
                    // 커서를 원래 위치로 이동 (삽입 후 한 칸 앞으로)
                    for (int i = cmdLength; i > cmdIndex; i--) {
                        Serial.write(8);
                    }
                } 
                // 커서가 라인 끝인 경우, 간단히 문자 추가
                else {
                    cmdBuffer[cmdIndex++] = c;
                    cmdLength++;
                    Serial.write(c);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


void initializeConsole() {
    Serial.println("\nWelcome to the System Console!");
    Serial.println("Type 'help' for available commands.");
    Serial.print(CONSOLE_PROMPT);
}

void processCommand(const char* cmd) {
    size_t cmd_len = strcspn(cmd, " ");  // 공백까지의 길이
    for (int i = 0; commands[i].command != NULL; i++) {
      #if 0
        if (strncmp(cmd, "sd ", 3) == 0) {
            // "sd " 접두사 제거
            handle_sd_command(cmd + 3);
        } 
        
        else
      #endif
        {
            if (strlen(commands[i].command) == cmd_len && strncmp(cmd, commands[i].command, cmd_len) == 0) {
                commands[i].handler(cmd + cmd_len + 1);
                return;
            }
        }
    }
    Serial.println("Unknown command. Type 'help' for available commands.");
}
#if 1
void printHelp(const char* params) {
    Serial.printf("Available commands:\n");
    for (int i = 0; commands[i].command != NULL; i++) {
        delay(5);
        Serial.printf("%s - %s\n", commands[i].command, commands[i].description);        
    }
}
#else
void printHelp(const char* params) {
    Serial.println("Available commands:");
    for (int i = 0; commands[i].command != NULL; i++) {
        Serial.print("  ");
        Serial.print(commands[i].command);
        Serial.print(" - ");
        Serial.println(commands[i].description);
    }
}
#endif

void parseParams(const char* params, char* param1, char* param2) {
    char* token;
    char params_copy[MAX_COMMAND_LENGTH];
    strncpy(params_copy, params, MAX_COMMAND_LENGTH);
    
    token = strtok(params_copy, " ");
    if (token != NULL) {
        strncpy(param1, token, MAX_PARAM_LENGTH - 1);
        param1[MAX_PARAM_LENGTH - 1] = '\0';
        token = strtok(NULL, " ");
        if (token != NULL) {
            strncpy(param2, token, MAX_PARAM_LENGTH - 1);
            param2[MAX_PARAM_LENGTH - 1] = '\0';
        } else {
            param2[0] = '\0';
        }
    } else {
        param1[0] = '\0';
        param2[0] = '\0';
    }
}

void handleGetAllSettings(const char* params) {
    PrintAllSettings();
}
#if 0
void handleScanWiFi(const char* params) {
    Serial.println("WiFi 네트워크 스캔 시작...");
    
    // WiFi 스캔 시작
    int networksFound = WiFi.scanNetworks();
    
    if (networksFound == 0) {
        Serial.println("발견된 네트워크가 없습니다.");
    } else {
        Serial.printf("\n발견된 네트워크 수: %d\n", networksFound);
        Serial.println("------------------------------------");
        Serial.println("| No | SSID            | RSSI | CH | Encryption |");
        Serial.println("------------------------------------");
        
        for (int i = 0; i < networksFound; ++i) {
            // 암호화 방식 확인
            String encryptionType;
            switch (WiFi.encryptionType(i)) {
                case WIFI_AUTH_OPEN:
                    encryptionType = "Open";
                    break;
                case WIFI_AUTH_WEP:
                    encryptionType = "WEP";
                    break;
                case WIFI_AUTH_WPA_PSK:
                    encryptionType = "WPA";
                    break;
                case WIFI_AUTH_WPA2_PSK:
                    encryptionType = "WPA2";
                    break;
                case WIFI_AUTH_WPA_WPA2_PSK:
                    encryptionType = "WPA+WPA2";
                    break;
                case WIFI_AUTH_WPA2_ENTERPRISE:
                    encryptionType = "WPA2-EAP";
                    break;
                case WIFI_AUTH_WPA3_PSK:
                    encryptionType = "WPA3";
                    break;
                case WIFI_AUTH_WPA2_WPA3_PSK:
                    encryptionType = "WPA2+WPA3";
                    break;
                default:
                    encryptionType = "Unknown";
            }

            // SSID가 숨겨져 있는지 확인
            String ssid = WiFi.SSID(i);
            if (ssid.length() == 0) {
                ssid = "(Hidden Network)";
            }
            
            // 결과 출력 (번호, SSID, 신호강도, 채널, 암호화방식)
            Serial.printf("| %2d | %-15.15s | %4d | %2d | %-9s |\n",
                i + 1,
                ssid.c_str(),
                WiFi.RSSI(i),
                WiFi.channel(i),
                encryptionType.c_str()
            );
        }
        Serial.println("------------------------------------");
    }
    
    // 스캔 메모리 해제
    WiFi.scanDelete();
}

void handleGetWiFiStatus(const char* params) {
    WifiStatus* wifi = &Wifi_state;
    Serial.print("WiFi Status:\n");
    Serial.printf("  Connected: %s\n", wifi->connected ? "Yes" : "No");
    if (wifi->connected) {
        Serial.printf("  SSID: %s\n", wifi->ssid);
        Serial.printf("  IP: %s\n", wifi->ip_addr);
        Serial.printf("  Gateway: %s\n", wifi->gateway);
        Serial.printf("  Signal Strength: %d%%\n", wifi->signal_strength);
    }
}

void handleSetWiFi(const char* params) {
    char ssid[MAX_PARAM_LENGTH] = {0};
    char password[MAX_PARAM_LENGTH] = {0};
    
    // SSID와 비밀번호 파싱
    const char* passwordStart = strrchr(params, ' ');
    if (passwordStart != NULL) {
        strncpy(password, passwordStart + 1, MAX_PARAM_LENGTH - 1);
        size_t ssidLength = passwordStart - params;
        if (ssidLength > 0 && ssidLength < MAX_PARAM_LENGTH) {
            strncpy(ssid, params, ssidLength);
            ssid[ssidLength] = '\0'; // Null 종료 문자 추가
        }
    }

    if (strlen(ssid) > 0 && strlen(password) > 0) {
        setWiFiCredentials(ssid, password);
        Serial.println("WiFi credentials updated. Please restart the system to apply changes.");
        //Serial.printf("SSID: %s\n", ssid);
        //Serial.printf("Password: %s\n", password);
    } else {
        Serial.println("Usage: set_wifi <SSID> <password>");
        Serial.println("Note: If SSID contains spaces, enclose it in quotes.");
    }
}

void handleGetWiFi(const char* params) {
    Serial.print("\nCurrent WiFi\n SSID: ");
    Serial.println(settings.wifi_ssid);
    //Serial.println("WiFi Password: [HIDDEN]");
    Serial.print(" Password: ");
    Serial.println(settings.wifi_password);
}

void handleDebugData(const char* params) {
    int flag;
    if (sscanf(params, "%d", &flag) == 1) {
        if (flag == 0 || flag == 1) {
            debug_data_flag = flag;
            Serial.printf("Data Processing Debug is %s\n", flag ? "Enabled" : "Disabled");
        } else {
            Serial.println("Invalid debug flag. Use 1 for Enable or 0 for Disable");
        }
    } else {
        Serial.println("Usage: debug_data <1 for Enable | 0 for Disable>");
    }
}

void handleDiagMode(const char* params) {
    int flag;
    if (sscanf(params, "%d", &flag) == 1) {
        if (flag == 0 || flag == 1) {
            diag_mode = flag;
            Serial.printf("Diagnostic Mode is %s\n", flag ? "Enabled" : "Disabled");
            if (flag) {
                startDiagnostics();
            }
        } else {
            Serial.println("Invalid flag. Use 1 for Enable or 0 for Disable");
        }
    }
}
#endif
#if 0
void handleTestSerial(const char* params) {
    int flag;
    if (sscanf(params, "%d", &flag) == 1) {
        if (flag == 0 || flag == 1) {
            test_serial_flag = flag;
            Serial.printf("Serial Protocol Test is %s\n", flag ? "Enabled" : "Disabled");
        } else {
            Serial.println("Invalid Test flag. Use 1 for Enable or 0 for Disable");
        }
    } else {
        Serial.println("Usage: test_serial <1 for Enable | 0 for Disable>");
    }
}
#endif
void handleDebugSerial(const char* params) {
    int flag;
    if (sscanf(params, "%d", &flag) == 1) {
        if (flag == 0 || flag == 1) {
            debug_serial_flag = flag;
            Serial.printf("Serial Protocol Debug is %s\n", flag ? "Enabled" : "Disabled");
        } else {
            Serial.println("Invalid debug flag. Use 1 for Enable or 0 for Disable");
        }
    } else {
        Serial.println("Usage: debug_serial <1 for Enable | 0 for Disable>");
    }
}
void handleSetStorageType(const char* params) {
    int flag;
    if (sscanf(params, "%d", &flag) == 1) {
        if (flag >=0 || flag < 3) {
            settings.storage_type = flag;
            saveSettings();
            Serial.printf("Storage Type is changed %d\n", settings.storage_type);
            Serial.printf("Please Reset Systems\n");
        } else {
            Serial.println("Invalid value : 0: sd_card, 1: RAM, 2: SPIFFS");
        }
    } else {
        Serial.println("Usage: set_storage_type 0(sd-card),1(RAM),2(SPIFFS)");
    }
}
void handleGetVolume(const char* params) {
    Serial.printf("settings.volume_value is %d\n", settings.volume_value);
    Serial.printf("current_volume is %d\n", current_volume);
    Serial.printf("volume_value is %d\n", volume_value);
}
void handleSetVolume(const char* params) {
    uint8_t volume;
    if (sscanf(params, "%d", &volume) == 1) {
        if (volume >=0 || volume <= 10) {
            audio_set_volume(volume);
            Serial.printf("Audio Volume is changed %d\n", settings.volume_value);
            //Serial.printf("Please Reset Systems\n"");
        } else {
            Serial.println("Invalid value : 0 ~ 10");
        }
    } else {
        Serial.println("Usage: set_volume <0 ~ 10>");
    }
}
void handleAudioWater(const char* params) {
    Serial.printf("Will Play Water Sound");
    play_water_sound();
}
void handleAudioUrine(const char* params) {
    Serial.printf("Will Play Urination Sound");
    play_urination_sound();
}
void handleAudioStatus(const char* params) {
    bool isPlay = is_audio_playing();
    Serial.printf("Audio is %s\n", isPlay ? "Playing": "Not Playing");
}
void handleAudioStop(const char* params) {
    Serial.printf("Will Stop Sound");
    stop_sound();
}
void handleUiMotorOn(const char* params) {
    Serial.printf("Motor On UI Activate");
    Event_motor_ON();
}
void handleUiMotorOff(const char* params) {
    Serial.printf("Motor Off UI Activate");
    need_to_restore_motor = false;
    Event_motor_OFF();
}

void handleUiStatus(const char* params) {
    Serial.println("=== UI Status Flags ===");
    Serial.printf("Cover: %s\n", cover_OPEN ? "OPEN" : "CLOSED");
    Serial.printf("Motor: %s\n", motor_ON ? "ON" : "OFF");
    Serial.printf("WiFi: %s\n", wifi_ON ? "ON" : "OFF");
    Serial.printf("Error: %s\n", error_ON ? "ON" : "OFF");
    Serial.printf("Urine Detection: %s\n", urine_ON ? "DETECTED" : "NONE");
    Serial.printf("Server Connect: %s\n", connect_ON ? "CONNECTED" : "DISCONNECTED");
    Serial.printf("Diaper: %s\n", diaper_ON ? "ATTACHED" : "DETACHED");
    Serial.printf("Full Level: %s\n", fulllevel_ON ? "REACHED" : "NOT REACHED");
    Serial.printf("Power: %s\n", power_ON ? "ON" : "OFF");
    Serial.printf("Menu Mode: %s\n", menu_ON ? "ACTIVE" : "INACTIVE");
    Serial.printf("Water Level: %d ml\n", water_level);
    Serial.println("=====================");
}
#if 0
void handleDebugLevel(const char* params) {
    int level;
    if (sscanf(params, "%d", &level) == 1) {
        if (level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_ERROR) {
            debug_level = level;
            Serial.printf("Debug level set to %s\n", 
                level == LOG_LEVEL_DEBUG ? "DEBUG" :
                level == LOG_LEVEL_INFO ? "INFO" :
                level == LOG_LEVEL_WARN ? "WARN" : "ERROR");
        } else {
            Serial.println("Invalid debug level. Use 0(DEBUG), 1(INFO), 2(WARN), or 3(ERROR)");
        }
    } else {
        Serial.println("Usage: debug_level <0:DEBUG | 1:INFO | 2:WARN | 3:ERROR>");
        Serial.printf("Current debug level: %s\n",
            debug_level == LOG_LEVEL_DEBUG ? "DEBUG" :
            debug_level == LOG_LEVEL_INFO ? "INFO" :
            debug_level == LOG_LEVEL_WARN ? "WARN" : "ERROR");
    }
}
#endif
#if 0
// 디렉토리 내 파일 출력 헬퍼 함수
void printDirectoryFiles(File &dir) {
    size_t total_size = 0;
    File file = dir.openNextFile();
    
    while (file) {
        if (!file.isDirectory()) {
            const char* filename = file.name();
            size_t size = file.size();
            total_size += size;
            Serial.printf("%-20s %8d bytes\n", filename, size);
        }
        file = dir.openNextFile();
    }
}
void listDir(File &dir, const char* prefix, int level) {
    while (File file = dir.openNextFile()) {
        for (int i = 0; i < level; i++) Serial.print("  ");
        if (file.isDirectory()) {
            Serial.printf("DIR: %s%s/\n", prefix, file.name());
            listDir(file, String(String(prefix) + file.name() + "/").c_str(), level + 1);
        } else {
            Serial.printf("FILE: %s%s (size: %d)\n", prefix, file.name(), file.size());
        }
        file.close();
    }
}
// 파일 목록 및 저장소 정보 표시
void handleListFiles(const char* params) {
    Serial.println("\nStorage Files:");
    Serial.println("----------------------------------------");
    
    // MQTT 데이터 파일들
    Serial.println("\nMQTT Data Files:");
    File dir = LittleFS.open(MQTT_DATA_PATH);
    if (dir && dir.isDirectory()) {
        File file = dir.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                Serial.printf("%-50s %8d bytes\n", 
                    String(MQTT_DATA_PATH "/" + String(file.name())).c_str(),
                    file.size());
            }
            file = dir.openNextFile();
        }
        dir.close();
    }

    // MQTT 백업 파일들
    Serial.println("\nBackup Files:");
    dir = LittleFS.open(MQTT_BACKUP_PATH);  // VIRTUAL_SD_PATH 대신 MQTT_BACKUP_PATH 사용
    if (dir && dir.isDirectory()) {
        File file = dir.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                Serial.printf("%-50s %8d bytes\n", 
                    String(MQTT_BACKUP_PATH "/" + String(file.name())).c_str(),
                    file.size());
            }
            file = dir.openNextFile();
        }
        dir.close();
    }

    // 저장소 통계
    size_t total_space = LittleFS.totalBytes();
    size_t used_space = 0;

    // MQTT Data 디렉토리 크기 계산
    dir = LittleFS.open(MQTT_DATA_PATH);
    if (dir && dir.isDirectory()) {
        File file = dir.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                used_space += file.size();
            }
            file = dir.openNextFile();
        }
        dir.close();
    }

    // MQTT Backup 디렉토리 크기 계산
    dir = LittleFS.open(MQTT_BACKUP_PATH);
    if (dir && dir.isDirectory()) {
        File file = dir.openNextFile();
        while (file) {
            if (!file.isDirectory()) {
                used_space += file.size();
            }
            file = dir.openNextFile();
        }
        dir.close();
    }

    size_t free_space = total_space - used_space;

    Serial.println("\n----------------------------------------");
    Serial.printf("Total Space: %d bytes\n", total_space);
    Serial.printf("Used Space:  %d bytes (%.1f%%)\n", 
                 used_space, (float)used_space/total_space * 100);
    Serial.printf("Free Space:  %d bytes\n", free_space);

    handleStorageStats(NULL);
}

void handleStorageStats(const char* params) {
    Serial.println("\nMQTT Storage Statistics:");
    Serial.println("----------------------------------------");

    File comp_file = LittleFS.open(MQTT_DATA_PATH"/mqtt_comp.bin", "r");
    File data_file = LittleFS.open(MQTT_DATA_PATH"/mqtt_data.bin", "r");
    File status_file = LittleFS.open(MQTT_DATA_PATH"/mqtt_status.json", "r");
    
    if (comp_file) {
        size_t comp_size = comp_file.size();
        comp_file.close();
        Serial.printf("Compressed file size: %d bytes\n", comp_size);
        
        if (data_file) {
            size_t orig_size = data_file.size();
            float ratio = (float)comp_size/orig_size * 100;
            Serial.printf("Compression ratio: %.1f%%\n", ratio);
        }
    }
    
    if (data_file) {
        size_t data_size = data_file.size();
        data_file.close();
        Serial.printf("Current data file size: %d bytes\n", data_size);
    }

    // 항상 상태 정보 표시
    Serial.println("\nStatus Information:");
    if (status_file) {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, status_file);
        status_file.close();
        
        if (!error) {
            Serial.printf("Total messages: %d\n", doc["msg_count"].as<int>());
            
            // time_t 값을 읽어서 변환
            time_t last_update = doc["last_update"].as<time_t>();
            struct tm timeinfo;
            localtime_r(&last_update, &timeinfo);
            char time_str[64];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);
            Serial.printf("Last update: %s\n", time_str);
            
            if (doc["comp_ratio"].as<float>() > 0) {
                Serial.printf("Overall compression ratio: %.1f%%\n", doc["comp_ratio"].as<float>());
            }
        }
    } else {
        Serial.println("No stored status information");
    }

    if (!comp_file && !data_file && !status_file) {
        Serial.println("No MQTT storage files present");
    }
}
// 와일드카드 매칭 함수
bool wildcardMatch(const char* pattern, const char* str) {
    if (*pattern == '*') {
        pattern++;
        return strstr(str, pattern) != NULL;
    }
    return strstr(str, pattern) == str;
}
#endif
#if 0
void handleRemoveFile(const char* params) {
    if (!params || strlen(params) == 0) {
        Serial.println("Usage: rm <pattern>");
        Serial.println("Examples:");
        Serial.println("  rm mqtt_backup*");
        return;
    }

    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open root directory");
        return;
    }

    const char* pattern = params;
    int deleted = 0, failed = 0;
    File file;

    while (file = root.openNextFile()) {
        String fname = String(file.name());
        file.close();

        // 파일명이 패턴으로 시작하는지 확인
        if (fname.startsWith(pattern) || 
            (pattern[0] == '/' && fname.startsWith(pattern + 1))) {
            if (LittleFS.remove("/" + fname)) {
                Serial.printf("Deleted: %s\n", fname.c_str());
                deleted++;
            } else {
                Serial.printf("Failed to delete: %s\n", fname.c_str());
                failed++;
            }
        }
    }
    root.close();

    Serial.printf("\nDeleted: %d files, Failed: %d files\n", deleted, failed);
}
#endif

void handleRemoveFile(const char* params) {
   if (!params || strlen(params) == 0) {
       Serial.println("Usage: rm <filename or pattern>");
       Serial.println("Examples:");
       Serial.println("  rm /mqtt_data/mqtt_data.bin");
       Serial.println("  rm /mqtt_data/mqtt*");
       Serial.println("  rm /virtual_sd/mqtt_backup*");
       return;
   }

   // 입력 문자열 처리
   char filepath[MAX_PARAM_LENGTH] = {0};
   strncpy(filepath, params, MAX_PARAM_LENGTH - 1);
   
   // 앞뒤 공백 제거
   char* path = filepath;
   while (*path == ' ') path++;
   char* end = path + strlen(path) - 1;
   while (end > path && *end == ' ') *end-- = '\0';

   if (strchr(path, '*') != NULL) {  // 와일드카드 처리
       // 디렉토리 경로와 패턴 분리
       char dir_path[64] = "/";
       char pattern[32] = "";
       char* last_slash = strrchr(path, '/');
       
       if (last_slash) {
           size_t dir_len = last_slash - path + 1;
           strncpy(dir_path, path, dir_len);
           dir_path[dir_len] = '\0';
           strcpy(pattern, last_slash + 1);
       } else {
           strcpy(pattern, path);
       }

       // 해당 디렉토리 열기
       File dir = LittleFS.open(dir_path);
       if (!dir || !dir.isDirectory()) {
           Serial.printf("Failed to open directory: %s\n", dir_path);
           return;
       }

       // 패턴에서 '*' 이전 부분 추출
       char pattern_prefix[32] = "";
       strncpy(pattern_prefix, pattern, strchr(pattern, '*') - pattern);

       int deleted = 0;
       File file;
       while (file = dir.openNextFile()) {
           String fname = String(file.name());
           file.close();

           if (fname.startsWith(pattern_prefix)) {
               String full_path = String(dir_path) + fname;
               if (LittleFS.remove(full_path)) {
                   Serial.printf("Deleted: %s\n", full_path.c_str());
                   deleted++;
               } else {
                   Serial.printf("Failed to delete: %s\n", full_path.c_str());
               }
           }
       }
       dir.close();
       Serial.printf("\nTotal %d files deleted\n", deleted);

   } else {  // 단일 파일 삭제
       if (LittleFS.remove(path)) {
           Serial.printf("File deleted: %s\n", path);
       } else {
           Serial.printf("Failed to delete file: %s\n", path);
       }
   }
}
#if 0
void handleBackup(const char* params) {
    mqtt_backup_data();
}

void handleRestore(const char* params) {
    mqtt_restore_data();
}
void handleSetWarmup(const char* params) {
    int minutes;

    if (strlen(params) >= 0) {
        sscanf(params, "%d", &minutes);
        if (minutes >= 0 && minutes <= 10) {
            warmup_time = (minutes * 60000) / portTICK_PERIOD_MS;  // 분을 tick으로 변환
            Serial.print("예열 시간이 ");
            Serial.print(minutes);
            Serial.println("분으로 설정되었습니다.");
            settings.warmupTime = minutes;
            saveSettings();
        } else {
            Serial.println("Error: 예열 시간은 0~10분 사이로 설정하세요.");
        }
    } else {
        Serial.println("Usage: set_warmup_time <0~10>");
    }
}
#endif
#if 0
void handleSetSerialNumber(const char* params) {
    char tempSerialNumber[MAX_PARAM_LENGTH] = {0};  // 적절한 크기의 임시 버퍼 선언

    if (strlen(params) > 0) {
        strncpy(tempSerialNumber, params, MAX_PARAM_LENGTH - 1);
        tempSerialNumber[sizeof(tempSerialNumber) - 1] = '\0';  // null-termination 보장
        setSerialNumber(tempSerialNumber);
        Serial.println("Serial number updated.");
        Serial.println(tempSerialNumber);
    } else {
        Serial.println("Usage: set_serial <number>");
    }
}

void handleGetSerialNumber(const char* params) {
    Serial.print("System Serial Number: ");
    Serial.println(getSerialNumber());
}

void handleSetVersion(const char* params) {
    if (strlen(params) > 0) {
        strncpy(settings.current_version, params, sizeof(settings.current_version) - 1);
        settings.current_version[sizeof(settings.current_version) - 1] = '\0';
        saveSettings();
        Serial.println("System version updated.");
    } else {
        Serial.println("Usage: set_version <version>");
    }
}

void handleGetVersion(const char* params) {
    Serial.print("System Version: ");
    Serial.println(settings.current_version);
}
#endif

// Function to wait for user input with timeout
bool waitForUserInput(unsigned long timeout) {
    unsigned long startTime = millis();
    while (!Serial.available()) {
        if (millis() - startTime > timeout) {
            Serial.println("Error: Timeout occurred. No input received.");
            return false;
        }
        delay(10);
    }
    while (Serial.available()) Serial.read(); // Clear the input buffer
    return true;
}

void printUptime() {
   uint32_t total_seconds = millis() / 1000;
   
   uint32_t days = total_seconds / 86400;
   uint32_t hours = (total_seconds % 86400) / 3600;
   uint32_t minutes = (total_seconds % 3600) / 60;
   uint32_t seconds = total_seconds % 60;
   
   char timeStr[32];
   
   if (days > 0) {
       sprintf(timeStr, "%u일 %02u시간 %02u분 %02u초", days, hours, minutes, seconds);
   } else if (hours > 0) {
       sprintf(timeStr, "%u시간 %02u분 %02u초", hours, minutes, seconds);
   } else if (minutes > 0) {
       sprintf(timeStr, "%u분 %02u초", minutes, seconds);
   } else {
       sprintf(timeStr, "%u초", seconds);
   }
   
   Serial.println(timeStr);
}

void handleUptime(const char* params) {
    Serial.println("System Status:");
    Serial.print("Uptime: ");
    Serial.print(millis() / 1000);
    Serial.print(" seconds , ");

    printUptime();
}

void handleMemoryStatus(const char* params) {
    //print_memory_info();
    print_memory_status();
}

void handleSystemStatus(const char* params) {
    Serial.println("System Status:");
    Serial.print("Uptime: ");
    Serial.print(millis() / 1000);
    Serial.print(" seconds , ");

    printUptime();
/*
    Serial.print("Current Weight Reading: ");
    Serial.print(readWeightSensor());
    Serial.println(" grams");
    
    Serial.print("Battery Level: ");
    Serial.print(readBatteryLevel());
    Serial.println("%");
    
    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
*/    
    // 추가적인 시스템 상태 정보...
}

void handleResetToDefault(const char* params) {
    Serial.println("Are you sure you want to reset all settings to default? (y/n)");
    while (!Serial.available()) {
        // 사용자 입력 대기
    }
    char response = Serial.read();
    if (response == 'y' || response == 'Y') {
        defaultSetSettings();
        Serial.println("All settings have been reset to default values.");
        Serial.println("The system will now restart to apply changes.");
        delay(1000); // 메시지를 보여주기 위한 짧은 대기
        ESP.restart(); // 시스템 재시작
    } else {
        Serial.println("Reset cancelled.");
    }
}

void requestSystemReset() {
    handleResetSystem(NULL);  // 콘솔의 리셋 함수 직접 호출
}

void handleResetSystem(const char* params) {
    Serial.println("Resetting system...");
    ESP.restart();
}
#if 0
void debugNVS(const char* params) {
    esp_err_t err;

    // Initialize NVS
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Open NVS handle
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("NVS handle opened successfully\n");

        // Try to read the value
        uint8_t value;
        err = nvs_get_u8(my_handle, "w_sensor_type", &value);
        switch (err) {
            case ESP_OK:
                printf("Value is %u\n", value);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default:
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }

        // Try to write the value
        uint8_t new_value = 1; // Example value
        err = nvs_set_u8(my_handle, "w_sensor_type", new_value);
        if (err != ESP_OK) {
            printf("Error (%s) writing value!\n", esp_err_to_name(err));
        } else {
            printf("Value written successfully\n");
            
            // Commit the value
            err = nvs_commit(my_handle);
            if (err != ESP_OK) {
                printf("Error (%s) committing changes!\n", esp_err_to_name(err));
            } else {
                printf("Changes committed successfully\n");
            }
        }

        // Close NVS handle
        nvs_close(my_handle);
    }
}
#endif