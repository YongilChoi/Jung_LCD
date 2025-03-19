
#if 0
// uart_simulation.cpp
//#ifdef USE_UART_SIMULATION
#include "common.h"
//#include "uart_simulation.h"
//#include "serial_protocol.h"
#include "mqtt_persistence.h"

static QueueHandle_t uart_tx_queue = NULL;  // Main -> LCD
static QueueHandle_t uart_rx_queue = NULL;  // LCD -> Main
static TaskHandle_t virtual_lcd_task = NULL;

// 가상 LCD 보드의 내부 상태
typedef struct {
    bool sd_mounted;
    uint32_t total_backup_size;
    uint16_t current_chunk;
    File current_file;
    char current_filename[MAX_FILENAME_LENGTH];
    bool is_backup_mode;  // true: 백업 중, false: 복원 중
} virtual_lcd_state_t;

// C++ 스타일의 초기화 사용
static virtual_lcd_state_t virtual_lcd_state = {
    false,              // sd_mounted
    0,                  // total_backup_size
    0,                  // current_chunk
    File(nullptr),      // current_file - File 객체 초기화
    "",                 // current_filename
    false               // is_backup_mode
};

// 내부 함수 선언
static void TaskVirtualLCD(void* pvParameters);
static void handle_virtual_backup_request(const uint8_t* data, size_t length);
static void handle_virtual_backup_data(const uint8_t* data, size_t length);
static void handle_virtual_restore_request(const uint8_t* data, size_t length);
static void handle_virtual_restore_data(uint16_t chunk_index);
static void handle_virtual_restore_ack(const uint8_t* data, size_t length);
static void send_virtual_ack(uint16_t chunk_index, uint8_t status);
static bool init_virtual_storage(void);
static time_t get_oldest_backup_file(char* filename, size_t max_length);

static uint8_t debug_level = LOG_LEVEL_INFO;
//static uint8_t debug_level = LOG_LEVEL_DEBUG;

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
    
    //
}

// 초기화 함수
bool init_uart_simulation() {

    logMessage("SIM", LOG_LEVEL_INFO, "Initializing UART simulation...");

    uart_tx_queue = xQueueCreate(30, sizeof(uart_message_t));
    uart_rx_queue = xQueueCreate(30, sizeof(uart_message_t));
    
    if (!uart_tx_queue || !uart_rx_queue) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Failed to create message queues");
        return false;
    }

    if (!init_virtual_storage()) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Failed to initialize virtual storage");
        return false;
    }

    BaseType_t ret = xTaskCreate(
        TaskVirtualLCD,
        "VirtualLCD",
        8192,
        NULL,
        tskIDLE_PRIORITY + 1,
        &virtual_lcd_task
    );

    if (ret != pdPASS) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Failed to create virtual LCD task");
        return false;
    }

    logMessage("SIM", LOG_LEVEL_INFO, "UART simulation initialized successfully");
    return true;
}

void deinit_uart_simulation(void) {
    if (virtual_lcd_task) {
        vTaskDelete(virtual_lcd_task);
        virtual_lcd_task = NULL;
    }
    if (uart_tx_queue) {
        vQueueDelete(uart_tx_queue);
        uart_tx_queue = NULL;
    }
    if (uart_rx_queue) {
        vQueueDelete(uart_rx_queue);
        uart_rx_queue = NULL;
    }
    if (virtual_lcd_state.current_file) {
        virtual_lcd_state.current_file.close();
    }
}

// 메시지 송수신 함수
bool send_simulated_message(const uint8_t* data, size_t length) {
      //logMessage("SIM", LOG_LEVEL_INFO, "send_simulated_message() called");
      //Serial.println("send_simulated_message() called");

    if (!uart_tx_queue || length > MAX_MSG_SIZE) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Invalid send parameters - queue:%p, length:%u, max:%u", 
                  uart_tx_queue, length, MAX_MSG_SIZE);
        return false;
    }

    MessageHeader* header = (MessageHeader*)data;
    logMessage("SIM", LOG_LEVEL_INFO, "Sending message - Type:0x%02X, Length:%u, Seq:%u", 
              header->type, length, header->seq_num);  // seq_num 추가

    uart_message_t msg;
    memcpy(msg.data, data, length);
    msg.length = length;

    if (xQueueSend(uart_tx_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Failed to send message to queue");
        return false;
    }

    logMessage("SIM", LOG_LEVEL_DEBUG, "Message queued successfully");
    return true;
}

