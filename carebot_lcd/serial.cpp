#include <Arduino.h>
#include "common.h"
#include "lvgl_controller.h"
#include "serial_protocol.h"
#include "serial.h"

#include "event.h"
#include "menu.h"
#include "audio.h"

#define BOARD_TYPE_LCD
#define DEBUG_ENABLED
int debug_serial_flag = false;

#ifdef DEBUG_ENABLED
#define DEBUG_LOG(...) \
  if (debug_serial_flag) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

// 전역 상태 변수 정의
QueueHandle_t serialQueue;  // 여기에서 한 번만 정의
GlobalState g_state = {};

static uint8_t serial2buffer[2048];

bool serial_init() {
  // PROTOCOL_SERIAL(Serial2) 핀 설정
  //PROTOCOL_SERIAL.begin(PROTOCOL_BAUD, SERIAL_8N1, PROTOCOL_RX, PROTOCOL_TX);
    PROTOCOL_SERIAL.begin(115200, SERIAL_8N1, 44, 43); 

  // 초기화 대기
  uint32_t timeout = millis();
  while (!PROTOCOL_SERIAL && (millis() - timeout < 1000)) {
    delay(10);
  }

  if (!PROTOCOL_SERIAL) {
    DEBUG_LOG("Protocol Serial initialization failed!\n");
    return false;
  }
    // 수신 버퍼 비우기
  while (PROTOCOL_SERIAL.available()) {
      PROTOCOL_SERIAL.read();
  }
    
    // TX 버퍼가 비워질 때까지 대기
  PROTOCOL_SERIAL.flush();
   DEBUG_LOG("Protocol Serial initialized and buffers cleared (TX: %d, RX: %d)\n", PROTOCOL_TX, PROTOCOL_RX);

  DEBUG_LOG("Protocol Serial initialized (TX: %d, RX: %d)\n", PROTOCOL_TX, PROTOCOL_RX);
  return true;
}

bool serial_protocol_init() {
#ifdef BOARD_TYPE_MAIN
  DEBUG_LOG("Initializing Main Board...\n");
  g_state.board_type = 1;
#else
  DEBUG_LOG("Initializing LCD Board...\n");
  g_state.board_type = 2;
#endif

  if (!serial_init()) {
    return false;
  }

  g_state.sequence_number = 0;
  g_state.peer_connected = false;
  g_state.last_hello_time = 0;
  g_state.last_received_time = 0;
  g_state.last_random_event = 0;
  g_state.initial_status_sent = false;

  // 초기 상태 설정
  g_state.wifi_connected = true;
  g_state.sys_status = SYS_FLAG_NORMAL;
  g_state.error_code = 0;
  g_state.motor_status = MOTOR_STATUS_STOP;
  g_state.motor_speed = 0;
  g_state.battery_level = 100;

  randomSeed(millis());

  return true;
}

// WiFi 상태 전송
void send_wifi_conn_info(char *ssid, const char *pw) {
    WifiSelectMessage msg = {};
    msg.header.start_marker = 0xFF;
    msg.header.type = MSG_WIFI_SSID_SELECT;
    msg.header.timestamp = time(NULL);
    msg.header.seq_num = g_state.sequence_number++;
    msg.header.length = sizeof(WifiSelectMessage) - sizeof(MessageHeader);
    
    strncpy(msg.ssid, ssid, sizeof(msg.ssid));
    strncpy(msg.password, pw, sizeof(msg.password));
    Serial.println("+++ msg.ssid: ");
    Serial.println(msg.ssid);
    Serial.println("+++ msg.password: ");
    Serial.println(msg.password);
 
    msg.checksum = calculate_checksum(&msg, sizeof(WifiSelectMessage) - 1);
    send_message(&msg, sizeof(WifiSelectMessage));

}

// WiFi 상태 전송
void send_factory_init() {

    FactoryInitMessage msg = {};
    msg.header.start_marker = 0xFF;
    msg.header.type = MSG_FACTORY_INIT;
    msg.header.timestamp = time(NULL);
    msg.header.seq_num = g_state.sequence_number++;
    msg.header.length = sizeof(FactoryInitMessage) - sizeof(MessageHeader);
 
    msg.checksum = calculate_checksum(&msg, sizeof(FactoryInitMessage) - 1);
    send_message(&msg, sizeof(FactoryInitMessage));


}

bool send_message(void* message, size_t size) {
   MessageHeader* header = (MessageHeader*)message;
   Serial.println("\n \n +++ send_message");
   Serial.println("+++ header->type:");
   Serial.println(header->type);
   Serial.println("+++ g_state.peer_connected:");
   Serial.println(g_state.peer_connected);

   /*
   if (header->type != MSG_HELLO && header->type != MSG_HELLO_ACK) {
       if (!g_state.peer_connected) {
           DEBUG_LOG("+++ Hello Peer not connected, message not sent\n");
           return false;
       }
   }
   */
   
   DEBUG_LOG("\n=== SENDING MESSAGE (decode_message_header) ===\n");
   decode_message_header(header);
   //DEBUG_LOG("Hex dump:\n");
   //print_hex_dump((uint8_t*)message, size);
   DEBUG_LOG("=====================\n");
   
   size_t written = PROTOCOL_SERIAL.write((uint8_t*)message, size);
   Serial.println("+++ written size:");
   Serial.println(written);
   PROTOCOL_SERIAL.flush();
   
   return (written == size);
}



void decode_message_header(const MessageHeader* header) {
   DEBUG_LOG("Message Header:\n");
   DEBUG_LOG("  Marker    : 0x%02X\n", header->start_marker);
   Serial.println("+++ header->type(in decode_message_header):");
   Serial.println(header->type);
   //DEBUG_LOG("  +++ Type 1     : %d (%s)\n", header->type, get_message_type_str(header->type));
   //DEBUG_LOG("  +++ Type 2    : 0x%02X (%s)\n", 13, get_message_type_str(13));

   DEBUG_LOG("  Type      : 0x%02X (%s)\n", header->type, get_message_type_str(header->type));
   DEBUG_LOG("  Flags     : 0x%02X [%s%s%s%s]\n", 
       header->flags,
       (header->flags & FLAG_SAVE_TO_SD) ? "SD " : "",
       (header->flags & FLAG_RESPONSE) ? "RSP " : "",
       (header->flags & FLAG_ERROR) ? "ERR " : "",
       (header->flags & FLAG_URGENT) ? "URG" : "");
   
   // 타임스탬프를 읽기 쉬운 형식으로 변환
   char timestamp_decode[32];
   time_t t = header->timestamp;
   strftime(timestamp_decode, sizeof(timestamp_decode), "%Y-%m-%d %H:%M:%S", localtime(&t));
   DEBUG_LOG("  +++ Timestamp : %s\n", timestamp_decode);
   DEBUG_LOG("  Sequence  : %u\n", header->seq_num);
   DEBUG_LOG("  Length    : %u bytes\n", header->length);
}

// 메시지 타입을 문자열로 변환하는 헬퍼 함수
const char* get_message_type_str(uint8_t type) {
   switch(type) {
    case MSG_HELLO: return "HELLO";
    case MSG_HELLO_ACK: return "HELLO_ACK";
    case MSG_SYSTEM_STATUS: return "SYSTEM_STATUS";
    case MSG_SENSOR_DATA: return "SENSOR_DATA";
    case MSG_BUTTON_EVENT: return "BUTTON_EVENT";
    case MSG_WIFI_STATUS: return "WIFI_STATUS";
    case MSG_TIME_SYNC: return "TIME_SYNC";
    case MSG_MOTOR_STATUS: return "MOTOR_STATUS";
    case MSG_REQUEST: return "REQUEST";
    case MSG_RESPONSE: return "RESPONSE";
    case MSG_EVENT: return "EVENT";
    case MSG_WIFI_SCAN_LIST: return "WIFI_SCAN_LIST";
    case MSG_WIFI_SSID_SELECT: return "WIFI_SSID_SELECT";
    case MSG_BLOOD_STATUS: return "BLOOD_STATUS";
    case MSG_FACTORY_INIT: return "FACTORY_INIT";
    case MSG_DATA_BACKUP_REQUEST: return "MSG_DATA_BACKUP_REQUEST";
    case MSG_DATA_BACKUP_DATA: return "MSG_DATA_BACKUP_DATA";
    case MSG_DATA_BACKUP_ACK: return "MSG_DATA_BACKUP_ACK";
    case MSG_DATA_RESTORE_REQUEST: return "MSG_DATA_RESTORE_REQUEST";
    case MSG_DATA_RESTORE_DATA: return "MSG_DATA_RESTORE_DATA";
    case MSG_DATA_RESTORE_ACK: return "MSG_DATA_RESTORE_ACK";
    case MSG_DIAGNOSTIC_RESULT: return "MSG_DIAGNOSTIC_RESULT";
    default: return "UNKNOWN";
   }
}


