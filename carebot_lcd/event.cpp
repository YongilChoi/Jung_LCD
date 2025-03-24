#include "common.h"
#include "lvgl_controller.h"
#include "event.h"
#include "menu.h"

void Event_motor_ON() {
    Serial.println("+++ Event_motor_ON");
    if (menu_ON == 1) {
      Serial.println("+++ Event_motor_ON menu_on == 1 return");
      return;
    }
   
    Serial.println("+++ lv_obj_del(hygera)");
    //lv_obj_del(hygera);
    hygera = NULL;
    hygera = lv_img_create(panel0);
    if (hygera == NULL) {
        Serial.println("+++ Failed to create image object hygera");
        return;
    }

    lv_img_set_src(hygera, &img_hygera);
    lv_obj_set_pos(hygera, 10, 25);
    lv_img_set_pivot(hygera, 30/2, 30/2); // 좌상단 기준 회전

    // 간단한 애니메이션 효과 추가
    lv_anim_t b;
    lv_anim_init(&b);
    lv_anim_set_var(&b, hygera);
    
    lv_anim_set_values(&b, 0, 3600); // 0도에서 360도 (10x 정확도)
    //lv_anim_set_time(&b, 3600);      // 3초 동안 회전
    lv_anim_set_time(&b, 7200);      // 3초 동안 회전
    lv_anim_set_exec_cb(&b, [](void *var, int32_t v) {lv_img_set_angle(static_cast<lv_obj_t *>(var), v);});
    lv_anim_set_path_cb(&b, lv_anim_path_ease_out);
    lv_anim_set_repeat_count(&b, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&b);

    Serial.println("+++ lv_obj_del(motor)");
    //lv_obj_del(motor);
    motor = NULL;
    motor = lv_img_create(panel0);

    if (motor == NULL) {
        Serial.println("+++ Failed to create image object motor");
       return;
    }
   
    lv_img_set_src(motor, &img_motor_off);
    //lv_obj_set_pos(motor, 30, 70); 
    lv_obj_set_pos(motor, 30, 70);    
       
    lv_img_set_pivot(motor, 50/2, 50/2); 

    // 간단한 애니메이션 효과 추가
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, motor);
    
    lv_anim_set_values(&a, 0, 3600); 
    lv_anim_set_time(&a, 7200);      
    lv_anim_set_exec_cb(&a, [](void *var, int32_t v) {lv_img_set_angle(static_cast<lv_obj_t *>(var), v);});
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);
    
}

void Event_motor_OFF() {
    Serial.println("+++ Event_motor_OFF");
    if (menu_ON == 1) {
      return;
    }	

    Serial.println("+++ lv_obj_del(hygera)");
    //lv_obj_del(hygera);
    hygera = NULL;
    hygera = lv_img_create(panel0);
    if (hygera == NULL) {
        Serial.println("+++ Failed to create image object hygera");
        return;
    }
   
    lv_img_set_src(hygera, &img_hygera);

    lv_obj_set_pos(hygera, 10, 25);
	
    Serial.println("+++ lv_obj_del(motor) in Event_motor_OFF");
    //lv_obj_del(motor);
    motor = NULL;
    motor = lv_img_create(panel0);
    if (motor == NULL) {
        Serial.println("+++ Failed to create image object motor");
        return;
    }

    lv_img_set_src(motor, &img_motor_off);

    lv_obj_set_pos(motor, 30, 70);
}

void Event_error_ON() {
    Serial.println("+++ Event_error_ON");
    if (menu_ON == 1) {
      return;
    }

    lv_obj_t * error = lv_img_create(panel0);
    if (error == NULL) {
        Serial.println("+++ Failed to create image object error");
        return;
    }
   
    lv_img_set_src(error, &img_error_on);
    lv_obj_set_pos(error, 115, 70);    
    
}

void Event_error_OFF() {
    Serial.println("+++ Event_error_OFF");
    if (menu_ON == 1) {
      return;
    }

    lv_obj_t * error = lv_img_create(panel0);
    if (error == NULL) {
        Serial.println("+++ Failed to create image object error");
        return;
    }
   
    lv_img_set_src(error, &img_error_off);
    lv_obj_set_pos(error, 115, 70);    
    
}

