#include "lvgl.h"
#include <Arduino.h>
#include "pins_arduino.h"
#include "FS.h"
#include <time.h>
//#include <WiFi.h>
#include <ctype.h>

static void setup_ui();
static void rotate_callback(void *var, int32_t v);

LV_IMG_DECLARE(img_power_off);
LV_IMG_DECLARE(img_power_on);

#define LV_USE_IMG_TRANSFORM 1
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_TRACE

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

    lv_img_set_pivot(icon_img, 0, 0); // 좌상단 기준 회전
    
    // 간단한 애니메이션 효과 추가
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, icon_img);
    //lv_anim_set_values(&a, 0, 256);
    //lv_anim_set_time(&a, 500);
    //lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_img_opa);
    //lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

    lv_anim_set_values(&a, 0, 900); // 0도에서 360도 (10x 정확도)
    lv_anim_set_time(&a, 3000);      // 3초 동안 회전
    //lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {    lv_img_set_angle(static_cast<lv_obj_t *>(var), v);    });
    lv_anim_set_exec_cb(&a, rotate_callback);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

void rotate_callback(void *var, int32_t v) {
    lv_img_set_angle(static_cast<lv_obj_t *>(var), v);
}

void setup_ui() {

    Serial.println("+++ ON setup_ui..");

    // 화면 생성
    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    
    // 이미지 객체 생성
    icon_img = lv_img_create(scr);
    
    // 이미지 중앙 정렬
    //lv_obj_center(icon_img);
    lv_obj_set_pos(icon_img, 340, 15);

    
    // 초기 이미지 설정
    //lv_img_set_src(icon_img, icons[0]);
    
    // 크기 설정 (예시 - 실제 아이콘 크기에 맞게 조정 필요)
    //lv_img_set_zoom(icon_img, 256);  // 기본 크기의 256%
    
    // 스타일 설정
    static lv_style_t style_img;
    lv_style_init(&style_img);
    lv_style_set_img_recolor_opa(&style_img, 0);
    lv_obj_add_style(icon_img, &style_img, 0);
    
    // 2초마다 아이콘 변경하는 타이머 설정
    //lv_timer_create(icon_change_timer_cb, 100, NULL);

    if (current_icon==1) {
            current_icon = current_icon - 1;       
        } else {
            current_icon = 1;
        }

    //current_icon = (current_icon + 1) % 2;

    Serial.println("+++ icon_change_timer_cb 222");
    
    // 페이드 효과로 이미지 변경
    lv_img_set_src(icon_img, icons[current_icon]);

    //lv_img_set_pivot(icon_img, 0, 0); // 좌상단 기준 회전
    //lv_img_set_pivot(icon_img, 25, 25); // 좌상단 기준 회전
    lv_img_set_pivot(icon_img, 30/2, 30/2); // 좌상단 기준 회전

    // 간단한 애니메이션 효과 추가
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, icon_img);
    //lv_anim_set_values(&a, 0, 256);
    //lv_anim_set_time(&a, 500);
    //lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_img_opa);
    //lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

    lv_anim_set_values(&a, 0, 900); // 0도에서 360도 (10x 정확도)
    lv_anim_set_time(&a, 3000);      // 3초 동안 회전
    //lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {    lv_img_set_angle(static_cast<lv_obj_t *>(var), v);    });
    lv_anim_set_exec_cb(&a, rotate_callback);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
    
}

void blinkON() {
    
    // UI 설정
    Serial.println("+++ setup_ui before");
    
    setup_ui();
    
}