size_t receive_simulated_message(uint8_t* buffer, size_t max_length) {
    if (!uart_rx_queue || !buffer) return 0;

    uart_message_t msg;
    if (xQueueReceive(uart_rx_queue, &msg, pdMS_TO_TICKS(SIMULATION_QUEUE_TIMEOUT_MS)) == pdPASS) {
        size_t copy_size = min(msg.length, max_length);
        memcpy(buffer, msg.data, copy_size);
        return copy_size;
    }
    return 0;
}

// 가상 LCD 태스크
void TaskVirtualLCD(void* pvParameters) {
    uart_message_t rx_msg;
    uint32_t msg_count = 0;  // 수신된 메시지 카운트
    
    logMessage("SIM", LOG_LEVEL_INFO, "Virtual LCD task started");
    
    while(1) {
        BaseType_t received = xQueueReceive(uart_tx_queue, &rx_msg, pdMS_TO_TICKS(50));
        
        if (received == pdPASS) {
            msg_count++;
            MessageHeader* header = (MessageHeader*)rx_msg.data;
            
            logMessage("SIM", LOG_LEVEL_INFO, "VirtualLCD received message %lu - Type: 0x%02X, Length: %u", 
                      msg_count, header->type, rx_msg.length);
            
             // 모든 수신 메시지 상세 로깅
            logMessage("SIM", LOG_LEVEL_INFO, "=== VIRTUAL LCD RECEIVED MESSAGE ===");
            logMessage("SIM", LOG_LEVEL_INFO, "Message Count: %lu", msg_count);
            logMessage("SIM", LOG_LEVEL_INFO, "Message Type: 0x%02X", header->type);
            logMessage("SIM", LOG_LEVEL_INFO, "Message Length: %u", header->length);
            logMessage("SIM", LOG_LEVEL_INFO, "Sequence: %u", header->seq_num);
                        
            switch(header->type) {
                case MSG_DATA_BACKUP_REQUEST:
                    logMessage("SIM", LOG_LEVEL_INFO, "Processing backup request");
                    handle_virtual_backup_request(rx_msg.data, rx_msg.length);
                    break;
                    
                case MSG_DATA_BACKUP_DATA: {
                    BackupDataMessage* msg = (BackupDataMessage*)rx_msg.data;
                    logMessage("SIM", LOG_LEVEL_INFO, "Processing backup data chunk %d/%d (size: %d)", 
                             msg->chunk_index + 1, 
                             (virtual_lcd_state.total_backup_size + BACKUP_CHUNK_SIZE - 1) / BACKUP_CHUNK_SIZE,
                             msg->chunk_size);
                    logMessage("SIM", LOG_LEVEL_INFO, "Processing backup data chunk %u (size: %u)", 
                            msg->chunk_index, msg->chunk_size);
                    handle_virtual_backup_data(rx_msg.data, rx_msg.length);
                    break;
                }
                    
                case MSG_DATA_RESTORE_REQUEST:
                    handle_virtual_restore_request(rx_msg.data, rx_msg.length);
                    break;
                
                case MSG_DATA_RESTORE_ACK:
                    handle_virtual_restore_ack(rx_msg.data, rx_msg.length);
                    break;

                default:
                    logMessage("SIM", LOG_LEVEL_WARN, "Unknown message type: 0x%02X", header->type);
                    break;
            }
        } else {
            // 큐가 비어있을 때 로그 (10초마다)
            static uint32_t last_empty_log = 0;
            uint32_t now = millis();
            if (now - last_empty_log >= 10000) {
                logMessage("SIM", LOG_LEVEL_DEBUG, "No messages received for 10s (total received: %lu)", msg_count);
                last_empty_log = now;
            }
        }
        
        // CPU 점유율을 줄이기 위해 짧은 딜레이 추가
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// Backup handling functions
static void handle_virtual_backup_request(const uint8_t* data, size_t length) {
    BackupRequestMessage* req = (BackupRequestMessage*)data;
    
    logMessage("SIM", LOG_LEVEL_INFO, "Received backup request - Size: %u, Chunk size: %u", 
               req->total_size, req->chunk_size);

    // LittleFS 마운트 상태 재확인
    if (!LittleFS.begin(false)) {
        logMessage("SIM", LOG_LEVEL_ERROR, "LittleFS not mounted");
        send_virtual_ack(0, DATA_FLAG_ERROR);
        return;
    }

    // 파일 이름에 타임스탬프 사용
    char filename[MAX_FILENAME_LENGTH];
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    snprintf(filename, sizeof(filename), 
             MQTT_BACKUP_PATH  "/mqtt_backup_%04d%02d%02d_%02d%02d%02d.dat",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    // 기존 파일 정리
    if (virtual_lcd_state.current_file) {
        virtual_lcd_state.current_file.close();
    }

    // 디렉토리 존재 확인 및 생성
    if (!LittleFS.exists(VIRTUAL_SD_PATH)) {
        if (!LittleFS.mkdir(VIRTUAL_SD_PATH)) {
            logMessage("SIM", LOG_LEVEL_ERROR, "Failed to create virtual SD directory");
            send_virtual_ack(0, DATA_FLAG_ERROR);
            return;
        }
    }

    // 새 파일 생성
    File new_file = LittleFS.open(filename, "w");
    if (!new_file) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Failed to create backup file: %s", filename);
        send_virtual_ack(0, DATA_FLAG_ERROR);
        return;
    }

    // 상태 업데이트
    virtual_lcd_state.current_file = new_file;
    strncpy(virtual_lcd_state.current_filename, filename, MAX_FILENAME_LENGTH - 1);
    virtual_lcd_state.current_filename[MAX_FILENAME_LENGTH - 1] = '\0';
    virtual_lcd_state.total_backup_size = req->total_size;
    virtual_lcd_state.current_chunk = 0;
    virtual_lcd_state.is_backup_mode = true;
    
    logMessage("SIM", LOG_LEVEL_INFO, "Starting backup to file: %s", filename);
    send_virtual_ack(0, 0);
}

static void handle_virtual_backup_data(const uint8_t* data, size_t length) {
    BackupDataMessage* msg = (BackupDataMessage*)data;
    
    logMessage("SIM", LOG_LEVEL_INFO, "Backup data chunk %d - State: file:%d, mode:%d, expected:%d", 
              msg->chunk_index,
              (int)virtual_lcd_state.current_file,
              virtual_lcd_state.is_backup_mode,
              virtual_lcd_state.current_chunk);

    if (!virtual_lcd_state.current_file || !virtual_lcd_state.is_backup_mode) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Invalid backup state");
        send_virtual_ack(msg->chunk_index, DATA_FLAG_ERROR);
        return;
    }

    // chunk_index 검증 추가
    if (msg->chunk_index != virtual_lcd_state.current_chunk) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Unexpected chunk index: got %d, expected %d",
                  msg->chunk_index, virtual_lcd_state.current_chunk);
        send_virtual_ack(msg->chunk_index, DATA_FLAG_ERROR);
        return;
    }

    // 파일 쓰기
    size_t written = virtual_lcd_state.current_file.write(msg->data, msg->chunk_size);
    if (written != msg->chunk_size) {
        logMessage("SIM", LOG_LEVEL_ERROR, "File write failed: %d/%d bytes", written, msg->chunk_size);
        send_virtual_ack(msg->chunk_index, DATA_FLAG_ERROR);
        return;
    }

    // 즉시 ACK 전송
    logMessage("SIM", LOG_LEVEL_INFO, "Sending ACK for chunk %d", msg->chunk_index);
    send_virtual_ack(msg->chunk_index, 0);
    virtual_lcd_state.current_chunk++;

    // 마지막 청크 처리
    if (msg->flags & DATA_FLAG_END) {
        virtual_lcd_state.current_file.flush();
        virtual_lcd_state.current_file.close();
        virtual_lcd_state.is_backup_mode = false;
        logMessage("SIM", LOG_LEVEL_INFO, "Backup completed: %s", virtual_lcd_state.current_filename);
    }
}

