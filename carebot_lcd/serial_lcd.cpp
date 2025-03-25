/**
 * @file serial_lcd.c
 * @brief LCD 보드 시리얼 통신 구현
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"

#include <Arduino.h>
#include "common.h"
#include "lvgl_controller.h"
#include "serial_lcd.h"

#include "event.h"
#include "menu.h"
#include "audio.h"

// 전역 상태 변수
LCDBoardState lcd_state = {0};

// FreeRTOS 태스크 핸들 정의
TaskHandle_t serial_lcd_task_handle = NULL;

// 수신 버퍼
static uint8_t serial_rx_buffer[2048];

// 태스크 초기화를 위한 정적 함수
static void init_lcd_state(void);

/**
 * @brief LCD 보드 시리얼 통신 초기화
 * 
 * @return true 초기화 성공
 * @return false 초기화 실패
 */
bool serial_lcd_init(void) {
    DEBUG_LOG("Initializing LCD Board Serial\n");
    
    // 상태 초기화
    init_lcd_state();
    
    // 시리얼 프로토콜 초기화 (Board Type 1 = LCD)
    if (!serial_protocol_init(&g_serial_ctx, LCD_BOARD, lcd_process_message)) {
        DEBUG_LOG("Failed to initialize serial protocol\n");
        return false;
    }
    
    // 태스크 생성
    BaseType_t task_created = xTaskCreatePinnedToCore(
        serial_lcd_task,
        "Serial_LCD",
        2*4096,
        NULL,
        5,
        &serial_lcd_task_handle,
        1
    );
    
    if (task_created != pdPASS) {
        DEBUG_LOG("Failed to create serial LCD task\n");
        return false;
    }
    
    DEBUG_LOG("LCD Board Serial initialized successfully\n");
    return true;
}

/**
 * @brief LCD 상태 초기화
 */
static void init_lcd_state(void) {
    memset(&lcd_state, 0, sizeof(LCDBoardState));
    
    // 버전 정보 설정
    //strncpy(lcd_state.hw_version, "HW1.0.0", sizeof(lcd_state.hw_version));
    //strncpy(lcd_state.sw_version, "SW1.0.0", sizeof(lcd_state.sw_version));
    //strncpy(lcd_state.serial_num, "LCD00000001", sizeof(lcd_state.serial_num));
    
    // 초기 상태 설정
    lcd_state.sys_status = SYS_FLAG_NORMAL;
    lcd_state.error_code = 0;
    
    lcd_state.cover_sensor     = -1;
    lcd_state.urine_sensor     = -1;
    lcd_state.diaper_sensor    = -1;
    lcd_state.fulllevel_sensor = -1;
    lcd_state.gas_sensor       = -1;
    lcd_state.weight_sensor    = -1;
    lcd_state.power_status     = -1;
    lcd_state.system_status    = -1;
    lcd_state.server_status    = -1;
    lcd_state.motor_status     = -1;
    lcd_state.wifi_connected   = -1;
    
    //lcd_state.motor_status = MOTOR_STATUS_STOP;
    //lcd_state.motor_speed = 0;
    //lcd_state.battery_level = 100.0f;
    
    // WiFi 초기 상태 설정
    //lcd_state.wifi_connected = false;
    //memset(lcd_state.ssid, 0, sizeof(lcd_state.ssid));
    //memset(lcd_state.ip_addr, 0, sizeof(lcd_state.ip_addr));
    //memset(lcd_state.gateway, 0, sizeof(lcd_state.gateway));
    //lcd_state.signal_strength = 0;
    
    // 혈뇨 상태 초기화
    lcd_state.blood_amount = 0;
    
    // 네트워크 정보 초기화
    memset(available_networks, 0, sizeof(available_networks));
    network_count = 0;
}

/**
 * @brief LCD 보드 시리얼 태스크
 * 
 * @param pvParameters 태스크 파라미터
 */
