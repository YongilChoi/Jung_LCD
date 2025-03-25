/**
 * @file serial_protocol.h
 * @brief 공통 시리얼 통신 프로토콜 헤더
 */
#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#define DEBUG_ENABLED

// 매크로 및 상수 정의
#define SERIAL_PROTOCOL_VERSION 0x01

#ifdef DEBUG_ENABLED
#define DEBUG_LOG(...) \
  if (debug_serial_flag) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

typedef enum {
  MAIN_BOARD,
  LCD_BOARD
} Board_Type;

// 시리얼 통신 설정
#define PROTOCOL_SERIAL Serial2
#define PROTOCOL_BAUD 115200
// Main 보드 시리얼2 포트 번호
#define MAIN_PROTOCOL_TX 17  // 메인 보드 TX 핀
#define MAIN_PROTOCOL_RX 18  // 메인 보드 RX 핀
// LCD 보드 시리얼2 포트 번호
#define LCD_PROTOCOL_TX 43  // LCD 보드 TX 핀
#define LCD_PROTOCOL_RX 44  // LCD 보드 RX 핀


// 타이머 설정
// serial_protocol.h에 추가
#define HELLO_INTERVAL_MS_DISCONNECTED   3000     // 연결 안됐을 때 간격: 3초
#define HELLO_INTERVAL_MS_CONNECTED      30000    // 연결됐을 때 간격: 10초
#define CONNECTION_TIMEOUT_MS            3*HELLO_INTERVAL_MS_CONNECTED
#define RANDOM_EVENT_INTERVAL            5000
#define UPDATE_SENSOR_DATA               10000

// 수신 버퍼 및 큐 설정
#define RX_BUFFER_SIZE 512
#define RX_QUEUE_SIZE 10
#define MAX_MESSAGE_SIZE 1024

// 백업 데이터 청크 크기
#define BACKUP_CHUNK_SIZE 512

// Message Types
#define MSG_HELLO               0x01
#define MSG_HELLO_ACK           0x02
#define MSG_SYSTEM_STATUS       0x03
#define MSG_SENSOR_DATA         0x04
#define MSG_WIFI_STATUS         0x06
#define MSG_TIME_SYNC           0x07
#define MSG_MOTOR_STATUS        0x08
#define MSG_REQUEST             0x09
#define MSG_RESPONSE            0x0A
#define MSG_EVENT               0x0B
#define MSG_WIFI_SCAN_LIST      0x0C  // SSID 리스트 전송
#define MSG_WIFI_SSID_SELECT    0x0D  // SSID 선택 전송
#define MSG_BLOOD_STATUS        0x0E  // 혈뇨 lcd->main
#define MSG_FACTORY_INIT        0x0F  // 공장초기화 lcd->main
#define MSG_DIAGNOSTIC_RESULT   0x10  // 진단 결과

// 데이터 저장/복원 관련 메시지 타입
#define MSG_DATA_BACKUP_REQUEST 0x20  // 백업 시작 요청 (Main -> LCD)
#define MSG_DATA_BACKUP_DATA    0x21  // 백업 데이터 전송 (Main -> LCD)
#define MSG_DATA_BACKUP_ACK     0x22  // 백업 확인 응답 (LCD -> Main)
#define MSG_DATA_RESTORE_REQUEST 0x23 // 복원 요청 (Main -> LCD)
#define MSG_DATA_RESTORE_DATA   0x24  // 복원 데이터 전송 (LCD -> Main)
#define MSG_DATA_RESTORE_ACK    0x25  // 복원 확인 응답 (Main -> LCD)

// 메시지 플래그
#define DATA_FLAG_START         0x01  // 시작 청크
#define DATA_FLAG_END           0x02  // 마지막 청크
#define DATA_FLAG_COMPRESSED    0x04  // 압축 데이터
#define DATA_FLAG_ERROR         0x80  // 오류 발생

