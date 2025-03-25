/**
 * @file serial_protocol.c
 * @brief 공통 시리얼 통신 프로토콜 구현
 */
#include "serial_protocol.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"

// 전역 변수
SerialProtocolContext g_serial_ctx = {0};
//BackupState backup_state = {0};
//RestoreState restore_state = {0};
int debug_serial_flag = false;

/**
 * @brief 시리얼 프로토콜 초기화
 * 
 * @param ctx 시리얼 프로토콜 컨텍스트
 * @param board_type 보드 타입 (0: Main, 1: LCD)
 * @param process_callback 메시지 처리 콜백 함수
 * @return true 초기화 성공
 * @return false 초기화 실패
 */
bool serial_protocol_init(SerialProtocolContext* ctx, Board_Type board_type, void (*process_callback)(void* message, size_t size)) {
    if (ctx == NULL || process_callback == NULL) {
        DEBUG_LOG("Invalid parameters\n");
        return false;
    }

    // 컨텍스트 초기화
    memset(ctx, 0, sizeof(SerialProtocolContext));
    
    // 보드 타입 및 콜백 설정
    ctx->board_type = board_type;
    ctx->process_message_callback = process_callback;
    
    // 뮤텍스 초기화
    ctx->rx_mutex = xSemaphoreCreateMutex();
    ctx->tx_mutex = xSemaphoreCreateMutex();
    
    if (ctx->rx_mutex == NULL || ctx->tx_mutex == NULL) {
        DEBUG_LOG("Failed to create mutex\n");
        return false;
    }
    
    // 초기 상태 설정
    ctx->sequence_number = 0;
    ctx->peer_connected = false;
    ctx->last_hello_time = 0;
    ctx->last_received_time = 0;
    ctx->last_random_event = 0;
    ctx->initial_status_sent = false;
    
    // 큐 초기화
    ctx->rx_queue_head = 0;
    ctx->rx_queue_tail = 0;
    ctx->rx_queue_count = 0;
    
    // 시리얼 초기화
    if (!serial_init(ctx)) {
        DEBUG_LOG("Serial initialization failed\n");
        return false;
    }
    
    // 태스크 생성
    BaseType_t task_created = xTaskCreatePinnedToCore(
        serial_protocol_task,
        "Serial_Protocol",
        2*4096,
        ctx,
        5,
        NULL,
        1
    );
    
    if (task_created != pdPASS) {
        DEBUG_LOG("Failed to create serial protocol task\n");
        return false;
    }
    
    DEBUG_LOG("Serial protocol initialized, board type: %d\n", board_type);
    return true;
}
/**
 * @brief UART 초기화
 * 
 * @param ctx 시리얼 프로토콜 컨텍스트
 * @return true 초기화 성공
 * @return false 초기화 실패
 */
