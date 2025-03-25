#ifndef CONSOLE_H
#define CONSOLE_H

//#include "common.h"
//#include "weight_task.h"

#define MAX_COMMAND_LENGTH 100
#define MAX_PARAM_LENGTH 50

// 명령어 처리 함수 타입 정의
typedef void (*CommandHandler)(const char*);

// 명령어 구조체
typedef struct {
    const char* command;
    const char* description;
    CommandHandler handler;
} ConsoleCommand;

// 함수 선언
void TaskConsole(void *pvParameters);
void initializeConsole();
void processCommand(const char* cmd);
void printHelp(const char* params);

void printUptime();

void requestSystemReset();

// 센서 관련 명령어 처리 함수
#if 0
void handleReadAllSensors(const char* params);
void handleReadUrineSensor(const char* params);
void handleReadWeightSensor(const char* params);
void handleReadGasSensor(const char* params);
void handleReadCoverSensor(const char* params);
void handleGetMotorStatus(const char* params);
void handleReadPowerSensor(const char* params);
void handleReadFulllevelSensor(const char* params);
void handleReadDiaperSensor(const char* params);
void handleReadBatteryLevel(const char* params);
#endif

// 시스템 관련 명령어 처리 함수
void handleUptime(const char* params);
void handleMemoryStatus(const char* params);
void handleSystemStatus(const char* params);
void handleResetToDefault(const char* params);
void handleResetSystem(const char* params);
void handleSetStorageType(const char* params);
void handleSetVolume(const char* params);
void handleGetVolume(const char* params);
void handleAudioWater(const char* params);
void handleAudioUrine(const char* params);
void handleAudioStop(const char* params);
void handleAudioStatus(const char* params);
void handleUiStatus(const char* params);
void handleUiMotorOn(const char* params);
void handleUiMotorOff(const char* params);

//void handleSetWiFi(const char* params);
//void handleGetWiFi(const char* params);
//void handleScanWiFi(const char* params);
//void handleGetWiFiStatus(const char* params);
//void handleSetSerialNumber(const char* params);
//void handleGetSerialNumber(const char* params);
//void handleSetVersion(const char* params);
//void handleGetVersion(const char* params);
void handleGetAllSettings(const char* params);

//void debugNVS(const char* params);
void handleTestSerial(const char* params);
//void handleDiagMode(const char* params);
void handleDebugSerial(const char* params);
//void handleDebugData(const char* params);
void handleDebugLevel(const char* params);
//void handleListFiles(const char* params);
//void handleStorageStats(const char* params);
//void handleRemoveFile(const char* params);
//void handleBackup(const char* params);
//void handleRestore(const char* params);

// 유틸리티 함수
void parseParams(const char* params, char* param1, char* param2);

#endif // CONSOLE_H