// 메시지 수신 처리
void handle_received_data(uint8_t* data, size_t length) {
    //if (length < sizeof(MessageHeader)) return;    
    
    MessageHeader* header = (MessageHeader*)data;
    if (header->start_marker != 0xFF) {
        DEBUG_LOG("Invalid start marker : \n");
        Serial.println(header->start_marker);
        //return;
    }
    
   DEBUG_LOG("\n=== RECEIVED MESSAGE ===\n");
   decode_message_header(header);
   DEBUG_LOG("Hex dump:\n");
   print_hex_dump(data, length);
   DEBUG_LOG("======================\n");
      
    g_state.last_received_time = millis();
    process_message(data);
}


void print_hex_dump(const uint8_t* data, size_t length) {
    DEBUG_LOG("Hex dump: ");
    
    // 첫 줄 시작
    DEBUG_LOG("\n%03X: ", 0);
    
    for (size_t i = 0; i < length; i++) {
        // 16바이트마다 ASCII 출력 및 새 줄 시작
        if (i > 0 && i % 16 == 0) {
            DEBUG_LOG(" |");
            // ASCII 출력
            for (size_t j = i - 16; j < i; j++) {
                char c = data[j];
                DEBUG_LOG("%c", (c >= 32 && c <= 126) ? c : '.');
            }
            DEBUG_LOG("|\n%03X: ", i);
        }
        // 바이트 출력
        DEBUG_LOG("%02X ", data[i]);
    }
    
    // 마지막 줄 남은 공간 처리
    if (length % 16 != 0) {
        // 남은 공간 채우기
        for (size_t i = 0; i < (16 - (length % 16)); i++) {
            DEBUG_LOG("   ");
        }
        
        // ASCII 출력
        DEBUG_LOG(" |");
        for (size_t i = length - (length % 16); i < length; i++) {
            char c = data[i];
            DEBUG_LOG("%c", (c >= 32 && c <= 126) ? c : '.');
        }
        // 남은 ASCII 공간 채우기
        for (size_t i = 0; i < (16 - (length % 16)); i++) {
            DEBUG_LOG(" ");
        }
        DEBUG_LOG("|");
    }
    DEBUG_LOG("\n");
}


bool decode_event_message(EventMessage* msg, char* output, size_t output_size) {
  if (!msg || !output || output_size == 0) return false;

  char temp[32];
  const char* event_type_str = "Unknown";

  switch (msg->event_type) {
    case EVENT_SENSOR_COVER:
      event_type_str = "Cover Sensor";
      snprintf(temp, sizeof(temp), "State: %s",
               msg->data.digital.state ? "Closed" : "Open");
      break;

    case EVENT_SENSOR_FULLLEVEL:
      event_type_str = "Full Level";
      snprintf(temp, sizeof(temp), "State: %s",
               msg->data.digital.state ? "Full Level is Reached" : "Full Level is Not Reached");
      break;

    case EVENT_SERVER_CONNECT:
      event_type_str = "Server";
      snprintf(temp, sizeof(temp), "State: %s",
               msg->data.digital.state ? "Server is Connected" : "Server is Not Connected");
      break;

    case EVENT_SENSOR_URINE:
      event_type_str = "Urine Sensor";
      snprintf(temp, sizeof(temp), "State: %s",
               msg->data.digital.state ? "Detected" : "None");
      break;

    case EVENT_SENSOR_WEIGHT:
      event_type_str = "Weight Sensor";
      snprintf(temp, sizeof(temp), "Value: %.1f g",
               msg->data.analog.value);
      break;

    case EVENT_SENSOR_GAS:
      event_type_str = "Gas Sensor";
      snprintf(temp, sizeof(temp), "Value: %.1f ppm",
               msg->data.analog.value);
      break;

    case EVENT_SENSOR_DIAPER:
      event_type_str = "Diaper Sensor";
      snprintf(temp, sizeof(temp), "State: %s",
               msg->data.digital.state ? "In" : "Out");
      break;

    case EVENT_SENSOR_POWER:
      event_type_str = "Power Sensor";
      snprintf(temp, sizeof(temp), "State: %s",
               msg->data.digital.state ? "On" : "Off");
      break;

    case EVENT_SENSOR_SYSTEM:
      event_type_str = "System Sensor";
      snprintf(temp, sizeof(temp), "State: %s",
               msg->data.digital.state ? "Error" : "Nomal");
      break;

    case EVENT_SENSOR_BATTERY:
      event_type_str = "Battery Level";
      snprintf(temp, sizeof(temp), "Level: %.1f%%",
               msg->data.analog.value);
      break;

    case EVENT_BUTTON_PRESS:
    case EVENT_BUTTON_RELEASE:
      event_type_str = "Button Event";
      {
        const char* button_name =
          msg->data.button.button_id == BUTTON_SW1_PIN  ? "Motor Manual" : msg->data.button.button_id == BUTTON_SW2_PIN  ? "Motor Stop"
                                                                         : msg->data.button.button_id == BUTTON_SW3_PIN  ? "Blood Flow"
                                                                         : msg->data.button.button_id == BUTTON_SW4_PIN  ? "Urine Sound"
                                                                         : "Unknown";
        snprintf(temp, sizeof(temp), "%s: %s",
                 button_name,
                 msg->event_type == EVENT_BUTTON_PRESS ? "Pressed" : "Released");
      }
      break;

    case EVENT_ERROR:
      event_type_str = "Error Event";
      snprintf(temp, sizeof(temp), "Error 0x%02X: Detail 0x%02X",
               msg->data.error.error_code, msg->data.error.detail);
      break;

    default:
      snprintf(temp, sizeof(temp), "Unknown event type: 0x%02X",
               msg->event_type);
      break;
  }

  snprintf(output, output_size, "Event: %s, %s", event_type_str, temp);
  return true;
}




// 체크섬 계산
uint8_t calculate_checksum(void* message, size_t size) {
    uint8_t sum = 0;
    uint8_t* data = (uint8_t*)message;
    
    for (size_t i = 0; i < size; i++) {
        sum += data[i];
    }
    
    return sum;
}

// 체크섬 검증
bool verify_checksum(void* message, size_t size) {
    uint8_t* msg = (uint8_t*)message;
    uint8_t received_checksum = msg[size - 1];
    uint8_t calculated_checksum = calculate_checksum(message, size - 1);
    
    return (received_checksum == calculated_checksum);
}


#ifdef BOARD_TYPE_LCD

// LCD 디스플레이 업데이트 함수들
void update_system_status_display(SystemStatusMessage* msg) {
    DEBUG_LOG("LCD: Updating system status display\n");
    DEBUG_LOG("  HW Version: %s\n", msg->hw_version);
    DEBUG_LOG("  SW Version: %s\n", msg->sw_version);
    DEBUG_LOG("  Serial Number: %s\n", msg->serial_num);
    DEBUG_LOG("  System Status: 0x%02X\n", msg->sys_status);
}

void update_sensor_display(SensorDataMessage* msg) {
    DEBUG_LOG("LCD: Updating sensor display\n");
    DEBUG_LOG("  Cover: %s\n", msg->cover_sensor ? "Closed" : "Open");
    DEBUG_LOG("  Urine: %s\n", msg->urine_sensor ? "Detected" : "None");
    DEBUG_LOG("  Weight: %.1f g\n", msg->weight_sensor);
    DEBUG_LOG("  Gas: %.1f ppm\n", msg->gas_sensor);
    DEBUG_LOG("  Battery: %d%%\n", msg->battery_level);
}

void update_button_status_display(ButtonEventMessage* msg) {
  DEBUG_LOG("LCD: Updating button status display\n");
  DEBUG_LOG("  Button %d (%s): %s\n",
            msg->button_id,
            msg->button_id == BUTTON_SW4_PIN  ? "Urine Sound" : msg->button_id == BUTTON_SW1_PIN ? "Motor Manual"
                                                              : msg->button_id == BUTTON_SW2_PIN ? "Motor Stop"
                                                              : msg->button_id == BUTTON_SW3_PIN ? "Blood in Flow"
                                                                                           : "Unknown",
            msg->pressed ? "Pressed" : "Released");
}

void update_wifi_status_display(WifiStatusMessage* msg) {
  DEBUG_LOG("LCD: Updating WiFi status display\n");
  DEBUG_LOG("  Connected: %s\n", msg->connected ? "Yes" : "No");
  if (msg->connected) {
    DEBUG_LOG("  SSID: %s\n", msg->ssid);
    DEBUG_LOG("  IP: %s\n", msg->ip_addr);
    DEBUG_LOG("  Gateway: %s\n", msg->gateway);
    DEBUG_LOG("  Signal: %d%%\n", msg->signal_strength);
  }
}

void update_motor_status_display(MotorMessage* msg) {
    DEBUG_LOG("LCD: Updating motor status display\n");
    DEBUG_LOG("  Status: %s\n", 
              msg->motor_status == MOTOR_STATUS_STOP ? "Stopped" :
              msg->motor_status == MOTOR_STATUS_RUNNING ? "Running" :
              msg->motor_status == MOTOR_STATUS_ERROR ? "Error" : "Unknown");
    DEBUG_LOG("  Speed: %d%%\n", msg->motor_speed);
}