void Event_feces_ON() {
    Serial.println("+++ Event_feces_ON");
    if (menu_ON == 1) {
      return;
    }
    
    Serial.println("+++ lv_obj_del(feces) in Event_feces_ON");
    //lv_obj_del(feces);
    feces = NULL; 

    feces = lv_img_create(panel0);
    if (feces == NULL) {
        Serial.println("+++ Failed to create image object feces");
        return;
    }
   
    lv_img_set_src(feces, &img_feces_on);
    lv_obj_set_pos(feces, 200, 70);    
    
}

void Event_feces_OFF() {
    Serial.println("+++ Event_feces_OFF");
    if (menu_ON == 1) {
      return;
    }
    Serial.println("+++ lv_obj_del(feces) in Event_feces_OFF");
    //lv_obj_del(feces);
    feces = NULL;

    feces = lv_img_create(panel0);
    if (feces == NULL) {
        Serial.println("+++ Failed to create image object feces");
        return;
    }
   
    lv_img_set_src(feces, &img_feces_off);
    lv_obj_set_pos(feces, 200, 70);    
}

void Event_connect_ON() {
    Serial.println("+++ Event_connect_ON");
    if (menu_ON == 1) {
      return;
    }

    Serial.println("+++ lv_obj_del(connect) in Event_connect_ON");
    //lv_obj_del(connect);
    connect = NULL; 

    connect = lv_img_create(panel0);
    if (connect == NULL) {
        Serial.println("+++ Failed to create image object connect");
        return;
    }
   
    lv_img_set_src(connect, &img_connect_on);
    lv_obj_set_pos(connect, 285, 70);    
    
}

void Event_connect_OFF() {
    Serial.println("+++ Event_connect_OFF");
    if (menu_ON == 1) {
      return;
    }
    Serial.println("+++ lv_obj_del(connect) in Event_connect_OFF");
    //lv_obj_del(connect);
    connect = NULL;

    connect = lv_img_create(panel0);
    if (connect == NULL) {
        Serial.println("+++ Failed to create image object connect");
        return;
    }
   
    lv_img_set_src(connect, &img_connect_off);
    lv_obj_set_pos(connect, 285, 70);    
    
}

void Event_diaper_ON() {
    Serial.println("+++ Event_diaper_ON");
    if (menu_ON == 1) {
      return;
    }
    
    diaper = lv_img_create(panel0);
    if (diaper == NULL) {
        Serial.println("+++ Failed to create image object diaper");
        return;
    }
   
    lv_img_set_src(diaper, &img_diaper_on);
    lv_obj_set_pos(diaper, 100, 170);    
}

void Event_diaper_OFF() {
    Serial.println("+++ Event_diaper_OFF");
    if (menu_ON == 1) {
      return;
    }
    diaper = lv_img_create(panel0);
    if (diaper == NULL) {
        Serial.println("+++ Failed to create image object diaper");
        return;
    }
   
    lv_img_set_src(diaper, &img_diaper_off);
    lv_obj_set_pos(diaper, 100, 170);    
    
}

