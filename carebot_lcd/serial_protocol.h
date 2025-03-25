// serial_protocol.h
#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#define BOARD_TYPE_LCD


//#include "common.h"

// SD 카드 사용 (LCD 보드에서만 활성화)
#ifdef BOARD_TYPE_LCD
//#include <SD.h>
#define USE_SD
#define SD_CHIP_SELECT     5     // SD 카드 CS 핀 (실제 사용하는 핀으로 변경 필요)
//#define WEIGHT_THRESHOLD_HIGH   1000.0f  // 1000g
#define WEIGHT_THRESHOLD_HIGH   1800.0f  // 2000g
#define WEIGHT_THRESHOLD_LOW    10.0f    // 10g
#define GAS_WARNING_LEVEL      500.0f    // 500ppm
#define GAS_DANGER_LEVEL       800.0f    // 800ppm
#define BATTERY_LOW_THRESHOLD   20.0f    // 20%
#define BATTERY_CRITICAL       10.0f     // 10%
#endif

// Serial 통신 설정
#define PROTOCOL_SERIAL Serial2
#define PROTOCOL_BAUD 115200
//#define PROTOCOL_TX GPIO_NUM_15  // 실제 사용할 핀 번호로 변경
//#define PROTOCOL_RX GPIO_NUM_16  // 실제 사용할 핀 번호로 변경
#ifdef BOARD_TYPE_MAIN 
#define PROTOCOL_TX GPIO_NUM_17  // 실제 사용할 핀 번호로 변경
#define PROTOCOL_RX GPIO_NUM_18  // 실제 사용할 핀 번호로 변경
#else //BOARD_TYPE_LCD
#define PROTOCOL_TX GPIO_NUM_43  // 
#define PROTOCOL_RX GPIO_NUM_44  // 
#endif

// 타이머 설정
#define HELLO_INTERVAL_MS 3000
#define CONNECTION_TIMEOUT_MS 10000
#define RANDOM_EVENT_INTERVAL 5000

// 수신 버퍼 및 큐 설정
#define RX_BUFFER_SIZE 512
#define RX_QUEUE_SIZE 10
#define MAX_MESSAGE_SIZE 1024

// 백업 데이터 청크 크기
#define BACKUP_CHUNK_SIZE 512

// Message Types
#define MSG_HELLO           0x01
#define MSG_HELLO_ACK      0x02
#define MSG_SYSTEM_STATUS   0x03
#define MSG_SENSOR_DATA     0x04
#define MSG_BUTTON_EVENT    0x05
#define MSG_WIFI_STATUS     0x06
#define MSG_TIME_SYNC       0x07
#define MSG_MOTOR_STATUS    0x08
#define MSG_REQUEST         0x09
#define MSG_RESPONSE        0x0A
#define MSG_EVENT           0x0B
#define MSG_WIFI_SCAN_LIST    0x0C  // SSID 리스트 전송
#define MSG_WIFI_SSID_SELECT  0x0D  // SSID 선택 전송
#define MSG_BLOOD_STATUS  0x0E  
#define MSG_FACTORY_INIT  0x0F
#define MSG_DIAGNOSTIC_RESULT  0x10  //  진단 결과 

// 데이터 저장할 메시지 타입과 구조체 정의
#define MSG_DATA_BACKUP_REQUEST  0x20    // 백업 시작 요청 (Main -> LCD)
#define MSG_DATA_BACKUP_DATA     0x21    // 백업 데이터 전송 (Main -> LCD)
#define MSG_DATA_BACKUP_ACK      0x22    // 백업 확인 응답 (LCD -> Main)
#define MSG_DATA_RESTORE_REQUEST 0x23    // 복원 요청 (Main -> LCD)
#define MSG_DATA_RESTORE_DATA    0x24    // 복원 데이터 전송 (LCD -> Main)
#define MSG_DATA_RESTORE_ACK     0x25    // 복원 확인 응답 (Main -> LCD)

// 메시지 플래그
#define DATA_FLAG_START          0x01    // 시작 청크
#define DATA_FLAG_END           0x02    // 마지막 청크
#define DATA_FLAG_COMPRESSED    0x04    // 압축 데이터
#define DATA_FLAG_ERROR         0x80    // 오류 발생