void toggle_menu_display() {
    static bool menu_visible = false;
    menu_visible = !menu_visible;
    
    if (menu_visible) {
        show_menu();
    } else {
        hide_menu();
    }
}

void show_menu() {
    DEBUG_LOG("LCD: Showing menu\n");
}

void hide_menu() {
    DEBUG_LOG("LCD: Hiding menu\n");
}
#endif

void print_message_type(uint8_t type) {
    const char* type_str = "Unknown";
    switch(type) {
        case MSG_HELLO:      type_str = "Hello"; break;
        case MSG_HELLO_ACK:  type_str = "Hello ACK"; break;
        case MSG_SYSTEM_STATUS: type_str = "System Status"; break;
        case MSG_SENSOR_DATA:   type_str = "Sensor Data"; break;
        case MSG_BUTTON_EVENT:  type_str = "Button Event"; break;
        case MSG_WIFI_STATUS:   type_str = "WiFi Status"; break;
        case MSG_TIME_SYNC:     type_str = "Time Sync"; break;
        case MSG_MOTOR_STATUS:  type_str = "Motor Status"; break;
        case MSG_REQUEST:       type_str = "Request"; break;
        case MSG_RESPONSE:      type_str = "Response"; break;
        case MSG_EVENT:      type_str = "Event"; break;
    }
    DEBUG_LOG("Message Type: %s (0x%02X)\n", type_str, type);
}


void print_message(void* message) {
  MessageHeader* header = (MessageHeader*)message;
  char timestamp[32];

  DEBUG_LOG("\n=== Message Information ===\n");
  print_message_type(header->type);
  DEBUG_LOG("Sequence: %d\n", header->seq_num);
  format_timestamp(header->timestamp, timestamp, sizeof(timestamp));
  DEBUG_LOG("Timestamp: %u\n", header->timestamp);
  DEBUG_LOG("Flags: 0x%02X", header->flags);
  if (header->flags & FLAG_SAVE_TO_SD) DEBUG_LOG(" [SD Save]");
  if (header->flags & FLAG_RESPONSE) DEBUG_LOG(" [Response]");
  if (header->flags & FLAG_ERROR) DEBUG_LOG(" [Error]");
  if (header->flags & FLAG_URGENT) DEBUG_LOG(" [Urgent]");
  DEBUG_LOG("\n");

  // 메시지 타입별 상세 정보 출력
  switch (header->type) {
    case MSG_HELLO:
    case MSG_HELLO_ACK:
      {
        HelloMessage* msg = (HelloMessage*)message;
        DEBUG_LOG("Board Type: %s\n",
                  msg->board_type == 1 ? "Main Board" : msg->board_type == 2 ? "LCD Board"
                                                                             : "Unknown");
        break;
      }

    case MSG_SYSTEM_STATUS:
      {
        SystemStatusMessage* msg = (SystemStatusMessage*)message;
        DEBUG_LOG("System Status Details:\n");
        DEBUG_LOG("  HW Version: %s\n", msg->hw_version);
        DEBUG_LOG("  SW Version: %s\n", msg->sw_version);
        DEBUG_LOG("  Serial Number: %s\n", msg->serial_num);
        DEBUG_LOG("  Status: %s (0x%02X)\n",
                  msg->sys_status == SYS_FLAG_NORMAL ? "Normal" : msg->sys_status == SYS_FLAG_ERROR    ? "Error"
                                                                : msg->sys_status == SYS_FLAG_WARNING  ? "Warning"
                                                                : msg->sys_status == SYS_FLAG_MAINTAIN ? "Maintenance"
                                                                                                       : "Unknown",
                  msg->sys_status);
        if (msg->error_code) {
          DEBUG_LOG("  Error Code: 0x%02X\n", msg->error_code);
        }
        break;
      }

    case MSG_SENSOR_DATA:
      {
        SensorDataMessage* msg = (SensorDataMessage*)message;
        DEBUG_LOG("Sensor Data:\n");
        DEBUG_LOG("  Cover Sensor: %s\n", msg->cover_sensor ? "Closed" : "Open");
        DEBUG_LOG("  Urine Sensor: %s\n", msg->urine_sensor ? "Detected" : "None");
        DEBUG_LOG("  Diaper Sensor: %s\n", msg->diaper_sensor ? "In" : "Out");
        DEBUG_LOG("  Full Level Sensor: %s\n", msg->fulllevel_sensor ? "Reached" : "Not Reached");
        DEBUG_LOG("  Weight: %.1f g\n", msg->weight_sensor);
        DEBUG_LOG("  Gas: %.1f ppm\n", msg->gas_sensor);
        DEBUG_LOG("  Battery: %d%%\n", msg->battery_level);
        break;
      }
#if 0
case MSG_BUTTON_EVENT:
{
    ButtonEventMessage* msg = (ButtonEventMessage*)message;
    const char* button_name = 
        msg->button_id == BUTTON_SW1_PIN ? "Motor Manual" :
        msg->button_id == BUTTON_SW2_PIN ? "Motor Stop" :
        msg->button_id == BUTTON_SW3_PIN ? "Blood Flow" :
        msg->button_id == BUTTON_SW4_PIN ? "Sound Control" :
                                          "Unknown";
    
    DEBUG_LOG("Button Event:\n");
    DEBUG_LOG("  Button: %s (ID: %d)\n", button_name, msg->button_id);
    
    if (msg->button_id == BUTTON_SW4_PIN && msg->pressed) {
        // SW4이고 pressed 상태일 때만 소리 상태 출력
        const char* sound_state = 
            msg->button_state == CMD_STOP_SOUND ? "Stop Sound" :
            msg->button_state == CMD_WATER_SOUND ? "Water Sound" :
            msg->button_state == CMD_URINE_SOUND ? "Urine Sound" :
            "Unknown State";
        DEBUG_LOG("  Sound State: %s\n", sound_state);
    } else {
        DEBUG_LOG("  Button State: %s\n", msg->pressed ? "Pressed" : "Released");
    }
    break;
}
#endif
    case MSG_WIFI_STATUS:
      {
        WifiStatusMessage* msg = (WifiStatusMessage*)message;
        DEBUG_LOG("WiFi Status:\n");
        DEBUG_LOG("  Connected: %s\n", msg->connected ? "Yes" : "No");
        if (msg->connected) {
          DEBUG_LOG("  SSID: %s\n", msg->ssid);
          DEBUG_LOG("  IP: %s\n", msg->ip_addr);
          DEBUG_LOG("  Gateway: %s\n", msg->gateway);
          DEBUG_LOG("  Signal Strength: %d%%\n", msg->signal_strength);
        }
        break;
      }
    case MSG_TIME_SYNC:
      {
        TimeSyncMessage* msg = (TimeSyncMessage*)message;
        char formatted_time[32];
        format_timestamp(msg->timestamp, formatted_time, sizeof(formatted_time));
        DEBUG_LOG("Time Sync Status:\n");
        DEBUG_LOG("  System Time: %s\n", formatted_time);
        DEBUG_LOG("  Unix Timestamp: %u\n", msg->timestamp);
        break;
      }
    case MSG_MOTOR_STATUS:
      {
        MotorMessage* msg = (MotorMessage*)message;
        const char* status_str =
          msg->motor_status == MOTOR_STATUS_STOP ? "Stopped" : msg->motor_status == MOTOR_STATUS_RUNNING ? "Running"
                                                             : msg->motor_status == MOTOR_STATUS_ERROR   ? "Error"
                                                                                                         : "Unknown";
        DEBUG_LOG("Motor Status:\n");
        DEBUG_LOG("  Status: %s (0x%02X)\n", status_str, msg->motor_status);
        DEBUG_LOG("  Speed: %d%%\n", msg->motor_speed);
        if (msg->motor_cmd) {
          const char* cmd_str =
            msg->motor_cmd == MOTOR_CMD_START ? "Start" : msg->motor_cmd == MOTOR_CMD_STOP   ? "Stop"
                                                        : msg->motor_cmd == MOTOR_CMD_MANUAL ? "Manual"
                                                        : msg->motor_cmd == MOTOR_CMD_AUTO   ? "Auto"
                                                                                             : "Unknown";
          DEBUG_LOG("  Command: %s (0x%02X)\n", cmd_str, msg->motor_cmd);
        }
        break;
      }

    case MSG_REQUEST:
      {
        RequestMessage* msg = (RequestMessage*)message;
        const char* req_type_str =
          msg->request_type == REQ_SYSTEM_STATUS ? "System Status" : msg->request_type == REQ_SENSOR_STATUS ? "Sensor Status"
                                                                   : msg->request_type == REQ_MOTOR_CONTROL ? "Motor Control"
                                                                   : msg->request_type == REQ_WIFI_STATUS   ? "WiFi Status"
                                                                                                            : "Unknown";
        DEBUG_LOG("Request:\n");
        DEBUG_LOG("  Type: %s (0x%02X)\n", req_type_str, msg->request_type);
        if (msg->request_param) {
          DEBUG_LOG("  Parameter: 0x%02X\n", msg->request_param);
        }
        break;
      }
    case MSG_EVENT:
      {
        EventMessage* msg = (EventMessage*)message;
        char event_desc[128];
        if (decode_event_message(msg, event_desc, sizeof(event_desc))) {
          DEBUG_LOG("Event Message:\n");
          DEBUG_LOG("  %s\n", event_desc);
        }
        break;
      }
  }
  DEBUG_LOG("===================================================\n\n");
}