// 이벤트 타입 정의
#define EVENT_SENSOR_COVER      0x01
#define EVENT_SENSOR_URINE      0x02
#define EVENT_SENSOR_WEIGHT     0x03
#define EVENT_SENSOR_GAS        0x04
#define EVENT_SENSOR_DIAPER     0x05
#define EVENT_SENSOR_BATTERY    0x06
#define EVENT_BUTTON_PRESS      0x07
#define EVENT_BUTTON_RELEASE    0x08
#define EVENT_MOTOR_STATE       0x09
#define EVENT_WIFI_CHANGE       0x0A
#define EVENT_SENSOR_FULLLEVEL  0x0B
#define EVENT_SERVER_CONNECT    0x0C
#define EVENT_SENSOR_POWER      0x0D
#define EVENT_SENSOR_SYSTEM     0x0E
#define EVENT_ERROR             0x55

// Request Types
#define REQ_SYSTEM_STATUS       0x01
#define REQ_SENSOR_STATUS       0x02
#define REQ_MOTOR_CONTROL       0x03
#define REQ_WIFI_STATUS         0x04
#define REQ_WIFI_SCANLIST       0x05 // scan lists
#define REQ_TIMESYNC_STATUS     0x06

// Button IDs
#define BUTTON_SW1_PIN   1  // Motor Manual
#define BUTTON_SW2_PIN   2  // Motor Stop
#define BUTTON_SW3_PIN   3  // Blood Flow
#define BUTTON_SW4_PIN   4  // Urine Sound

// Motor Commands
#define MOTOR_CMD_START         0x01
#define MOTOR_CMD_STOP          0x02
#define MOTOR_CMD_MANUAL        0x03
#define MOTOR_CMD_AUTO          0x04

// Motor Status
#define MOTOR_STATUS_STOP       0x00
#define MOTOR_STATUS_RUNNING    0x01
#define MOTOR_STATUS_ERROR      0x02

// Message Flags
#define FLAG_SAVE_TO_SD         0x80
#define FLAG_RESPONSE           0x40
#define FLAG_ERROR              0x20
#define FLAG_URGENT             0x10

// System Status Flags
#define SYS_FLAG_NORMAL         0x00
#define SYS_FLAG_ERROR          0x01
#define SYS_FLAG_WARNING        0x02
#define SYS_FLAG_MAINTAIN       0x03

// WiFi 이벤트 타입 정의
#define WIFI_EVENT_CONNECTED       0x01
#define WIFI_EVENT_DISCONNECTED    0x02
#define WIFI_EVENT_CONNECT_FAILED  0x03
#define WIFI_EVENT_WEAK_SIGNAL     0x04
#define WIFI_EVENT_IP_CHANGED      0x05

// WiFi 에러 코드 정의
#define WIFI_ERROR_AUTH_FAILED     0x10
#define WIFI_ERROR_NO_AP_FOUND     0x11
#define WIFI_ERROR_CONNECT_TIMEOUT 0x12
#define WIFI_ERROR_LOST_CONN       0x13
#define WIFI_ERROR_SIGNAL_LOST     0x14

// WiFi 신호 강도 임계값
#define WIFI_SIGNAL_WEAK           20  // 20% 이하면 약한 신호
#define WIFI_SIGNAL_WARNING        10  // 10% 이하면 경고

// 메시지 구조체 정의 (바이트 패킹)
#pragma pack(push, 1)

// 기본 메시지 헤더
typedef struct {
    uint8_t start_marker;   // 시작 마커 (0xFF)
    uint8_t type;           // 메시지 타입
    uint8_t flags;          // 메시지 플래그
    uint32_t timestamp;     // Unix timestamp
    uint16_t seq_num;       // 시퀀스 번호
    uint16_t length;        // 페이로드 길이
} MessageHeader;

// Hello 메시지
typedef struct {
    MessageHeader header;
    uint8_t board_type;     // 1: Main, 2: LCD
    uint8_t reserved[3];    // 예약됨
    uint8_t checksum;       // 체크섬
} HelloMessage;

