#include <lvgl.h>
#include <rtc.h>
#include <time.h>


static lv_obj_t* label_date;
static lv_obj_t* label_time;

static void update_clock(void) {
    struct tm* local_time;
    time_t current_time;

    // RTC로부터 현재 시간 정보 획득
    current_time = rtc_get_time();
    local_time = localtime(&current_time);

    // 연월일, 요일 표시 업데이트
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%Y/%m/%d (%a)", local_time);
    lv_label_set_text(label_date, date_str);

    // 시분초 표시 업데이트  
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", local_time);
    lv_label_set_text(label_time, time_str);
}

static void create_clock(void) {
    // 날짜 표시 라벨 생성
    label_date = lv_label_create(lv_scr_act());
    lv_obj_set_pos(label_date, 10, 10);
    lv_label_set_text(label_date, "");

    // 시간 표시 라벨 생성
    label_time = lv_label_create(lv_scr_act());
    lv_obj_set_pos(label_time, 10, 40);
    lv_label_set_text(label_time, "");

    // 1초 마다 시계 업데이트
    lv_timer_create(update_clock, 1000, NULL);
}

void rtc_time(void) {
    //lv_init();
    create_clock();    
}