// 이벤트 타입 정의
#define EVENT_SENSOR_COVER    0x01
#define EVENT_SENSOR_URINE    0x02
#define EVENT_SENSOR_WEIGHT   0x03
#define EVENT_SENSOR_GAS      0x04
#define EVENT_SENSOR_DIAPER   0x05
#define EVENT_SENSOR_BATTERY  0x06
#define EVENT_BUTTON_PRESS    0x07
#define EVENT_BUTTON_RELEASE  0x08
#define EVENT_MOTOR_STATE     0x09
#define EVENT_WIFI_CHANGE     0x0A
#define EVENT_SENSOR_FULLLEVEL   0x0B
#define EVENT_SERVER_CONNECT   0x0C
#define EVENT_SENSOR_POWER   0x0D
#define EVENT_SENSOR_SYSTEM   0x0E
#define EVENT_ERROR           0x55

// Request Types
#define REQ_SYSTEM_STATUS   0x01
#define REQ_SENSOR_STATUS   0x02
#define REQ_MOTOR_CONTROL   0x03
#define REQ_WIFI_STATUS     0x04
#define REQ_WIFI_SCANLIST   0x05 // scan lists
#define REQ_TIMESYNC_STATUS   0x06


// Button IDs
#define BUTTON_SW1_PIN   1
#define BUTTON_SW2_PIN   2
#define BUTTON_SW3_PIN   3
#define BUTTON_SW4_PIN   4

// Motor Commands
#define MOTOR_CMD_START     0x01
#define MOTOR_CMD_STOP      0x02
#define MOTOR_CMD_MANUAL    0x03
#define MOTOR_CMD_AUTO      0x04

// Motor Status
#define MOTOR_STATUS_STOP    0x00
#define MOTOR_STATUS_RUNNING 0x01
#define MOTOR_STATUS_ERROR   0x02

// Message Flags
#define FLAG_SAVE_TO_SD     0x80
#define FLAG_RESPONSE       0x40
#define FLAG_ERROR          0x20
#define FLAG_URGENT         0x10

// System Status Flags
#define SYS_FLAG_NORMAL     0x00
#define SYS_FLAG_ERROR      0x01
#define SYS_FLAG_WARNING    0x02
#define SYS_FLAG_MAINTAIN   0x03

// WiFi 이벤트 타입 정의
#define WIFI_EVENT_CONNECTED       0x01
#define WIFI_EVENT_DISCONNECTED    0x02
#define WIFI_EVENT_CONNECT_FAILED  0x03
#define WIFI_EVENT_WEAK_SIGNAL    0x04
#define WIFI_EVENT_IP_CHANGED     0x05

// WiFi 에러 코드 정의
#define WIFI_ERROR_AUTH_FAILED     0x10
#define WIFI_ERROR_NO_AP_FOUND    0x11
#define WIFI_ERROR_CONNECT_TIMEOUT 0x12
#define WIFI_ERROR_LOST_CONN      0x13
#define WIFI_ERROR_SIGNAL_LOST    0x14

// WiFi 신호 강도 임계값
#define WIFI_SIGNAL_WEAK          20    // 20% 이하면 약한 신호
#define WIFI_SIGNAL_WARNING       10    // 10% 이하면 경고

typedef struct {
    char ssid[32];
    char ip[16];
    char detail[32];
} WifiEventDetail;
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


// LCD 보드용 함수 선언
#ifdef BOARD_TYPE_LCD
void update_display_time(uint32_t timestamp);
#endif

#pragma pack(push, 1)
// 기본 메시지 헤더
typedef struct {
    uint8_t start_marker;   // 시작 마커 (0xFF)
    uint8_t type;          // 메시지 타입
    uint8_t flags;         // 메시지 플래그
    uint32_t timestamp;    // Unix timestamp
    uint16_t seq_num;      // 시퀀스 번호
    uint16_t length;       // 페이로드 길이
} MessageHeader;