void Event_tank_ON(float water_level) {
    Serial.println("+++ Event_tank_ON, water_level: ");
    Serial.println(water_level);
    if (menu_ON == 1) {
      return;
    }

    Serial.println("+++ lv_obj_del(tank) in Event_tank_ON");
    lv_obj_del(tank);
    tank = NULL; 

    // Create a new bar object
    tank = lv_bar_create(panel0);
    lv_obj_remove_style_all(tank);

    lv_obj_set_size(tank, 80, 175);
    lv_obj_set_pos(tank, 370, 75);

    // Set the bar's minimum and maximum values
    lv_bar_set_range(tank, 0, 2200); // Assuming max water level is 2200 ml

    lv_obj_set_style_radius(tank, 0, 0); // Set the corners to be square
    lv_obj_set_style_radius(tank, 0, LV_PART_INDICATOR); // 인디케이터 모서리도 사각형으로

    //lv_bar_set_mode(tank, LV_BAR_MODE_VERTICAL); // Set the bar to be vertical
    lv_bar_set_mode(tank, LV_BAR_MODE_SYMMETRICAL); // Set the bar to be vertical

    lv_obj_set_style_border_color(tank, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(tank, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(tank, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(tank, LV_OPA_100, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_bar_set_value(tank, water_level, LV_ANIM_ON);

    if (water_level >= 2000) {
        lv_obj_set_style_bg_color(tank, lv_color_hex(0xFF0000), LV_PART_INDICATOR  | LV_STATE_DEFAULT);
    }
    else if (water_level >= 1300 && water_level <2000) {
        lv_obj_set_style_bg_color(tank, lv_color_hex(0xFFD400), LV_PART_INDICATOR  | LV_STATE_DEFAULT);
    }
    else if (water_level < 1300){
        lv_obj_set_style_bg_color(tank, lv_color_hex(0x87ceeb), LV_PART_INDICATOR  | LV_STATE_DEFAULT);
    }

    //lv_bar_set_value(tank, water_level, LV_ANIM_ON);
    
    //lv_obj_move_foreground(tank); 

    //lv_obj_set_style_bg_opa(tank, LV_OPA_100, 0);
    // Set indicator opacity
    lv_obj_set_style_bg_opa(tank, LV_OPA_100, LV_PART_INDICATOR | LV_STATE_DEFAULT);
	
	//int water_level = 1000;

  Serial.println("+++ lv_obj_del(water_level_label) in Event_tank_ON");
    lv_obj_del(water_level_label);
    water_level_label = NULL; 
	
	///// water_level 값 표시 레이블
    water_level_label = lv_label_create(panel0);
    char water_level_text[20];  // 충분한 크기의 버퍼 생성
    //sprintf(water_level_text, "%.2f ml", water_level);  // 문자열 포매팅
    sprintf(water_level_text, "%.0f ml", water_level);  // 문자열 포매팅
    lv_label_set_text(water_level_label, water_level_text);
    lv_obj_align_to(water_level_label, tank, LV_ALIGN_TOP_MID, 0, 10);
    //lv_obj_set_pos(water_level_label, 385, 65);

    // 레이블 크기 설정
    lv_obj_set_width(water_level_label, 65);  // 너비를 100픽셀로 설정
    lv_obj_set_height(water_level_label, 25);  // 높이를 30픽셀로 설정
	
	// 레이블 스타일 설정 (불투명도 추가)
	static lv_style_t style_label;
	lv_style_init(&style_label);
	lv_style_set_bg_opa(&style_label, LV_OPA_100);  // 70% 불투명도 설정 
	//lv_style_set_bg_color(&style_label, lv_color_white());  // 배경색 흰색으로 설정
	lv_obj_add_style(water_level_label, &style_label, 0);
  //lv_obj_add_style(tank, &style_label, 0);		
    
}

void Event_power_ON() {
    Serial.println("+++ Event_power_ON");
    if (menu_ON == 1) {
      return;
    }

    Serial.println("+++ lv_obj_del(power)");
    //lv_obj_del(power);
    power = NULL;  

    power = lv_img_create(panel0);
    if (power == NULL) {
        Serial.println("+++ Failed to create image object power");
        return;
    }
   
    lv_img_set_src(power, &img_power_on);
    lv_obj_set_pos(power, 340, 25);    
    
}

void Event_power_OFF() {
    Serial.println("+++ Event_power_OFF");
    if (menu_ON == 1) {
      return;
    }

    Serial.println("+++ lv_obj_del(power)");
    //lv_obj_del(power);
    power = NULL;   

    power = lv_img_create(panel0);
    if (power == NULL) {
        Serial.println("+++ Failed to create image object power");
        return;
    }
   
    lv_img_set_src(power, &img_power_off);
    lv_obj_set_pos(power, 340, 25);    
    
}

void Event_wifi_ON() {
    Serial.println("+++ Event_wifi_ON");
    if (menu_ON == 1) {  
      return;
    }

    Serial.println("+++ lv_obj_del(wifi)");
    #if 1
      lv_obj_del(wifi);
    #else
    if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {        
      lv_obj_del(wifi);
      xSemaphoreGive(sdMutex);
    }
    #endif
    wifi = NULL; 

    //lv_obj_t * wifi = lv_img_create(lv_scr_act());
    wifi = lv_img_create(panel0);
    if (wifi == NULL) {
        Serial.println("+++ Failed to create image object wifi");
        return;
    }
   
    lv_img_set_src(wifi, &img_wifi_on);
    lv_obj_set_pos(wifi, 380, 24); 

    lv_obj_add_event_cb(wifi, img_click_event_cb_wifi, LV_EVENT_CLICKED, NULL);  
    lv_obj_add_flag(wifi, LV_OBJ_FLAG_CLICKABLE); 
    lv_obj_set_style_anim_time(wifi, 0, LV_STATE_PRESSED);  // 애니메이션 제거   
    
}

void Event_wifi_OFF() {
    Serial.println("+++ Event_wifi_OFF");
    if (menu_ON == 0) {  //왜 여기만 0이면, 빠져 나가게 했나? 이상함. 
      return;
    }
    Serial.println("+++ lv_obj_del(wifi)");
    #if 1
      lv_obj_del(wifi);
    #else
    if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {        
      lv_obj_del(wifi);
      xSemaphoreGive(sdMutex);
    }
    #endif
    wifi = NULL;

    wifi = lv_img_create(panel0);
    if (wifi == NULL) {
        Serial.println("+++ Failed to create image object wifi");
        return;
    }
   
    lv_img_set_src(wifi, &img_wifi_off);
    lv_obj_set_pos(wifi, 380, 21);  

    lv_obj_add_event_cb(wifi, img_click_event_cb_wifi, LV_EVENT_CLICKED, NULL);  
    lv_obj_add_flag(wifi, LV_OBJ_FLAG_CLICKABLE); 
    lv_obj_set_style_anim_time(wifi, 0, LV_STATE_PRESSED);  // 애니메이션 제거  
    
}

void Event_bat_ON() {
    Serial.println("+++ Event_bat_ON");
    if (menu_ON == 1) {
      return;
    }
    Serial.println("+++ lv_obj_del(bat)");
    //lv_obj_del(bat);
    bat = NULL;

    //lv_obj_t * bat = lv_img_create(lv_scr_act());
    bat = lv_img_create(panel0);
    
    if (bat == NULL) {
        Serial.println("+++ Failed to create image object bat");
        return;
    }
   
    lv_img_set_src(bat, &img_bat_on);
    lv_obj_set_pos(bat, 420, 25);    
    
}

void Event_bat_OFF() {
    Serial.println("+++ Event_bat_OFF");
    if (menu_ON == 1) {
      return;
    }
    
    Serial.println("+++ lv_obj_del(bat)");
    //lv_obj_del(bat);
    bat = NULL;

    bat = lv_img_create(panel0);
    if (bat == NULL) {
        Serial.println("+++ Failed to create image object bat");
        return;
    }
   
    lv_img_set_src(bat, &img_bat_off);
    lv_obj_set_pos(bat, 420, 25);    
}

void Event_bat_LOW() {
    Serial.println("+++ Event_bat_LOW");
    if (menu_ON == 1) {
      return;
    }
    
    Serial.println("+++ lv_obj_del(bat)");
    //lv_obj_del(bat);
    bat = NULL;

    bat = lv_img_create(panel0);
    if (bat == NULL) {
        Serial.println("+++ Failed to create image object bat");
        return;
    }
   
    lv_img_set_src(bat, &img_bat_low);
    lv_obj_set_pos(bat, 420, 25);    
    
}


void cover_open_event_handler() {
   
            // 1. 메시지 박스 상단 이미지 준비
            Serial.println("+++ 1");
            msg_top_img_check = lv_img_create(lv_scr_act());
            lv_img_set_src(msg_top_img_check, &img_msgbox_check);
            //lv_img_set_src(icn_01, &img_lvgl_icn_01);

            Serial.println("+++ 1");

            // 2. 닫기 버튼 이미지 준비
            close_btn_img = lv_img_create(lv_scr_act());
            lv_img_set_src(close_btn_img, &img_close_btn);
            lv_obj_add_flag(close_btn_img, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정
            lv_obj_set_style_anim_time(close_btn_img, 0, LV_STATE_PRESSED);  // 애니메이션 제거
        

            // 3. 사용자 지정 메시지 박스 생성
            alert_msgbox = lv_obj_create(lv_scr_act());
            lv_obj_set_size(alert_msgbox, 300, 150);
            lv_obj_align(alert_msgbox, LV_ALIGN_CENTER, 0, 0);
            //lv_obj_set_pos(alert, (lv_disp_get_hor_res(NULL) - 300) / 2, (lv_disp_get_ver_res(NULL) - 150) / 2);
            Serial.println("+++ 1");

            // 4. msg_top_img를 alert 메시지 박스의 상단에 추가
            lv_obj_set_parent(msg_top_img_check, alert_msgbox);         // msg_top_img를 alert의 자식으로 설정합니다.
            lv_obj_align(msg_top_img_check, LV_ALIGN_TOP_MID, 0, 5); // 메시지 박스 상단 중앙에 이미지 정렬, Y축으로 약간 내림

            Serial.println("+++ 1");

            // 5. 닫기 버튼을 alert 메시지 박스의 우측 상단에 추가 (예시)
            lv_obj_set_parent(close_btn_img, alert_msgbox);       // close_btn_img를 alert의 자식으로 설정합니다.
            Serial.println("+++ 2");
            //lv_obj_align(close_btn_img, LV_ALIGN_TOP_RIGHT, -10, 10); // 메시지 박스 우측 상단에 버튼 정렬
            lv_obj_align(close_btn_img, LV_ALIGN_TOP_RIGHT, 10, -10); // 메시지 박스 우측 상단에 버튼 정렬


            Serial.println("+++ 2");

            msg_label = lv_label_create(alert_msgbox);
            lv_label_set_text(msg_label, "박스 커버가 열렸습니다.");
            lv_obj_center(msg_label);

            // 6. 닫기 버튼 클릭 이벤트 핸들러 등록
            lv_obj_add_event_cb(close_btn_img, msg_close_handler_parent, LV_EVENT_CLICKED, close_btn_img);
            //lv_obj_add_event_cb(close_btn_img, msg_close_handler_current, LV_EVENT_CLICKED, NULL);
           
            Serial.println("+++ 3");  

                        // 7. 전송 버튼 등록

            btn_hema = lv_btn_create(alert_msgbox);

            lv_obj_set_size(btn_hema, 50, 25);
            lv_obj_align(btn_hema, LV_ALIGN_BOTTOM_MID, 0, -5);

            // 버튼에 라벨 추가
            label_hema = lv_label_create(btn_hema);
            lv_label_set_text(label_hema, "확인");  // 확인버튼 닫기 안됨 by yichoi 
            lv_obj_center(label_hema); 
            lv_obj_add_event_cb(btn_hema, msg_close_handler_parent, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_anim_time(btn_hema, 0, LV_STATE_PRESSED);  // 애니메이션 제거          
    
}

void hematuria_client_event_handler() {

            // 10초 후 자동 닫기를 위한 타이머 생성
            lv_timer_create([](lv_timer_t * timer) {
                lv_timer_del(timer);  // 타이머 삭제하여 1회만 실행  
                //lv_demo_widgets();
                lvgl_update_app_ui();
            }, 10000, NULL);  // 10000ms (10초) 후 실행
          
            // 1. 메시지 박스 상단 이미지 준비
            Serial.println("+++ 1");
            msg_top_img_check = lv_img_create(lv_scr_act());
            lv_img_set_src(msg_top_img_check, &img_msgbox_check);
            //lv_img_set_src(icn_01, &img_lvgl_icn_01);

            Serial.println("+++ 1");

            // 2. 닫기 버튼 이미지 준비
            close_btn_img = lv_img_create(lv_scr_act());
            lv_img_set_src(close_btn_img, &img_close_btn);
            lv_obj_add_flag(close_btn_img, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정
            lv_obj_set_style_anim_time(close_btn_img, 0, LV_STATE_PRESSED);  // 애니메이션 제거
        

            // 3. 사용자 지정 메시지 박스 생성
            alert_msgbox = lv_obj_create(lv_scr_act());
            lv_obj_set_size(alert_msgbox, 300, 150);
            lv_obj_align(alert_msgbox, LV_ALIGN_CENTER, 0, 0);
            //lv_obj_set_pos(alert, (lv_disp_get_hor_res(NULL) - 300) / 2, (lv_disp_get_ver_res(NULL) - 150) / 2);
            Serial.println("+++ 1");

            // 4. msg_top_img를 alert 메시지 박스의 상단에 추가
            lv_obj_set_parent(msg_top_img_check, alert_msgbox);         // msg_top_img를 alert의 자식으로 설정합니다.
            lv_obj_align(msg_top_img_check, LV_ALIGN_TOP_MID, 0, 5); // 메시지 박스 상단 중앙에 이미지 정렬, Y축으로 약간 내림

            Serial.println("+++ 1");

            // 5. 닫기 버튼을 alert 메시지 박스의 우측 상단에 추가 (예시)
            lv_obj_set_parent(close_btn_img, alert_msgbox);       // close_btn_img를 alert_msgbox의 자식으로 설정합니다.
            Serial.println("+++ 2");
            //lv_obj_align(close_btn_img, LV_ALIGN_TOP_RIGHT, -10, 10); // 메시지 박스 우측 상단에 버튼 정렬
            lv_obj_align(close_btn_img, LV_ALIGN_TOP_RIGHT, 10, -10); // 메시지 박스 우측 상단에 버튼 정렬


            Serial.println("+++ 2");

            msg_label = lv_label_create(alert_msgbox);
            lv_label_set_text(msg_label, "혈뇨가 감지되었습니다.");
            lv_obj_center(msg_label);

            // 6. 닫기 버튼 클릭 이벤트 핸들러 등록
            lv_obj_add_event_cb(close_btn_img, msg_close_handler_parent, LV_EVENT_CLICKED, NULL);
           
            Serial.println("+++ 3"); 

            // 7. 전송 버튼 등록

            btn_hema = lv_btn_create(alert_msgbox);

            lv_obj_set_size(btn_hema, 50, 25);
            lv_obj_align(btn_hema, LV_ALIGN_BOTTOM_MID, 0, -5);

            // 버튼에 라벨 추가
            label_hema = lv_label_create(btn_hema);
            lv_label_set_text(label_hema, "확인");
            lv_obj_center(label_hema); 
            // ??amount = 0;
            lv_obj_add_event_cb(btn_hema, msg_close_handler_parent, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_anim_time(btn_hema, 0, LV_STATE_PRESSED);  // 애니메이션 제거     


                //Serial.printf("Button addr in hematuria_client_event_handler: %p\n\n", close_btn_img);
                //Serial.printf("Alert addr in hematuria_client_event_handler: %p\n\n", alert_msgbox);      
    
}
#if 1
void waiting_event_handler(char* status) {

            // 10초 후 자동 닫기를 위한 타이머 생성
            lv_timer_create([](lv_timer_t * timer) {
                create_wifi_selection_screen(); 
                Serial.println("+++ 0 in waiting_event_handler");
                lv_timer_del(timer);  // 타이머 삭제하여 1회만 실행  
            }, 10000, NULL);  // 10000ms (10초) 후 실행
          
            // 1. 메시지 박스 상단 이미지 준비
            Serial.println("+++ 1 in waiting_event_handler");
            msg_top_img_check = lv_img_create(lv_scr_act());
            lv_img_set_src(msg_top_img_check, &img_msgbox_check);
            //lv_img_set_src(icn_01, &img_lvgl_icn_01);

            Serial.println("+++ 1 in waiting_event_handler");

            // 2. 닫기 버튼 이미지 준비
            /*
            close_btn_img = lv_img_create(lv_scr_act());
            lv_img_set_src(close_btn_img, &img_close_btn);
            lv_obj_add_flag(close_btn_img, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정
            lv_obj_set_style_anim_time(close_btn_img, 0, LV_STATE_PRESSED);  // 애니메이션 제거
            */
        

            // 3. 사용자 지정 메시지 박스 생성
            alert_msgbox = lv_obj_create(lv_scr_act());
            lv_obj_set_size(alert_msgbox, 300, 150);
            lv_obj_align(alert_msgbox, LV_ALIGN_CENTER, 0, 0);
            //lv_obj_set_pos(alert, (lv_disp_get_hor_res(NULL) - 300) / 2, (lv_disp_get_ver_res(NULL) - 150) / 2);
            Serial.println("+++ 1 in waiting_event_handler");

            // 4. msg_top_img를 alert 메시지 박스의 상단에 추가
            lv_obj_set_parent(msg_top_img_check, alert_msgbox);         // msg_top_img를 alert의 자식으로 설정합니다.
            lv_obj_align(msg_top_img_check, LV_ALIGN_TOP_MID, 0, 5); // 메시지 박스 상단 중앙에 이미지 정렬, Y축으로 약간 내림

            Serial.println("+++ 1 in waiting_event_handler");

            // 5. 닫기 버튼을 alert 메시지 박스의 우측 상단에 추가 (예시)
            /*
            lv_obj_set_parent(close_btn_img, alert_msgbox);       // close_btn_img를 alert_msgbox의 자식으로 설정합니다.
            Serial.println("+++ 2");
            //lv_obj_align(close_btn_img, LV_ALIGN_TOP_RIGHT, -10, 10); // 메시지 박스 우측 상단에 버튼 정렬
            lv_obj_align(close_btn_img, LV_ALIGN_TOP_RIGHT, 10, -10); // 메시지 박스 우측 상단에 버튼 정렬
            



            // 6. 닫기 버튼 클릭 이벤트 핸들러 등록
            //lv_obj_add_event_cb(close_btn_img, msg_close_handler_parent, LV_EVENT_CLICKED, NULL);

            */

            Serial.println("+++ 2 in waiting_event_handler");

            msg_label = lv_label_create(alert_msgbox);
            lv_label_set_text(msg_label, status);
            lv_obj_center(msg_label);

           
            Serial.println("+++ 3 in waiting_event_handler"); 

            // 7. 전송 버튼 등록

            /*

            btn_hema = lv_btn_create(alert_msgbox);

            lv_obj_set_size(btn_hema, 50, 25);
            lv_obj_align(btn_hema, LV_ALIGN_BOTTOM_MID, 0, -5);

            // 버튼에 라벨 추가
            
            label_hema = lv_label_create(btn_hema);
            lv_label_set_text(label_hema, "확인");
            lv_obj_center(label_hema); 
            amount = 0;
            lv_obj_add_event_cb(btn_hema, msg_close_handler_parent, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_anim_time(btn_hema, 0, LV_STATE_PRESSED);  // 애니메이션 제거   
            */  


                //Serial.printf("Button addr in hematuria_client_event_handler: %p\n\n", close_btn_img);
                //Serial.printf("Alert addr in hematuria_client_event_handler: %p\n\n", alert_msgbox);      
    
}
void waiting_event_handler_2(char* status) {

            // 10초 후 자동 닫기를 위한 타이머 생성
            lv_timer_create([](lv_timer_t * timer) {
                //connect_to_wifi(network->ssid, password_wifi); 
                Serial.println("+++ 0 in waiting_event_handler_2");
                // alert_msgbox 제거
                lv_obj_del(alert_msgbox);
                lv_timer_del(timer);  // 타이머 삭제하여 1회만 실행                  
            }, 10000, NULL);  // 10000ms (10초) 후 실행
          
            // 1. 메시지 박스 상단 이미지 준비
            Serial.println("+++ 1 waiting_event_handler_2");
            msg_top_img_check = lv_img_create(lv_scr_act());
            lv_img_set_src(msg_top_img_check, &img_msgbox_check);
            //lv_img_set_src(icn_01, &img_lvgl_icn_01);

            Serial.println("+++ 1 waiting_event_handler_2");

            // 2. 닫기 버튼 이미지 준비
            /*
            close_btn_img = lv_img_create(lv_scr_act());
            lv_img_set_src(close_btn_img, &img_close_btn);
            lv_obj_add_flag(close_btn_img, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정
            lv_obj_set_style_anim_time(close_btn_img, 0, LV_STATE_PRESSED);  // 애니메이션 제거
            */
        

            // 3. 사용자 지정 메시지 박스 생성
            alert_msgbox = lv_obj_create(lv_scr_act());
            lv_obj_set_size(alert_msgbox, 300, 150);
            lv_obj_align(alert_msgbox, LV_ALIGN_CENTER, 0, 0);
            //lv_obj_set_pos(alert, (lv_disp_get_hor_res(NULL) - 300) / 2, (lv_disp_get_ver_res(NULL) - 150) / 2);
            Serial.println("+++ 1 waiting_event_handler_2");

            // 4. msg_top_img를 alert 메시지 박스의 상단에 추가
            lv_obj_set_parent(msg_top_img_check, alert_msgbox);         // msg_top_img를 alert의 자식으로 설정합니다.
            lv_obj_align(msg_top_img_check, LV_ALIGN_TOP_MID, 0, 5); // 메시지 박스 상단 중앙에 이미지 정렬, Y축으로 약간 내림

            Serial.println("+++ 1 waiting_event_handler_2");

            // 5. 닫기 버튼을 alert 메시지 박스의 우측 상단에 추가 (예시)
            /*
            lv_obj_set_parent(close_btn_img, alert_msgbox);       // close_btn_img를 alert_msgbox의 자식으로 설정합니다.
            Serial.println("+++ 2");
            //lv_obj_align(close_btn_img, LV_ALIGN_TOP_RIGHT, -10, 10); // 메시지 박스 우측 상단에 버튼 정렬
            lv_obj_align(close_btn_img, LV_ALIGN_TOP_RIGHT, 10, -10); // 메시지 박스 우측 상단에 버튼 정렬
            



            // 6. 닫기 버튼 클릭 이벤트 핸들러 등록
            //lv_obj_add_event_cb(close_btn_img, msg_close_handler_parent, LV_EVENT_CLICKED, NULL);

            */

            Serial.println("+++ 2 waiting_event_handler_2");

            msg_label = lv_label_create(alert_msgbox);
            lv_label_set_text(msg_label, status);
            lv_obj_center(msg_label);

           
            Serial.println("+++ 3 waiting_event_handler_2"); 

            // 7. 전송 버튼 등록

            /*

            btn_hema = lv_btn_create(alert_msgbox);

            lv_obj_set_size(btn_hema, 50, 25);
            lv_obj_align(btn_hema, LV_ALIGN_BOTTOM_MID, 0, -5);

            // 버튼에 라벨 추가
            
            label_hema = lv_label_create(btn_hema);
            lv_label_set_text(label_hema, "확인");
            lv_obj_center(label_hema); 
            amount = 0;
            lv_obj_add_event_cb(btn_hema, msg_close_handler_parent, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_anim_time(btn_hema, 0, LV_STATE_PRESSED);  // 애니메이션 제거   
            */  


                //Serial.printf("Button addr in hematuria_client_event_handler: %p\n\n", close_btn_img);
                //Serial.printf("Alert addr in hematuria_client_event_handler: %p\n\n", alert_msgbox);      
    
}
#endif
/* 만수위 */ 
void fulllevel_event_handler() {
   
            // 1. 메시지 박스 상단 이미지 준비
            Serial.println("+++ 1");
            msg_top_img_check = lv_img_create(lv_scr_act());
            lv_img_set_src(msg_top_img_check, &img_msgbox_check);
            //lv_img_set_src(icn_01, &img_lvgl_icn_01);

            Serial.println("+++ 1");

            // 2. 닫기 버튼 이미지 준비
            close_btn_img = lv_img_create(lv_scr_act());
            lv_img_set_src(close_btn_img, &img_close_btn);
            lv_obj_add_flag(close_btn_img, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정
            lv_obj_set_style_anim_time(close_btn_img, 0, LV_STATE_PRESSED);  // 애니메이션 제거
        

            // 3. 사용자 지정 메시지 박스 생성
            alert_msgbox = lv_obj_create(lv_scr_act());
            lv_obj_set_size(alert_msgbox, 300, 150);
            lv_obj_align(alert_msgbox, LV_ALIGN_CENTER, 0, 0);
            //lv_obj_set_pos(alert, (lv_disp_get_hor_res(NULL) - 300) / 2, (lv_disp_get_ver_res(NULL) - 150) / 2);
            Serial.println("+++ 1");

            // 4. msg_top_img를 alert 메시지 박스의 상단에 추가
            lv_obj_set_parent(msg_top_img_check, alert_msgbox);         // msg_top_img를 alert의 자식으로 설정합니다.
            lv_obj_align(msg_top_img_check, LV_ALIGN_TOP_MID, 0, 5); // 메시지 박스 상단 중앙에 이미지 정렬, Y축으로 약간 내림

            Serial.println("+++ 1");

            // 5. 닫기 버튼을 alert 메시지 박스의 우측 상단에 추가 (예시)
            lv_obj_set_parent(close_btn_img, alert_msgbox);       // close_btn_img를 alert_msgbox의 자식으로 설정합니다.
            Serial.println("+++ 2");
            //lv_obj_align(close_btn_img, LV_ALIGN_TOP_RIGHT, -10, 10); // 메시지 박스 우측 상단에 버튼 정렬
            lv_obj_align(close_btn_img, LV_ALIGN_TOP_RIGHT, 10, -10); // 메시지 박스 우측 상단에 버튼 정렬


            Serial.println("+++ 2");

            msg_label = lv_label_create(alert_msgbox);
            lv_label_set_text(msg_label, "배뇨통이 만수위가 되었습니다.");
            lv_obj_center(msg_label);

            // 6. 닫기 버튼 클릭 이벤트 핸들러 등록
            lv_obj_add_event_cb(close_btn_img, msg_close_handler_parent, LV_EVENT_CLICKED, NULL);
           
            Serial.println("+++ 3"); 

            // 7. 전송 버튼 등록

            btn_hema = lv_btn_create(alert_msgbox);

            lv_obj_set_size(btn_hema, 50, 25);
            lv_obj_align(btn_hema, LV_ALIGN_BOTTOM_MID, 0, -5);

            // 버튼에 라벨 추가
            label_hema = lv_label_create(btn_hema);
            lv_label_set_text(label_hema, "확인");
            lv_obj_center(label_hema); 
            lv_obj_add_event_cb(btn_hema, msg_close_handler_parent, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_anim_time(btn_hema, 0, LV_STATE_PRESSED);  // 애니메이션 제거     


                Serial.printf("+++ fulllevel_event_handler\n");
    
}