#ifdef BOARD_TYPE_LCD
// 알림 표시 함수
void show_notification(const char* message) {
    DEBUG_LOG("Notification: %s\n", message);
    // 실제 LCD 구현 시 팝업이나 알림 표시
}

// 경고 팝업 표시
void show_warning_popup(const char* title, const char* message) {
    DEBUG_LOG("Warning - %s: %s\n", title, message);
    // 실제 LCD 구현 시 경고 팝업 표시
}

// 에러 화면 표시
void show_error_screen(uint8_t error_code, const char* message) {
    DEBUG_LOG("Error Screen - Code 0x%02X: %s\n", error_code, message);
    // 실제 LCD 구현 시 에러 화면 표시
}

#if 0
// 에러 메시지 획득
const char* get_error_message(uint8_t error_code, uint8_t detail) {
    switch (error_code) {
        case 0x01: return "WiFi 연결 실패";
        case 0x02: return "모터 동작 오류";
        case 0x03: return "시스템 상태 이상";
        case 0x04: return "센서 오류";
        default: return "알 수 없는 오류";
    }
}
#endif

// 심각한 에러 여부 확인
bool is_critical_error(uint8_t error_code) {
    return (error_code == 0x02 || error_code == 0x03);  // 모터나 시스템 오류
}

// 경고음 재생
void play_error_sound() {
    DEBUG_LOG("Playing error sound\n");
    // 실제 하드웨어에서 부저 등으로 구현
}

// 각종 디스플레이 업데이트 함수들
void update_cover_sensor_display(bool state) {
    DEBUG_LOG("Updating cover sensor display: %s\n", state ? "Closed" : "Open");
}

void update_urine_sensor_display(bool state) {
    DEBUG_LOG("Updating urine sensor display: %s\n", state ? "Detected" : "None");
}

void update_weight_display(float weight) {
    DEBUG_LOG("Updating weight display: %.1f g\n", weight);
}

void update_gas_display(float level) {
    DEBUG_LOG("Updating gas display: %.1f ppm\n", level);
}

void update_battery_display(float level) {
    DEBUG_LOG("Updating battery display: %.1f%%\n", level);
}

void update_button_status_display(uint8_t button_id, bool pressed) {
    DEBUG_LOG("Updating button display - ID: %d, State: %s\n", 
             button_id, pressed ? "Pressed" : "Released");
}
#endif

// 타임스탬프 출력 함수
void print_timestamp(uint32_t timestamp) {
    char datetime[32];
    format_timestamp(timestamp, datetime, sizeof(datetime));
    DEBUG_LOG("Timestamp: %s\n", datetime);
}

// 타임스탬프 포맷팅 함수
char* format_timestamp(uint32_t timestamp, char* buffer, size_t size) {
    time_t time = timestamp;
    struct tm* timeinfo = localtime(&time);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", timeinfo);
    return buffer;
}


// Hello 메시지 전송
void send_hello() {
    HelloMessage msg = {};
    msg.header.start_marker = 0xFF;
    msg.header.type = MSG_HELLO;
    msg.header.timestamp = time(NULL);
    msg.header.seq_num = g_state.sequence_number++;
    msg.header.length = sizeof(HelloMessage) - sizeof(MessageHeader);
    msg.board_type = g_state.board_type;
    msg.checksum = calculate_checksum(&msg, sizeof(HelloMessage) - 1);
    
    send_message(&msg, sizeof(HelloMessage));
}


// 에러 메시지 획득
const char* get_error_message(uint8_t error_code, const char* detail) {
  static char error_message[128];  // 충분한 크기의 버퍼

  switch (error_code) {
    case WIFI_ERROR_AUTH_FAILED:
      snprintf(error_message, sizeof(error_message),
               "Authentication Failed - %s", detail);
      break;
    case WIFI_ERROR_NO_AP_FOUND:
      snprintf(error_message, sizeof(error_message),
               "AP Not Found - %s", detail);
      break;
    case WIFI_ERROR_CONNECT_TIMEOUT:
      snprintf(error_message, sizeof(error_message),
               "Connection Timeout - %s", detail);
      break;
    case WIFI_ERROR_LOST_CONN:
      snprintf(error_message, sizeof(error_message),
               "Connection Lost - %s", detail);
      break;
    case WIFI_ERROR_SIGNAL_LOST:
      snprintf(error_message, sizeof(error_message),
               "Signal Lost - %s", detail);
      break;
    default:
      snprintf(error_message, sizeof(error_message),
               "Unknown Error (0x%02X) - %s", error_code, detail);
      break;
  }

  return error_message;
}

void show_system_error(uint8_t error_code) {
  // 시스템 에러 표시
  DEBUG_LOG("Display: System Error - Code: 0x%02X\n", error_code);
}

void update_display_time(uint32_t timestamp) {
  char time_str[32];
  time_t t = timestamp;
  struct tm* timeinfo = localtime(&t);
  strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
  DEBUG_LOG("+++ Display time updated: %s\n", time_str);
  // 실제 LCD 업데이트 코드 추가
}

// SD 카드 저장
/*
bool save_to_sd(const char* filename, void* data, size_t size) {

  //return true;
  
    if (!check_sd_card()) {
        DEBUG_LOG("SD Card not available\n");
        return false;
    }
  
  
    
    FsFile file = SD.open(filename, FILE_APPEND);
    if (!file) {
        DEBUG_LOG("Failed to open file: %s\n", filename);
        return false;
    }
    
    // 타임스탬프 추가
    MessageHeader* header = (MessageHeader*)data;
    char timestamp[32];
    format_timestamp(header->timestamp, timestamp, sizeof(timestamp));
    file.printf("[%s] ", timestamp);

        // 데이터 저장
    size_t written = file.write((uint8_t*)data, size);
    file.println();
    file.close();
    
    DEBUG_LOG("Saved to SD: %s (%d bytes) at %s\n", 
              filename, written, timestamp);
    return (written == size);
    
}
*/


// 백업 확인 응답 인코딩 (LCD -> Main)
void encode_backup_ack(uint16_t chunk_index, uint8_t status) {
    BackupAckMessage msg = {};
    
    msg.header.start_marker = 0xFF;
    msg.header.type = MSG_DATA_BACKUP_ACK;
    msg.header.timestamp = time(NULL);
    msg.header.seq_num = g_state.sequence_number++;
    msg.header.length = sizeof(BackupAckMessage) - sizeof(MessageHeader);
    
    msg.chunk_index = chunk_index;
    msg.status = status;
    msg.checksum = calculate_checksum(&msg, sizeof(BackupAckMessage) - 1);
    
    send_message(&msg, sizeof(BackupAckMessage));
}



