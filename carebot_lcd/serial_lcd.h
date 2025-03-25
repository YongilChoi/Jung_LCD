/**
 * @file serial_lcd.h
 * @brief LCD 보드 시리얼 통신 헤더
 */
#ifndef SERIAL_LCD_H
#define SERIAL_LCD_H

#include "serial_protocol.h"

#define WEIGHT_THRESHOLD_HIGH   1800.0f  // 1000g
#define BATTERY_LOW_THRESHOLD   20.0f    // 20%

/**
 * @brief LCD 보드 상태 구조체
 */
typedef struct {
      // 단말기 정보
    char model_name[32];      // 모델명
    char sw_version[16];      // SW 버전
    char serial_num[32];   // 시리얼 번호
    char ssid[32];            // SSID
    char ip_addr[16];         // IP 주소
    char gateway[16];         // 게이트웨이
    uint8_t signal_strength; // 신호 강도 (퍼센트)

    // 시스템 정보
    char hw_version[8];
    //char sw_version[16];
    //char serial_num[32];
    uint8_t sys_status;
    uint8_t error_code;
    
    // 센서 상태 (메인 보드에서 수신)
    int wifi_connected;     // Wifi 연결 여부: true=연결됨, false=연결안됨
    int cover_sensor;      // 커버 센서: true=닫힘, false=열림
    int urine_sensor;      // 소변 센서: true=감지됨, false=감지안됨
    int diaper_sensor;     // 기저귀 센서: true=삽입됨, false=제거됨
    int fulllevel_sensor;  // 만수위 센서: true=만수위, false=정상
    int gas_sensor;        // 가스 센서: true=감지됨, false=감지안됨
    int power_status;      // 전원 상태: true=켜짐, false=꺼짐
    int server_status;     // 서버 연결: true=연결됨, false=연결안됨 
    uint8_t system_status;  // 시스템 상태: 0=정상, 1=오류
    float weight_sensor;    // 무게 센서 값 (그램)
    float battery_level;    // 배터리 수준 (퍼센트)
    
    #if 0
    // WiFi 정보
    bool wifi_connected;
    char ssid[32];
    char ip_addr[16];
    char gateway[16];
    uint8_t signal_strength;
    #endif

    // 모터 상태
    int motor_status;
    uint8_t motor_speed;
    
    // 혈뇨 상태
    uint8_t blood_amount;
    
    // 시간 정보
    uint32_t last_timestamp;
} LCDBoardState;

// 사용 가능한 WiFi 네트워크 정보
typedef struct {
    char ssid[33];
    char password[65];
} WifiNetwork;

// 함수 선언
// 초기화 및 태스크 함수
bool serial_lcd_init(void);
void serial_lcd_task(void* pvParameters);

// 메시지 처리 함수
void lcd_process_message(void* message, size_t size);

// Main 보드로의 송신 함수
void request_wifi_scan(void);
void send_blood_status(uint8_t amount);
void request_to_main(uint8_t request_type);

// UI 업데이트 함수
void update_system_status_display(SystemStatusMessage* msg);
void update_sensor_display(SensorDataMessage* msg);
void update_wifi_status_display(WifiStatusMessage* msg);
void update_motor_status_display(MotorMessage* msg);
void update_display_time(uint32_t timestamp);

// 센서 이벤트 UI 업데이트 함수
void update_cover_sensor_display(bool state);
void update_diaper_sensor_display(bool state);
void update_power_sensor_display(bool state);
void update_full_level_display(bool state);
void update_urine_sensor_display(bool state);
void update_weight_display(float weight);
void update_gas_display(float level);
void update_battery_display(float level);
void update_button_status_display(uint8_t button_id, bool pressed);

// 알림 및 경고 표시 함수
void show_notification(const char* message);
void show_warning_popup(const char* title, const char* message);
void show_error_screen(uint8_t error_code, const char* message);
const char* get_error_message(uint8_t error_code, const char* detail);
bool is_critical_error(uint8_t error_code);
void play_error_sound(void);

// WiFi 상태 표시 함수
void display_wifi_connected_status(const char* detail);
void display_wifi_disconnected_status(uint8_t error_code);
void display_wifi_ip_status(const char* ip_info);
void display_wifi_signal_warning(const char* signal_info);
void show_wifi_error_message(const char* message);
void show_system_error(uint8_t error_code);

// WiFi 관련 함수
void send_wifi_conn_info(char* ssid, const char* pw);
void handle_wifi_scan_list(WifiScanListMessage* msg);

// 버튼 이벤트 함수
void send_button_event(uint8_t button_id, uint8_t state);
void send_blood_status(uint8_t amount);
void send_factory_init(void);

// 메뉴 관련 함수
void toggle_menu_display(void);
void show_menu(void);
void hide_menu(void);

// 외부 참조 변수 선언
extern LCDBoardState lcd_state;
extern int network_count;

// 외부 이벤트 핸들러 함수 참조 (LVGL 관련)
extern void lvgl_update_app_ui(void);
extern void cover_open_event_handler(void);
extern void fulllevel_event_handler(void);
extern void hematuria_client_event_handler(void);
extern void Event_power_ON(void);
extern void Event_power_OFF(void);
extern void Event_bat_ON(void);
extern void Event_bat_LOW(void);
extern void Event_diaper_ON(void);
extern void Event_diaper_OFF(void);
extern void Event_feces_ON(void);
extern void Event_feces_OFF(void);
extern void Event_wifi_ON(void);
extern void Event_wifi_OFF(void);
extern void Event_motor_ON(void);
extern void Event_motor_OFF(void);
extern void Event_connect_ON(void);
extern void Event_connect_OFF(void);
extern void Event_error_ON(void);
extern void Event_error_OFF(void);
extern void Event_tank_ON(float value);

// 오디오 관련 함수 참조
extern void play_water_sound(int repeat, int delay_ms);
extern void play_urination_sound(int repeat, int delay_ms);
extern void stop_sound(void);

#endif // SERIAL_LCD_H