#if 0
#define REQ_SYSTEM_STATUS       0x01
#define REQ_SENSOR_STATUS       0x02
#define REQ_MOTOR_CONTROL       0x03
#define REQ_WIFI_STATUS         0x04
#define REQ_WIFI_SCANLIST       0x05
#define REQ_TIMESYNC_STATUS     0x06
#endif
void serial_lcd_task(void* pvParameters) {
    DEBUG_LOG("LCD Board Serial Task started\n");
    
    uint32_t last_update_time = 0;
    bool initial_info_requested = false;
    
    while (1) {
        uint32_t current_time = xTaskGetTickCount();
        
        // 연결되었고 초기 정보를 아직 요청하지 않은 경우
        if (g_serial_ctx.peer_connected && !initial_info_requested) {
            // 시스템 정보 요청
            request_to_main(REQ_SYSTEM_STATUS);
            
            // WiFi 정보 요청
            request_to_main(REQ_WIFI_STATUS);
            
            // 시간 정보 요청
            request_to_main(REQ_TIMESYNC_STATUS);

            // 센서 정보 요청
//            request_to_main(REQ_SENSOR_STATUS);

            initial_info_requested = true;
            DEBUG_LOG("Initial device info requested\n");
        }
        
        // 주기적인 업데이트 (5초마다 정보 확인)
        if (current_time - last_update_time >= pdMS_TO_TICKS(5000)) {
            // 필요한 정보가 없는 경우 재요청
            // 센서 정보 요청
            request_to_main(REQ_SENSOR_STATUS);
#if 0
    Serial.printf("  Model Name: %s\n", lcd_state.model_name);
    Serial.printf("  HW Version: %s\n", lcd_state.hw_version);
    Serial.printf("  SW Version: %s\n", lcd_state.sw_version);
    Serial.printf("  Serial Number: %s\n", lcd_state.serial_num);
#endif
            if (strlen(lcd_state.model_name) == 0 || 
                strlen(lcd_state.serial_num) == 0) {
                request_to_main(REQ_SYSTEM_STATUS);
            }
            #if 0
            if (strlen(lcd_state.ssid) == 0 || 
                strlen(lcd_state.ip_addr) == 0 || 
                strlen(lcd_state.gateway) == 0) {
                request_to_main(REQ_WIFI_STATUS);
            }
            #endif
            
            last_update_time = current_time;
        }
        
        // CPU 사용률 조절을 위한 딜레이
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
/**
 * @brief WiFi 스캔 요청 전송
 */
void request_wifi_scan(void) {
    RequestMessage request = {0};
    request.header.start_marker = 0xFF;
    request.header.type = MSG_REQUEST;
    request.header.timestamp = time(NULL);
    request.header.seq_num = g_serial_ctx.sequence_number++;
    request.header.length = sizeof(RequestMessage) - sizeof(MessageHeader);
    request.request_type = REQ_WIFI_SCANLIST;
    request.checksum = calculate_checksum(&request, sizeof(RequestMessage) - 1);
    
    send_message(&g_serial_ctx, &request, sizeof(RequestMessage));
    DEBUG_LOG("WiFi scan request sent\n");
}
/**
 * @brief 메인보드에 정보 요청
 */
#if 0  // request type
#define REQ_SYSTEM_STATUS       0x01
#define REQ_SENSOR_STATUS       0x02
#define REQ_MOTOR_CONTROL       0x03
#define REQ_WIFI_STATUS         0x04
#define REQ_WIFI_SCANLIST       0x05
#define REQ_TIMESYNC_STATUS     0x06
#endif
void request_to_main(uint8_t request_type) {
    RequestMessage request = {0};

    request.header.start_marker = 0xFF;
    request.header.type = MSG_REQUEST;
    request.header.timestamp = time(NULL);
    request.header.seq_num = g_serial_ctx.sequence_number++;
    request.header.length = sizeof(RequestMessage) - sizeof(MessageHeader);
    request.request_type = request_type;
    request.checksum = calculate_checksum(&request, sizeof(RequestMessage) - 1);
    
    send_message(&g_serial_ctx, &request, sizeof(RequestMessage));
    DEBUG_LOG("Request to Main Board (message type %d)\n", request_type);
}

/**
 * @brief WiFi 연결 정보 전송
 * 
 * @param ssid SSID
 * @param pw 비밀번호
 */
void send_wifi_conn_info(char* ssid, const char* pw) {
    WifiSelectMessage msg = {0};
    
    msg.header.start_marker = 0xFF;
    msg.header.type = MSG_WIFI_SSID_SELECT;
    msg.header.timestamp = time(NULL);
    msg.header.seq_num = g_serial_ctx.sequence_number++;
    msg.header.length = sizeof(WifiSelectMessage) - sizeof(MessageHeader);
    
    strncpy(msg.ssid, ssid, sizeof(msg.ssid) - 1);
    strncpy(msg.password, pw, sizeof(msg.password) - 1);
    
    DEBUG_LOG("+++ msg.ssid: %s\n", msg.ssid);
    DEBUG_LOG("+++ msg.password: %s\n", msg.password);
    
    msg.checksum = calculate_checksum(&msg, sizeof(WifiSelectMessage) - 1);
    send_message(&g_serial_ctx, &msg, sizeof(WifiSelectMessage));
    
    DEBUG_LOG("WiFi connection info sent\n");
}

/**
 * @brief 공장 초기화 요청 전송
 */
void send_factory_init(void) {
    FactoryInitMessage msg = {0};
    
    msg.header.start_marker = 0xFF;
    msg.header.type = MSG_FACTORY_INIT;
    msg.header.timestamp = time(NULL);
    msg.header.seq_num = g_serial_ctx.sequence_number++;
    msg.header.length = sizeof(FactoryInitMessage) - sizeof(MessageHeader);
    
    msg.checksum = calculate_checksum(&msg, sizeof(FactoryInitMessage) - 1);
    send_message(&g_serial_ctx, &msg, sizeof(FactoryInitMessage));
    
    DEBUG_LOG("Factory init request sent\n");
}

/**
 * @brief 혈뇨 상태 전송
 * 
 * @param amount 혈뇨량 (0: 없음, 1: 소, 2: 중, 3: 대)
 */
void send_blood_status(uint8_t amount) {
    BloodStatusMessage msg = {0};
    
    msg.header.start_marker = 0xFF;
    msg.header.type = MSG_BLOOD_STATUS;
    msg.header.timestamp = time(NULL);
    msg.header.seq_num = g_serial_ctx.sequence_number++;
    msg.header.length = sizeof(BloodStatusMessage) - sizeof(MessageHeader);
    
    msg.amount = amount;
    
    msg.checksum = calculate_checksum(&msg, sizeof(BloodStatusMessage) - 1);
    send_message(&g_serial_ctx, &msg, sizeof(BloodStatusMessage));
    
    DEBUG_LOG("Blood status sent - Amount: %d\n", amount);
}

/**
 * @brief 메시지 처리 콜백 함수
 * 
 * @param message 수신 메시지
 */
void lcd_process_message(void* message, size_t size) {
    if (message == NULL) {
        return;
    }
    
    // 처음 전원 켜질 때 초기화
    if (power_ON == 0) {
        Event_power_ON();
        //Event_bat_LOW();
        power_ON = 1;
    }
    
    MessageHeader* header = (MessageHeader*)message;

    DEBUG_LOG("Received message type: %s (0x%02X)\n", 
              get_message_type_str(header->type), header->type);
    #if 0
    Serial.printf("Received message type: %s (0x%02X), size(%d)\n", 
              get_message_type_str(header->type), header->type, size);
    #endif
    print_message(message);
    print_hex_dump((uint8_t*)message, size);
    
    // 메시지 타입에 따른 처리
    switch (header->type) {
        case MSG_HELLO:
            {
                HelloMessage* msg = (HelloMessage*)message;
                g_serial_ctx.peer_connected = true;
                
                // Hello ACK 응답
                HelloMessage response = {0};
                response.header.start_marker = 0xFF;
                response.header.type = MSG_HELLO_ACK;
                response.header.timestamp = time(NULL);
                response.header.seq_num = g_serial_ctx.sequence_number++;
                response.header.length = sizeof(HelloMessage) - sizeof(MessageHeader);
                response.board_type = LCD_BOARD;  // LCD 보드
                response.checksum = calculate_checksum(&response, sizeof(HelloMessage) - 1);
                
                send_message(&g_serial_ctx, &response, sizeof(HelloMessage));
                DEBUG_LOG("Received Hello from Main Board, sent Hello ACK\n");
            }
            break;
            
        case MSG_HELLO_ACK:
            {
                HelloMessage* msg = (HelloMessage*)message;
                g_serial_ctx.peer_connected = true;
                DEBUG_LOG("Received Hello ACK from Main Board\n");
                //Serial.printf("Received Hello ACK from Main Board\n");
            }
            break;
            
        case MSG_SYSTEM_STATUS:
            {
                SystemStatusMessage* msg = (SystemStatusMessage*)message;
                DEBUG_LOG("System Status Update:\n");
                DEBUG_LOG("  Model Name: %s\n", msg->model_name);
                DEBUG_LOG("  HW Version: %s\n", msg->hw_version);
                DEBUG_LOG("  SW Version: %s\n", msg->sw_version);
                DEBUG_LOG("  Serial Num: %s\n", msg->serial_num);
                DEBUG_LOG("  Status: 0x%02X\n", msg->sys_status);
                #if 0
                Serial.printf("System Status Update from msg:\n");
                Serial.printf("  Model Name: %s\n", msg->model_name);
                Serial.printf("  HW Version: %s\n", msg->hw_version);
                Serial.printf("  SW Version: %s\n", msg->sw_version);
                Serial.printf("  Serial Num: %s\n", msg->serial_num);
                Serial.printf("  Status: 0x%02X\n", msg->sys_status);
                #endif
                // 시스템 정보 업데이트
                strncpy(lcd_state.model_name, msg->model_name, sizeof(lcd_state.model_name));
                strncpy(lcd_state.hw_version, msg->hw_version, sizeof(lcd_state.hw_version));
                strncpy(lcd_state.sw_version, msg->sw_version, sizeof(lcd_state.sw_version));
                strncpy(lcd_state.serial_num, msg->serial_num, sizeof(lcd_state.serial_num));
                lcd_state.sys_status = msg->sys_status;
                lcd_state.error_code = msg->error_code;
                #if 0
                Serial.printf("System Status Update in lcd_state:\n");
                Serial.printf("  Model Name: %s\n", lcd_state.model_name);
                Serial.printf("  HW Version: %s\n", lcd_state.hw_version);
                Serial.printf("  SW Version: %s\n", lcd_state.sw_version);
                Serial.printf("  Serial Num: %s\n", lcd_state.serial_num);
                Serial.printf("  Status: 0x%02X\n", lcd_state.sys_status);
                #endif
                // 시스템 상태가 에러인 경우
                if (msg->sys_status == SYS_FLAG_ERROR) {
                    // Error 이벤트 처리
                }
            }
            break;
            
        case MSG_SENSOR_DATA:
            {
                if (!current_ui_state.initialized) {
                    Serial.println("+++ UI가 아직 초기화되지 않음, 업데이트 하지 않음");
                    return;
                }

                SensorDataMessage* msg = (SensorDataMessage*)message;
                DEBUG_LOG("Received Sensor Data:\n");
                // 로그 부분은 동일...
    
                // 이전 상태 백업 (변화 감지용)
                int prev_cover_sensor     = lcd_state.cover_sensor;
                int prev_diaper_sensor    = lcd_state.diaper_sensor;
                int prev_fulllevel_sensor = lcd_state.fulllevel_sensor;
                int prev_gas_sensor       = lcd_state.gas_sensor;
                int prev_server_status    = lcd_state.server_status;
                int prev_motor_status     = lcd_state.motor_status;
                int prev_power_status     = lcd_state.power_status;
                int prev_system_status    = lcd_state.system_status;
                int prev_wifi_connected   = lcd_state.wifi_connected;
                float prev_weight_sensor  = lcd_state.weight_sensor;
                float prev_battery_level  = lcd_state.battery_level;
    
                // 센서 상태 업데이트
                lcd_state.cover_sensor     = msg->cover_sensor;
                lcd_state.urine_sensor     = msg->urine_sensor;
                lcd_state.diaper_sensor    = msg->diaper_sensor;
                lcd_state.fulllevel_sensor = msg->fulllevel_sensor;
                lcd_state.gas_sensor       = msg->gas_sensor;
                lcd_state.weight_sensor    = msg->weight_sensor;
                lcd_state.power_status     = msg->power_status;
                lcd_state.system_status    = msg->system_status;
                lcd_state.server_status    = msg->server_status;
                lcd_state.motor_status     = msg->motor_running;
                lcd_state.wifi_connected   = msg->wifi_connected;
                lcd_state.battery_level    = msg->battery_level;
                #if 0
                Serial.printf("prev Sensor Data:\n");
                Serial.printf("  Cover: %d\n", prev_cover_sensor);
                Serial.printf("  Diaper Sensor: %d\n",prev_diaper_sensor);
                Serial.printf("  Gas Sensor: %d\n", prev_gas_sensor);
                Serial.printf("  Full Level Sensor: %d\n", prev_fulllevel_sensor);
                Serial.printf("  Motor: %d\n", prev_motor_status);
                Serial.printf("  WiFi: %d\n", prev_wifi_connected);
                Serial.printf("  Server: %d\n", prev_server_status);
                Serial.printf("  System Error: %d\n", prev_system_status);
                Serial.printf("  Power: %d\n", prev_power_status);
                #endif
    
                // 이벤트 처리 - 상태 변화가 있을 때만 호출
    
                // 커버 열림/닫힘 이벤트 - 상태 변화 있을 때만
                if (prev_cover_sensor != lcd_state.cover_sensor) {
                    if (lcd_state.cover_sensor == 0) {
                        cover_open_event_handler();
                    } else {
                        lvgl_update_app_ui();
                    }
                }
    
                // 무게 이벤트 - 무게 변화가 임계값 이상일 때만
                if (abs(prev_weight_sensor - lcd_state.weight_sensor) > 10.0f) { // 10g 이상 변화 시
                    Event_tank_ON(lcd_state.weight_sensor);
                }
    
                // Motor - 상태 변화 있을 때만
                if (prev_motor_status != lcd_state.motor_status) {
                    if (lcd_state.motor_status) {
                        Event_motor_ON();
                        motor_ON = 1;
                    } else {
                        need_to_restore_motor = false;
                        Event_motor_OFF();
                        motor_ON = 0;
                    }
                }
    
                // Diaper - 상태 변화 있을 때만
                if (prev_diaper_sensor != lcd_state.diaper_sensor) {
                    if (lcd_state.diaper_sensor == 1 && diaper_ON == 0) {
                        Event_diaper_ON();
                        diaper_ON = 1;
                    }
                    if (lcd_state.diaper_sensor == 0 && diaper_ON == 1) {
                        Event_diaper_OFF();
                        diaper_ON = 0;
                    }
                }
    
                // 만수위 - 상태 변화 있을 때만
                if (prev_fulllevel_sensor != lcd_state.fulllevel_sensor) {
                    if (lcd_state.fulllevel_sensor == 1 && fulllevel_ON == 0) {
                        fulllevel_event_handler();
                        fulllevel_ON = 1;
                    }
                    if (lcd_state.fulllevel_sensor == 0 && fulllevel_ON == 1) {
                        fulllevel_ON = 0;
                    }
                }
    
                // 배변 - 상태 변화 있을 때만
                if (prev_gas_sensor != lcd_state.gas_sensor) {
                    if (lcd_state.gas_sensor == 1 && urine_ON == 0) {
                        Event_feces_ON();
                        urine_ON = 1;
                    }
                    if (lcd_state.gas_sensor == 0 && urine_ON == 1) {
                        Event_feces_OFF();
                        urine_ON = 0;
                    }
                }
    
                // 서버 연결 - 상태 변화 있을 때만
                if (prev_server_status != lcd_state.server_status) {
                    if (lcd_state.server_status == 1 && connect_ON == 0) {
                        Event_connect_ON();
                        connect_ON = 1;
                    }
                    if (lcd_state.server_status == 0 && connect_ON == 1) {
                        Event_connect_OFF();
                        connect_ON = 0;
                    }
                }
    
                // 배터리 - 주기적 업데이트 필요 (상태 변화 체크 안함)
                {
                    float battery_level = lcd_state.battery_level;
                    // 기존 값과 크게 다르지 않으면 업데이트 스킵 (옵션)
                    // 작은 변동은 무시하여 불필요한 UI 업데이트 방지
                    if (abs(prev_battery_level - battery_level) < 10.0f) {
                        return;
                    }
                    Event_bat_ON();
                
                    if (battery_level < BATTERY_LOW_THRESHOLD) {
                        Event_bat_LOW();
                    }
                }
    
                // Power - 상태 변화 있을 때만
                if (prev_power_status != lcd_state.power_status) {
                    if (lcd_state.power_status == 1 && power_ON == 0) {
                        Event_power_ON();
                        power_ON = 1;
                    }
                    if (lcd_state.power_status == 0 && power_ON == 1) {
                        Event_power_OFF();
                        power_ON = 0;
                    }
                }
    
                // System Error - 상태 변화 있을 때만
                if (prev_system_status != lcd_state.system_status) {
                    if (lcd_state.system_status == 0 && error_ON == 1) {
                        Event_error_OFF();
                        error_ON = 0;
                    } else if (lcd_state.system_status != 0) {
                        Event_error_ON();
                        error_ON = 1;
                    }
                }
    
                // WiFi - 상태 변화 있을 때만
                if (prev_wifi_connected != lcd_state.wifi_connected) {
                    if (lcd_state.wifi_connected) {
                        Event_wifi_ON();
                        wifi_ON = 1;
                    } else {
                        Event_wifi_OFF();
                        wifi_ON = 0;
                    }
                }
            }
            break;
            
        case MSG_DIAGNOSTIC_RESULT:
            {
                DiagnosticResultMessage* msg = (DiagnosticResultMessage*)message;
                DEBUG_LOG("\nReceived Diagnostic Results:\n");
                
                // 센서 상태 출력
                DEBUG_LOG("\n[Sensor Status: 0x%02X]\n", msg->sensor_status);
                DEBUG_LOG("  Cover Sensor: %s\n", (msg->sensor_status & (1<<0)) ? "Closed" : "Open");
                DEBUG_LOG("  BME Sensor: %s\n", (msg->sensor_status & (1<<1)) ? "Normal" : "Abnormal");
                DEBUG_LOG("  Urine Sensor: %s\n", (msg->sensor_status & (1<<2)) ? "Normal" : "Abnormal");
                DEBUG_LOG("  Diaper Sensor: %s\n", (msg->sensor_status & (1<<3)) ? "In" : "Out");
                DEBUG_LOG("  Weight Sensor: %s\n", (msg->sensor_status & (1<<4)) ? "Normal" : "Abnormal");
                DEBUG_LOG("  Full Level Sensor: %s\n", (msg->sensor_status & (1<<5)) ? "Full Level" : "Not Full Level");
                
                // 환경 데이터
                DEBUG_LOG("\n[Environmental Data]\n");
                DEBUG_LOG("  Gas: %.1fKohm\n", msg->gas);
                DEBUG_LOG("  Temperature: %.1f°C\n", msg->temperature);
                DEBUG_LOG("  Humidity: %.1f%%\n", msg->humidity);
                DEBUG_LOG("  Pressure: %.1fhPa\n", msg->pressure);
                
                // 무게 센서 데이터
                DEBUG_LOG("\n[Weight Sensor Data]\n");
                DEBUG_LOG("  Weight Sensor 1: %d\n", msg->weight_sensor1);
                DEBUG_LOG("  Weight Sensor 2: %d\n", msg->weight_sensor2);
                
                // 액추에이터 상태
                DEBUG_LOG("\n[Actuator Status: 0x%02X]\n", msg->actuator_status);
                DEBUG_LOG("  Motor: %s\n", (msg->actuator_status & (1<<0)) ? "Normal" : "Abnormal");
                DEBUG_LOG("  LED: %s\n", (msg->actuator_status & (1<<1)) ? "Normal" : "Abnormal");
                
                // 전원 상태
                DEBUG_LOG("\n[Power Status: 0x%02X]\n", msg->power_status);
                DEBUG_LOG("  Battery: %s (Voltage: %dmV)\n", 
                    (msg->power_status & (1<<0)) ? "Normal" : "Low",
                    msg->battery_voltage);
                DEBUG_LOG("  Power Adapter: %s\n", 
                    (msg->power_status & (1<<1)) ? "Connected" : "Disconnected");
                
                // 시스템 상태
                DEBUG_LOG("\n[System Status: 0x%02X]\n", msg->system_status);
                DEBUG_LOG("  RTC: %s\n", (msg->system_status & (1<<0)) ? "Normal" : "Abnormal");
                DEBUG_LOG("  NVS: %s\n", (msg->system_status & (1<<1)) ? "Normal" : "Abnormal");
                DEBUG_LOG("  WiFi: %s\n", (msg->system_status & (1<<2)) ? "Connected" : "Disconnected");
                
                // 테스트 결과 세부사항
                DEBUG_LOG("\n[Test Details]\n");
                for (int i = 0; i < 8; i++) {
                    if (msg->test_results[i] != 0) {
                        DEBUG_LOG("  Test %d Result: 0x%02X\n", i, msg->test_results[i]);
                    }
                }
            }
            break;
            
        case MSG_WIFI_STATUS:
            {
                WifiStatusMessage* msg = (WifiStatusMessage*)message;
                DEBUG_LOG("Received WiFi Status:\n");
                DEBUG_LOG("  Connected: %s\n", msg->connected ? "Yes" : "No");
                DEBUG_LOG("  SSID: %s\n", msg->ssid);
                DEBUG_LOG("  IP: %s\n", msg->ip_addr);
                DEBUG_LOG("  Gateway: %s\n", msg->gateway);
                DEBUG_LOG("  Signal: %d%%\n", msg->signal_strength);
                
                // WiFi 상태 업데이트
                lcd_state.wifi_connected = msg->connected;
                if (msg->connected) {
                    strncpy(lcd_state.ssid, msg->ssid, sizeof(lcd_state.ssid));
                    strncpy(lcd_state.ip_addr, msg->ip_addr, sizeof(lcd_state.ip_addr));
                    strncpy(lcd_state.gateway, msg->gateway, sizeof(lcd_state.gateway));
                    lcd_state.signal_strength = msg->signal_strength;
                }
                
                // UI 이벤트 처리
                if (msg->connected) {
                    Event_wifi_ON();
                    wifi_ON = 1;
                } else {
                    Event_wifi_OFF();
                    wifi_ON = 0;
                }
            }
            break;
            
        case MSG_MOTOR_STATUS:
            {
                MotorMessage* msg = (MotorMessage*)message;
                const char* status_str;
                
                switch (msg->motor_status) {
                    case MOTOR_STATUS_STOP:
                        status_str = "Stopped";
                        DEBUG_LOG("+++ case MOTOR_STATUS_STOP, motor_ON: %d\n", motor_ON);
                        need_to_restore_motor = false;
                        Event_motor_OFF();
                        motor_ON = 0;
                        break;
                        
                    case MOTOR_STATUS_RUNNING:
                        status_str = "Running";
                        DEBUG_LOG("+++ case MOTOR_STATUS_RUNNING, motor_ON: %d\n", motor_ON);
                        Event_motor_ON();
                        motor_ON = 1;
                        break;
                        
                    case MOTOR_STATUS_ERROR:
                        status_str = "Error";
                        break;
                        
                    default:
                        status_str = "Unknown";
                        break;
                }
                
                DEBUG_LOG("+++ Received Motor Status:\n");
                DEBUG_LOG("  Status: %s\n", status_str);
                DEBUG_LOG("  Speed: %d%%\n", msg->motor_speed);
                
                // 모터 상태 업데이트
                lcd_state.motor_status = msg->motor_status;
                lcd_state.motor_speed = msg->motor_speed;

            }
            break;
            
        case MSG_TIME_SYNC:
            {
                TimeSyncMessage* msg = (TimeSyncMessage*)message;
                char time_str[32];
                time_t t = msg->timestamp;
                struct tm* timeinfo = localtime(&t);
    
                DEBUG_LOG("+++ Time Sync Received:\n");
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
                DEBUG_LOG("  New Time: %s\n", time_str);
    
                // 타임스탬프 저장
                lcd_state.last_timestamp = msg->timestamp;
    
                // 시스템 시간 설정
                struct timeval now = { .tv_sec = msg->timestamp, .tv_usec = 0 };
                if (settimeofday(&now, NULL) == 0) {
                    DEBUG_LOG("System time updated successfully\n");
                } else {
                    DEBUG_LOG("Failed to update system time\n");
                }
            }
            break;
            
        case MSG_WIFI_SCAN_LIST:
            {
                WifiScanListMessage* msg = (WifiScanListMessage*)message;
                DEBUG_LOG("Received WiFi Scan List: %d networks\n", msg->ap_count);
                
                // 네트워크 목록 저장
                network_count = msg->ap_count > 10 ? 10 : msg->ap_count;
                
                for (int i = 0; i < network_count; i++) {
                    DEBUG_LOG("  +++ Network %d: SSID: %s, RSSI: %d\n", 
                             i, msg->ap_list[i].ssid, msg->ap_list[i].rssi);
                    
                    strncpy(available_networks[i].ssid, msg->ap_list[i].ssid, sizeof(available_networks[i].ssid) - 1);
                    available_networks[i].ssid[sizeof(available_networks[i].ssid) - 1] = '\0';
                    
                    DEBUG_LOG("+++ Network %d: SSID 1: %s\n", i, available_networks[i].ssid);
                }
            }
            break;
            
        case MSG_EVENT:
            {
                EventMessage* msg = (EventMessage*)message;
                char event_desc[128];
                char timestamp[32];
                
                DEBUG_LOG("\n=== Event Message Processing ===\n");
                format_timestamp(msg->header.timestamp, timestamp, sizeof(timestamp));
                DEBUG_LOG("Time: %s\n", timestamp);
                
                if (decode_event_message(msg, event_desc, sizeof(event_desc))) {
                    DEBUG_LOG("%s\n", event_desc);
                }
                
                // 이벤트 타입별 처리
                switch (msg->event_type) {
                    // 커버 센서 이벤트
                    case EVENT_SENSOR_COVER:
                        {
                            bool cover_state = msg->data.digital.state;
                            DEBUG_LOG("[%s] Cover Sensor Update: %s\n",
                                     timestamp, cover_state ? "Closed" : "Open");
                            
                            // 커버 열림 이벤트 처리
                            if (cover_state == 0) {
                                cover_open_event_handler();
                            } else {
                                lvgl_update_app_ui();
                            }
                        }
                        break;
                        
                    case EVENT_SENSOR_DIAPER:
                        {
                            bool diaper_state = msg->data.digital.state;
                            DEBUG_LOG("[%s] Diaper Sensor Update: %s\n",
                                     timestamp, diaper_state ? "In" : "Out");
                            
                            // 이벤트 처리
                            if (diaper_state == 1 && diaper_ON == 0) {
                                Event_diaper_ON();
                                diaper_ON = 1;
                            }
                            if (diaper_state == 0 && diaper_ON == 1) {
                                Event_diaper_OFF();
                                diaper_ON = 0;
                            }
                        }
                        break;
                        
                    case EVENT_SENSOR_POWER:
                        {
                            bool power_state = msg->data.digital.state;
                            DEBUG_LOG("[%s] Power Sensor Update: %s\n",
                                     timestamp, power_state ? "On" : "Off");
                            
                            // 이벤트 처리
                            if (power_state == 1 && power_ON == 0) {
                                Event_power_ON();
                                power_ON = 1;
                            }
                            if (power_state == 0 && power_ON == 1) {
                                Event_power_OFF();
                                power_ON = 0;
                            }
                        }
                        break;
                        
                    case EVENT_SENSOR_SYSTEM:
                        {
                            uint8_t system_state = msg->data.digital.state;
                            DEBUG_LOG("[%s] System Sensor Update: %s(0x%02x)\n",
                                     timestamp, system_state ? "Error" : "Normal", system_state);
                            
                            DEBUG_LOG("+++ system_state: %d, error_ON: %d\n", system_state, error_ON);
                            
                            // 에러 상태 업데이트
                            if (system_state == 0 && error_ON == 1) {
                                Event_error_OFF();
                                error_ON = 0;
                            } else if (system_state != 0) {
                                Event_error_ON();
                                error_ON = 1;
                            }
                        }
                        break;
                        
                    case EVENT_SENSOR_FULLLEVEL:
                        {
                            bool fulllevel_state = msg->data.digital.state;
                            DEBUG_LOG("[%s] Full Level Sensor Update: %s\n",
                                     timestamp, fulllevel_state ? "Full Level is Reached" : "Full Level is Not Reached");
                            
                            // 이벤트 처리
                            if (fulllevel_state == 1 && fulllevel_ON == 0) {
                                fulllevel_event_handler();
                                fulllevel_ON = 1;
                            }
                            if (fulllevel_state == 0 && fulllevel_ON == 1) {
                                fulllevel_ON = 0;
                            }
                        }
                        break;
                        
                    case EVENT_SERVER_CONNECT:
                        {
                            bool server_state = msg->data.digital.state;
                            DEBUG_LOG("[%s] Server Connection Update: %s\n",
                                     timestamp, server_state ? "Server is Connected" : "Server is Not Connected");
                            
                            // 이벤트 처리
                            if (server_state == 1 && connect_ON == 0) {
                                Event_connect_ON();
                                connect_ON = 1;
                            }
                            if (server_state == 0 && connect_ON == 1) {
                                Event_connect_OFF();
                                connect_ON = 0;
                            }
                        }
                        break;
                        
                    case EVENT_SENSOR_URINE:
                        {
                            bool urine_detected = msg->data.digital.state;
                            DEBUG_LOG("[%s] Urine Sensor Update: %s\n",
                                     timestamp, urine_detected ? "Detected" : "None");
                        }
                        break;
                        
                    case EVENT_SENSOR_WEIGHT:
                        {
                            float weight = msg->data.analog.value;
                            DEBUG_LOG("[%s] Weight Sensor Update: %.1f g\n", timestamp, weight);
                            
                            // 이벤트 처리
                            Event_tank_ON(weight);
                            
                            // 만수위 알림
                            if (weight > WEIGHT_THRESHOLD_HIGH) {
                                fulllevel_event_handler();
                            }
                        }
                        break;
                        
                    case EVENT_SENSOR_GAS:
                        {
                            bool gas_detected = msg->data.digital.state;
                            DEBUG_LOG("[%s] Gas Sensor Update: %s\n",
                                     timestamp, gas_detected ? "Detected" : "None");
                            
                            // 이벤트 처리
                            if (gas_detected == 1 && urine_ON == 0) {
                                Event_feces_ON();
                                urine_ON = 1;
                            }
                            if (gas_detected == 0 && urine_ON == 1) {
                                Event_feces_OFF();
                                urine_ON = 0;
                            }
                        }
                        break;
                        
                    case EVENT_SENSOR_BATTERY:
                        {
                            float battery_level = msg->data.analog.value;
                            DEBUG_LOG("[%s] Battery Level Update: %.1f%%\n", timestamp, battery_level);
                            
                            // 배터리 상태 표시
                            Event_bat_ON();
                            
                            // 배터리 부족 알림
                            if (battery_level < BATTERY_LOW_THRESHOLD) {
                                Event_bat_LOW();
                            }
                        }
                        break;
                        
                    case EVENT_BUTTON_PRESS:
                    case EVENT_BUTTON_RELEASE:
                        {
                            const char* button_name;
                            switch (msg->data.button.button_id) {
                                case BUTTON_SW1_PIN:
                                    button_name = "Motor Manual";
                                    break;
                                case BUTTON_SW2_PIN:
                                    button_name = "Motor Stop";
                                    break;
                                case BUTTON_SW3_PIN:
                                    button_name = "Blood In Flow";
                                    break;
                                case BUTTON_SW4_PIN:
                                    button_name = "Urine Sound";
                                    break;
                                default:
                                    button_name = "Unknown";
                                    break;
                            }
                            DEBUG_LOG("Button Event:\n");
                            DEBUG_LOG("  Button: %s (ID: %d)\n", button_name, msg->data.button.button_id);
                            
                            // 버튼별 이벤트 처리
                            if (strcmp(button_name, "Blood In Flow") == 0) {
                                hematuria_client_event_handler();
                            }
                            
                            if (msg->data.button.button_id == BUTTON_SW4_PIN && msg->event_type == EVENT_BUTTON_PRESS) {
                                const char* sound_state = 
                                    msg->data.button.state == CMD_STOP_SOUND ? "Stop Sound" :
                                    msg->data.button.state == CMD_WATER_SOUND ? "Water Sound" :
                                    msg->data.button.state == CMD_URINE_SOUND ? "Urine Sound" :
                                    "Unknown State";
                                DEBUG_LOG("%s: %s\n", button_name, sound_state);
                                
                                // 사운드 제어
                                if (strcmp(sound_state, "Stop Sound") == 0) {
                                    DEBUG_LOG("+++ Stop Sound\n");
                                    stop_sound();
                                } else if (strcmp(sound_state, "Water Sound") == 0) {
                                    DEBUG_LOG("+++ Water Sound\n");
                                    play_water_sound(REPEAT_INFINITE, 0);
                                } else if (strcmp(sound_state, "Urine Sound") == 0) {
                                    DEBUG_LOG("+++ Urine Sound\n");
                                    play_urination_sound(REPEAT_INFINITE, 0);
                                }
                            }
                        }
                        break;
                        
                    case EVENT_ERROR:
                        {
                            uint8_t error_code = msg->data.error.error_code;
                            const char* error_detail = msg->data.error.detail;
                            DEBUG_LOG("Error Event:\n");
                        }
                        break;
                        
                    case EVENT_WIFI_CHANGE:
                        {
                            // WiFi 이벤트 처리
                            switch (msg->event_source) {
                                case WIFI_EVENT_CONNECTED:
                                    {
                                        DEBUG_LOG("WiFi Connected:\n");
                                        DEBUG_LOG("  Details: %s\n", msg->data.error.detail);
                                        
                                        request_to_main(REQ_WIFI_STATUS);
                                        // WiFi 연결 상태 표시
                                        Event_wifi_ON();
                                        wifi_ON = 1;
                                    }
                                    break;
                                    
                                case WIFI_EVENT_DISCONNECTED:
                                    {
                                        DEBUG_LOG("WiFi Disconnected:\n");
                                        DEBUG_LOG("  Error Code: 0x%02X\n", msg->data.error.error_code);
                                        DEBUG_LOG("  Details: %s\n", msg->data.error.detail);
                                        
                                        // WiFi 연결 해제 표시
                                        Event_wifi_OFF();
                                        wifi_ON = 0;

                                    }
                                    break;
                                    
                                case WIFI_EVENT_IP_CHANGED:
                                    {
                                        DEBUG_LOG("WiFi IP Changed:\n");
                                        DEBUG_LOG("  New IP: %s\n", msg->data.error.detail);
                                        
                                        request_to_main(REQ_WIFI_STATUS);
                                        // WiFi 연결 상태 표시
                                        Event_wifi_ON();
                                        wifi_ON = 1;
                                    }
                                    break;
                                    
                                case WIFI_EVENT_WEAK_SIGNAL:
                                    {
                                        DEBUG_LOG("WiFi Weak Signal:\n");
                                        DEBUG_LOG("  Details: %s\n", msg->data.error.detail);
                                    }
                                    break;
                            }
                        }
                        break;
                        
                    default:
                        DEBUG_LOG("Unknown event type: 0x%02X\n", msg->event_type);
                        break;
                }
                DEBUG_LOG("=== Event Processing Complete ===\n\n");
            }
            break;
            
        default:
            DEBUG_LOG("Unknown message type: 0x%02X\n", header->type);
            break;
    }
}