// Restore handling functions
static void handle_virtual_restore_request(const uint8_t* data, size_t length) {
    if (virtual_lcd_state.current_file) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Previous operation not completed");
        send_virtual_ack(0, DATA_FLAG_ERROR);
        return;
    }

    char latest_backup[MAX_FILENAME_LENGTH];
    time_t backup_time = get_oldest_backup_file(latest_backup, MAX_FILENAME_LENGTH);
    
    if (backup_time == 0) {
        logMessage("SIM", LOG_LEVEL_ERROR, "No backup files found");
        send_virtual_ack(0, DATA_FLAG_ERROR);
        return;
    }

    virtual_lcd_state.current_file = LittleFS.open(latest_backup, "r");
    if (!virtual_lcd_state.current_file) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Failed to open backup file: %s", latest_backup);
        send_virtual_ack(0, DATA_FLAG_ERROR);
        return;
    }

    // 파일 유효성 체크 추가
    size_t file_size = virtual_lcd_state.current_file.size();
    logMessage("SIM", LOG_LEVEL_INFO, "Backup file size: %u bytes", file_size);

    if (file_size == 0) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Backup file is empty");
        virtual_lcd_state.current_file.close();
        send_virtual_ack(0, DATA_FLAG_ERROR);
        return;
    }

    // 파일 읽기 테스트
    uint8_t test_buffer[10];
    size_t bytes_read = virtual_lcd_state.current_file.read(test_buffer, sizeof(test_buffer));
    if (bytes_read == 0) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Cannot read from backup file");
        virtual_lcd_state.current_file.close();
        send_virtual_ack(0, DATA_FLAG_ERROR);
        return;
    }

    // 파일 포인터를 다시 처음으로
    virtual_lcd_state.current_file.seek(0);
    
    strncpy(virtual_lcd_state.current_filename, latest_backup, MAX_FILENAME_LENGTH);
    virtual_lcd_state.total_backup_size = virtual_lcd_state.current_file.size();
    virtual_lcd_state.current_chunk = 0;
    virtual_lcd_state.is_backup_mode = false;

    logMessage("SIM", LOG_LEVEL_INFO, "Starting restore from: %s", latest_backup);
    
    // Send first chunk
    handle_virtual_restore_data(0);
}
static void handle_virtual_restore_data(uint16_t chunk_index) {
    if (!virtual_lcd_state.current_file || virtual_lcd_state.is_backup_mode) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Invalid restore state");
        RestoreDataMessage msg = {0};
        msg.header.start_marker = 0xFF;
        msg.header.type = MSG_DATA_RESTORE_DATA;
        msg.header.timestamp = time(NULL);
        msg.header.seq_num = g_state.sequence_number++;
        msg.header.length = sizeof(RestoreDataMessage) - sizeof(MessageHeader);
        msg.chunk_index = chunk_index;
        msg.flags = DATA_FLAG_ERROR;
        msg.chunk_size = 0;
        msg.checksum = calculate_checksum(&msg, sizeof(RestoreDataMessage) - 1);

        uart_message_t uart_msg = {0};
        memcpy(uart_msg.data, &msg, sizeof(RestoreDataMessage));
        uart_msg.length = sizeof(RestoreDataMessage);
        xQueueSend(uart_rx_queue, &uart_msg, pdMS_TO_TICKS(100));
        return;
    }

    uint8_t buffer[RESTORE_CHUNK_SIZE];
    size_t remaining_size = virtual_lcd_state.total_backup_size - (chunk_index * RESTORE_CHUNK_SIZE);
    size_t bytes_to_read = (remaining_size < RESTORE_CHUNK_SIZE) ? remaining_size : RESTORE_CHUNK_SIZE;
    
    size_t bytes_read = virtual_lcd_state.current_file.read(buffer, bytes_to_read);
    logMessage("SIM", LOG_LEVEL_INFO, "Read %u bytes from file", bytes_read);

    if (bytes_read == 0) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Failed to read restore data");
        RestoreDataMessage msg = {0};
        msg.header.start_marker = 0xFF;
        msg.header.type = MSG_DATA_RESTORE_DATA;
        msg.header.timestamp = time(NULL);
        msg.header.seq_num = g_state.sequence_number++;
        msg.header.length = sizeof(RestoreDataMessage) - sizeof(MessageHeader);
        msg.chunk_index = chunk_index;
        msg.flags = DATA_FLAG_ERROR;
        msg.chunk_size = 0;
        msg.checksum = calculate_checksum(&msg, sizeof(RestoreDataMessage) - 1);

        uart_message_t uart_msg = {0};
        memcpy(uart_msg.data, &msg, sizeof(RestoreDataMessage));
        uart_msg.length = sizeof(RestoreDataMessage);
        xQueueSend(uart_rx_queue, &uart_msg, pdMS_TO_TICKS(100));
        
        virtual_lcd_state.current_file.close();
        return;
    }

    RestoreDataMessage msg = {0};
    msg.header.start_marker = 0xFF;
    msg.header.type = MSG_DATA_RESTORE_DATA;
    msg.header.timestamp = time(NULL);
    msg.header.seq_num = g_state.sequence_number++;
    msg.header.length = sizeof(RestoreDataMessage) - sizeof(MessageHeader);
    msg.chunk_index = chunk_index;
    msg.chunk_size = bytes_read;
    memcpy(msg.data, buffer, bytes_read);

    if ((chunk_index + 1) * RESTORE_CHUNK_SIZE >= virtual_lcd_state.total_backup_size) {
        msg.flags |= DATA_FLAG_END;
    }

    msg.checksum = calculate_checksum(&msg, sizeof(RestoreDataMessage) - 1);

    uart_message_t uart_msg = {0};
    memcpy(uart_msg.data, &msg, sizeof(RestoreDataMessage));
    uart_msg.length = sizeof(RestoreDataMessage);

    if (xQueueSend(uart_rx_queue, &uart_msg, pdMS_TO_TICKS(100)) != pdPASS) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Failed to send restore data");
    }
}