bool serial_init(SerialProtocolContext* ctx) {
    uart_config_t uart_config = {
        .baud_rate = PROTOCOL_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT
    };
    
    // UART 설정
    if (uart_param_config(UART_NUM_2, &uart_config) != ESP_OK) {
        DEBUG_LOG("UART parameter configuration failed\n");
        Serial.printf("UART parameter configuration failed\n");
        return false;
    }
    
    // UART 핀 설정
    switch (ctx->board_type) {
      case MAIN_BOARD:
          DEBUG_LOG("Board_Type is MAIN_Board\n");
          Serial.printf("Board_Type is MAIN_Board\n");
          if (uart_set_pin(UART_NUM_2, MAIN_PROTOCOL_TX, MAIN_PROTOCOL_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
            DEBUG_LOG("UART pin configuration failed\n");
            return false;
          }
          break;
      case LCD_BOARD:
          DEBUG_LOG("Board_Type is LCD_Board\n");
          Serial.printf("Board_Type is LCD_Board\n");
          if (uart_set_pin(UART_NUM_2, LCD_PROTOCOL_TX, LCD_PROTOCOL_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
            DEBUG_LOG("UART pin configuration failed\n");
            return false;
          }
          break;
      default :
          DEBUG_LOG("Unknown Board_Type %d", ctx->board_type);
          break;
    }
    
    // 드라이버 설치 및 RX 버퍼 설정
    if (uart_driver_install(UART_NUM_2, RX_BUFFER_SIZE * 2, 0, 0, NULL, 0) != ESP_OK) {
        DEBUG_LOG("UART driver installation failed\n");
        return false;
    }
    
    // 수신 버퍼 비우기
    uart_flush(UART_NUM_2);
    
    if (ctx->board_type == MAIN_BOARD) Serial.printf("UART initialized (TX: %d, RX: %d)\n", MAIN_PROTOCOL_TX, MAIN_PROTOCOL_RX);
    else if (ctx->board_type == LCD_BOARD) Serial.printf("UART initialized (TX: %d, RX: %d)\n", LCD_PROTOCOL_TX, LCD_PROTOCOL_RX);
    
    return true;
}

/**
 * @brief 시리얼 프로토콜 태스크 함수
 * 
 * @param pvParameters 태스크 파라미터 (SerialProtocolContext 포인터)
 */
void serial_protocol_task(void* pvParameters) {
    SerialProtocolContext* ctx = (SerialProtocolContext*)pvParameters;
    
    if (ctx == NULL) {
        DEBUG_LOG("Invalid context\n");
        vTaskDelete(NULL);
        return;
    }
    
    uint8_t temp_buffer[RX_BUFFER_SIZE];
    int length;
    
    DEBUG_LOG("Serial protocol task started\n");
    
    while (1) {
        // 연결 상태 체크
        check_serial_connection(ctx);
        
        // 수신 큐 처리
        process_rx_queue(ctx);
        
        // UART 데이터 수신
        length = uart_read_bytes(UART_NUM_2, temp_buffer, RX_BUFFER_SIZE, pdMS_TO_TICKS(10));
        
        if (length > 0) {
            DEBUG_LOG("Received %d bytes from UART\n", length);
            handle_received_data(ctx, temp_buffer, length);
        }
        
        // CPU 사용률 조절을 위한 딜레이
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/**
 * @brief 수신 큐 처리 함수
 * 
 * @param ctx 시리얼 프로토콜 컨텍스트
 */
void process_rx_queue(SerialProtocolContext* ctx) {
    if (ctx == NULL) {
        return;
    }
    
    // 뮤텍스 획득
    if (xSemaphoreTake(ctx->rx_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return;
    }
    
    uint8_t buffer[MAX_MESSAGE_SIZE];
    size_t size;
    
    // 큐에 아이템이 있으면 처리
    while (!rx_queue_is_empty(ctx)) {
        if (rx_queue_pop(ctx, buffer, &size)) {
            // 메시지 처리 콜백 호출
            if (ctx->process_message_callback != NULL && size >= sizeof(MessageHeader)) {
                ctx->process_message_callback(buffer, size);
            }
        }
    }
    
    // 뮤텍스 반환
    xSemaphoreGive(ctx->rx_mutex);
}

/**
 * @brief 수신 데이터 처리 함수
 * 
 * @param ctx 시리얼 프로토콜 컨텍스트
 * @param data 수신 데이터 버퍼
 * @param length 데이터 길이
 */
void handle_received_data(SerialProtocolContext* ctx, uint8_t* data, size_t length) {
    if (ctx == NULL || data == NULL || length == 0) {
        return;
    }
    
    // 마지막 수신 시간 업데이트
    ctx->last_received_time = xTaskGetTickCount();
    
    // 데이터 파싱 및 메시지 조립
    for (size_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        
        // 시작 마커를 찾는 상태이면
        if (ctx->rx_buffer_index == 0) {
            if (byte == 0xFF) {
                ctx->rx_buffer[ctx->rx_buffer_index++] = byte;
            }
            continue;
        }
        
        // 버퍼에 바이트 추가
        if (ctx->rx_buffer_index < RX_BUFFER_SIZE) {
            ctx->rx_buffer[ctx->rx_buffer_index++] = byte;
        } else {
            DEBUG_LOG("RX buffer overflow, resetting\n");
            ctx->rx_buffer_index = 0;
            continue;
        }
        
        // 헤더가 완성되었는지 확인
        if (ctx->rx_buffer_index >= sizeof(MessageHeader)) {
            MessageHeader* header = (MessageHeader*)ctx->rx_buffer;
            
            // 메시지 길이 유효성 검사
            size_t total_length = sizeof(MessageHeader) + header->length;
            if (total_length > RX_BUFFER_SIZE) {
                DEBUG_LOG("Invalid message length: %d\n", total_length);
                ctx->rx_buffer_index = 0;
                continue;
            }
            
            // 전체 메시지를 받았는지 확인
            if (ctx->rx_buffer_index >= total_length) {

                // 체크섬 검증
                if (!verify_checksum(ctx->rx_buffer, total_length)) {
                    DEBUG_LOG("Checksum verification failed\n");
                    ctx->rx_buffer_index = 0;
                    continue;
                }              
                // 체크섬이 유효한 경우에만 메시지를 큐에 추가
                if (!rx_queue_push(ctx, ctx->rx_buffer, total_length)) {
                    DEBUG_LOG("Failed to push message to queue\n");
                }
                
                // 버퍼 초기화
                ctx->rx_buffer_index = 0;
            }
        }
    }
}

/**
 * @brief 큐에 수신 데이터 추가
 * 
 * @param ctx 시리얼 프로토콜 컨텍스트
 * @param data 데이터 버퍼
 * @param size 데이터 크기
 * @return true 성공
 * @return false 실패
 */
bool rx_queue_push(SerialProtocolContext* ctx, uint8_t* data, size_t size) {
    if (ctx == NULL || data == NULL || size == 0 || size > MAX_MESSAGE_SIZE) {
        return false;
    }
    
    // 뮤텍스 획득
    if (xSemaphoreTake(ctx->rx_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return false;
    }
    
    // 큐가 가득 찼는지 확인
    if (rx_queue_is_full(ctx)) {
        DEBUG_LOG("RX queue is full\n");
        xSemaphoreGive(ctx->rx_mutex);
        return false;
    }
    
    // 큐에 데이터 복사
    memcpy(ctx->rx_queue[ctx->rx_queue_tail].buffer, data, size);
    ctx->rx_queue[ctx->rx_queue_tail].size = size;
    
    // 큐 포인터 및 카운터 업데이트
    ctx->rx_queue_tail = (ctx->rx_queue_tail + 1) % RX_QUEUE_SIZE;
    ctx->rx_queue_count++;
    
    // 뮤텍스 반환
    xSemaphoreGive(ctx->rx_mutex);
    return true;
}

/**
 * @brief 큐에서 수신 데이터 가져오기
 * 
 * @param ctx 시리얼 프로토콜 컨텍스트
 * @param buffer 데이터를 저장할 버퍼
 * @param size 데이터 크기를 저장할 포인터
 * @return true 성공
 * @return false 실패
 */
bool rx_queue_pop(SerialProtocolContext* ctx, uint8_t* buffer, size_t* size) {
    if (ctx == NULL || buffer == NULL || size == NULL) {
        return false;
    }
    
    // 큐가 비었는지 확인
    if (rx_queue_is_empty(ctx)) {
        return false;
    }
    
    // 큐에서 데이터 복사
    memcpy(buffer, ctx->rx_queue[ctx->rx_queue_head].buffer, ctx->rx_queue[ctx->rx_queue_head].size);
    *size = ctx->rx_queue[ctx->rx_queue_head].size;
    
    // 큐 포인터 및 카운터 업데이트
    ctx->rx_queue_head = (ctx->rx_queue_head + 1) % RX_QUEUE_SIZE;
    ctx->rx_queue_count--;
    
    return true;
}

/**
 * @brief 큐가 비었는지 확인
 * 
 * @param ctx 시리얼 프로토콜 컨텍스트
 * @return true 큐가 비었음
 * @return false 큐에 데이터가 있음
 */
bool rx_queue_is_empty(SerialProtocolContext* ctx) {
    if (ctx == NULL) {
        return true;
    }
    
    return (ctx->rx_queue_count == 0);
}

/**
 * @brief 큐가 가득 찼는지 확인
 * 
 * @param ctx 시리얼 프로토콜 컨텍스트
 * @return true 큐가 가득 참
 * @return false 큐에 여유 공간이 있음
 */
bool rx_queue_is_full(SerialProtocolContext* ctx) {
    if (ctx == NULL) {
        return true;
    }
    
    return (ctx->rx_queue_count >= RX_QUEUE_SIZE);
}

/**
 * @brief 메시지 전송 함수
 * 
 * @param ctx 시리얼 프로토콜 컨텍스트
 * @param message 전송할 메시지
 * @param size 메시지 크기
 * @return true 전송 성공
 * @return false 전송 실패
 */
bool send_message(SerialProtocolContext* ctx, void* message, size_t size) {
    if (ctx == NULL || message == NULL || size < sizeof(MessageHeader)) {
        return false;
    }
    MessageHeader* header = (MessageHeader*)message;

    // Hello 또는 Hello ACK가 아닌 경우 연결 확인
#if 1
        if (header->type != MSG_HELLO && header->type != MSG_HELLO_ACK) {
            if (!g_serial_ctx.peer_connected) {
                DEBUG_LOG("Peer not connected, message not sent\n");
                return false;
            }
        }
#endif

    // 뮤텍스 획득
    if (xSemaphoreTake(ctx->tx_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        DEBUG_LOG("Failed to acquire TX mutex\n");
        return false;
    }
    
    DEBUG_LOG("Sending message type: %s (0x%02X)\n", 
              get_message_type_str(header->type), header->type);
    #if 0
    Serial.printf("Sending message type: %s (0x%02X), size(%d)\n", 
              get_message_type_str(header->type), header->type, size);
    #endif
    decode_message_header(header);
    print_message((uint8_t*)message);
    print_hex_dump((uint8_t*)message, size);
    DEBUG_LOG("===================================================\n");
    
    int written = uart_write_bytes(UART_NUM_2, message, size);
    
    // TX 버퍼가 비워질 때까지 대기
    uart_wait_tx_done(UART_NUM_2, pdMS_TO_TICKS(100));
    
    // 뮤텍스 반환
    xSemaphoreGive(ctx->tx_mutex);
    
    if (written != size) {
        DEBUG_LOG("Failed to send message: written %d of %d bytes\n", written, size);
        return false;
    }
    
    return true;
}

/**
 * @brief 연결 상태 확인
 * 
 * @param ctx 시리얼 프로토콜 컨텍스트
 */

void check_serial_connection(SerialProtocolContext* ctx) {
    if (ctx == NULL) {
        return;
    }
    uint32_t current_time = xTaskGetTickCount();
    
    // 연결 상태에 따라 다른 Hello 간격 적용
    uint32_t hello_interval = ctx->peer_connected ? 
                            HELLO_INTERVAL_MS_CONNECTED : 
                            HELLO_INTERVAL_MS_DISCONNECTED;
    
    // Hello 메시지 주기적 전송
    if (current_time - ctx->last_hello_time >= pdMS_TO_TICKS(hello_interval)) {
        send_hello(ctx);
        ctx->last_hello_time = current_time;
    }
    
    // 연결 타임아웃 체크
    if (current_time - ctx->last_received_time > pdMS_TO_TICKS(CONNECTION_TIMEOUT_MS)) {
        if (ctx->peer_connected) {
            ctx->peer_connected = false;
            DEBUG_LOG("Peer connection lost\n");
        }
    }
}

/**
 * @brief Hello 메시지 전송
 * 
 * @param ctx 시리얼 프로토콜 컨텍스트
 */
void send_hello(SerialProtocolContext* ctx) {
    if (ctx == NULL) {
        return;
    }
    
    HelloMessage msg = { 0 };
    msg.header.start_marker = 0xFF;
    msg.header.type = MSG_HELLO;
    msg.header.timestamp = time(NULL);
    msg.header.seq_num = ctx->sequence_number++;
    msg.header.length = sizeof(HelloMessage) - sizeof(MessageHeader);
    msg.board_type = ctx->board_type;
    msg.checksum = calculate_checksum(&msg, sizeof(HelloMessage) - 1);
    
    send_message(ctx, &msg, sizeof(HelloMessage));
}

/**
 * @brief 체크섬 계산
 * 
 * @param message 메시지 포인터
 * @param size 계산할 크기 (체크섬 바이트 제외)
 * @return uint8_t 계산된 체크섬
 */
uint8_t calculate_checksum(void* message, size_t size) {
    if (message == NULL || size == 0) {
        return 0;
    }
    
    uint8_t sum = 0;
    uint8_t* data = (uint8_t*)message;
    
    for (size_t i = 0; i < size; i++) {
        sum += data[i];
    }
    
    return sum;
}

/**
 * @brief 체크섬 검증
 * 
 * @param message 메시지 포인터
 * @param size 메시지 전체 크기 (체크섬 포함)
 * @return true 체크섬 일치
 * @return false 체크섬 불일치
 */
bool verify_checksum(void* message, size_t size) {
    if (message == NULL || size <= 1) {
        return false;
    }
    
    uint8_t* msg = (uint8_t*)message;
    uint8_t received_checksum = msg[size - 1];
    uint8_t calculated_checksum = calculate_checksum(message, size - 1);
    
    return (received_checksum == calculated_checksum);
}

/**
 * @brief 타임스탬프 포맷팅
 * 
 * @param timestamp Unix 타임스탬프
 * @param buffer 결과를 저장할 버퍼
 * @param size 버퍼 크기
 * @return char* 형식화된 타임스탬프 문자열
 */
char* format_timestamp(uint32_t timestamp, char* buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return NULL;
    }
    
    time_t time = timestamp;
    struct tm* timeinfo = localtime(&time);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", timeinfo);
    
    return buffer;
}

/**
 * @brief 타임스탬프 출력
 * 
 * @param timestamp Unix 타임스탬프
 */
void print_timestamp(uint32_t timestamp) {
    char datetime[32];
    format_timestamp(timestamp, datetime, sizeof(datetime));
    DEBUG_LOG("Timestamp: %s\n", datetime);
}

/**
 * @brief 16진수 데이터 덤프
 * 
 * @param data 데이터 버퍼
 * @param length 데이터 길이
 */
void print_hex_dump(const uint8_t* data, size_t length) {
    if (data == NULL || length == 0) {
        return;
    }
    
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

/**
 * @brief 메시지 타입 문자열 반환
 * 
 * @param type 메시지 타입
 * @return const char* 메시지 타입 문자열
 */
const char* get_message_type_str(uint8_t type) {
    switch (type) {
        case MSG_HELLO: return "HELLO";
        case MSG_HELLO_ACK: return "HELLO_ACK";
        case MSG_SYSTEM_STATUS: return "SYSTEM_STATUS";
        case MSG_SENSOR_DATA: return "SENSOR_DATA";
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
        case MSG_DATA_BACKUP_REQUEST: return "DATA_BACKUP_REQUEST";
        case MSG_DATA_BACKUP_DATA: return "DATA_BACKUP_DATA";
        case MSG_DATA_BACKUP_ACK: return "DATA_BACKUP_ACK";
        case MSG_DATA_RESTORE_REQUEST: return "DATA_RESTORE_REQUEST";
        case MSG_DATA_RESTORE_DATA: return "DATA_RESTORE_DATA";
        case MSG_DATA_RESTORE_ACK: return "DATA_RESTORE_ACK";
        case MSG_DIAGNOSTIC_RESULT: return "DIAGNOSTIC_RESULT";
        default: return "UNKNOWN";
    }
}

/**
 * @brief 이벤트 타입 문자열 반환
 * 
 * @param event_type 이벤트 타입
 * @return const char* 이벤트 타입 문자열
 */
const char* get_event_type_str(uint8_t event_type) {
    switch (event_type) {
        case EVENT_SENSOR_COVER: return "SENSOR_COVER";
        case EVENT_SENSOR_URINE: return "SENSOR_URINE";
        case EVENT_SENSOR_WEIGHT: return "SENSOR_WEIGHT";
        case EVENT_SENSOR_GAS: return "SENSOR_GAS";
        case EVENT_SENSOR_DIAPER: return "SENSOR_DIAPER";
        case EVENT_SENSOR_BATTERY: return "SENSOR_BATTERY";
        case EVENT_SENSOR_POWER: return "SENSOR_POWER";
        case EVENT_SENSOR_SYSTEM: return "SENSOR_SYSTEM";
        case EVENT_SENSOR_FULLLEVEL: return "SENSOR_FULLLEVEL";
        case EVENT_SERVER_CONNECT: return "SERVER_CONNECT";
        case EVENT_BUTTON_PRESS: return "BUTTON_PRESS";
        case EVENT_BUTTON_RELEASE: return "BUTTON_RELEASE";
        case EVENT_MOTOR_STATE: return "MOTOR_STATE";
        case EVENT_WIFI_CHANGE: return "WIFI_CHANGE";
        case EVENT_ERROR: return "ERROR";
        default: return "UNKNOWN_EVENT";
    }
}

/**
 * @brief WiFi 이벤트 문자열 반환
 * 
 * @param wifi_event WiFi 이벤트
 * @return const char* WiFi 이벤트 문자열
 */
const char* get_wifi_event_str(uint8_t wifi_event) {
    switch (wifi_event) {
        case WIFI_EVENT_CONNECTED: return "CONNECTED";
        case WIFI_EVENT_DISCONNECTED: return "DISCONNECTED";
        case WIFI_EVENT_CONNECT_FAILED: return "CONNECT_FAILED";
        case WIFI_EVENT_WEAK_SIGNAL: return "WEAK_SIGNAL";
        case WIFI_EVENT_IP_CHANGED: return "IP_CHANGED";
        default: return "UNKNOWN_WIFI_EVENT";
    }
}

/**
 * @brief 메시지 헤더 디코딩
 * 
 * @param header 메시지 헤더 포인터
 */
void decode_message_header(const MessageHeader* header) {
    if (header == NULL) {
        return;
    }
    
    DEBUG_LOG("Message Header:\n");
    DEBUG_LOG("  Marker    : 0x%02X\n", header->start_marker);
    DEBUG_LOG("  Type      : 0x%02X (%s)\n", header->type, get_message_type_str(header->type));
    DEBUG_LOG("  Flags     : 0x%02X [%s%s%s%s]\n",
              header->flags,
              (header->flags & FLAG_SAVE_TO_SD) ? "SD " : "",
              (header->flags & FLAG_RESPONSE) ? "RSP " : "",
              (header->flags & FLAG_ERROR) ? "ERR " : "",
              (header->flags & FLAG_URGENT) ? "URG" : "");
    
    // 타임스탬프를 읽기 쉬운 형식으로 변환
    char timestamp[32];
    time_t t = header->timestamp;
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&t));
    DEBUG_LOG("  Timestamp : %s\n", timestamp);
    DEBUG_LOG("  Sequence  : %u\n", header->seq_num);
    DEBUG_LOG("  Length    : %u bytes\n", header->length);
    
    // 특정 메시지 타입에 따른 추가 정보 출력
    if (header->type == MSG_EVENT) {
        EventMessage* event_msg = (EventMessage*)header;
        DEBUG_LOG("Event Type: %s\n", get_event_type_str(event_msg->event_type));
        if (event_msg->event_type == EVENT_WIFI_CHANGE) {
            DEBUG_LOG("WiFi Event: %s\n", get_wifi_event_str(event_msg->event_source));
        }
    }
    
    DEBUG_LOG("====================================================\n");
}

/**
 * @brief 이벤트 메시지 디코딩
 * 
 * @param msg 이벤트 메시지 포인터
 * @param output 출력 버퍼
 * @param output_size 출력 버퍼 크기
 * @return true 디코딩 성공
 * @return false 디코딩 실패
 */
bool decode_event_message(EventMessage* msg, char* output, size_t output_size) {
    if (!msg || !output || output_size == 0) {
        return false;
    }
    
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
            snprintf(temp, sizeof(temp), "State: %s",
                     msg->data.digital.state ? "Detected" : "None");
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
                     msg->data.digital.state ? "Error" : "Normal");
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
                    msg->data.button.button_id == BUTTON_SW1_PIN ? "Motor Manual" :
                    msg->data.button.button_id == BUTTON_SW2_PIN ? "Motor Stop" :
                    msg->data.button.button_id == BUTTON_SW3_PIN ? "Blood Flow" :
                    msg->data.button.button_id == BUTTON_SW4_PIN ? "Urine Sound" :
                    "Unknown";
                snprintf(temp, sizeof(temp), "%s: %s",
                         button_name,
                         msg->event_type == EVENT_BUTTON_PRESS ? "Pressed" : "Released");
            }
            break;
            
        case EVENT_ERROR:
            event_type_str = "Error Event";
            snprintf(temp, sizeof(temp), "Error 0x%02X: %s",
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

/**
 * @brief 메시지 출력
 * 
 * @param message 메시지 포인터
 */
void print_message(void* message) {
    if (message == NULL) {
        return;
    }
    
    MessageHeader* header = (MessageHeader*)message;
    
    DEBUG_LOG("\n=== Message Information ===\n");
    DEBUG_LOG("Message Type: %s (0x%02X)\n", get_message_type_str(header->type), header->type);
    DEBUG_LOG("Sequence: %d\n", header->seq_num);
    
    char timestamp[32];
    format_timestamp(header->timestamp, timestamp, sizeof(timestamp));
    DEBUG_LOG("Timestamp: %s\n", timestamp);
    
    DEBUG_LOG("Flags: 0x%02X", header->flags);
    if (header->flags & FLAG_SAVE_TO_SD) DEBUG_LOG(" [SD Save]");
    if (header->flags & FLAG_RESPONSE) DEBUG_LOG(" [Response]");
    if (header->flags & FLAG_ERROR) DEBUG_LOG(" [Error]");
    if (header->flags & FLAG_URGENT) DEBUG_LOG(" [Urgent]");
    DEBUG_LOG("\n");
    
    // 메시지 타입별 추가 정보 출력
    switch (header->type) {
        case MSG_HELLO:
        case MSG_HELLO_ACK:
            {
                HelloMessage* msg = (HelloMessage*)message;
                DEBUG_LOG("Board Type: %s\n",
                          msg->board_type == 0 ? "Main Board" : 
                          msg->board_type == 1 ? "LCD Board" : "Unknown");
            }
            break;
        #if 1
        case MSG_SYSTEM_STATUS:
            {
                SystemStatusMessage* msg = (SystemStatusMessage*)message;
                DEBUG_LOG("System Status Update:\n");
                DEBUG_LOG("  Model Name: %s\n", msg->model_name);
                DEBUG_LOG("  HW Version: %s\n", msg->hw_version);
                DEBUG_LOG("  SW Version: %s\n", msg->sw_version);
                DEBUG_LOG("  Serial Num: %s\n", msg->serial_num);
                DEBUG_LOG("  Status: 0x%02X\n", msg->sys_status);
            }
            break;
            
        case MSG_SENSOR_DATA:
            {
                SensorDataMessage* msg = (SensorDataMessage*)message;
                DEBUG_LOG("Received Sensor Data:\n");
                DEBUG_LOG("  Cover: %s\n", msg->cover_sensor ? "Closed" : "Open");
                DEBUG_LOG("  Urine: %s\n", msg->urine_sensor ? "Detected" : "None");
                DEBUG_LOG("  Diaper Sensor: %s\n", msg->diaper_sensor ? "In" : "Out");
                DEBUG_LOG("  Gas Sensor: %s\n", msg->gas_sensor ? "Detected" : "None");
                DEBUG_LOG("  Full Level Sensor: %s\n", msg->fulllevel_sensor ? "Reached" : "Not Reached");
                DEBUG_LOG("  Weight: %.1f g\n", msg->weight_sensor);
                DEBUG_LOG("  Motor: %s\n", msg->motor_running ? "Running" : "Stop");
                DEBUG_LOG("  WiFi: %s\n", msg->wifi_connected ? "Connected" : "DisConnected");
                DEBUG_LOG("  Server: %s\n", msg->server_status ? "Connected" : "DisConnected");
                DEBUG_LOG("  System Error: %s\n", msg->system_status ? "Error" : "Normal");
                DEBUG_LOG("  Power: %s\n", msg->power_status ? "On" : "Off");
                DEBUG_LOG("  Battery: %.1f%%\n", msg->battery_level);
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
                
            }
            break;
            
        case MSG_MOTOR_STATUS:
            {
                MotorMessage* msg = (MotorMessage*)message;
                const char* status_str;
                
                switch (msg->motor_status) {
                    case MOTOR_STATUS_STOP:
                        status_str = "Stopped";
                        break;
                        
                    case MOTOR_STATUS_RUNNING:
                        status_str = "Running";
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
                
            }
            break;
          
        #endif
        case MSG_EVENT:
            {
                EventMessage* msg = (EventMessage*)message;
                char event_desc[128];
                
                if (decode_event_message(msg, event_desc, sizeof(event_desc))) {
                    DEBUG_LOG("%s\n", event_desc);
                }
            }
            break;
            
        // 기타 메시지 타입에 대한 처리 추가 가능
    }
    
    DEBUG_LOG("===================================================\n");
}

/**
 * @brief 메시지 타입 출력
 * 
 * @param type 메시지 타입
 */
void print_message_type(uint8_t type) {
    DEBUG_LOG("Message Type: %s (0x%02X)\n", get_message_type_str(type), type);
}