// Hello 메시지
typedef struct {
    MessageHeader header;
    uint8_t board_type;
    uint8_t reserved[3];
    uint8_t checksum;
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
// 센서 데이터 메시지
typedef struct {
    MessageHeader header;
    bool cover_sensor;
    bool urine_sensor;
    bool diaper_sensor;
    bool fulllevel_sensor;
    bool gas_sensor;
    bool power_status;
	bool server_status;     // 서버 연결: true=연결됨, false=연결안됨 
    bool motor_running;     // 모터 동작 여부: true=동작중, false=멈춤
    bool wifi_connected;     // Wifi 연결 여부: true=연결됨, false=연결안됨
    uint8_t system_status;
    float weight_sensor;
    uint8_t battery_level;
    uint8_t reserved[3];
    uint8_t checksum;
} SensorDataMessage;

// WiFi 상태 메시지
typedef struct {
    MessageHeader header;
    bool connected;
    char ssid[32];
    //char pw[32];
    char ip_addr[16];
    char gateway[16];
    uint8_t signal_strength;
    uint8_t reserved;
    uint8_t checksum;
} WifiStatusMessage;

// SSID 정보 구조체
typedef struct {
    char ssid[33];     // SSID (null 포함 33bytes)
    int32_t rssi;      // 신호 강도
    uint8_t security;  // 보안 타입
} WifiAPInfo;

// SSID 리스트 메시지
typedef struct {
    MessageHeader header;
    uint8_t ap_count;        // AP 개수
    WifiAPInfo ap_list[10];  // 최대 10개 AP 정보
    uint8_t checksum;
} WifiScanListMessage;

// SSID 선택 메시지
typedef struct {
    MessageHeader header;
    char ssid[33];          // 선택된 SSID
    char password[65];      // WiFi 비밀번호
    uint8_t checksum;
} WifiSelectMessage;

typedef struct {
    MessageHeader header; 
    uint8_t checksum;   
} FactoryInitMessage;

typedef struct {
    MessageHeader header;
    uint8_t sensor_status;     // 각 비트가 센서 상태를 나타냄
    uint8_t actuator_status;   // 모터, LED 등의 상태
    uint8_t power_status;      // 배터리, 어댑터 상태
    uint8_t system_status;     // RTC, NVS, WiFi 상태
    uint16_t battery_voltage;
    uint16_t weight_sensor1;
    uint16_t weight_sensor2;
    float gas;
    float temperature;
    float humidity;
    float pressure;
    uint8_t test_results[8];   // 각 테스트의 세부 결과
    uint8_t checksum;
} DiagnosticResultMessage;

typedef struct {
    MessageHeader header;
    uint8_t amount;      // 혈뇨량 (0, 1(소), 2(중), 3(대))
    uint8_t checksum;
} BloodStatusMessage;

/////////////////////
// 데이터 저장
typedef struct {
   MessageHeader header;
   uint32_t total_size;     // 전체 데이터 크기
   uint32_t chunk_size;     // 청크 크기
   uint16_t total_chunks;   // 총 청크 수
   uint8_t flags;          // 백업 플래그
   uint8_t checksum;
} BackupRequestMessage;

typedef struct {
   MessageHeader header;
   uint16_t chunk_index;    // 현재 청크 번호
   uint16_t chunk_size;     // 현재 청크 크기
   uint8_t flags;          // 데이터 플래그
   uint8_t data[];         // 가변 데이터
} BackupDataMessage;

typedef struct {
   MessageHeader header;
   uint16_t chunk_index;    // 확인된 청크 번호
   uint8_t status;         // 상태 코드
   uint8_t checksum;
} BackupAckMessage;

// 복원 요청 메시지
typedef struct {
    MessageHeader header;
    uint8_t flags;          // 복원 플래그
    uint8_t checksum;
} RestoreRequestMessage;

#define BACKUP_CHUNK_SIZE 1024  // 백업 청크 크기
#define RESTORE_CHUNK_SIZE 1024  // 백업과 동일하게 1KB 단위

// 복원 데이터 메시지 (LCD -> Main)
typedef struct {
    MessageHeader header;
    uint16_t chunk_index;    // 현재 청크 번호
    uint16_t chunk_size;     // 현재 청크 크기
    uint8_t flags;          // 데이터 플래그
    uint8_t data[BACKUP_CHUNK_SIZE]; // 고정 크기 데이터 버퍼
    uint8_t checksum;
} RestoreDataMessage;

// 복원 확인 메시지 (Main -> LCD)
typedef struct {
    MessageHeader header;
    uint16_t chunk_index;    // 확인된 청크 번호
    uint8_t status;         // 상태 코드
    uint8_t checksum;
} RestoreAckMessage;

////////////////////////////

// 시간 동기화 메시지 구조체
typedef struct {
    MessageHeader header;
    uint32_t timestamp;
    uint8_t reserved[3];
    uint8_t checksum;
} TimeSyncMessage;

// 버튼 이벤트 메시지
typedef struct {
    MessageHeader header;
    uint8_t button_id;
    bool pressed;
	uint8_t button_state;   // SW4의 소리 상태
    uint8_t reserved[2];
    uint8_t checksum;
} ButtonEventMessage;

// 모터 상태/제어 메시지
typedef struct {
    MessageHeader header;
    uint8_t motor_cmd;
    uint8_t motor_status;
    uint8_t motor_speed;
    uint8_t reserved;
    uint8_t checksum;
} MotorMessage;

// 요청 메시지
typedef struct {
    MessageHeader header;
    uint8_t request_type;
    uint8_t request_param;
    uint8_t reserved[2];
    uint8_t checksum;
} RequestMessage;

typedef struct {
    MessageHeader header;
    uint8_t event_type;     // 이벤트 타입
    uint8_t event_source;   // 이벤트 소스 (센서/버튼 ID 등)
    union {
        struct {
            uint8_t state;  // On/Off 상태
            uint8_t reserved[7]; // 유니온 패딩
        } digital;
        struct {
            float value;    // 아날로그 값
             uint8_t reserved[4]; // 유니온 패딩
        } analog;
        struct {
            uint8_t button_id;
            uint8_t state;
			uint8_t reserved[6]; // 유니온 패딩
        } button;
        struct {
            uint8_t error_code;
            char detail[16];  // 적당한 크기는??
        } error;
    } data;
    uint8_t checksum;
} EventMessage;

#pragma pack(pop)

// 전역 상태 구조체
typedef struct {
    // 통신 상태
    uint8_t board_type;
    uint16_t sequence_number;
    bool peer_connected;
    uint32_t last_hello_time;
    uint32_t last_received_time;
    uint32_t last_random_event;
    bool initial_status_sent;
    
    // 시스템 상태
    bool wifi_connected;
    uint8_t sys_status;
    uint8_t error_code;
    
    // 모터 상태
    uint8_t motor_status;
    uint8_t motor_speed;
    
    // 센서 상태
    bool cover_sensor;
    bool urine_sensor;
    float weight_sensor;
    float gas_sensor;
    uint8_t battery_level;
    
    // 버튼 상태
    bool button_states[4];
    uint32_t last_debounce_time[4];
    //#ifdef BOARD_TYPE_LCD
    bool sd_initialized;     // SD 카드 초기화 상태
    bool sd_available;       // SD 카드 사용 가능 상태
    //#endif
} GlobalState;

// 초기화 함수
bool serial_protocol_init();
bool serial_init();

#ifdef BOARD_TYPE_MAIN
void start_motor(uint8_t mode);
void stop_motor();
#endif

// 프로토콜 태스크 함수
void check_serial_connection();
void generate_random_event();

void print_timestamp(uint32_t timestamp);
char* format_timestamp(uint32_t timestamp, char* buffer, size_t size);

// Message 출력 함수
void print_message(void* message);
void print_message_type(uint8_t type);
void print_hex_dump(const uint8_t* data, size_t length);
void decode_message_header(const MessageHeader* header);
const char* get_message_type_str(uint8_t type);
//const char* get_message_type_str(uint8_t type);
const char* get_event_type_str(uint8_t event_type);
const char* get_wifi_event_str(uint8_t wifi_event);
//void decode_message_header(const MessageHeader* header);
void decode_message_payload(void* message);

// 메시지 전송 함수
bool send_message(void* message, size_t size);
void send_hello();
void send_system_status();
void send_wifi_status();
void scan_and_send_wifi_list();
void send_sensor_data(bool save_to_sd);
void send_motor_status();
void send_button_event(uint8_t button_id, bool pressed);

void encode_sensor_event(uint8_t sensor_type, float value);
void encode_button_event(uint8_t button_id, bool pressed);
void encode_wifi_event(uint8_t event_type, uint8_t error_code, WifiEventDetail* detail);
void encode_error_event(uint8_t error_code, uint8_t detail);
bool decode_event_message(EventMessage* msg, char* output, size_t output_size);
const char* get_error_message(uint8_t error_code, const char* detail);


// 메시지 수신 및 처리 함수
void handle_received_data(uint8_t* data, size_t length);
void process_message(void* message);
void process_message_backup(void* message);

uint8_t calculate_checksum(void* message, size_t size);
bool verify_checksum(void* message, size_t size);

void TaskSerialCommnunication(void* pvParameters);

#ifdef BOARD_TYPE_LCD
void display_wifi_connected_status(const char* detail);
void display_wifi_disconnected_status(uint8_t error_code);
void display_wifi_ip_status(const char* ip_info); 
void display_wifi_signal_warning(const char* signal_info);
void show_wifi_error_message(const char* message);
void show_system_error(uint8_t error_code);

// LCD 보드 전용 함수
bool check_sd_card();
bool save_to_sd(const char* filename, void* data, size_t size);
void update_system_status_display(SystemStatusMessage* msg);
void update_sensor_display(SensorDataMessage* msg);
void update_button_status_display(ButtonEventMessage* msg);
void update_wifi_status_display(WifiStatusMessage* msg);
void update_motor_status_display(MotorMessage* msg);
void toggle_menu_display();
void show_menu();
void hide_menu();
#endif

// 전역 상태 변수
extern GlobalState g_state;
extern int debug_serial_flag;
extern void hematuria_client_event_handler(); 
extern void cover_open_event_handler();
extern void mp3play(char* type);
extern bool init_uart_simulation();


#endif // SERIAL_PROTOCOL_H