static void handle_virtual_restore_ack(const uint8_t* data, size_t length) {
    RestoreAckMessage* ack = (RestoreAckMessage*)data;
    
    if (!virtual_lcd_state.current_file || virtual_lcd_state.is_backup_mode) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Invalid restore state");
        return;
    }

    if (ack->status != 0) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Restore error reported by main board");
        virtual_lcd_state.current_file.close();
        return;
    }

    if (ack->chunk_index != virtual_lcd_state.current_chunk) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Restore chunk index mismatch");
        return;
    }

    virtual_lcd_state.current_chunk++;
    
    // Check if we need to send more data
    if (virtual_lcd_state.current_chunk * BACKUP_CHUNK_SIZE < virtual_lcd_state.total_backup_size) {
        handle_virtual_restore_data(virtual_lcd_state.current_chunk);
    } else {
        logMessage("SIM", LOG_LEVEL_INFO, "Restore completed: %s", virtual_lcd_state.current_filename);
        virtual_lcd_state.current_file.close();
    }
}

// Utility functions
static time_t parseBackupTimestamp(const String& timestamp) {
    // _YYYYMMDD_HHMMSS 형식 파싱
    struct tm tm = {0};
    int year, month, day, hour, min, sec;
    
    if (sscanf(timestamp.c_str(), "_%4d%2d%2d_%2d%2d%2d",
               &year, &month, &day, &hour, &min, &sec) == 6) {
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = min;
        tm.tm_sec = sec;
        return mktime(&tm);
    }
    return 0;
}