// 시스템 상태 메시지
typedef struct {
    MessageHeader header;
    char model_name[32];
    char hw_version[8];
    char sw_version[16];
    char serial_num[32];
    uint8_t sys_status;
    uint8_t error_code;
    uint8_t reserved[2];
    uint8_t checksum;
} SystemStatusMessage;

// 센서 데이터 메시지
typedef struct {
    MessageHeader header;
    bool cover_sensor;      // 커버 센서: true=닫힘, false=열림
    bool urine_sensor;      // 소변 센서: true=감지됨, false=감지안됨
    bool diaper_sensor;     // 기저귀 센서: true=삽입됨, false=제거됨
    bool fulllevel_sensor;  // 만수위 센서: true=만수위, false=정상
    bool gas_sensor;        // 가스 센서: true=감지됨, false=감지안됨
    bool power_status;      // 전원 상태: true=켜짐, false=꺼짐
    bool server_status;     // 서버 연결: true=연결됨, false=연결안됨 
    bool motor_running;     // 모터 동작 여부: true=동작중, false=멈춤
    bool wifi_connected;     // Wifi 연결 여부: true=연결됨, false=연결안됨
    uint8_t system_status;  // 시스템 상태: 0=정상, 1=오류
    float weight_sensor;    // 무게 센서 값 (그램)
    float battery_level;    // 배터리 수준 (퍼센트)
    uint8_t reserved[3];    // 예약됨
    uint8_t checksum;       // 체크섬
} SensorDataMessage;

// WiFi 상태 메시지
typedef struct {
    MessageHeader header;
    bool connected;         // 연결 상태: true=연결됨, false=연결안됨
    char ssid[32];          // 연결된 SSID
    char ip_addr[16];       // IP 주소
    char gateway[16];       // 게이트웨이 주소
    uint8_t signal_strength; // 신호 강도 (퍼센트)
    uint8_t reserved;       // 예약됨
    uint8_t checksum;       // 체크섬
} WifiStatusMessage;

// SSID 정보 구조체
typedef struct {
    char ssid[33];          // SSID (null 포함 33bytes)
    int32_t rssi;           // 신호 강도
    uint8_t security;       // 보안 타입
} WifiAPInfo;

// SSID 리스트 메시지
typedef struct {
    MessageHeader header;
    uint8_t ap_count;       // AP 개수
    WifiAPInfo ap_list[10]; // 최대 10개 AP 정보
    uint8_t checksum;       // 체크섬
} WifiScanListMessage;

// SSID 선택 메시지
typedef struct {
    MessageHeader header;
    char ssid[33];          // 선택된 SSID
    char password[65];      // WiFi 비밀번호
    uint8_t checksum;       // 체크섬
} WifiSelectMessage;

// 시간 동기화 메시지
typedef struct {
    MessageHeader header;
    uint32_t timestamp;     // 동기화할 시간 (Unix timestamp)
    uint8_t reserved[3];    // 예약됨
    uint8_t checksum;       // 체크섬
} TimeSyncMessage;

// 버튼 이벤트 메시지
typedef struct {
    MessageHeader header;
    uint8_t button_id;      // 버튼 ID
    uint8_t pressed;        // 버튼 pressed 상태
    uint8_t button_state;   // SW4의 소리 상태
    uint8_t reserved;       // 예약됨
    uint8_t checksum;       // 체크섬
} ButtonEventMessage;

// 모터 상태/제어 메시지
typedef struct {
    MessageHeader header;
    uint8_t motor_cmd;      // 모터 명령
    uint8_t motor_status;   // 모터 상태
    uint8_t motor_speed;    // 모터 속도
    uint8_t reserved;       // 예약됨
    uint8_t checksum;       // 체크섬
} MotorMessage;