// 메시지 처리
void process_message(void* message) {    
        
    ///////////////////////////////////////////////////////////  

    if (power_ON == 0) {
          Event_power_ON();
          Event_bat_LOW();
          //Event_tank_ON(2100);
          //vTaskDelay(pdMS_TO_TICKS(3000));
          //Event_tank_ON(1500);
          //vTaskDelay(pdMS_TO_TICKS(3000));
          //Event_tank_ON(800);
          power_ON = 1;          
        }
    /*
    if (power_ON == 1) {
          Event_power_OFF();
          power_ON = 0;
        }
    */

  MessageHeader* header = (MessageHeader*)message;

  if (!verify_checksum(message, header->length + sizeof(MessageHeader))) {
    DEBUG_LOG("Checksum verification failed\n");
    return;
  }

  time_t t = header->timestamp;
  strftime(timestamp_display, LENGTH_TIMESTAMP, "%Y-%m-%d %H:%M:%S", localtime(&t));
  DEBUG_LOG("+++ timestamp_display : %s\n", timestamp_display);

  DEBUG_LOG("Received ");
  print_message(message);

  switch (header->type) {

    case MSG_WIFI_SCAN_LIST: {
          WifiScanListMessage* msg = (WifiScanListMessage*)message;
          DEBUG_LOG("Received WiFi Scan List: %d networks\n", msg->ap_count);

          // 수신된 SSID 리스트를 UI에 표시
          for (int i = 0; i < msg->ap_count; i++) {
              DEBUG_LOG("  +++ Network %d: SSID: %s, RSSI: %d\n", 
                        i, msg->ap_list[i].ssid, msg->ap_list[i].rssi);
              // 리스트 추가
              /*
              lv_obj_t* btn = lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, msg->ap_list[i].ssid);
              lv_obj_set_user_data(btn, (void*)&msg->ap_list[i]);
              lv_obj_add_event_cb(btn, wifi_network_selected, LV_EVENT_CLICKED, NULL);
              lv_obj_set_style_anim_time(btn, 0, LV_STATE_PRESSED);  // 애니메이션 제거
              */
              strncpy(available_networks[i].ssid, msg->ap_list[i].ssid, sizeof(available_networks[i].ssid) - 1);
              //strncpy(available_networks[i].ssid, msg->ssid, sizeof(available_networks[i].ssid) - 1);
              available_networks[i].ssid[sizeof(available_networks[i].ssid) - 1] = '\0';
              Serial.printf("+++ Network %d: SSID 1: %s\n", i, available_networks[i].ssid);
          }
          break;
      }

    case MSG_HELLO:
      {
        HelloMessage* msg = (HelloMessage*)message;
        g_state.peer_connected = true;

        // Hello ACK 응답
        HelloMessage response = { 0 };
        response.header.start_marker = 0xFF;
        response.header.type = MSG_HELLO_ACK;
        response.header.timestamp = time(NULL);
        response.header.seq_num = g_state.sequence_number++;
        response.header.length = sizeof(HelloMessage) - sizeof(MessageHeader);
        response.board_type = g_state.board_type;
        response.checksum = calculate_checksum(&response, sizeof(HelloMessage) - 1);

        send_message(&response, sizeof(HelloMessage));

        DEBUG_LOG("Received Hello from board type %d\n", msg->board_type);
        break;
      }

    case MSG_HELLO_ACK:
      {
        HelloMessage* msg = (HelloMessage*)message;
        g_state.peer_connected = true;
        DEBUG_LOG("Received Hello ACK from board type %d\n", msg->board_type);
        break;
      }
    case MSG_REQUEST:
      {
        RequestMessage* msg = (RequestMessage*)message;
        DEBUG_LOG("Received Request:\n");
        DEBUG_LOG("  Type: 0x%02X\n", msg->request_type);
        DEBUG_LOG("  Param: 0x%02X\n", msg->request_param);

        Serial.println("+++ BEFORE save_to_sd requests.log..");
        if (msg->header.flags & FLAG_SAVE_TO_SD) {
          //save_to_sd("requests.log", msg, sizeof(RequestMessage));
        }
        break;
      }
    case MSG_WIFI_SSID_SELECT:
      {
        WifiSelectMessage* msg = (WifiSelectMessage*)message;
        DEBUG_LOG("Received WiFi selection: %s\n", msg->ssid);


        break;
      }
    case MSG_BUTTON_EVENT:
      {
        ButtonEventMessage* msg = (ButtonEventMessage*)message;
        DEBUG_LOG("+++ Received Button Event:\n");
        DEBUG_LOG("  Button %d: %s\n", msg->button_id,
                  msg->pressed ? "Pressed" : "Released");
        if (msg->button_id == 3)
        {          
          Serial.println("+++ BEFORE save_to_sd button.log..");
          //save_to_sd("/button.log", msg, sizeof(ButtonEventMessage));
          char * AA = "WK Test 250116";
          //save_to_sd("/button.log", AA, sizeof(AA));
          hematuria_client_event_handler();

        }
        if (msg->button_id == 4)
        { 
          Serial.println("+++ BEFORE mp3play for msg->button_id == 4");

          //if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {        
                //mp3play("001");     
                    play_urination_sound(REPEAT_INFINITE, 0);     // repeat infinite
                //xSemaphoreGive(sdMutex);
          //}       
        }

        if (msg->header.flags & FLAG_SAVE_TO_SD) {
          //save_to_sd("button.log", msg, sizeof(ButtonEventMessage));
        }

        break;
      }
    case MSG_SYSTEM_STATUS:
      {
        SystemStatusMessage* msg = (SystemStatusMessage*)message;
        DEBUG_LOG("System Status Update:\n");
        DEBUG_LOG("  HW Version: %s\n", msg->hw_version);
        DEBUG_LOG("  SW Version: %s\n", msg->sw_version);
        DEBUG_LOG("  Serial Num: %s\n", msg->serial_num);
        DEBUG_LOG("  Status: 0x%02X\n", msg->sys_status);

        // LCD 화면 업데이트
        update_system_status_display(msg);

        // 시스템 상태가 에러인 경우
        if (msg->sys_status == SYS_FLAG_ERROR) {
          show_system_error(msg->error_code);
        }

        // SD 카드 저장
        if (msg->header.flags & FLAG_SAVE_TO_SD) {
          //save_to_sd("/system/status.log", msg, sizeof(SystemStatusMessage));
        }
        break;
      }
    case MSG_SENSOR_DATA:
      {
        SensorDataMessage* msg = (SensorDataMessage*)message;
        DEBUG_LOG("Received Sensor Data:\n");
        DEBUG_LOG("  Cover: %s\n", msg->cover_sensor ? "Closed" : "Open");
        DEBUG_LOG("  Urine: %s\n", msg->urine_sensor ? "Detected" : "None");
        DEBUG_LOG("  Weight: %.1f g\n", msg->weight_sensor);
        DEBUG_LOG("  Gas: %.1f ppm\n", msg->gas_sensor);
        DEBUG_LOG("  Battery: %d%%\n", msg->battery_level);

      ////////////////////////////////

      //SensorDataMessage* msg = (SensorDataMessage*)message;
        DEBUG_LOG("Received Sensor Data:\n");
        DEBUG_LOG("  Cover: %s\n", msg->cover_sensor ? "Closed" : "Open");
        DEBUG_LOG("  Urine: %s\n", msg->urine_sensor ? "Detected" : "None");
        DEBUG_LOG("  Diaper Sensor: %s\n", msg->diaper_sensor ? "In" : "Out");
        DEBUG_LOG("  Gas Sensor: %s\n", msg->gas_sensor ? "Detected" : "None");
        DEBUG_LOG("  Full Level Sensor: %s\n", msg->fulllevel_sensor ? "Reached" : "Not Reached");
        DEBUG_LOG("  Weight: %.1f g\n", msg->weight_sensor);
        DEBUG_LOG("  System Error: %s\n", msg->system_status ? "Error" : "Normal");
        DEBUG_LOG("  Power: %s\n", msg->power_status ? "On" : "Off");
        DEBUG_LOG("  Battery: %d%%\n", msg->battery_level);
      ////////////////////////////////




        //update_sensor_display(msg);
        Serial.println("+++ msg->cover_sensor:");
          Serial.println(msg->cover_sensor); 

          if (msg->cover_sensor == 0)
          {
            cover_open_event_handler();
          }  
#if 0
        if (msg->header.flags & FLAG_SAVE_TO_SD) {
          char filename[32];
          time_t t = msg->header.timestamp;
          struct tm* timeinfo = localtime(&t);
          strftime(filename, sizeof(filename), "/sensor_%Y%m%d.log", timeinfo);
          
          
    ////////////////////////////////////////////////////////////

          //save_to_sd(filename, msg, sizeof(SensorDataMessage));

    //const char* filename, void* data, size_t size

    size_t size = sizeof(SensorDataMessage);
    void* data;

    
    // 타임스탬프 추가
    MessageHeader* header = (MessageHeader*)data;
    char timestamp[32];
    format_timestamp(header->timestamp, timestamp, sizeof(timestamp));
    file.printf("[%s] ", timestamp);

        // 데이터 저장
    size_t written = file.write((uint8_t*)data, size);
    file.println();
    file.close();
    
    DEBUG_LOG("Saved to SD: %s (%d bytes) at %s\n", 
              filename, written, timestamp);
    written == size;

    ////////////////////////////////////////////////////////////

        }
#endif
        break;
      }

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
    
    // BME 센서 데이터
    DEBUG_LOG("\n[Environmental Data]\n");
    DEBUG_LOG("  Temperature: %.1fKohm\n", msg->gas);
    DEBUG_LOG("  Temperature: %.1f°C\n", msg->temperature);
    DEBUG_LOG("  Humidity: %.1f%%\n", msg->humidity);
    DEBUG_LOG("  Pressure: %.1fhPa\n", msg->pressure);
    
    // 중량 센서 데이터
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
    for(int i = 0; i < 8; i++) {
        if(msg->test_results[i] != 0) {
            DEBUG_LOG("  Test %d Result: 0x%02X\n", i, msg->test_results[i]);
        }
    }
    
    DEBUG_LOG("\n===================================\n");
    break;
  }


    case MSG_WIFI_STATUS:
      {
        WifiStatusMessage* msg = (WifiStatusMessage*)message;
        DEBUG_LOG("Received WiFi Status:\n");
        DEBUG_LOG("  Connected: %s\n", msg->connected ? "Yes" : "No");
		Serial.println("msg->connected : ");
		Serial.println(msg->connected);

    Serial.printf("wifi_ON: %d \n", wifi_ON);

		
        //if (msg->connected == 1 && wifi_ON == 0) {
        if (msg->connected == 1) {

            Event_wifi_ON();
            wifi_ON = 1;
          DEBUG_LOG("  +++ SSID: %s\n", msg->ssid);
          ssid_main = msg->ssid;
          DEBUG_LOG("  IP: %s\n", msg->ip_addr);
          ip_addr_main = msg->ip_addr;
          DEBUG_LOG("  Gateway: %s\n", msg->gateway);
          gateway_main = msg->gateway;
          DEBUG_LOG("  Signal: %d%%\n", msg->signal_strength);
        }
		
		//if (msg->connected == false && wifi_ON == 1)
    if (msg->connected == 0)
        {
          Event_wifi_OFF();
          wifi_ON = 0;
        }
		

        //update_wifi_status_display(msg);
        if (msg->header.flags & FLAG_SAVE_TO_SD) {
          //save_to_sd("wifi.log", msg, sizeof(WifiStatusMessage));
        }
        break;
      }
    case MSG_MOTOR_STATUS:
      {
        MotorMessage* msg = (MotorMessage*)message;
        const char* status_str;

        switch (msg->motor_status) {
          case MOTOR_STATUS_STOP:
            status_str = "Stopped";
              Serial.println("+++ case MOTOR_STATUS_STOP, motor_ON:");
              Serial.println(motor_ON);
              //if (motor_ON == 1)
              //      {
                      Event_motor_OFF();
              //        motor_ON = 0;
              //      }
                    break;

          case MOTOR_STATUS_RUNNING:
            status_str = "Running";
              Serial.println("+++ case MOTOR_STATUS_RUNNING, motor_ON:");
            
                      Event_motor_ON();
              
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

        update_motor_status_display(msg);
        if (msg->header.flags & FLAG_SAVE_TO_SD) {
          //save_to_sd("motor.log", msg, sizeof(MotorMessage));
        }

        break;
      }
    case MSG_TIME_SYNC:
      {
        TimeSyncMessage* msg = (TimeSyncMessage*)message;
        char time_str[32];
        time_t t = msg->timestamp;
        struct tm* timeinfo = localtime(&t);
        DEBUG_LOG("+++ Time Sync Received:\n");
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
        DEBUG_LOG("  New Time: %s\n", time_str);

        // LCD에 시간 표시 업데이트
        update_display_time(msg->timestamp);
        break;
      }

///////////////////// Event Log SD 카드 저장 /////////////////////////

/*
    case MSG_DATA_BACKUP_REQUEST: 
      {
        BackupRequestMessage* msg = (BackupRequestMessage*)message;
        //if (length < sizeof(BackupRequestMessage)) return;
    
        DEBUG_LOG("Backup request received - Total: %u bytes, Chunks: %u\n", msg->total_size, msg->total_chunks);
              
        // 백업 파일 생성
        if (!start_backup_file()) {
            encode_backup_ack(0, DATA_FLAG_ERROR);
            return;
        }
    
        // ACK 전송
        encode_backup_ack(0, 0);
        break;
      }
    case MSG_DATA_BACKUP_DATA:
      {
        BackupDataMessage* msg = (BackupDataMessage*)message;
        //if (length < sizeof(BackupDataMessage)) return;
    
        // 데이터를 하나의 파일에 추가
        if (!append_backup_data(msg->data, msg->chunk_size)) {
            encode_backup_ack(msg->chunk_index, DATA_FLAG_ERROR);
            backup_file.close();
            return;
        }
    
        // ACK 전송
        encode_backup_ack(msg->chunk_index, 0);
    
        // 마지막 청크인 경우 파일 닫기
        if (msg->flags & DATA_FLAG_END) {
            finish_backup_file();
            DEBUG_LOG("Backup completed\n");
        }
        break;
      }
    case MSG_DATA_RESTORE_REQUEST:
     {
        DEBUG_LOG("Restore request received\n");
        
        // 최신 백업 파일 찾기
        File root = SD.open("/backup");
        File entry;
        char latest_backup[64] = {};
        time_t latest_time = 0;
        
        while((entry = root.openNextFile())) {
            if(!entry.isDirectory()) {
                const char* fname = entry.name();
                // mqtt_backup_YYYYMMDD_HHMMSS.dat 형식 확인
                if(strncmp(fname, "mqtt_backup_", 11) == 0) {
                    time_t file_time = parse_backup_timestamp(fname);
                    if(file_time > latest_time) {
                        latest_time = file_time;
                        strncpy(latest_backup, fname, sizeof(latest_backup)-1);
                    }
                }
            }
            entry.close();
        }
        root.close();

        if(latest_backup[0] == 0) {
            DEBUG_LOG("No backup file found\n");
            return;
        }

        // 백업 파일 열기
        char full_path[128];
        snprintf(full_path, sizeof(full_path), "/backup/%s", latest_backup);
        File backup_file = SD.open(full_path, FILE_READ);
        if(!backup_file) {
            DEBUG_LOG("Failed to open backup file\n");
            return;
        }

        // 파일 정보로 응답 전송
        BackupRequestMessage resp = {};
        resp.header.start_marker = 0xFF;
        resp.header.type = MSG_DATA_RESTORE_DATA;
        resp.header.timestamp = time(NULL);
        resp.header.seq_num = g_state.sequence_number++;
        resp.header.length = sizeof(BackupRequestMessage) - sizeof(MessageHeader);
        
        resp.total_size = backup_file.size();
        resp.chunk_size = BACKUP_CHUNK_SIZE;
        resp.total_chunks = (resp.total_size + BACKUP_CHUNK_SIZE - 1) / BACKUP_CHUNK_SIZE;
        resp.flags = DATA_FLAG_START;
        resp.checksum = calculate_checksum(&resp, sizeof(BackupRequestMessage) - 1);
        
        send_message(&resp, sizeof(BackupRequestMessage));
        DEBUG_LOG("Sending backup file: %s (%u bytes)\n", latest_backup, resp.total_size);

        // 청크 단위로 데이터 전송
        uint8_t buffer[BACKUP_CHUNK_SIZE];
        uint16_t chunk_index = 0;
        bool transfer_success = true;

        while(backup_file.available() && transfer_success) {
            // ACK 대기
            unsigned long start_time = millis();
            bool got_ack = false;
            
            while(millis() - start_time < 1000) {
                if(check_restore_ack(chunk_index)) {
                    got_ack = true;
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(10));
            }

            if(!got_ack) {
                DEBUG_LOG("No ACK for chunk %u\n", chunk_index);
                transfer_success = false;
                break;
            }

            // 데이터 읽고 전송
            size_t bytes_read = backup_file.read(buffer, BACKUP_CHUNK_SIZE);
            if(bytes_read == 0) break;

            bool is_last = !backup_file.available();
            encode_restore_data(chunk_index, buffer, bytes_read, is_last);
            
            DEBUG_LOG("Sent chunk %u/%u (%u bytes)%s", chunk_index + 1, resp.total_chunks, bytes_read, is_last ? " (Last)" : "");
            
            chunk_index++;
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        backup_file.close();
        
        if(transfer_success) {
            DEBUG_LOG(Restore data transfer completed\n");
        } else {
            DEBUG_LOG("Restore transfer failed\n");
        }
        break;
    }


    */

  ///////////////////// Event Log SD 카드 저장 /////////////////////////



    case MSG_EVENT:
      {
        EventMessage* msg = (EventMessage*)message;
        char event_desc[128];
        char timestamp[32];
        char log_filename[64];
        struct tm* timeinfo = localtime((time_t*)&msg->header.timestamp);
        strftime(log_filename, sizeof(log_filename), "/log/%Y%m%d_events.log", timeinfo);

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

              // LCD 화면 업데이트
              //update_cover_sensor_display(cover_state);
              Serial.printf("+++ cover_state: %d \n", cover_state);
              Serial.printf("+++ cover_OPEN: %d \n", cover_OPEN);

              /*
              if (cover_state == 0 && cover_OPEN == 0)
              {
                cover_open_event_handler();
                cover_OPEN = 1;
              }
              */  

              if (cover_state == 0)
              {
                cover_open_event_handler();
              }  
              if (cover_state == 1)
              {
                //lv_demo_widgets();
                lvgl_update_app_ui();
              }  
        

              // SD 카드 저장
              if (msg->header.flags & FLAG_SAVE_TO_SD) {
                char log_path[32];
                snprintf(log_path, sizeof(log_path), "/sensor/cover_%Y%m%d.log");
                //save_to_sd(log_path, msg, sizeof(EventMessage));
              }
              break;
            }

          case EVENT_SENSOR_DIAPER:
            {
              bool diaper_state = msg->data.digital.state;
              DEBUG_LOG("[%s] Diaper Sensor Update: %s\n",
                        timestamp, diaper_state ? "In" : "Out");

              // LCD 화면 업데이트
              //update_diaper_sensor_display(diaper_state);

              if (diaper_state == 1 && diaper_ON == 0)
              {
                Event_diaper_ON();
                diaper_ON = 1;
              }
              if (diaper_state == 0 && diaper_ON == 1)
              {
                Event_diaper_OFF();
                diaper_ON = 0;
              }

              // SD 카드 저장
              if (msg->header.flags & FLAG_SAVE_TO_SD) {
                char log_path[32];
                snprintf(log_path, sizeof(log_path), "/sensor/diaper_%Y%m%d.log");
                //save_to_sd(log_path, msg, sizeof(EventMessage));
              }
              break;
            }

          case EVENT_SENSOR_POWER:
            {
              bool power_state = msg->data.digital.state;
              DEBUG_LOG("[%s] Power Sensor Update: %s\n",
                        timestamp, power_state ? "On" : "Off");

              // LCD 화면 업데이트
              //update_power_sensor_display(power_state);

              if (power_state == 1 && power_ON == 0)
              {
                Event_power_ON();
                power_ON = 1;
              }
              if (power_state == 0 && power_ON == 1)
              {
                Event_power_OFF();
                power_ON = 0;
              }

              // SD 카드 저장
              if (msg->header.flags & FLAG_SAVE_TO_SD) {
                char log_path[32];
                snprintf(log_path, sizeof(log_path), "/sensor/power_%Y%m%d.log");
              //  save_to_sd(log_path, msg, sizeof(EventMessage));
              }
              break;
            }
            case EVENT_SENSOR_SYSTEM:
            {
              uint8_t system_state = msg->data.digital.state;
              DEBUG_LOG("[%s] System Sensor Update: %s(0x%02x)\n",
                        timestamp, system_state ? "Error" : "Normal", system_state);

              // LCD 화면 업데이트
              //update_power_sensor_display(system_state);

              Serial.printf("+++ system_state : %d, error_ON : %d \n", system_state, error_ON);

              // if (system_state == 1 && error_ON == 0)
              // {
              //   Event_error_ON();
              //   error_ON = 1;
              // }
              if (system_state == 0 && error_ON == 1)
              {
                Event_error_OFF();
                error_ON = 0;
              }
              else
              {
                Event_error_ON();
                error_ON = 1;
              }



              // SD 카드 저장
              if (msg->header.flags & FLAG_SAVE_TO_SD) {
                char log_path[32];
                snprintf(log_path, sizeof(log_path), "/sensor/system_%Y%m%d.log");
              //  save_to_sd(log_path, msg, sizeof(EventMessage));
              }
              break;
            }




          case EVENT_SENSOR_FULLLEVEL:
            {
              bool fulllevel_state = msg->data.digital.state;
              DEBUG_LOG("[%s] Full Level Sensor Update: %s\n",
                        timestamp, fulllevel_state ? "Full Level is Reached" : "Full Level is Not Reached");

              // LCD 화면 업데이트
              //update_full_level_display(fulllevel_state);

              if (fulllevel_state == 1 && fulllevel_ON == 0)
              {
                fulllevel_event_handler();
                fulllevel_ON = 1;
              }
              if (fulllevel_state == 0 && fulllevel_ON == 1)
              {
                //Event_diaper_OFF();
                fulllevel_ON = 0;
              }

              // SD 카드 저장
              if (msg->header.flags & FLAG_SAVE_TO_SD) {
                char log_path[32];
                snprintf(log_path, sizeof(log_path), "/sensor/fulllevel_%Y%m%d.log");
                //save_to_sd(log_path, msg, sizeof(EventMessage));
              }
              break;
            }
          
          // 서버 연결 여부
          case EVENT_SERVER_CONNECT:
            {
              bool server_state = msg->data.digital.state;
              DEBUG_LOG("[%s] Server Connection Update: %s\n",
                        timestamp, server_state ? "Server is Connected" : "Server is Not Connected");

              // LCD 화면 업데이트
              //update_full_level_display(server_state);

              if (server_state == 1 && connect_ON == 0)
              {
                Event_connect_ON();
                connect_ON = 1;
              }
              if (server_state == 0 && connect_ON == 1)
              {
                Event_connect_OFF();
                connect_ON = 0;
              }

              // SD 카드 저장
              if (msg->header.flags & FLAG_SAVE_TO_SD) {
                char log_path[32];
                snprintf(log_path, sizeof(log_path), "/sensor/server_%Y%m%d.log");
                //save_to_sd(log_path, msg, sizeof(EventMessage));
              }
              break;
            }

          // 배뇨 센서 이벤트
          case EVENT_SENSOR_URINE:
            {
              bool urine_detected = msg->data.digital.state;
              DEBUG_LOG("[%s] Urine Sensor Update: %s\n",
                        timestamp, urine_detected ? "Detected" : "None");

              // LCD 화면 업데이트
              //update_urine_sensor_display(urine_detected);

              // 배뇨 감지 시 알림 표시
              //if (urine_detected) {
                //show_notification("배뇨가 감지되었습니다");
              //}

              // if (urine_detected == 1 && urine_ON == 0)
              // {
              //   Event_feces_ON();
              //   urine_ON = 1;
              // }
              // if (urine_detected == 0 && urine_ON == 1)
              // {
              //   Event_feces_OFF();
              //   urine_ON = 0;
              // }

              // SD 카드 저장
              if (msg->header.flags & FLAG_SAVE_TO_SD) {
                //save_to_sd("/sensor/urine.log", msg, sizeof(EventMessage));
              }
              break;
            }
            // 무게 센서 이벤트
          case EVENT_SENSOR_WEIGHT:
            {
              float weight = msg->data.analog.value;
              DEBUG_LOG("[%s] Weight Sensor Update: %.1f g\n", timestamp, weight);

              // LCD 화면 업데이트
              //update_weight_display(weight);
              Event_tank_ON(weight);

              // 임계값 체크 및 알림
              if (weight > WEIGHT_THRESHOLD_HIGH) {
                //show_notification("무게가 너무 높습니다");
                fulllevel_event_handler();
              }

              // SD 카드 저장
              if (msg->header.flags & FLAG_SAVE_TO_SD) {
                //save_to_sd("/sensor/weight.log", msg, sizeof(EventMessage));
              }
              break;
            }
          // 가스 센서 이벤트
          case EVENT_SENSOR_GAS:
            {
              float gas_level = msg->data.analog.value;
              bool gas_detected = msg->data.digital.state;

              DEBUG_LOG("[%s] Gas Sensor Update: %.1f ppm\n", timestamp, gas_level);

              bool urine_detected = msg->data.digital.state;
              DEBUG_LOG("[%s] Gas Sensor Update: %s\n",
                        timestamp, gas_detected ? "Detected" : "None");

              if (gas_detected == 1)
              {
                Event_feces_ON();
                urine_ON = 1;
              }
              if (gas_detected == 0)
              {
                Event_feces_OFF();
                urine_ON = 0;
              }

              // LCD 화면 업데이트
              //update_gas_display(gas_level);

              // 가스 레벨 경고
              //if (gas_level > GAS_WARNING_LEVEL) {
              //  //show_warning_popup("가스 농도 경고", "가스 농도가 높습니다");
              //}

              // if (gas_level >= GAS_WARNING_LEVEL && urine_ON == 0)
              // {
              //   Event_feces_ON();
              //   urine_ON = 1;
              // }
              // if (gas_level < GAS_WARNING_LEVEL && urine_ON == 1)
              // {
              //   Event_feces_OFF();
              //   urine_ON = 0;
              // }

              // SD 카드 저장
              if (msg->header.flags & FLAG_SAVE_TO_SD) {
                //save_to_sd("/sensor/gas.log", msg, sizeof(EventMessage));
              }
              break;
            }
          // 배터리 레벨 이벤트
          case EVENT_SENSOR_BATTERY:
            {
              float battery_level = msg->data.analog.value;
              DEBUG_LOG("[%s] Battery Level Update: %.1f%%\n", timestamp, battery_level);

              // LCD 화면 업데이트
              //update_battery_display(battery_level);
              Event_bat_ON();

              // 배터리 부족 경고
              if (battery_level < BATTERY_LOW_THRESHOLD) {
                //show_warning_popup("배터리 부족", "배터리를 충전해주세요");
                Event_bat_LOW();
              }

              // SD 카드 저장 (배터리 변화가 큰 경우만)
              if (msg->header.flags & FLAG_SAVE_TO_SD) {
                //save_to_sd("/system/battery.log", msg, sizeof(EventMessage));
              }
              break;
            }
          case EVENT_BUTTON_PRESS:  // 버튼 이벤트
          case EVENT_BUTTON_RELEASE:
            {
              const char* button_name;
              switch (msg->data.button.button_id) {
                case BUTTON_SW1_PIN :
                  button_name = "Motor Manual";
                  break;
                case BUTTON_SW2_PIN :
                  button_name = "Motor Stop";
                  break;
                case BUTTON_SW3_PIN :
                  button_name = "Blood In Flow";
                  break;
                case BUTTON_SW4_PIN :
                  button_name = "Urine Sound";
                  break;
                default:
                  button_name = "Unknown";
                  break;
              }
              DEBUG_LOG("Button Event:\n");
              DEBUG_LOG("  Button: %s (ID: %d)\n", button_name, msg->data.button.button_id);

              if (button_name == "Blood In Flow")
              {          
                Serial.println("+++ BEFORE save_to_sd button.log..");            

                hematuria_client_event_handler();
              }


              if (msg->data.button.button_id == BUTTON_SW4_PIN && msg->event_type == EVENT_BUTTON_PRESS) {
                const char* sound_state = 
                    msg->data.button.state == CMD_STOP_SOUND ? "Stop Sound" :
                    msg->data.button.state == CMD_WATER_SOUND ? "Water Sound" :
                    msg->data.button.state == CMD_URINE_SOUND ? "Urine Sound" :
                    "Unknown State";
                DEBUG_LOG("%s: %s\n", button_name, sound_state);

                if (sound_state == "Stop Sound")
                {                  
                  Serial.println("+++ Stop Sound");
                  stop_sound();
                }

                if (sound_state == "Water Sound")
                {
                  Serial.println("+++ Water Sound");
                  //if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {        
                    //mp3play("001");     
                    //play_water_sound();     // one time play
                    play_water_sound(REPEAT_INFINITE, 0);     // repeat infinite
                    //xSemaphoreGive(sdMutex);
                  //}
                }

                if (sound_state == "Urine Sound")
                {
                  Serial.println("+++ Urine Sound");
                  //if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {        
                    //mp3play("002");     
                    //play_urination_sound();     // one time play
                    play_urination_sound(REPEAT_INFINITE, 0);     // repeat infinite
                    //xSemaphoreGive(sdMutex);
                  //}
                }

              } else if (msg->event_type == EVENT_BUTTON_PRESS) {
                const char* status = 
                    msg->data.button.button_id == BUTTON_SW1_PIN ? (msg->data.button.state ? "Motor Start" : "Motor Stop") :
                    msg->data.button.button_id == BUTTON_SW2_PIN ? "Motor Stop" :
                    msg->data.button.button_id == BUTTON_SW3_PIN ? "Blood Detected" :
                    "Unknown Action";
                DEBUG_LOG("%s: %s\n", button_name, status);


              }

              // LCD 화면 업데이트
              //update_button_status_display(msg->data.button.button_id, msg->event_type == EVENT_BUTTON_PRESS);

              // SD 카드 저장
              if (msg->header.flags & FLAG_SAVE_TO_SD) {
                char log_path[32];
                snprintf(log_path, sizeof(log_path), "/button/%s.log", button_name);
                //save_to_sd(log_path, msg, sizeof(EventMessage));
              }
              break;
            }
            
          // 에러 이벤트
          case EVENT_ERROR:
            {
              uint8_t error_code = msg->data.error.error_code;
              const char* error_detail = msg->data.error.detail;
              const char* error_msg = get_error_message(error_code, error_detail);

              DEBUG_LOG("Error Event:\n");
              DEBUG_LOG("  %s\n", error_msg);

              //show_error_screen(error_code, error_msg);
              if (msg->header.flags & FLAG_SAVE_TO_SD) {
                //save_to_sd("/system/error.log", msg, sizeof(EventMessage));
              }
              break;
            }
          case EVENT_WIFI_CHANGE:
            {
              // WiFi 이벤트 처리
              switch (msg->event_source) {
                case WIFI_EVENT_CONNECTED:
                  {
                    DEBUG_LOG("WiFi Connected:\n");
                    DEBUG_LOG("  Details: %s\n", msg->data.error.detail);

                    // LCD에 WiFi 연결 상태 표시
                    //display_wifi_connected_status(msg->data.error.detail);

                    Event_wifi_ON();


                    // SD 카드 로깅
                    if (msg->header.flags & FLAG_SAVE_TO_SD) {
                      //save_to_sd("/wifi/connection.log", msg, sizeof(EventMessage));
                    }
                    break;
                  }
                case WIFI_EVENT_DISCONNECTED:
                  {
                    DEBUG_LOG("WiFi Disconnected:\n");
                    DEBUG_LOG("  Error Code: 0x%02X\n", msg->data.error.error_code);
                    DEBUG_LOG("  Details: %s\n", msg->data.error.detail);

                    // LCD에 연결 해제 표시
                    //display_wifi_disconnected_status(msg->data.error.error_code);
                    Event_wifi_OFF();

                    // 에러 코드에 따른 추가 처리
                    switch (msg->data.error.error_code) {
                      case WIFI_ERROR_AUTH_FAILED:
                        //show_wifi_error_message("Authentication Failed");
                        break;
                      case WIFI_ERROR_NO_AP_FOUND:
                        //show_wifi_error_message("AP Not Found");
                        break;
                      case WIFI_ERROR_CONNECT_TIMEOUT:
                        //show_wifi_error_message("Connection Timeout");
                        break;
                      case WIFI_ERROR_LOST_CONN:
                        //show_wifi_error_message("Connection Lost");
                        break;
                      case WIFI_ERROR_SIGNAL_LOST:
                        //show_wifi_error_message("Signal Lost");
                        break;
                    }
                    if (msg->header.flags & FLAG_SAVE_TO_SD) {
                      //save_to_sd("/wifi/error.log", msg, sizeof(EventMessage));
                    }
                    break;
                  }
                case WIFI_EVENT_IP_CHANGED:
                  {
                    DEBUG_LOG("WiFi IP Changed:\n");
                    DEBUG_LOG("  New IP: %s\n", msg->data.error.detail);

                    // LCD에 IP 변경 표시
                    //display_wifi_ip_status(msg->data.error.detail);

                    if (msg->header.flags & FLAG_SAVE_TO_SD) {
                      //save_to_sd("/wifi/ip_change.log", msg, sizeof(EventMessage));
                    }
                    break;
                  }
                case WIFI_EVENT_WEAK_SIGNAL:
                  {
                    DEBUG_LOG("WiFi Weak Signal:\n");
                    DEBUG_LOG("  Details: %s\n", msg->data.error.detail);

                    // LCD에 신호 강도 경고 표시
                    //display_wifi_signal_warning(msg->data.error.detail);

                    if (msg->header.flags & FLAG_SAVE_TO_SD) {
                      //save_to_sd("/wifi/signal.log", msg, sizeof(EventMessage));
                    }
                    break;
                  }
              }
              break;
            }
          default:
            DEBUG_LOG("Unknown event type: 0x%02X\n", msg->event_type);
            break;
        }

        // 모든 이벤트를 통합 로그에도 저장
        if (msg->header.flags & FLAG_SAVE_TO_SD) {
          char log_filename[64];
          struct tm* timeinfo = localtime((time_t*)&msg->header.timestamp);
          strftime(log_filename, sizeof(log_filename), "/log/%Y%m%d_events.log", timeinfo);
          //save_to_sd(log_filename, msg, sizeof(EventMessage));
        }
        DEBUG_LOG("=== Event Processing Complete ===\n\n");
        break;
      }
    default:
      DEBUG_LOG("Unknown message type: 0x%02X\n", header->type);
      break;
  }  // end of switch(header->type)
}  // end of process_message()