static time_t get_oldest_backup_file(char* filename, size_t max_length) {
    File root = LittleFS.open(MQTT_BACKUP_PATH);  // 변경: VIRTUAL_SD_PATH -> MQTT_BACKUP_PATH
    if (!root || !root.isDirectory()) {
        logMessage("SIM", LOG_LEVEL_ERROR, "Failed to open backup directory");
        return 0;
    }

    time_t oldest_time = 0;
    String oldest_fname;
    File file;
    
    while (file = root.openNextFile()) {
        String fname = String(file.name());
        size_t fsize = file.size();
        file.close();
        
        if (fname.startsWith("mqtt_backup_") && fsize > 0) {
            if (oldest_time == 0 || fname < oldest_fname) {
                oldest_time = 1;
                oldest_fname = fname;
                logMessage("SIM", LOG_LEVEL_DEBUG, "New oldest file found: %s", fname.c_str());
            }
        }
    }
    root.close();

    if (oldest_time > 0) {
        // 전체 경로를 포함하여 파일명 생성
        snprintf(filename, max_length, "%s/%s", MQTT_BACKUP_PATH, oldest_fname.c_str());  // 변경
        logMessage("SIM", LOG_LEVEL_INFO, "Selected backup file: %s", filename);
        return oldest_time;
    }

    logMessage("SIM", LOG_LEVEL_ERROR, "No valid backup files found");
    return 0;
}

