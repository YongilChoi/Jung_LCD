#if 0
//#ifndef UART_SIMULATION_H
//#define UART_SIMULATION_H
//#ifdef USE_UART_SIMULATION
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "common.h"
#include "mqtt_persistence.h"
#include <time.h>

// 시뮬레이션 설정
#define MAX_MSG_SIZE 2048  // 최대 메시지 크기
#define VIRTUAL_SD_PATH "/virtual_sd"  // 가상 SD 카드 경로
#define SIMULATION_TASK_DELAY_MS 10
#define SIMULATION_QUEUE_TIMEOUT_MS 100
#define MAX_FILENAME_LENGTH 64

typedef struct {
    uint8_t data[MAX_MSG_SIZE];
    size_t length;
} uart_message_t;


//extern bool init_uart_simulation();
void deinit_uart_simulation(void);
bool send_simulated_message(const uint8_t* data, size_t length);
size_t receive_simulated_message(uint8_t* buffer, size_t max_length);

//#endif // UART_SIMULATION_H
//#endif // #ifdef USE_UART_SIMULATION
#endif