// 혈뇨 상태 메시지
typedef struct {
    MessageHeader header;
    uint8_t amount;         // 혈뇨량 (0, 1(소), 2(중), 3(대))
    uint8_t checksum;       // 체크섬
} BloodStatusMessage;

// 공장 초기화 메시지
typedef struct {
    MessageHeader header; 
    uint8_t checksum;       // 체크섬
} FactoryInitMessage;

// 진단 결과 메시지
typedef struct {
    MessageHeader header;
    uint8_t sensor_status;     // 각 비트가 센서 상태를 나타냄
    uint8_t actuator_status;   // 모터, LED 등의 상태
    uint8_t power_status;      // 배터리, 어댑터 상태
    uint8_t system_status;     // RTC, NVS, WiFi 상태
    uint16_t battery_voltage;  // 배터리 전압
    uint16_t weight_sensor1;   // 무게 센서1 값
    uint16_t weight_sensor2;   // 무게 센서2 값
    float gas;                 // 가스 센서 값
    float temperature;         // 온도
    float humidity;            // 습도
    float pressure;            // 기압
    uint8_t test_results[8];   // 각 테스트의 세부 결과
    uint8_t checksum;          // 체크섬
} DiagnosticResultMessage;

// 백업 요청 메시지
typedef struct {
   MessageHeader header;
   uint32_t total_size;     // 전체 데이터 크기
   uint32_t chunk_size;     // 청크 크기
   uint16_t total_chunks;   // 총 청크 수
   uint8_t flags;           // 백업 플래그
   uint8_t checksum;        // 체크섬
} BackupRequestMessage;

// 백업 데이터 메시지 (가변 크기)
typedef struct {
   MessageHeader header;
   uint16_t chunk_index;    // 현재 청크 번호
   uint16_t chunk_size;     // 현재 청크 크기
   uint8_t flags;           // 데이터 플래그
   uint8_t data[];          // 가변 데이터
} BackupDataMessage;

// 백업 확인 메시지
typedef struct {
   MessageHeader header;
   uint16_t chunk_index;    // 확인된 청크 번호
   uint8_t status;          // 상태 코드
   uint8_t checksum;        // 체크섬
} BackupAckMessage;

// 복원 요청 메시지
typedef struct {
    MessageHeader header;
    uint8_t flags;          // 복원 플래그
    uint8_t checksum;       // 체크섬
} RestoreRequestMessage;

// 복원 데이터 메시지
typedef struct {
    MessageHeader header;
    uint16_t chunk_index;    // 현재 청크 번호
    uint16_t chunk_size;     // 현재 청크 크기
    uint8_t flags;           // 데이터 플래그
    uint8_t data[BACKUP_CHUNK_SIZE]; // 고정 크기 데이터 버퍼
    uint8_t checksum;        // 체크섬
} RestoreDataMessage;

// 복원 확인 메시지
typedef struct {
    MessageHeader header;
    uint16_t chunk_index;    // 확인된 청크 번호
    uint8_t status;          // 상태 코드
    uint8_t checksum;        // 체크섬
} RestoreAckMessage;

// 요청 메시지
typedef struct {
    MessageHeader header;
    uint8_t request_type;    // 요청 타입
    uint8_t request_param;   // 요청 파라미터
    uint8_t reserved[2];     // 예약됨
    uint8_t checksum;        // 체크섬
} RequestMessage;

// 이벤트 메시지
typedef struct {
    MessageHeader header;
    uint8_t event_type;      // 이벤트 타입
    uint8_t event_source;    // 이벤트 소스 (센서/버튼 ID 등)
    union {
        struct {
            uint8_t state;   // On/Off 상태
            uint8_t reserved[7]; // 유니온 패딩
        } digital;
        struct {
            float value;     // 아날로그 값
            uint8_t reserved[4]; // 유니온 패딩
        } analog;
        struct {
            uint8_t button_id;
            uint8_t state;
            uint8_t reserved[6]; // 유니온 패딩
        } button;
        struct {
            uint8_t error_code;
            char detail[16];  // 에러 설명
        } error;
    } data;
    uint8_t checksum;         // 체크섬
} EventMessage;