// Serial 통신을 처리할 task 함수
void serialTask(void *parameter) {  

    DEBUG_LOG("Serial Task Created\n");

    

//init_uart_simulation(); 
#if 0 // ???? bgjung
if (!backlightSet) {
        digitalWrite(GFX_BL, HIGH);  // 또는 필요한 값
        backlightSet = true;  // 플래그 설정
}
#endif 

    while (1) {
      
      if (Serial2.available()) {  //4pin serial 
          Serial.println("+++ in serialTask");
               
          uint8_t buffer[512];
          size_t length = Serial2.readBytes(buffer, sizeof(buffer));
          if (length > 0) {
            
			Serial.println("\n \n +++ Data received, adding to queue...");
		  
           // handle_received_data(serial2buffer, length);
           
		  	 if (xQueueSend(serialQueue, &buffer, portMAX_DELAY) != pdTRUE) {
                    Serial.println("Queue full, dropping data...");
         	 }
          }
      }

      vTaskDelay(pdMS_TO_TICKS(20)); // 딜레이 시간 단축
  }
}
// 수신 핸들러 태스크 (Core 0에서 실행)
void handleTask(void *parameter) {
    uint8_t buffer[512];
    size_t length;

    while (1) {
        if (xQueueReceive(serialQueue, &buffer, portMAX_DELAY)) {  
            Serial.println("\n \n +++ Processing received data...");
            handle_received_data(buffer, length); // 데이터 처리 함수 호출
        }
    }
}


