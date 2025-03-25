#include "lvgl.h"
#include <Arduino.h>
#include "pins_arduino.h"
#include "FS.h"
#include <time.h>
//#include <WiFi.h>
#include <ctype.h>

static void setup_ui();

LV_IMG_DECLARE(img_power_off);
LV_IMG_DECLARE(img_power_on);

static lv_obj_t* icon_img;  // 이미지 객체
static uint8_t current_icon = 0;  // 현재 표시중인 아이콘 인덱스

// 아이콘 배열
static const lv_img_dsc_t* icons[] = {
    &img_power_off,
    &img_power_on,
    &img_power_off
};

// 타이머 콜백 함수
static void icon_change_timer_cb(lv_timer_t* timer) {
    if (current_icon==1) {
            current_icon = current_icon - 1;       
        } else {
            current_icon = 1;
        }

    //current_icon = (current_icon + 1) % 2;

    Serial.println("+++ icon_change_timer_cb");
    
    // 페이드 효과로 이미지 변경
    lv_img_set_src(icon_img, icons[current_icon]);
    
    // 간단한 애니메이션 효과 추가
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, icon_img);
    lv_anim_set_values(&a, 0, 256);
    lv_anim_set_time(&a, 500);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_img_opa);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

void setup_ui() {
    Serial.println("+++ OFF setup_ui..");

    // 화면 생성
    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    
    // 이미지 객체 생성
    icon_img = lv_img_create(scr);
    
    // 이미지 중앙 정렬
    //lv_obj_center(icon_img);
    lv_obj_set_pos(icon_img, 10, 10);

    lv_img_set_src(icon_img, &img_power_off);
    
    // 간단한 애니메이션 효과 추가
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, icon_img);
    lv_anim_set_values(&a, 0, 256);
    lv_anim_set_time(&a, 500);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_img_opa);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);

}

void blinkOFF() {
    // LVGL 초기화
    //lv_init();
    
    // 디스플레이 드라이버 초기화 (실제 환경에 맞게 수정 필요)
    //display_driver_init();
    
    // UI 설정
    Serial.println("+++ OFF setup_ui before");
    
    setup_ui();
   
}