static bool init_virtual_storage(void) {
    // LittleFS 초기화
    if (!LittleFS.begin(true)) {  // true: 포맷 옵션 활성화
        logMessage("SIM", LOG_LEVEL_ERROR, "Failed to mount LittleFS");
        return false;
    }

    // 가상 SD 경로 생성
    if (!LittleFS.exists(VIRTUAL_SD_PATH)) {
        if (!LittleFS.mkdir(VIRTUAL_SD_PATH)) {
            logMessage("SIM", LOG_LEVEL_ERROR, "Failed to create virtual SD directory");
            return false;
        }
    }

    virtual_lcd_state.sd_mounted = true;
    logMessage("SIM", LOG_LEVEL_INFO, "Virtual storage initialized successfully");
    return true;
}
// ACK 메시지 전송
static void send_virtual_ack(uint16_t chunk_index, uint8_t status) {
    BackupAckMessage ack = {0};
    
    // 헤더 설정
    ack.header.start_marker = 0xFF;
    ack.header.type = MSG_DATA_BACKUP_ACK;
    ack.header.timestamp = time(NULL);
    ack.header.seq_num = g_state.sequence_number++;
    ack.header.length = sizeof(BackupAckMessage) - sizeof(MessageHeader);
    
    ack.chunk_index = chunk_index;
    ack.status = status;
    ack.checksum = calculate_checksum(&ack, sizeof(BackupAckMessage) - 1);

    logMessage("SIM", LOG_LEVEL_INFO, "Sending ACK - Size:%u, ExpectedSize:%u", 
               sizeof(BackupAckMessage), sizeof(ack));

    // ACK 전송 시도 (최대 3번)
    for (int retry = 0; retry < 3; retry++) {
        uart_message_t msg = {0};
        memcpy(msg.data, &ack, sizeof(BackupAckMessage));  // 명시적으로 BackupAckMessage 크기 사용
        msg.length = sizeof(BackupAckMessage);             // 명시적으로 BackupAckMessage 크기 사용
        
        if (xQueueSend(uart_rx_queue, &msg, pdMS_TO_TICKS(100)) == pdPASS) {
            logMessage("SIM", LOG_LEVEL_INFO, "ACK sent for chunk %d (retry: %d, size: %u)", 
                      chunk_index, retry, msg.length);
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    logMessage("SIM", LOG_LEVEL_ERROR, "Failed to send ACK for chunk %d after 3 retries", 
              chunk_index);
}
//#endif //#ifdef USE_UART_SIMULATION
#endif