// WiFi 이벤트 상세 정보
typedef struct {
    char ssid[32];            // SSID
    char ip[16];              // IP 주소
    char detail[32];          // 추가 상세 정보
} WifiEventDetail;

#pragma pack(pop)

// 수신 큐를 위한 구조체
typedef struct {
    uint8_t buffer[MAX_MESSAGE_SIZE];
    size_t size;
} SerialRxQueueItem;

// 시리얼 프로토콜 컨텍스트 구조체
typedef struct {
    // 통신 상태
    uint8_t board_type;
    uint16_t sequence_number;
    bool peer_connected;
    uint32_t last_hello_time;
    uint32_t last_received_time;
    uint32_t last_random_event;
    bool initial_status_sent;
    
    // 수신 버퍼 관련
    uint8_t rx_buffer[RX_BUFFER_SIZE];
    size_t rx_buffer_index;
    
    // 수신 큐 관련
    SerialRxQueueItem rx_queue[RX_QUEUE_SIZE];
    int rx_queue_head;
    int rx_queue_tail;
    int rx_queue_count;
    
    // 뮤텍스 핸들 (FreeRTOS)
    SemaphoreHandle_t rx_mutex;
    SemaphoreHandle_t tx_mutex;

    // 콜백 함수 포인터 (보드별 메시지 처리 함수)
    void (*process_message_callback)(void* message, size_t size);
} SerialProtocolContext;

// 백업 및 복원 상태 관리 구조체
typedef struct {
    bool waiting_for_ack;
    bool ack_received;
    uint16_t ack_chunk;
    uint8_t ack_status;
} BackupState;

typedef struct {
    bool waiting_for_data;
    bool data_received;
    uint16_t data_chunk;
    uint16_t chunk_size;
    uint8_t* data;
    bool is_last_chunk;
} RestoreState;

// 함수 선언
// 초기화 및 작업 함수
bool serial_protocol_init(SerialProtocolContext* ctx, Board_Type board_type, void (*process_callback)(void* message, size_t size));
bool serial_init(SerialProtocolContext* ctx);
void serial_protocol_task(void* pvParameters);

// 메시지 송수신 함수
bool send_message(SerialProtocolContext* ctx, void* message, size_t size);
void handle_received_data(SerialProtocolContext* ctx, uint8_t* data, size_t length);
void process_rx_queue(SerialProtocolContext* ctx);

// 큐 관리 함수
bool rx_queue_push(SerialProtocolContext* ctx, uint8_t* data, size_t size);
bool rx_queue_pop(SerialProtocolContext* ctx, uint8_t* buffer, size_t* size);
bool rx_queue_is_empty(SerialProtocolContext* ctx);
bool rx_queue_is_full(SerialProtocolContext* ctx);

// 유틸리티 함수
uint8_t calculate_checksum(void* message, size_t size);
bool verify_checksum(void* message, size_t size);
char* format_timestamp(uint32_t timestamp, char* buffer, size_t size);
void print_timestamp(uint32_t timestamp);
void print_hex_dump(const uint8_t* data, size_t length);

// 디버그 및 로그 함수
void print_message(void* message);
void print_message_type(uint8_t type);
void decode_message_header(const MessageHeader* header);
const char* get_message_type_str(uint8_t type);
const char* get_event_type_str(uint8_t event_type);
const char* get_wifi_event_str(uint8_t wifi_event);
bool decode_event_message(EventMessage* msg, char* output, size_t output_size);

// 연결 관리 함수
void check_serial_connection(SerialProtocolContext* ctx);
void send_hello(SerialProtocolContext* ctx);

// 외부 참조 변수 선언
extern SerialProtocolContext g_serial_ctx;
//extern BackupState backup_state;
//extern RestoreState restore_state;
extern int debug_serial_flag;


#endif // SERIAL_PROTOCOL_H