#include "common.h"
#include "lvgl_controller.h"
#include "event.h"
#include "menu.h"

// 애니메이션 각도 상태를 추적하기 위한 전역 변수
volatile int hygera_angle = 0;
volatile int motor_angle = 0;
volatile bool animation_running = false;
SemaphoreHandle_t animation_mutex = NULL;

lv_timer_t *wifi_scan_timer = NULL;
bool timer_callback_in_progress = false;  // 타이머 콜백 실행 중 플래그

// 애니메이션 업데이트 태스크
void animationTask(void *pvParameters) {
    const int increment = 10;  // 한 번에 증가시킬 각도
    const int max_angle = 3600; // 최대 각도
    const TickType_t delay_time = pdMS_TO_TICKS(50); // 20fps에 해당
    
    while(1) {
        // 애니메이션이 실행 중인지 확인
        if (animation_running) {
            // 뮤텍스 획득
            if (xSemaphoreTake(animation_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                // 각도 업데이트
                hygera_angle = (hygera_angle + increment) % max_angle;
                motor_angle = (motor_angle + increment) % max_angle;
                
                // 뮤텍스 해제
                xSemaphoreGive(animation_mutex);
            }
        }
        
        // 작업 주기 제어를 위한 딜레이
        vTaskDelay(delay_time);
    }
}

// 애니메이션 렌더링 함수 (메인 UI 스레드에서 주기적으로 호출)
void updateAnimations() {
    if (!animation_running) return;
    
    // 뮤텍스 획득
    if (xSemaphoreTake(animation_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        // 현재 각도로 이미지 업데이트
        if (hygera != NULL && motor_ON) {
            lv_img_set_angle(hygera, hygera_angle);
        }
        
        if (motor != NULL && motor_ON) {
            lv_img_set_angle(motor, motor_angle);
        }
        
        // 뮤텍스 해제
        xSemaphoreGive(animation_mutex);
    }
}

// 초기화 함수에서 애니메이션 태스크 시작
void setupAnimationTask() {
    // 뮤텍스 생성
    animation_mutex = xSemaphoreCreateMutex();
    
    // 태스크 생성
    xTaskCreate(
        animationTask,       // 태스크 함수
        "AnimationTask",     // 태스크 이름
        4096,                // 스택 크기
        NULL,                // 태스크 파라미터
        1,                   // 낮은 우선순위 (UI 태스크보다 낮게)
        NULL                 // 태스크 핸들 저장 안 함
    );
}
void Event_motor_ON() {
    Serial.println("+++ Event_motor_ON");
    if (menu_ON == 1) {
      Serial.println("+++ Event_motor_ON menu_on == 1 return");
      return;
    }
   
    motor_ON = 1;

    // hygera 객체 관리
    if (hygera == NULL) {
        Serial.println("+++ Creating hygera object");
        hygera = lv_img_create(panel0);
        if (hygera == NULL) {
            Serial.println("+++ Failed to create image object hygera");
            return;
        }
    }

    // 객체 속성 설정
    lv_img_set_src(hygera, &img_hygera);
    lv_obj_set_pos(hygera, 10, 25);
    lv_img_set_pivot(hygera, 30/2, 30/2); // 좌상단 기준 회전

    // motor 객체 관리
    if (motor == NULL) {
        Serial.println("+++ Creating motor object");
        motor = lv_img_create(panel0);
        if (motor == NULL) {
            Serial.println("+++ Failed to create image object motor");
            return;
        }
    }
   
    lv_img_set_src(motor, &img_motor_off);
    lv_obj_set_pos(motor, 30, 70);    
    lv_img_set_pivot(motor, 50/2, 50/2); 

    // 애니메이션 시작 - 별도의 태스크로
    if (xSemaphoreTake(animation_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        hygera_angle = 0;
        motor_angle = 0;
        animation_running = true;
        xSemaphoreGive(animation_mutex);
    }
}

void Event_motor_OFF() {
    Serial.println("+++ Event_motor_OFF");
    if (menu_ON == 1) {
      return;
    }	

    motor_ON = 0;
    
    // 애니메이션 중지
    if (xSemaphoreTake(animation_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        animation_running = false;
        xSemaphoreGive(animation_mutex);
    }
    
    // hygera 객체 관리
    if (hygera == NULL) {
        Serial.println("+++ Creating hygera object");
        hygera = lv_img_create(panel0);
        if (hygera == NULL) {
            Serial.println("+++ Failed to create image object hygera");
            return;
        }
    }
   
    // 객체 속성 설정
    lv_img_set_src(hygera, &img_hygera);
    lv_obj_set_pos(hygera, 10, 25);
    // 각도 초기화
    lv_img_set_angle(hygera, 0);
	
    // motor 객체 관리
    if (motor == NULL) {
        Serial.println("+++ Creating motor object");
        motor = lv_img_create(panel0);
        if (motor == NULL) {
            Serial.println("+++ Failed to create image object motor");
            return;
        }
    }

    // 객체 속성 설정
    lv_img_set_src(motor, &img_motor_off);
    lv_obj_set_pos(motor, 30, 70);
    // 각도 초기화
    lv_img_set_angle(motor, 0);
}
void Event_error_ON() {
    Serial.println("+++ Event_error_ON");
    if (menu_ON == 1) {
      return;
    }

    // error 객체 관리 - 전역 객체 사용
    if (error == NULL) {
        error = lv_img_create(panel0);
        if (error == NULL) {
            Serial.println("+++ Failed to create image object error");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.error_state = true;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);   
}

void Event_error_OFF() {
    Serial.println("+++ Event_error_OFF");
    if (menu_ON == 1) {
      return;
    }

    // error 객체 관리 - 전역 객체 사용
    if (error == NULL) {
        error = lv_img_create(panel0);
        if (error == NULL) {
            Serial.println("+++ Failed to create image object error");
            return;
        }
    }
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.error_state = false;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);   
}

void Event_feces_ON() {
    Serial.println("+++ Event_feces_ON");
    if (menu_ON == 1) {
      return;
    }
    
    // feces 객체 관리
    if (feces == NULL) {
        Serial.println("+++ Creating feces object");
        feces = lv_img_create(panel0);
        if (feces == NULL) {
            Serial.println("+++ Failed to create image object feces");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.feces_state = true;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);    
}

void Event_feces_OFF() {
    Serial.println("+++ Event_feces_OFF");
    if (menu_ON == 1) {
      return;
    }
    
    // feces 객체 관리
    if (feces == NULL) {
        Serial.println("+++ Creating feces object");
        feces = lv_img_create(panel0);
        if (feces == NULL) {
            Serial.println("+++ Failed to create image object feces");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.feces_state = false;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);   
}

void Event_connect_ON() {
    Serial.println("+++ Event_connect_ON");
    if (menu_ON == 1) {
        return;
    }

    // connect 객체 관리
    if (connect == NULL) {
        Serial.println("+++ Creating connect object");
        connect = lv_img_create(panel0);
        if (connect == NULL) {
            Serial.println("+++ Failed to create image object connect");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.connect_state = true;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);
}

void Event_connect_OFF() {
    Serial.println("+++ Event_connect_OFF");
    if (menu_ON == 1) {
        return;
    }
    
    // connect 객체 관리
    if (connect == NULL) {
        Serial.println("+++ Creating connect object");
        connect = lv_img_create(panel0);
        if (connect == NULL) {
            Serial.println("+++ Failed to create image object connect");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.connect_state = false;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);
}

void Event_diaper_ON() {
    Serial.println("+++ Event_diaper_ON");
    if (menu_ON == 1) {
        return;
    }
    
    // diaper 객체 관리
    if (diaper == NULL) {
        diaper = lv_img_create(panel0);
        if (diaper == NULL) {
            Serial.println("+++ Failed to create image object diaper");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.diaper_state = true;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);
}

void Event_diaper_OFF() {
    Serial.println("+++ Event_diaper_OFF");
    if (menu_ON == 1) {
        return;
    }
    
    // diaper 객체 관리
    if (diaper == NULL) {
        diaper = lv_img_create(panel0);
        if (diaper == NULL) {
            Serial.println("+++ Failed to create image object diaper");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.diaper_state = false;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);
}
void Event_tank_ON(float water_level) {
    Serial.println("+++ Event_tank_ON, water_level: ");
    Serial.println(water_level);
    
    // 메뉴 모드이면 스킵
    if (menu_ON == 1) {
        return;
    }
    
    // 수위 값이 기존 값과 크게 다르지 않으면 업데이트 스킵 (옵션)
    // 작은 변동은 무시하여 불필요한 UI 업데이트 방지
    if (abs(current_ui_state.water_level - water_level) < 10.0f) {
        return;
    }
    
    // 현재 UI 상태 업데이트
    current_ui_state.water_level = water_level;
    ui_update_requested = true;
    
    // tank 객체 관리
    if (tank == NULL) {
        // 객체가 없는 경우에만 새로 생성
        tank = lv_bar_create(panel0);
        if (tank == NULL) {
            Serial.println("+++ Failed to create tank bar object");
            return;
        }
        
        // 기본 설정은 한 번만 수행
        lv_obj_remove_style_all(tank);
        lv_obj_set_size(tank, 80, 175);
        lv_obj_set_pos(tank, 370, 75);
        lv_bar_set_range(tank, 0, 2200); // 최대 수위 2200 ml 가정
        lv_obj_set_style_radius(tank, 0, 0);
        lv_obj_set_style_radius(tank, 0, LV_PART_INDICATOR);
        lv_bar_set_mode(tank, LV_BAR_MODE_SYMMETRICAL);
        lv_obj_set_style_border_color(tank, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(tank, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(tank, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(tank, LV_OPA_100, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    
    // 수위에 따른 색상 설정
    if (water_level >= 2000) {
        lv_obj_set_style_bg_color(tank, lv_color_hex(0xFF0000), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    else if (water_level >= 1300 && water_level < 2000) {
        lv_obj_set_style_bg_color(tank, lv_color_hex(0xFFD400), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    else if (water_level < 1300) {
        lv_obj_set_style_bg_color(tank, lv_color_hex(0x87ceeb), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    
    // 수위 값 설정
    lv_bar_set_value(tank, water_level, LV_ANIM_ON);
    lv_obj_set_style_bg_opa(tank, LV_OPA_100, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    
    // water_level_label 객체 관리
    if (water_level_label == NULL) {
        // 레이블이 없는 경우에만 새로 생성
        water_level_label = lv_label_create(panel0);
        if (water_level_label == NULL) {
            Serial.println("+++ Failed to create water_level_label");
            return;
        }
        
        // 레이블 스타일 설정 (한 번만)
        static lv_style_t style_label;
        lv_style_init(&style_label);
        lv_style_set_bg_opa(&style_label, LV_OPA_100);
        lv_obj_add_style(water_level_label, &style_label, 0);
        
        // 레이블 크기 설정 (한 번만)
        lv_obj_set_width(water_level_label, 65);
        lv_obj_set_height(water_level_label, 25);
    }
    
    // 레이블 텍스트 업데이트
    char water_level_text[20];
    sprintf(water_level_text, "%.0f ml", water_level);
    lv_label_set_text(water_level_label, water_level_text);
    lv_obj_align_to(water_level_label, tank, LV_ALIGN_TOP_MID, 0, 10);
    
    // 화면 강제 갱신
    lv_obj_invalidate(tank);
    lv_obj_invalidate(water_level_label);
}

void Event_power_ON() {
    Serial.println("+++ Event_power_ON");
    if (menu_ON == 1) {
      return;
    }

    // power 객체 관리
    if (power == NULL) {
        Serial.println("+++ Creating power object");
        power = lv_img_create(panel0);
        if (power == NULL) {
            Serial.println("+++ Failed to create image object power");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.power_state = true;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);
}

void Event_power_OFF() {
    Serial.println("+++ Event_power_OFF");
    if (menu_ON == 1) {
      return;
    }

    // power 객체 관리
    if (power == NULL) {
        Serial.println("+++ Creating power object");
        power = lv_img_create(panel0);
        if (power == NULL) {
            Serial.println("+++ Failed to create image object power");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.power_state = false;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);   
}

void Event_wifi_ON() {
    Serial.println("+++ Event_wifi_ON");
    if (menu_ON == 1) {
      return;
    }

    // wifi 객체 관리
    if (wifi == NULL) {
        Serial.println("+++ Creating wifi object");
        wifi = lv_img_create(panel0);
        if (wifi == NULL) {
            Serial.println("+++ Failed to create image object wifi");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.wifi_state = true;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);

    // 이벤트 콜백 및 속성 설정
    lv_obj_add_event_cb(wifi, img_click_event_cb_wifi, LV_EVENT_CLICKED, NULL);  
    lv_obj_add_flag(wifi, LV_OBJ_FLAG_CLICKABLE); 
    lv_obj_set_style_anim_time(wifi, 0, LV_STATE_PRESSED);  // 애니메이션 제거   
}

void Event_wifi_OFF() {
    Serial.println("+++ Event_wifi_OFF");
    if (menu_ON == 1) {
      return;
    }
    
    // wifi 객체 관리
    if (wifi == NULL) {
        Serial.println("+++ Creating wifi object");
        wifi = lv_img_create(panel0);
        if (wifi == NULL) {
            Serial.println("+++ Failed to create image object wifi");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.wifi_state = false;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);  

    // 이벤트 콜백 및 속성 설정
    lv_obj_add_event_cb(wifi, img_click_event_cb_wifi, LV_EVENT_CLICKED, NULL);  
    lv_obj_add_flag(wifi, LV_OBJ_FLAG_CLICKABLE); 
    lv_obj_set_style_anim_time(wifi, 0, LV_STATE_PRESSED);  // 애니메이션 제거  
}

void Event_bat_ON() {
    Serial.println("+++ Event_bat_ON");
    if (menu_ON == 1) {
      return;
    }
    
    // bat 객체 관리
    if (bat == NULL) {
        Serial.println("+++ Creating bat object");
        bat = lv_img_create(panel0);
        if (bat == NULL) {
            Serial.println("+++ Failed to create image object bat");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.bat_state = 1;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);   
}

void Event_bat_OFF() {
    Serial.println("+++ Event_bat_OFF");
    if (menu_ON == 1) {
      return;
    }
    
    // bat 객체 관리
    if (bat == NULL) {
        Serial.println("+++ Creating bat object");
        bat = lv_img_create(panel0);
        if (bat == NULL) {
            Serial.println("+++ Failed to create image object bat");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.bat_state = 0;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);
}

void Event_bat_LOW() {
    Serial.println("+++ Event_bat_LOW");
    if (menu_ON == 1) {
      return;
    }
    
    // bat 객체 관리
    if (bat == NULL) {
        Serial.println("+++ Creating bat object");
        bat = lv_img_create(panel0);
        if (bat == NULL) {
            Serial.println("+++ Failed to create image object bat");
            return;
        }
    }
   
    // 상태 업데이트 - 직접 UI 조작 대신 상태만 변경
    current_ui_state.bat_state = 2;
    ui_update_requested = true;
    
    // 화면 강제 갱신 요청
    lv_obj_invalidate(panel0);
}
void waiting_event_handler_2(char* status) {
    Serial.println("+++ BEGIN waiting_event_handler_2");
    
    // 10초 후 자동 닫기를 위한 타이머 생성
    lv_timer_create([](lv_timer_t * timer) {
        Serial.println("+++ 0 in waiting_event_handler_2");
        // alert_msgbox 제거
        if (alert_msgbox != NULL) {
            lv_obj_del(alert_msgbox);
            alert_msgbox = NULL;
        }
        lv_timer_del(timer);  // 타이머 삭제하여 1회만 실행                  
    }, 10000, NULL);  // 10000ms (10초) 후 실행
          
    // 1. 메시지 박스 상단 이미지 준비
    Serial.println("+++ 1 waiting_event_handler_2");
    msg_top_img_check = lv_img_create(lv_scr_act());
    if (msg_top_img_check == NULL) {
        Serial.println("+++ Failed to create msg_top_img_check");
        return;
    }
    
    lv_img_set_src(msg_top_img_check, &img_msgbox_check);

    // 3. 사용자 지정 메시지 박스 생성
    alert_msgbox = lv_obj_create(lv_scr_act());
    if (alert_msgbox == NULL) {
        Serial.println("+++ Failed to create alert_msgbox");
        return;
    }
    
    lv_obj_set_size(alert_msgbox, 300, 150);
    lv_obj_align(alert_msgbox, LV_ALIGN_CENTER, 0, 0);

    // 4. msg_top_img를 alert 메시지 박스의 상단에 추가
    lv_obj_set_parent(msg_top_img_check, alert_msgbox);
    lv_obj_align(msg_top_img_check, LV_ALIGN_TOP_MID, 0, 5);

    Serial.println("+++ 2 waiting_event_handler_2");

    // 메시지 라벨 생성
    msg_label = lv_label_create(alert_msgbox);
    if (msg_label == NULL) {
        Serial.println("+++ Failed to create msg_label");
        return;
    }
    
    lv_label_set_text(msg_label, status);
    lv_obj_center(msg_label);
    
    Serial.println("+++ END waiting_event_handler_2");
}
/**
 * 알림 대화상자 생성 함수
 * 
 * @param message 표시할 메시지
 * @param confirm_callback 확인 버튼 클릭 시 호출될 콜백 함수
 * @param auto_close_ms 자동 닫힘 시간(밀리초), 0이면 자동 닫힘 비활성화
 * @return 성공 여부
 */
bool create_alert_dialog(const char* message, lv_event_cb_t confirm_callback, uint32_t auto_close_ms = 0) {
    Serial.println("+++ create_alert_dialog 시작");
   
    // 이미 존재하는 알림 대화상자 정리
    if (alert_msgbox != NULL) {
        lv_obj_del(alert_msgbox);
        alert_msgbox = NULL;
        msg_top_img_check = NULL;
        close_btn_img = NULL;
        msg_label = NULL;
        btn_hema = NULL;
        label_hema = NULL;
        
        // 약간의 지연을 추가하여 LVGL이 객체를 완전히 삭제할 시간을 줍니다
        delay(10);
    }
    
    // 화면이 초기화되었는지 확인
    if (lv_scr_act() == NULL) {
        Serial.println("+++ 활성화된 화면이 없어 알림 생성 취소");
        return false;
    }
    
    // 자동 종료 타이머 설정 (요청된 경우)
    if (auto_close_ms > 0) {
        lv_timer_create([](lv_timer_t * timer) {
            // alert_msgbox 제거
            if (alert_msgbox != NULL) {
                lv_obj_del(alert_msgbox);
                alert_msgbox = NULL;
                msg_top_img_check = NULL;
                close_btn_img = NULL;
                msg_label = NULL;
                btn_hema = NULL;
                label_hema = NULL;
            }
            lv_timer_del(timer);  // 타이머 삭제하여 1회만 실행
        }, auto_close_ms, NULL);
    }
    
    // 1. 메시지 박스 상단 이미지 준비
    msg_top_img_check = lv_img_create(lv_scr_act());
    if (msg_top_img_check == NULL) {
        Serial.println("+++ 상단 이미지 생성 실패");
        return false;
    }
    
    lv_img_set_src(msg_top_img_check, &img_msgbox_check);

    // 2. 닫기 버튼 이미지 준비
    close_btn_img = lv_img_create(lv_scr_act());
    if (close_btn_img == NULL) {
        Serial.println("+++ 닫기 버튼 이미지 생성 실패");
        if (msg_top_img_check != NULL) {
            lv_obj_del(msg_top_img_check);
            msg_top_img_check = NULL;
        }
        return false;
    }
    
    lv_img_set_src(close_btn_img, &img_close_btn);
    lv_obj_add_flag(close_btn_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_anim_time(close_btn_img, 0, LV_STATE_PRESSED);

    // 3. 사용자 지정 메시지 박스 생성
    alert_msgbox = lv_obj_create(lv_scr_act());
    if (alert_msgbox == NULL) {
        Serial.println("+++ 알림 메시지 박스 생성 실패");
        if (msg_top_img_check != NULL) {
            lv_obj_del(msg_top_img_check);
            msg_top_img_check = NULL;
        }
        if (close_btn_img != NULL) {
            lv_obj_del(close_btn_img);
            close_btn_img = NULL;
        }
        return false;
    }
    
    lv_obj_set_size(alert_msgbox, 300, 150);
    lv_obj_align(alert_msgbox, LV_ALIGN_CENTER, 0, 0);

    // 4. msg_top_img를 alert 메시지 박스의 상단에 추가
    lv_obj_set_parent(msg_top_img_check, alert_msgbox);
    lv_obj_align(msg_top_img_check, LV_ALIGN_TOP_MID, 0, 5);

    // 5. 닫기 버튼을 alert 메시지 박스의 우측 상단에 추가
    lv_obj_set_parent(close_btn_img, alert_msgbox);
    lv_obj_align(close_btn_img, LV_ALIGN_TOP_RIGHT, 10, -10);

    // 메시지 라벨 생성
    msg_label = lv_label_create(alert_msgbox);
    if (msg_label == NULL) {
        Serial.println("+++ 메시지 라벨 생성 실패");
        lv_obj_del(alert_msgbox);
        alert_msgbox = NULL;
        msg_top_img_check = NULL;
        close_btn_img = NULL;
        return false;
    }
    
    // 메시지 설정
    lv_label_set_text(msg_label, message);
    lv_obj_center(msg_label);

    // 6. 닫기 버튼 이벤트 핸들러 등록
    lv_obj_add_event_cb(close_btn_img, msg_close_handler_parent, LV_EVENT_CLICKED, NULL);

    // 7. 확인 버튼 등록
    btn_hema = lv_btn_create(alert_msgbox);
    if (btn_hema == NULL) {
        Serial.println("+++ 확인 버튼 생성 실패");
        lv_obj_del(alert_msgbox);
        alert_msgbox = NULL;
        msg_top_img_check = NULL;
        close_btn_img = NULL;
        msg_label = NULL;
        return false;
    }
    
    lv_obj_set_size(btn_hema, 50, 25);
    lv_obj_align(btn_hema, LV_ALIGN_BOTTOM_MID, 0, -5);

    // 버튼에 라벨 추가
    label_hema = lv_label_create(btn_hema);
    if (label_hema == NULL) {
        Serial.println("+++ 버튼 라벨 생성 실패");
        lv_obj_del(alert_msgbox);
        alert_msgbox = NULL;
        msg_top_img_check = NULL;
        close_btn_img = NULL;
        msg_label = NULL;
        btn_hema = NULL;
        return false;
    }
    
    lv_label_set_text(label_hema, "확인");
    lv_obj_center(label_hema);
    
    // 사용자 정의 콜백 또는 기본 핸들러 설정
    if (confirm_callback != NULL) {
        lv_obj_add_event_cb(btn_hema, confirm_callback, LV_EVENT_CLICKED, NULL);
    } else {
        lv_obj_add_event_cb(btn_hema, msg_close_handler_parent, LV_EVENT_CLICKED, NULL);
    }
    
    lv_obj_set_style_anim_time(btn_hema, 0, LV_STATE_PRESSED);
    
    // 화면 강제 갱신 - lv_refr_now는 제거하고 대신 일반적인 무효화만 사용
    lv_obj_invalidate(alert_msgbox);
    
    Serial.println("+++ 알림 대화상자 생성 완료");
    return true;
}
/**
 * 커버 열림 이벤트 핸들러
 */
void cover_open_event_handler() {
    Serial.println("+++ cover_open_event_handler 시작");
    //delay(2000);
    // 메뉴 모드 또는 UI 초기화 안 된 경우 무시
    if (menu_ON == 1) {
        Serial.println("+++ 메뉴 모드에서는 이벤트 무시");
        return;
    }
    
    if (!current_ui_state.initialized) {
        Serial.println("+++ UI가 아직 초기화되지 않음, 이벤트 무시");
        return;
    }
    
    // 메인 화면이 아직 로드되지 않은 경우 무시
    if (panel0 == NULL || lv_scr_act() != panel0) {
        Serial.println("+++ 메인 화면이 아직 로드되지 않음, 이벤트 무시");
        return;
    }
    
    Serial.printf("current_ui_state.initialized(%d)\n", current_ui_state.initialized);
    
    // 알림 대화상자 생성
    create_alert_dialog("박스 커버가 열렸습니다.", msg_close_handler_parent);
}

/**
 * 배뇨통 만수위 이벤트 핸들러
 */
void fulllevel_event_handler() {
    Serial.println("+++ fulllevel_event_handler 시작");
    create_alert_dialog("배뇨통이 만수위가 되었습니다.", msg_close_handler_parent);
}

/**
 * 혈뇨 감지 이벤트 핸들러
 */
void hematuria_client_event_handler() {
    Serial.println("+++ hematuria_client_event_handler 시작");
    // 10초 후 자동으로 닫히도록 설정
    create_alert_dialog("혈뇨가 감지되었습니다.", msg_close_handler_parent, 10000);
}

/**
 * 대기 상태 이벤트 핸들러
 */

// 객체 숨기기/표시 유틸리티 함수
void hide_alert_dialog() {
    if (alert_msgbox != NULL) {
        lv_obj_add_flag(alert_msgbox, LV_OBJ_FLAG_HIDDEN);
    }
}

void show_alert_dialog() {
    if (alert_msgbox != NULL) {
        lv_obj_clear_flag(alert_msgbox, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(alert_msgbox);
    }
}

#if 1
void log_ui_timer() {
    static uint32_t last_debug_time = 0;
    if (millis() - last_debug_time > 5000) {
        last_debug_time = millis();
        
        if (wifi_scan_timer != NULL) {
            // 타이머 존재 여부만 확인
            Serial.printf("+++ WiFi 타이머 상태: 활성화됨\n");
        } else {
            Serial.printf("+++ WiFi 타이머 상태: 비활성화됨\n");
        }
        
        Serial.printf("+++ UI 상태: menu_ON=%d, wifi_screen_active=%d, alert_msgbox=%p\n", 
                    menu_ON, wifi_screen_active, alert_msgbox);
    }
}
#endif

// WiFi 버튼 이벤트 콜백 함수 추가
void wifi_button_event_cb(lv_event_t *e) {
    Serial.println("+++ 다음 버튼 클릭됨");
    
    // 타이머 삭제
    safe_delete_wifi_timer();
    
    // 알림창 숨기기 (삭제하지 않음)
    if (alert_msgbox != NULL) {
        lv_obj_add_flag(alert_msgbox, LV_OBJ_FLAG_HIDDEN);
    }
    
    // WiFi 선택 화면으로 전환
    Serial.println("+++ 버튼 클릭으로 WiFi 선택 화면 전환");
    create_wifi_selection_screen();
}
void waiting_event_handler(char* status) {
    Serial.println("+++ BEGIN waiting_event_handler");
    
    // 이전 타이머가 있으면 삭제
    safe_delete_wifi_timer();
    
    // 타이머 중복 실행 방지 플래그 초기화
    timer_callback_in_progress = false;
    
    // 메뉴 모드 설정
    menu_ON = 1;
    
    // 화면이 초기화되었는지 확인
    if (lv_scr_act() == NULL) {
        Serial.println("+++ 활성화된 화면이 없어 알림 생성 취소");
        return;
    }
    
    // alert_msgbox가 이미 있는 경우 내용만 업데이트
    if (alert_msgbox != NULL) {
        Serial.println("+++ 기존 알림창 재사용");
        
        // 메시지 업데이트
        if (msg_label != NULL) {
            lv_label_set_text(msg_label, status);
            lv_obj_center(msg_label);
        }
        
        // 화면에 표시
        lv_obj_clear_flag(alert_msgbox, LV_OBJ_FLAG_HIDDEN);
    } else {
        // 새 알림창 생성 (기존 코드)
        Serial.println("+++ 새 알림창 생성");
        
        // 1. 메시지 박스 상단 이미지 준비
        msg_top_img_check = lv_img_create(lv_scr_act());
        if (msg_top_img_check == NULL) {
            Serial.println("+++ 상단 이미지 생성 실패");
            return;
        }
        
        lv_img_set_src(msg_top_img_check, &img_msgbox_check);
        
        // 2. 사용자 지정 메시지 박스 생성
        alert_msgbox = lv_obj_create(lv_scr_act());
        if (alert_msgbox == NULL) {
            Serial.println("+++ 알림 메시지 박스 생성 실패");
            if (msg_top_img_check != NULL) {
                lv_obj_add_flag(msg_top_img_check, LV_OBJ_FLAG_HIDDEN);
            }
            return;
        }
        
        lv_obj_set_size(alert_msgbox, 300, 150);
        lv_obj_align(alert_msgbox, LV_ALIGN_CENTER, 0, 0);
        
        // 3. msg_top_img를 alert 메시지 박스의 상단에 추가
        lv_obj_set_parent(msg_top_img_check, alert_msgbox);
        lv_obj_align(msg_top_img_check, LV_ALIGN_TOP_MID, 0, 5);
        
        // 메시지 라벨 생성
        msg_label = lv_label_create(alert_msgbox);
        if (msg_label == NULL) {
            Serial.println("+++ 메시지 라벨 생성 실패");
            lv_obj_add_flag(alert_msgbox, LV_OBJ_FLAG_HIDDEN);
            return;
        }
        
        lv_label_set_text(msg_label, status);
        lv_obj_center(msg_label);
        
        // 수동 전환 버튼 추가
        lv_obj_t *btn = lv_btn_create(alert_msgbox);
        if (btn != NULL) {
            lv_obj_set_size(btn, 100, 30);
            lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
            
            lv_obj_t *btn_label = lv_label_create(btn);
            if (btn_label != NULL) {
                lv_label_set_text(btn_label, "다음");
                lv_obj_center(btn_label);
            }
            
            // 버튼 클릭 이벤트
            lv_obj_add_event_cb(btn, wifi_button_event_cb, LV_EVENT_CLICKED, NULL);
        }
    }
    
    // 타이머 콜백 함수 정의
    wifi_scan_timer = lv_timer_create([](lv_timer_t *timer) {
        // 중복 실행 방지
        if (timer_callback_in_progress) {
            Serial.println("+++ 타이머 콜백이 이미 실행 중, 무시");
            return;
        }
        
        timer_callback_in_progress = true;
        Serial.println("+++ WiFi 타이머 콜백 실행");
        
        // 타이머 객체 기록
        wifi_scan_timer = NULL;
        
        // 알림창 숨기기 (삭제하지 않음)
        if (alert_msgbox != NULL) {
            lv_obj_add_flag(alert_msgbox, LV_OBJ_FLAG_HIDDEN);
        }
        
        // WiFi 선택 화면으로 전환
        Serial.println("+++ WiFi 선택 화면 전환 시작");
        create_wifi_selection_screen();
        Serial.println("+++ WiFi 선택 화면 전환 완료");
        
        // 타이머 삭제 및 플래그 초기화
        lv_timer_del(timer);
        timer_callback_in_progress = false;
    }, 10000, NULL);
    
    Serial.println("+++ END waiting_event_handler");
}