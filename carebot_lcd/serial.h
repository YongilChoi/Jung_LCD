#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define QUEUE_SIZE 10
extern QueueHandle_t serialQueue; //// 메시지 큐 핸들

extern int debug_serial_flag;
void serialTask(void *parameter);
void handleTask(void *parameter);
void send_factory_init();
