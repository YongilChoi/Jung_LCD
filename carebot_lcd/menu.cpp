#include "common.h"
#include "lvgl_controller.h"
#include "serial_protocol.h"
#include "serial.h"
#include "menu.h"
#include "event.h"
#include "audio.h"
#include "image_assets.h"
#include "lv_conf.h"


lv_obj_t *header_label;
lv_obj_t *header_label_back;

WiFiNetwork *network;
const char *password_wifi;
char* ssid_main;
char* ip_addr_main;
char* gateway_main;

WiFiNetwork available_networks[MAX_WIFI_NETWORKS];

lv_obj_t * wifi_ap_setup_screen = NULL;
lv_obj_t * alarm_setup_screen = NULL; 
lv_obj_t * wifi_screen = NULL;
lv_obj_t *wifi_list_screen = NULL;
lv_obj_t *wifi_list = NULL;
lv_obj_t *password_keyboard = NULL;
lv_obj_t * password_screen = NULL;
lv_obj_t * status_screen = NULL;

lv_obj_t *password_ta = NULL;
lv_obj_t *password_ta_2 = NULL;

int network_count = 0;

void img_click_event_cb_icn01(lv_event_t * e);



// 전역 변수 선언
//static lv_obj_t *wifi_screen;
lv_obj_t *menu_list;
//static lv_obj_t *header_label;
//static lv_obj_t *header_label_back;
lv_obj_t *inform_label;

lv_style_t style_list;
lv_style_t style_header;
lv_style_t style_header_18;
lv_style_t style_header_18_2;

lv_style_t style_btn;
lv_style_t style_btn_2;

lv_style_t style_panel;
lv_obj_t *close_btn;

// 스타일 객체 생성
lv_style_t style_indicator;
lv_style_t style_knob;

//static lv_obj_t* checkboxes[3];
lv_obj_t * cont2;

uint8_t amount = 0;



// 메뉴 항목 정의
typedef struct {
    const char *name;
    void (*callback)(void);
} menu_item_t;

menu_item_t menu_item;

// Screen IDs
typedef enum {
    SCREEN_MAIN,
    SCREEN_MENU,
    SCREEN_SETTINGS,
    SCREEN_MAX
} screen_id_t;

// Screen structure
typedef struct {
    lv_obj_t *screen;
    void (*create_cb)(void);
    void (*destroy_cb)(void);
} screen_t;

// Global variables
screen_t g_screens[SCREEN_MAX];
screen_id_t g_current_screen = SCREEN_MAIN;
lv_obj_t *g_loading_screen = NULL;

lv_obj_t *slider = NULL;
lv_obj_t *volume_label = NULL;
lv_obj_t * speaker;

//static lv_obj_t *speaker_icon;

lv_obj_t *radio_group;
lv_obj_t *progress_bar;
lv_obj_t *progress_label;

uint32_t radio_1_active_id = 0;
uint32_t radio_2_active_id = 0;

/////////////// 5. 단말 정보 보기 ///////////////

lv_obj_t* network_info_screen;
lv_obj_t* title_label;
lv_obj_t* model_label;
lv_obj_t* sw_ver_label;
lv_obj_t* serial_label;
lv_obj_t* ssid_label;
lv_obj_t* ip_label;
lv_obj_t* gateway_label;



/////////////// 암호 ///////////////

// 전역 변수 선언
#define PASSWORD_LENGTH 15  // 최대 암호 길이
#define CORRECT_PASSWORD "Hygera2024!@#" // 올바른 암호 설정
//#define CORRECT_PASSWORD "AAAbbb!@#" // 올바른 암호 설정
//static const char * CORRECT_PASSWORD = "AAAbbb1!@";
#define MIN_PASSWORD_LENGTH 8  // 최소 암호 길이


//static char entered_password[PASSWORD_LENGTH + 1]; // 입력된 암호를 저장할 버퍼
const char * entered_password; // 입력된 암호를 저장할 버퍼

bool is_authenticated = false;
lv_obj_t* error_label = NULL;  // 오류 메시지를 표시할 레이블

// 함수 전방 선언
void factory_event_handler(lv_event_t* e);
//static void cover_open_event_handler();
void password_event_handler(lv_event_t* e);
bool verify_password(const char* input);
int check_password_requirements(const char* password);

//////////////////////////////////////////


const char* buttons[] = {"닫기", ""};


// 메뉴 항목 배열
const menu_item_t menu_items[] = {
    {"1. WiFi AP 등록", wifi_ap_setup_cb},
    {"2. 알람 볼륨", alarm_setup_cb},
    {"3. 혈뇨 정보 전송", relay_info_cb},
    {"4. 배뇨 유도", urination_cb},
    {"5. 단말 정보 보기", terminal_info_cb},
    {"6. 공장 초기화", factory_menu_cb}
};

#define MENU_ITEM_COUNT (sizeof(menu_items) / sizeof(menu_item_t))

// 버튼 이벤트 핸들러
void menu_btn_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);

    Serial.println("+++ menu_btn_event_cb");

    
    if (code == LV_EVENT_CLICKED) {
        uint32_t idx = lv_obj_get_index(btn);
        if (idx < MENU_ITEM_COUNT && menu_items[idx].callback) {
            menu_items[idx].callback();
        }
    }
}
// 화면 전환 함수 개선 (menu.cpp에 추가)
void transition_to_screen(lv_obj_t *screen) {
    logMessage("MENU", LOG_LEVEL_INFO, "화면 전환 시작");
    
    // 화면 전환 시도
    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_ON, 150, 0, false);
    
    // 화면 전환 후 즉시 flush 호출
    #ifdef CANVAS
        Arduino_GFX* gfx = get_gfx_instance();
        if (gfx) {
            gfx->flush();
            logMessage("MENU", LOG_LEVEL_INFO, "화면 전환 후 flush 호출");
        }
    #endif
    
    logMessage("MENU", LOG_LEVEL_INFO, "화면 전환 완료");
}
// 스타일 초기화style_btn
void init_styles(void) {
    // 버튼 스타일
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0x2196F3));
    lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
    lv_style_set_border_width(&style_btn, 0);
    lv_style_set_pad_all(&style_btn, 10);
    //lv_style_set_text_color(&style_btn, lv_color_white());
    lv_style_set_text_font(&style_btn, &nanum_gothic_16);    
    lv_style_set_text_color(&style_btn, lv_color_black());
    lv_style_set_radius(&style_btn, 5);
    
    // 리스트 스타일
    lv_style_init(&style_list);
    //lv_style_set_pad_row(&style_list, 5);
    //lv_style_set_pad_column(&style_list, 5);
    lv_style_set_pad_row(&style_list, 1);
    lv_style_set_pad_column(&style_list, 0);
    
    // 헤더 스타일
    lv_style_init(&style_header);
    //lv_style_set_text_font(&style_header, &lv_font_montserrat_16);
    lv_style_set_bg_color(&style_header, lv_color_hex(0x0019F4));
    lv_style_set_bg_opa(&style_header, LV_OPA_COVER);
    //lv_style_set_border_width(&style_header, 0);
    lv_style_set_border_width(&style_header, 0);
    //lv_style_set_pad_all(&style_header, 10);
    //lv_style_set_pad_all(&style_header, 0);
    lv_style_set_text_font(&style_header, &nanum_gothic_16);    
    lv_style_set_text_color(&style_header, lv_color_white());
    //lv_style_set_text_color(&style_header, lv_color_black());

    // 헤더 스타일
    lv_style_init(&style_header_18);
    lv_style_set_bg_color(&style_header_18, lv_color_hex(0x0019F4));
    lv_style_set_bg_opa(&style_header_18, LV_OPA_COVER);
    lv_style_set_border_width(&style_header_18, 1);
    lv_style_set_border_color(&style_header_18, lv_color_black());
    //lv_style_set_border_side(&style_header_18, LV_BORDER_SIDE_FULL);     // 모든 방향
    //lv_style_set_pad_all(&style_header_18, 10);
    lv_style_set_text_font(&style_header_18, &nanum_gothic_18);  
    lv_style_set_text_color(&style_header_18, lv_color_white());

    lv_style_init(&style_header_18_2);
    //lv_style_set_bg_color(&style_header_18, lv_color_hex(0x0019F4));
    //lv_style_set_bg_color(&style_header_18_2, lv_color_hex(0x0019F4));
    lv_style_set_bg_opa(&style_header_18_2, LV_OPA_COVER);
    lv_style_set_border_width(&style_header_18_2, 1);
    lv_style_set_border_color(&style_header_18_2, lv_color_hex(0x0019F4));
    //lv_style_set_border_side(&style_header_18, LV_BORDER_SIDE_FULL);     // 모든 방향
    //lv_style_set_pad_all(&style_header_18, 10);
    lv_style_set_text_font(&style_header_18_2, &nanum_gothic_18);  
    lv_style_set_text_color(&style_header_18_2, lv_color_black());

}



// 패스워드 요구사항 검증 함수
int check_password_requirements(const char* password) {
    if (password == NULL) return 2;
    
    size_t len = strlen(password);
    if (len < MIN_PASSWORD_LENGTH) return 3;

    int has_upper = 0;
    int has_lower = 0;
    int has_digit = 0;
    int has_special = 0;
    
    for (size_t i = 0; i < len; i++) {

        if (isupper(password[i])) has_upper = 1;
        else if (islower(password[i])) has_lower = 1;
        else if (isdigit(password[i])) has_digit = 1;
        else if (strchr("!@#$%^&*()", password[i])) has_special = 1;
    }
    
    if (has_upper == 1 && has_lower == 1 && has_digit == 1 && has_special == 1)
     { return 1;}
    else
     { return 0;}

}

// 오류 메시지 업데이트 함수
void update_error_message(lv_obj_t* parent, const char* password) {

      size_t len = strlen(password);

    /*
    if (error_label == NULL) {
        error_label = lv_label_create(parent);
        //lv_obj_align_to(error_label, parent, LV_ALIGN_TOP_MID, 0, 80);
        lv_obj_align(error_label, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xFF0000), 0);
    }
    

    if (len < MIN_PASSWORD_LENGTH) {
        lv_label_set_text(error_label, "최소 8자 이상 입력하세요");
        return;
    }
    */
    
    bool has_upper = false, has_lower = false, has_digit = false, has_special = false;
    for (size_t i = 0; i < len; i++) {
        if (isupper(password[i])) has_upper = true;
        else if (islower(password[i])) has_lower = true;
        else if (isdigit(password[i])) has_digit = true;
        else if (strchr("!@#$%^&*()", password[i])) has_special = true;
    }
    
    static char msg[100];
    strcpy(msg, "필수 포함: ");
    if (!has_upper) strcat(msg, "대문자, ");
    if (!has_lower) strcat(msg, "소문자, ");
    if (!has_digit) strcat(msg, "숫자, ");
    if (!has_special) strcat(msg, "특수문자, ");
    
    if (has_upper && has_lower && has_digit && has_special) {
        lv_label_set_text(error_label, "유효한 암호입니다");
        lv_obj_set_style_text_color(error_label, lv_color_hex(0x00FF00), 0);
    } else {
        msg[strlen(msg) - 2] = '\0';  // 마지막 ", " 제거
        lv_label_set_text(error_label, msg);
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xFF0000), 0);
    }
}



// 암호 검증 함수 구현
bool verify_password(const char* input) {
    Serial.printf("+++ passwd input:%s \n", input);
    Serial.println("+++ strcmp(input, CORRECT_PASSWORD):");
    Serial.println(strcmp(input, CORRECT_PASSWORD));
    //if (!check_password_requirements(input)) return false;    
    return strcmp(input, CORRECT_PASSWORD) == 0;    
}

// 암호 입력 이벤트 처리
void password_event_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    //lv_obj_t* ta = lv_event_get_target(e);
    
    if (code == LV_EVENT_VALUE_CHANGED) {
        //const char* text = lv_textarea_get_text(ta);
        //strncpy(entered_password, text, PASSWORD_LENGTH);
        //entered_password[PASSWORD_LENGTH] = '\0';

        entered_password = lv_textarea_get_text(password_ta);

        //update_error_message(lv_obj_get_parent(ta), entered_password);
    }
}

// 키보드 맵 및 스타일 설정
void setup_keyboard_with_special_chars(lv_obj_t *kb) {

 // 일반 문자 키보드 맵
    static const char* kb_map_lower[] = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "\n",
        "a", "s", "d", "f", "g", "h", "j", "k", "l", "\n",
        "ABC", "z", "x", "c", "v", "b", "n", "m", "#+=", LV_SYMBOL_OK, "\n",
        LV_SYMBOL_CLOSE, " ", LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, NULL
    };

    // 대문자 키보드 맵
    static const char* kb_map_upper[] = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", LV_SYMBOL_BACKSPACE, "\n",
        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "\n",
        "A", "S", "D", "F", "G", "H", "J", "K", "L", "\n",
        "abc", "Z", "X", "C", "V", "B", "N", "M", "#+=", LV_SYMBOL_OK, "\n",
        LV_SYMBOL_CLOSE, " ", LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, NULL
    };

    // 특수문자 키보드 맵
    static const char* kb_map_special[] = {
        "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", LV_SYMBOL_BACKSPACE, "\n",
        "~", "`", "{", "}", "[", "]", "+", "=", "|", "\\", "\n",
        "-", "_", ":", ";", "'", "\"", "<", ">", "?", "\n",
        "abc", "/", ",", ".", "@", "$", "^", "&", "#+=", LV_SYMBOL_OK, "\n",
        LV_SYMBOL_CLOSE, " ", LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, NULL
    };

    // 키보드 컨트롤 맵
    static const lv_btnmatrix_ctrl_t kb_ctrl_map[] = {
        LV_KEYBOARD_CTRL_BTN_FLAGS | 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1,
        LV_KEYBOARD_CTRL_BTN_FLAGS | 2, 1, 1, 1, 1, 1, 1, 1, LV_KEYBOARD_CTRL_BTN_FLAGS | 2, LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
        LV_KEYBOARD_CTRL_BTN_FLAGS | 2, 6, 1, 1
    };

    // 키보드 맵 설정
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, kb_map_lower, kb_ctrl_map);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_UPPER, kb_map_upper, kb_ctrl_map);
    lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_SPECIAL, kb_map_special, kb_ctrl_map);

    // 키보드 스타일 설정
    static lv_style_t style_kb;
    lv_style_init(&style_kb);
    lv_style_set_text_font(&style_kb, &lv_font_montserrat_16);
    lv_style_set_bg_color(&style_kb, lv_color_hex(0xF5F5F5));
    lv_obj_add_style(kb, &style_kb, 0);

    // 버튼 스타일 설정
    static lv_style_t style_kb_btn;
    lv_style_init(&style_kb_btn);
    lv_style_set_bg_color(&style_kb_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_border_width(&style_kb_btn, 1);
    lv_style_set_border_color(&style_kb_btn, lv_color_hex(0xDDDDDD));
    lv_style_set_text_font(&style_kb_btn, &lv_font_montserrat_16);
    lv_obj_add_style(kb, &style_kb_btn, LV_PART_ITEMS);

    // 선택된 버튼 스타일
    static lv_style_t style_kb_btn_pressed;
    lv_style_init(&style_kb_btn_pressed);
    lv_style_set_bg_color(&style_kb_btn_pressed, lv_color_hex(0x2196F3));
    lv_style_set_text_color(&style_kb_btn_pressed, lv_color_hex(0xFFFFFF));
    lv_obj_add_style(kb, &style_kb_btn_pressed, LV_PART_ITEMS | LV_STATE_PRESSED);
}

// 수정된 키보드 이벤트 핸들러
void keyboard_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * kb = lv_event_get_target(e);

    Serial.println("+++ in keyboard_event_cb");

    //////////////
    //const char *password = lv_textarea_get_text(password_ta_2);
    entered_password = lv_textarea_get_text(password_ta_2);

    Serial.printf("+++ in keyboard_event_cb password: %s \n", entered_password);
    /////////////
    

    
    //if(code == LV_EVENT_VALUE_CHANGED) {
        uint16_t btn_id = lv_btnmatrix_get_selected_btn(kb);
    Serial.println("+++ 1");


        if(btn_id == LV_BTNMATRIX_BTN_NONE) return;
        
        const char * txt = lv_btnmatrix_get_btn_text(kb, btn_id);
            Serial.println("+++ 1");

        if(txt == NULL) return;

        // 특수 버튼 처리를 위한 플래그
        bool is_special_button = false;
        
        // 모드 전환 버튼 처리
        if(strcmp(txt, "ABC") == 0) {
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
            //is_special_button = true;
        } 
        else if(strcmp(txt, "abc") == 0) {
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_UPPER);
            //is_special_button = true;
        }
        else if(strcmp(txt, "#+=") == 0) {
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_SPECIAL);
            is_special_button = true;
        }
        else if(strcmp(txt, "123") == 0) {
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
            is_special_button = true;
        }
    Serial.println("+++ 2");

        // 특수 버튼인 경우 텍스트 영역에 문자가 입력되지 않도록 처리
        if (is_special_button) {
            lv_obj_t * ta = lv_keyboard_get_textarea(kb);
            if(ta != NULL) {
                // 현재 커서 위치에서 마지막으로 입력된 문자를 삭제
                uint32_t cur_pos = lv_textarea_get_cursor_pos(ta);
                if(cur_pos > 0) {
                    Serial.println("+++ lv_textarea_del_char before");
                    lv_textarea_del_char(ta);
                }
            }
        }
        
        Serial.printf("+++ txt: %s \n", txt);
        Serial.printf("+++ btn_id: %d \n", btn_id);
        Serial.printf("+++ is_special_button: %d \n", is_special_button);
    //}
  
}



void factory_screen(lv_obj_t *parent) {
    // 스타일 생성
    static lv_style_t style_ta;
    lv_style_init(&style_ta);
    lv_style_set_text_font(&style_ta, &lv_font_montserrat_16);
    lv_style_set_border_width(&style_ta, 2);
    lv_style_set_border_color(&style_ta, lv_color_hex(0x2196F3));
    
    // 제목 레이블 생성
    lv_obj_t *title2 = lv_label_create(parent);
    //lv_obj_t *title2 = lv_label_create(NULL);

    lv_label_set_text(title2, "잘못된 사용방지를 위해 Factory 메뉴 접근이 제한됩니다.");
    lv_obj_align(title2, LV_ALIGN_TOP_MID, 0, 50);
    // 배경색을 빨간색으로 설정
    lv_obj_set_style_bg_color(title2, lv_color_hex(0xFF0000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(title2, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(title2, 5, LV_PART_MAIN);
    // 텍스트 색상을 하얀색으로 변경
    lv_obj_set_style_text_color(title2, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    // 텍스트 영역 생성
    password_ta_2 = lv_textarea_create(parent);
    lv_obj_set_size(password_ta_2, 130, 30);
    lv_obj_align(password_ta_2, LV_ALIGN_CENTER, -100, -20);
    lv_obj_add_style(password_ta_2, &style_ta, 0);
    
    // 플레이스홀더 텍스트 설정
    //lv_textarea_set_placeholder_text(ta, "암호 입력...");
    lv_textarea_set_max_length(password_ta_2, PASSWORD_LENGTH);
    lv_textarea_set_password_mode(password_ta_2, true);
    lv_textarea_set_one_line(password_ta_2, true);
    
    // 입력 제한 설정 (영문, 숫자, 특수문자만 허용)
    lv_textarea_set_accepted_chars(password_ta_2, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()");
    
    // 키보드 생성 및 설정
    lv_obj_t *kb = lv_keyboard_create(parent);
    lv_keyboard_set_textarea(kb, password_ta_2);
    setup_keyboard_with_special_chars(kb);
    
    // 이벤트 핸들러 등록 - 수정된 부분
    lv_obj_set_style_anim_time(kb, 0, LV_STATE_PRESSED);  // 애니메이션 제거
    //lv_obj_add_event_cb(kb, keyboard_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(kb, keyboard_event_cb, LV_EVENT_CLICKED, NULL);

    
    // 키보드 크기 및 위치 조정
    //lv_obj_set_size(kb, LV_HOR_RES - 20, LV_VER_RES / 2);
    lv_obj_set_size(kb, LV_HOR_RES - 20, LV_VER_RES / 2.5);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // 확인 버튼 생성
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 100, 30);
    //lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_align(btn, LV_ALIGN_CENTER, 100, -20);
    
    
    // 버튼 스타일 설정
    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0x2196F3));
    lv_obj_add_style(btn, &style_btn, 0);
    
    // 버튼 레이블 추가
    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "확 인");
    lv_obj_center(btn_label);
    
    // 이벤트 핸들러 연결
    //lv_obj_add_event_cb(ta, password_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(btn, factory_event_handler, LV_EVENT_CLICKED, NULL);
}


void menu(void) {     
    Serial.println("+++ in menu()");
    menu_ON = 1;
    initial_scr();
    
    transition_to_screen(main_screen);
}

void initial_scr(void) {

   init_styles();    

    static lv_style_t style_panel;

     // 패널 스타일 설정
   
    lv_style_set_bg_color(&style_panel, lv_color_hex(0xEEEEEE));  //배경색 매우 밝은 회색, lv_color_hex()는 16진수 색상을 lv_color_t 타입으로 변환
    lv_style_set_border_color(&style_panel, lv_color_hex(0x999999)); //경계선 색상, 중간 정도의 회색
    lv_style_set_border_width(&style_panel, 2); // 경계선의 두께
    lv_style_set_pad_all(&style_panel, 10); //**모든 방향의 패딩(padding)**, 모든 방향(상, 하, 좌, 우)에서 10픽셀만큼의 여백을 설정합니다. 패딩은 내부 요소와 경계선 사이의 간격을 설정
    lv_style_set_radius(&style_panel, 5); //**모서리의 둥근 정도(반지름)**를 설정, 모서리 반지름을 5로 설정하여, 패널의 모서리가 둥글게 보이도록 만듭니다. 숫자가 클수록 더 둥글고, 숫자가 작을수록 더 직각에 가까워집니다.     


  // 메인 스크린 생성
    Serial.println("+++ lv_obj_create(NULL)");

    //lv_obj_t * main_screen = lv_obj_create(NULL);
    if (main_screen == NULL)
    {
    Serial.println("+++ BEFORE main_screen lv_obj_create");
    main_screen = lv_obj_create(NULL);
    lv_obj_set_size(main_screen, 480, 272);  
    lv_obj_align(main_screen, LV_ALIGN_LEFT_MID, 0, 0);  
    }
    lv_scr_load(main_screen);  

  
    // 헤더 레이블 생성
    header_label = lv_label_create(main_screen);
    lv_label_set_text(header_label, "설정 메뉴");
    lv_obj_add_style(header_label, &style_header_18, 0);
    lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15);
    //lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 0);
    //lv_obj_set_style_pad_all(header_label, 5, LV_STATE_DEFAULT);  // 패딩 추가
    lv_obj_set_size(header_label, 480, 25);
    lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    //////////////////////////////////////////////////////////////////////////////// 

   // header_label_back = lv_label_create(main_screen); //기존 텍스트 뒤로가 버튼 생성 
	header_label_back = lv_imgbtn_create(main_screen);  //1. 이미지로 변경 
	    // 버튼 크기 설정 (이미지 크기에 맞게 조정)
    lv_obj_set_size(header_label_back, 25, 25);

	// 버튼에 이미지 설정 (기본 상태)
    lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn	, NULL);

	    // 위치 설정 (기존 코드와 동일한 위치)
    lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
		
	    // 배경 투명도 설정 (이미지 자체의 불투명도를 100% 유지)
    lv_obj_set_style_img_opa(header_label_back, LV_OPA_COVER, LV_PART_MAIN);

    // 특정 색상(예: 0x000000, 검은색)을 투명하게 만들기
    lv_obj_set_style_img_recolor_opa(header_label_back, LV_OPA_TRANSP, LV_PART_MAIN);	
		
		
   // lv_label_set_text(header_label_back, "^");  //back btn text 설정 
    
   // lv_obj_add_style(header_label_back, &style_header_18, 0);
	
  //  lv_obj_align(header_label_back, LV_ALIGN_TOP_RIGHT, 0, 15);
	
  //  lv_obj_set_size(header_label_back, 50, 25);
	
  //  lv_obj_set_style_text_align(header_label_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

// 라벨을 클릭 가능하도록 설정initial_cb
lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정

// 클릭 영역을 더 크게 만들기 (선택사항)
//lv_obj_set_style_pad_all(header_label_back, 5, LV_STATE_DEFAULT);  // 패딩 추가

// 클릭 이벤트 추가
// lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_1, LV_EVENT_CLICKED, NULL);  //back btn 
lv_obj_add_event_cb(header_label_back, initial_cb_1, LV_EVENT_CLICKED, NULL);  //back btn 


// 선택사항: 클릭 시각적 피드백을 위한 스타일 추가
lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);  // 클릭시 배경 투명도
lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);  // 클릭시 배경색

lv_obj_set_style_anim_time(header_label_back, 0, LV_STATE_PRESSED);  // 애니메이션 제거

//////////////////////////////////////////////////////////////////////////////// 

    
    // 메뉴 리스트 생성
    menu_list = lv_list_create(main_screen);
    //lv_obj_set_size(menu_list, LV_PCT(90), LV_PCT(80));
    lv_obj_set_size(menu_list, 480, LV_PCT(90));
    //lv_obj_align(menu_list, LV_ALIGN_CENTER, 0, 10);
    lv_obj_align(menu_list, LV_ALIGN_BOTTOM_MID, 0, 15);
    lv_obj_add_style(menu_list, &style_list, 0);
    lv_obj_set_scrollbar_mode(menu_list, LV_SCROLLBAR_MODE_OFF);  // 스크롤바 제거

    
    // 메뉴 항목 추가
    for (uint32_t i = 0; i < MENU_ITEM_COUNT; i++) {
        lv_obj_t *btn = lv_list_add_btn(menu_list, NULL, menu_items[i].name);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정
        lv_obj_add_event_cb(btn, menu_btn_event_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_set_style_anim_time(btn, 0, LV_STATE_PRESSED);  // 애니메이션 제거
        
    }
}

// 메뉴 콜백 함수 구현
void show_menu_content(const char *title, const char *content) {
  
    lv_obj_t *popup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(popup, LV_PCT(80), LV_PCT(60));
    lv_obj_center(popup);
    
    // 팝업 제목
    lv_obj_t *title_label = lv_label_create(popup);
    lv_label_set_text(title_label, title);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);
    
    // 팝업 내용
    lv_obj_t *content_label = lv_label_create(popup);
    lv_label_set_text(content_label, content);
    lv_obj_align(content_label, LV_ALIGN_CENTER, 0, 0);
    
    // 닫기 버튼
    lv_obj_t *close_btn = lv_btn_create(popup);
    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "Close");
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(close_btn, close_popup_cb, LV_EVENT_CLICKED, popup); 


}

void close_popup_cb(lv_event_t *e) {
    //lv_obj_t *popup = lv_event_get_user_data(e);
    lv_obj_t *popup = lv_event_get_target(e);
	lv_obj_del(popup);  // 알림창 삭제

	//lv_obj_del(close_btn);
    //lv_event_get_user_data(e);
}

void wifi_ap_setup_cb(void) {

    // wifi 등록 스크린 생성
    if (wifi_ap_setup_screen == NULL)
    {
    wifi_ap_setup_screen = lv_obj_create(NULL);

    lv_obj_set_size(wifi_ap_setup_screen, 480, 272);  // 너비 140, 높이 200
    lv_obj_align(wifi_ap_setup_screen, LV_ALIGN_LEFT_MID, 0, 0);  // 왼쪽 중앙에 정렬, x 오프셋 10
    //lv_obj_add_style(sub_screen, &style_panel, 0);
    }

    lv_scr_load(wifi_ap_setup_screen);

  ////////////////////////////////////////////////////////////////////////////////

    // 헤더 레이블 생성
    header_label = lv_label_create(wifi_ap_setup_screen);
    lv_label_set_text(header_label, "1. WiFi AP 등록");
    lv_obj_add_style(header_label, &style_header_18, 0);
    lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
    //lv_obj_set_style_pad_all(header_label, 5, LV_STATE_DEFAULT);  // 패딩 추가
    lv_obj_set_size(header_label, 480, 25);
    lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

//////////////////////////////////////////////////////////////////////////////// 

   // header_label_back = lv_label_create(wifi_ap_setup_screen);
	header_label_back = lv_imgbtn_create(wifi_ap_setup_screen);	//1. 이미지로 변경 

  //  lv_label_set_text(header_label_back, "^");
	lv_obj_set_size(header_label_back, 25, 25);
	lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn	, NULL);
	
    lv_obj_add_style(header_label_back, &style_header_18, 0);
	
   // lv_obj_align(header_label_back, LV_ALIGN_TOP_RIGHT, 0, 15);
	lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);

	    // 배경 투명도 설정 (이미지 자체의 불투명도를 100% 유지)
    lv_obj_set_style_img_opa(header_label_back, LV_OPA_COVER, LV_PART_MAIN);

    // 특정 색상(예: 0x000000, 검은색)을 투명하게 만들기
    lv_obj_set_style_img_recolor_opa(header_label_back, LV_OPA_TRANSP, LV_PART_MAIN);
    // lv_obj_set_size(header_label_back, 50, 25);
	
   // lv_obj_set_style_text_align(header_label_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);


// 라벨을 클릭 가능하도록 설정initial_cb
lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정

// 클릭 영역을 더 크게 만들기 (선택사항)
//lv_obj_set_style_pad_all(header_label_back, 5, LV_STATE_DEFAULT);  // 패딩 추가

// 클릭 이벤트 추가
//lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_2, LV_EVENT_CLICKED, NULL);
lv_obj_add_event_cb(header_label_back, initial_cb_2, LV_EVENT_CLICKED, NULL);  //back btn 


// 선택사항: 클릭 시각적 피드백을 위한 스타일 추가
lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);  // 클릭시 배경 투명도
lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);  // 클릭시 배경색

lv_obj_set_style_anim_time(header_label_back, 0, LV_STATE_PRESSED);  // 애니메이션 제거

//////////////////////////////////////////////////////////////////////////////// 


    lv_obj_t * wifi_info = lv_img_create(wifi_ap_setup_screen);
    if (wifi_info == NULL) {
        Serial.println("+++ Failed to create image object wifi_info");
        return;
    }

    lv_img_set_src(wifi_info, &img_qr);

    // if (load_image_from_sd(wifi_info, "/LCD_Icon/QR.bmp")) {
    //     Serial.println("+++ wifi_info image loaded successfully");
    // }

    lv_obj_set_pos(wifi_info, 20, 50);
    


////////////////////////////////////////////////////////////////////////////////


inform_label = lv_label_create(wifi_ap_setup_screen);
lv_label_set_text(inform_label, " 스마트폰에서 WiFi 검색하여 \n \"Carebot\" 에 접속한 후에 \n \n 옆의 QR 코드를 촬영하거나 \n 스마트폰 웹브라우저에서 \n 10.10.0.1 에 접속하세요. ");
lv_obj_add_style(inform_label, &style_header_18_2, 0);
lv_obj_align(inform_label, LV_ALIGN_RIGHT_MID, -20, 10);   

    //show_menu_content("WiFi AP Setup", "Configure WiFi Access Point settings...");
}

void initial_cb_1(lv_event_t * e) {

  Serial.println("+++ initial_cb_1 initial before");

  menu_ON = 0;

  //lvgl_create_app_ui();
  //lvgl_update_app_ui();
  lv_scr_load(panel0);

  Serial.println("+++ initial_cb_1 initial after");

    //show_menu_content("Alarm Setup", "Configure system alarm settings...");
}

void initial_cb_2(lv_event_t * e) {

  Serial.println("+++ initial_cb_2 initial before");

  stop_sound();
  
  lv_scr_load(main_screen);

  //initial_scr();

   // 현재 화면을 삭제하고 이전 화면으로 돌아가기
  // lv_obj_t *wifi_screen = lv_scr_act();
  // lv_obj_del(wifi_screen);
   // 이전 화면을 로드 (이전 화면이 있다고 가정)
  // lv_scr_load(lv_obj_create(NULL));
  
  Serial.println("+++ initial_cb_2 initial after");

    //show_menu_content("Alarm Setup", "Configure system alarm settings...");
}

// WiFi network selection event handler
void wifi_network_selected(lv_event_t *e) {

    menu_ON=1;

    //menu();
    init_styles();

    lv_obj_t *btn = lv_event_get_target(e);
    WiFiNetwork *selected_network = (WiFiNetwork*)lv_obj_get_user_data(btn);

    // Create password input screen if network is encrypted
    //if (selected_network->is_encrypted) {
        // Password input screen
        if(password_screen == NULL)
        {
          Serial.println("+++ BEFORE password_screen password_screen");
          password_screen = lv_obj_create(NULL);
        }
        lv_scr_load(password_screen);

        header_label = lv_label_create(password_screen);
        lv_label_set_text_fmt(header_label, "Enter Password for %s", selected_network->ssid);
        lv_obj_add_style(header_label, &style_header_18, 0);
        lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
        lv_obj_set_size(header_label, 480, 25);
        lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); 

        // Text area for password
        password_ta = lv_textarea_create(password_screen);
        lv_textarea_set_placeholder_text(password_ta, "Password");
        lv_textarea_set_password_mode(password_ta, true);
        lv_obj_set_width(password_ta, 200);
        lv_obj_set_height(password_ta, 30);
        //lv_obj_align(password_ta, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_pos(password_ta, 100, 70);

        //lv_textarea_set_max_length(password_ta, PASSWORD_LENGTH);
        lv_textarea_set_password_mode(password_ta, true);
        lv_textarea_set_one_line(password_ta, true);

        // Keyboard
        password_keyboard = lv_keyboard_create(password_screen);
        lv_keyboard_set_textarea(password_keyboard, password_ta);
        lv_obj_align(password_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        setup_keyboard_with_special_chars(password_keyboard);

        // Connect button
        lv_obj_t *connect_btn = lv_btn_create(password_screen);
        lv_obj_t *connect_label = lv_label_create(connect_btn);
        lv_label_set_text(connect_label, "연결");
        //lv_obj_align(connect_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
        lv_obj_set_pos(connect_btn, 370, 70);

        // Store network info with button for later use
        lv_obj_set_user_data(connect_btn, selected_network);
        // ?? lv_obj_add_event_cb(connect_btn, wifi_connect_with_password, LV_EVENT_CLICKED, NULL);
    //} else {
        // Open network, connect directly
    //    connect_to_wifi(selected_network->ssid, "");
    //}

   // header_label_back = lv_label_create(password_screen);
	header_label_back = lv_imgbtn_create(main_screen); 
	lv_obj_set_size(header_label_back, 25, 25);
	
    //lv_label_set_text(header_label_back, "^");
    lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn	, NULL);
	
    lv_obj_add_style(header_label_back, &style_header_18, 0);
	
  // lv_obj_align(header_label_back, LV_ALIGN_TOP_RIGHT, 0, 15);
	lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
  
      // 배경 투명도 설정 (이미지 자체의 불투명도를 100% 유지)
    lv_obj_set_style_img_opa(header_label_back, LV_OPA_COVER, LV_PART_MAIN);

    // 특정 색상(예: 0x000000, 검은색)을 투명하게 만들기
    lv_obj_set_style_img_recolor_opa(header_label_back, LV_OPA_TRANSP, LV_PART_MAIN);
	
    lv_obj_set_size(header_label_back, 50, 25);
    lv_obj_set_style_text_align(header_label_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
	

    // 라벨을 클릭 가능하도록 설정initial_cb
    lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정

    // 클릭 이벤트 추가
  //  lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_1, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(header_label_back, initial_cb_1, LV_EVENT_CLICKED, NULL);//back btn 


    // 선택사항: 클릭 시각적 피드백을 위한 스타일 추가
    lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);  // 클릭시 배경 투명도
    lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);  // 클릭시 배경색

//////////////////////////////////////////////////////////////////////////////// 
}

void create_wifi_selection_screen() {
    // wifi 등록 스크린 생성
    Serial.println("+++ BEGIN in create_wifi_selection_screen:");

    menu_ON=1;
    
    //menu();

    init_styles();
   
    if (wifi_screen == NULL)
    {
    Serial.println("+++BEFORE wifi_screen lv_obj_create in create_wifi_selection_screen");

    wifi_screen = lv_obj_create(NULL);

    lv_obj_set_size(wifi_screen, 480, 272);  // 너비 140, 높이 200
    lv_obj_align(wifi_screen, LV_ALIGN_LEFT_MID, 0, 0);  // 왼쪽 중앙에 정렬, x 오프셋 10
    //lv_obj_add_style(sub_screen, &style_panel, 0);
    }

    Serial.println("+++ 1 in create_wifi_selection_screen:");


    lv_scr_load(wifi_screen);

  ////////////////////////////////////////////////////////////////////////////////
    // 헤더 레이블 생성
    header_label = lv_label_create(wifi_screen);
	

	
    lv_label_set_text(header_label, "WIFI 목록");
    lv_obj_add_style(header_label, &style_header_18, 0);
    lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
    lv_obj_set_size(header_label, 480, 25);
    lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); 

	header_label_back = lv_imgbtn_create(wifi_screen);
	lv_obj_set_size(header_label_back, 25, 25);
	
 // lv_label_set_text(header_label_back, "^");
	lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn	, NULL);
	
    lv_obj_add_style(header_label_back, &style_header_18, 0);
  //  lv_obj_align(header_label_back, LV_ALIGN_TOP_RIGHT, 0, 15);
	lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
 //   lv_obj_set_size(header_label_back, 50, 25);
     // 배경 투명도 설정 (이미지 자체의 불투명도를 100% 유지)
    lv_obj_set_style_img_opa(header_label_back, LV_OPA_COVER, LV_PART_MAIN);

    // 특정 색상(예: 0x000000, 검은색)을 투명하게 만들기
    lv_obj_set_style_img_recolor_opa(header_label_back, LV_OPA_TRANSP, LV_PART_MAIN);
 
    lv_obj_set_style_text_align(header_label_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

    // 라벨을 클릭 가능하도록 설정initial_cb
    lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정

    // 클릭 이벤트 추가
   // lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_1, LV_EVENT_CLICKED, NULL);  //백버튼.. 메뉴 화면에서 뒤로 가기 
	lv_obj_add_event_cb(header_label_back, initial_cb_1, LV_EVENT_CLICKED, NULL);  //back btn 

    // 선택사항: 클릭 시각적 피드백을 위한 스타일 추가
    lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);  // 클릭시 배경 투명도
    lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);  // 클릭시 배경색

//////////////////////////////////////////////////////////////////////////////// 
       Serial.println("+++ 1 in create_wifi_selection_screen:");

    // WiFi List
    wifi_list = lv_list_create(wifi_screen);
    //wifi_list = lv_list_create(wifi_list_screen);
    lv_obj_set_size(wifi_list, 320, 200);
    lv_obj_align(wifi_list, LV_ALIGN_CENTER, 0, 20);

    // Populate list
    //scan_wifi_networks();


    network_count = 10;

    if (available_networks[0].ssid == "")
    {
      Serial.println("+++ wifi 리스트 수신 실패, 다시 시도 바람");
    }
    for (int i = 0; i < network_count; i++) {
        //lv_obj_t *btn = lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, available_networks[i].ssid);
        lv_obj_t *btn = lv_list_add_btn(wifi_list, ".", available_networks[i].ssid);
        Serial.printf("+++ Network %d: SSID: %s\n", i, available_networks[i].ssid);
        lv_obj_set_user_data(btn, (void*)&available_networks[i]);
        lv_obj_add_event_cb(btn, wifi_network_selected, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_anim_time(btn, 0, LV_STATE_PRESSED);  // 애니메이션 제거
        
    }

  Serial.println("+++ END in create_wifi_selection_screen:");
    
}

// 이전에 구현했던 img_click_event_cb_wifi 함수는 그대로 유지
void img_click_event_cb_wifi(lv_event_t *e) {

    menu_ON = 1;

    Serial.println("+++ serial2_wifilist in img_click_event_cb_wifi");
    // 요청 메시지 구조체 사용    
      RequestMessage request = {};
      request.header.start_marker = 0xFF;
      request.header.type = MSG_REQUEST;
      request.header.timestamp = time(NULL);
      request.header.seq_num = g_state.sequence_number++;
      request.header.length = sizeof(RequestMessage) - sizeof(MessageHeader);
      request.request_type = REQ_WIFI_SCANLIST;
      request.checksum = calculate_checksum(&request, sizeof(RequestMessage) - 1);
      send_message(&request, sizeof(RequestMessage));   
///////////////////////////////////////////////////////////////

    waiting_event_handler("\nWIFI 리스트 수신중...\n잠시만(약10초) 기다려 주세요.\n수신결과가 원활치 않은 경우 \n잠시후 재시도 부탁 드립니다.");

     
}

void alarm_setup_cb(void) {

  Serial.println("+++ cb alarm initial before");

  ////////////////////////////////////////////////////////////////////////////////
  
  if (alarm_setup_screen == NULL)
    {
    alarm_setup_screen = lv_obj_create(NULL);

    lv_obj_set_size(alarm_setup_screen, 480, 272);  // 너비 140, 높이 200
    //lv_obj_align(wifi_screen, LV_ALIGN_LEFT_MID, 0, 0);  // 왼쪽 중앙에 정렬, x 오프셋 10
    lv_obj_add_style(alarm_setup_screen, &style_panel, 0);
    }

    lv_scr_load(alarm_setup_screen);


    // 헤더 레이블 생성
    header_label = lv_label_create(alarm_setup_screen);
    lv_label_set_text(header_label, "2. 알람 볼륨");
    lv_obj_add_style(header_label, &style_header_18, 0);
    lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
    //lv_obj_set_style_pad_all(header_label, 5, LV_STATE_DEFAULT);  // 패딩 추가
    lv_obj_set_size(header_label, 480, 25);
    lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

 

   // header_label_back = lv_label_create(alarm_setup_screen);
	header_label_back = lv_imgbtn_create(alarm_setup_screen);  //1. 이미지로 변경 
    //lv_label_set_text(header_label_back, "^");
	lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn	, NULL);
	
    lv_obj_add_style(header_label_back, &style_header_18, 0);
    //lv_obj_align(header_label_back, LV_ALIGN_TOP_RIGHT, 0, 15);
	lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
	    // 배경 투명도 설정 (이미지 자체의 불투명도를 100% 유지)
    lv_obj_set_style_img_opa(header_label_back, LV_OPA_COVER, LV_PART_MAIN);

    // 특정 색상(예: 0x000000, 검은색)을 투명하게 만들기
    lv_obj_set_style_img_recolor_opa(header_label_back, LV_OPA_TRANSP, LV_PART_MAIN);
	
    lv_obj_set_size(header_label_back, 25, 25);
    lv_obj_set_style_text_align(header_label_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

// 라벨을 클릭 가능하도록 설정initial_cb
lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정

// 클릭 영역을 더 크게 만들기 (선택사항)
//lv_obj_set_style_pad_all(header_label_back, 5, LV_STATE_DEFAULT);  // 패딩 추가

// 클릭 이벤트 추가
//lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_2, LV_EVENT_CLICKED, NULL);
lv_obj_add_event_cb(header_label_back, initial_cb_2, LV_EVENT_CLICKED, NULL);  //back btn


// 선택사항: 클릭 시각적 피드백을 위한 스타일 추가
lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);  // 클릭시 배경 투명도
lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);  // 클릭시 배경색



  create_volume_control(alarm_setup_screen);
   
  //Serial.println("+++ cb alarm initial after");

    //show_menu_content("Alarm Setup", "Configure system alarm settings...");
}

void relay_info_cb(void) {

  Serial.println("+++ relay_info_cb initial before");

////////////////////////////////////////////////////////////////////////////////
  
    lv_obj_t * sub_screen = lv_obj_create(NULL);

    lv_obj_set_size(sub_screen, 480, 270);  // 너비 140, 높이 200
    //lv_obj_align(wifi_screen, LV_ALIGN_LEFT_MID, 0, 0);  // 왼쪽 중앙에 정렬, x 오프셋 10
    lv_obj_add_style(sub_screen, &style_panel, 0);

    lv_scr_load(sub_screen);


    // 헤더 레이블 생성
    header_label = lv_label_create(sub_screen);
    lv_label_set_text(header_label, "3. 혈뇨 정보 전송");
    lv_obj_add_style(header_label, &style_header_18, 0);
    lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
    //lv_obj_set_style_pad_all(header_label, 5, LV_STATE_DEFAULT);  // 패딩 추가
    lv_obj_set_size(header_label, 480, 25);
    lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

 

    //header_label_back = lv_label_create(sub_screen);
		header_label_back = lv_imgbtn_create(sub_screen);
	
    //lv_label_set_text(header_label_back, "^");
		lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn	, NULL);
    lv_obj_add_style(header_label_back, &style_header_18, 0);
	
    //lv_obj_align(header_label_back, LV_ALIGN_TOP_RIGHT, 0, 15);
		lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
	    // 배경 투명도 설정 (이미지 자체의 불투명도를 100% 유지)
    lv_obj_set_style_img_opa(header_label_back, LV_OPA_COVER, LV_PART_MAIN);

    // 특정 색상(예: 0x000000, 검은색)을 투명하게 만들기
    lv_obj_set_style_img_recolor_opa(header_label_back, LV_OPA_TRANSP, LV_PART_MAIN);
    //lv_obj_set_size(header_label_back, 50, 25);
		lv_obj_set_size(header_label_back, 25, 25);
	
    lv_obj_set_style_text_align(header_label_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

	
// 라벨을 클릭 가능하도록 설정initial_cb
lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정

// 클릭 영역을 더 크게 만들기 (선택사항)
//lv_obj_set_style_pad_all(header_label_back, 5, LV_STATE_DEFAULT);  // 패딩 추가

// 클릭 이벤트 추가
//lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_2, LV_EVENT_CLICKED, NULL);
lv_obj_add_event_cb(header_label_back, initial_cb_2, LV_EVENT_CLICKED, NULL);  //back btn


// 선택사항: 클릭 시각적 피드백을 위한 스타일 추가
lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);  // 클릭시 배경 투명도
lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);  // 클릭시 배경색

//////////////////////////////////////////////////////////////////////////////// 

   create_relay_settings(sub_screen);
   
  Serial.println("+++ relay_info_cb initial after");


}

void urination_cb(void) {

  Serial.println("+++ urination_cb initial before");

  ////////////////////////////////////////////////////////////////////////////////
  
    lv_obj_t * sub_screen = lv_obj_create(NULL);

    lv_obj_set_size(sub_screen, 480, 270);  // 너비 140, 높이 200
    //lv_obj_align(wifi_screen, LV_ALIGN_LEFT_MID, 0, 0);  // 왼쪽 중앙에 정렬, x 오프셋 10
    lv_obj_add_style(sub_screen, &style_panel, 0);

    lv_scr_load(sub_screen);


    // 헤더 레이블 생성
    header_label = lv_label_create(sub_screen);
    lv_label_set_text(header_label, "4. 배뇨 유도");
    lv_obj_add_style(header_label, &style_header_18, 0);
    lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
    //lv_obj_set_style_pad_all(header_label, 5, LV_STATE_DEFAULT);  // 패딩 추가
    lv_obj_set_size(header_label, 480, 25);
    lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

 

//    header_label_back = lv_label_create(sub_screen);
//    lv_label_set_text(header_label_back, "^");
//    lv_obj_add_style(header_label_back, &style_header_18, 0);
//    lv_obj_align(header_label_back, LV_ALIGN_TOP_RIGHT, 0, 15);
//    lv_obj_set_size(header_label_back, 50, 25);
//    lv_obj_set_style_text_align(header_label_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);


	header_label_back = lv_imgbtn_create(sub_screen); 
	lv_obj_set_size(header_label_back, 25, 25);
	lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn	, NULL);
	lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
	//lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);
	    // 배경 투명도 설정 (이미지 자체의 불투명도를 100% 유지)
    lv_obj_set_style_img_opa(header_label_back, LV_OPA_COVER, LV_PART_MAIN);

    // 특정 색상(예: 0x000000, 검은색)을 투명하게 만들기
    lv_obj_set_style_img_recolor_opa(header_label_back, LV_OPA_TRANSP, LV_PART_MAIN);
	


	

// 라벨을 클릭 가능하도록 설정initial_cb
lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정

// 클릭 영역을 더 크게 만들기 (선택사항)
//lv_obj_set_style_pad_all(header_label_back, 5, LV_STATE_DEFAULT);  // 패딩 추가

// 클릭 이벤트 추가
// lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_2, LV_EVENT_CLICKED, NULL);
lv_obj_add_event_cb(header_label_back, initial_cb_2, LV_EVENT_CLICKED, NULL);  //back btn 


// 선택사항: 클릭 시각적 피드백을 위한 스타일 추가
lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);  // 클릭시 배경 투명도
lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);  // 클릭시 배경색

//////////////////////////////////////////////////////////////////////////////// 
   urination_screen(sub_screen);
   
  Serial.println("+++ urination_cb initial after");


}

void terminal_info_cb(void) {
  Serial.println("+++ terminal_info_cb initial before");

 ////////////////////////////////////////////////////////////////////////////////
  
    lv_obj_t * sub_screen = lv_obj_create(NULL);

    lv_obj_set_size(sub_screen, 480, 270);  // 너비 140, 높이 200
    //lv_obj_align(wifi_screen, LV_ALIGN_LEFT_MID, 0, 0);  // 왼쪽 중앙에 정렬, x 오프셋 10
    lv_obj_add_style(sub_screen, &style_panel, 0);

    lv_scr_load(sub_screen);


    // 헤더 레이블 생성
    header_label = lv_label_create(sub_screen);
    lv_label_set_text(header_label, "5. 단말 정보 보기");
    lv_obj_add_style(header_label, &style_header_18, 0);
    lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
    //lv_obj_set_style_pad_all(header_label, 5, LV_STATE_DEFAULT);  // 패딩 추가
    lv_obj_set_size(header_label, 480, 25);
    lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

 

//    header_label_back = lv_label_create(sub_screen);
//    lv_label_set_text(header_label_back, "^");
//    lv_obj_add_style(header_label_back, &style_header_18, 0);
//    lv_obj_align(header_label_back, LV_ALIGN_TOP_RIGHT, 0, 15);
//    lv_obj_set_size(header_label_back, 50, 25);
    lv_obj_set_style_text_align(header_label_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

	header_label_back = lv_imgbtn_create(sub_screen); 
	lv_obj_set_size(header_label_back, 25, 25);
	lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn	, NULL);
	lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
	
	    // 배경 투명도 설정 (이미지 자체의 불투명도를 100% 유지)
    lv_obj_set_style_img_opa(header_label_back, LV_OPA_COVER, LV_PART_MAIN);

    // 특정 색상(예: 0x000000, 검은색)을 투명하게 만들기
    lv_obj_set_style_img_recolor_opa(header_label_back, LV_OPA_TRANSP, LV_PART_MAIN);
	
	lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(header_label_back, initial_cb_2, LV_EVENT_CLICKED, NULL);  //back btn 

// 라벨을 클릭 가능하도록 설정initial_cb
//lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정

// 클릭 영역을 더 크게 만들기 (선택사항)
//lv_obj_set_style_pad_all(header_label_back, 5, LV_STATE_DEFAULT);  // 패딩 추가

// 클릭 이벤트 추가
//lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_2, LV_EVENT_CLICKED, NULL);

// 선택사항: 클릭 시각적 피드백을 위한 스타일 추가
lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);  // 클릭시 배경 투명도
lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);  // 클릭시 배경색

//////////////////////////////////////////////////////////////////////////////// 
    terminal_screen(sub_screen);
   
  Serial.println("+++ terminal_info_cb initial after");
}

void factory_menu_cb(void) {
    //show_menu_content("Factory Menu", "Warning: Factory settings modification...");
    Serial.println("+++ factory_menu_cb initial before");

  ////////////////////////////////////////////////////////////////////////////////
  
    lv_obj_t * sub_screen = lv_obj_create(NULL);

    lv_obj_set_size(sub_screen, 480, 270);  // 너비 140, 높이 200
    //lv_obj_align(wifi_screen, LV_ALIGN_LEFT_MID, 0, 0);  // 왼쪽 중앙에 정렬, x 오프셋 10
    lv_obj_add_style(sub_screen, &style_panel, 0);

    lv_scr_load(sub_screen);


    // 헤더 레이블 생성
    header_label = lv_label_create(sub_screen);
    //lv_label_set_text(header_label, "6. Factory 메뉴");
    lv_label_set_text(header_label, "6. 공장 초기화");
    lv_obj_add_style(header_label, &style_header_18, 0);
    lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
    //lv_obj_set_style_pad_all(header_label, 5, LV_STATE_DEFAULT);  // 패딩 추가
    lv_obj_set_size(header_label, 480, 25);
    lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);

 

//    header_label_back = lv_label_create(sub_screen);
//    lv_label_set_text(header_label_back, "^");
//    lv_obj_add_style(header_label_back, &style_header_18, 0);
//    lv_obj_align(header_label_back, LV_ALIGN_TOP_RIGHT, 0, 15);
//    lv_obj_set_size(header_label_back, 50, 25);
	header_label_back = lv_imgbtn_create(sub_screen); 

	lv_obj_set_size(header_label_back, 25, 25);
	lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn	, NULL);
	lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
	    // 배경 투명도 설정 (이미지 자체의 불투명도를 100% 유지)
    lv_obj_set_style_img_opa(header_label_back, LV_OPA_COVER, LV_PART_MAIN);

    // 특정 색상(예: 0x000000, 검은색)을 투명하게 만들기
    lv_obj_set_style_img_recolor_opa(header_label_back, LV_OPA_TRANSP, LV_PART_MAIN);
	lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(header_label_back, initial_cb_2, LV_EVENT_CLICKED, NULL);  //back btn 
    lv_obj_set_style_text_align(header_label_back, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
	

// 라벨을 클릭 가능하도록 설정initial_cb
//lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);  // 클릭 가능하도록 설정

// 클릭 영역을 더 크게 만들기 (선택사항)
//lv_obj_set_style_pad_all(header_label_back, 5, LV_STATE_DEFAULT);  // 패딩 추가

// 클릭 이벤트 추가
//lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_2, LV_EVENT_CLICKED, NULL);

// 선택사항: 클릭 시각적 피드백을 위한 스타일 추가
lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);  // 클릭시 배경 투명도
lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);  // 클릭시 배경색

//////////////////////////////////////////////////////////////////////////////// 
    factory_screen(sub_screen);
   
  Serial.println("+++ factory_menu_cb initial after");
}
void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider_recv = lv_event_get_target(e);
    volume_value = lv_slider_get_value(slider_recv);
    
    // 볼륨 값 레이블 업데이트
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d", volume_value);
    lv_label_set_text(volume_label, buf);

    Serial.printf("+++ 볼륨값 : %d \n", volume_value);
    
    audio_set_volume(volume_value);
}

void create_volume_control(lv_obj_t *parent)
{
    // 스피커 아이콘 생성
    speaker = lv_img_create(alarm_setup_screen);
    if (speaker == NULL) {
        Serial.println("+++ Failed to create image object speaker");
        return;
    }

    lv_img_set_src(speaker, &img_speaker);
    lv_obj_set_pos(speaker, 20, 50);
    lv_obj_add_event_cb(speaker, urination_event_handler_1, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(speaker, LV_OBJ_FLAG_CLICKABLE); 
    lv_obj_set_style_anim_time(speaker, 0, LV_STATE_PRESSED);  // 애니메이션 제거  

    // 볼륨 값 표시 레이블
    if (volume_label == NULL)
    {
        volume_label = lv_label_create(parent);
        char buf[10];  // 충분한 버퍼 크기 확보
        
        // 저장된 volume_value 값으로 레이블 초기화
        sprintf(buf, "%ld", volume_value);
        lv_label_set_text(volume_label, buf);
        
        lv_obj_set_pos(volume_label, 360, 150);
        lv_obj_move_foreground(volume_label);
    }
    
    // 세로 슬라이더 생성
    if (slider == NULL)
    {
        slider = lv_slider_create(parent);
        lv_obj_set_size(slider, 15, 120);  // 너비와 높이 설정
        lv_obj_set_pos(slider, 400, 100);
        
        // 세로 방향으로 설정
        lv_slider_set_mode(slider, LV_SLIDER_MODE_SYMMETRICAL); // 세로 모드
        lv_slider_set_range(slider, 0, 10);
        
        // 저장된 volume_value 값으로 슬라이더 초기값 설정
        lv_slider_set_value(slider, volume_value, LV_ANIM_OFF);
        
        // 스타일 초기화
        lv_style_init(&style_indicator);
        lv_style_init(&style_knob);

        // 스타일 속성 설정
        lv_style_set_bg_color(&style_indicator, lv_color_hex(0x0000FF));  // 파란색 표시바
        lv_style_set_bg_color(&style_knob, lv_color_hex(0x000000));      // 검은색 노브

        // 슬라이더 스타일 적용
        lv_obj_set_style_bg_color(slider, lv_color_hex(0x808080), 0);    // 회색 배경
        lv_obj_add_style(slider, &style_indicator, LV_PART_INDICATOR);
        lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);
        
        // 이벤트 핸들러 설정
        lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // 화면 표시 전에 오디오 볼륨 설정 적용
    audio_set_volume(volume_value);
    
    Serial.printf("+++ 초기 볼륨값 적용: %d \n", volume_value);
}

void radio_event_handler(lv_event_t *e)
{
    Serial.println("+++ radio_event_handler");

    // 이벤트가 발생한 체크박스 객체 가져오기
    lv_obj_t *checkbox = lv_event_get_target(e);
        
    // 체크박스의 부모 컨테이너(item_cont)를 가져오기
    lv_obj_t *item_cont = lv_obj_get_parent(checkbox);
    // item_cont의 부모 컨테이너(cont2) 가져오기
    lv_obj_t *cont2 = lv_obj_get_parent(item_cont);
    
    // 현재 체크박스의 상태 확인
    bool is_checked = lv_obj_has_state(checkbox, LV_STATE_CHECKED);
    
    // 체크된 상태라면
    if(is_checked) {
        // cont2의 모든 자식 컨테이너(item_cont)를 순회
        uint32_t item_cnt = lv_obj_get_child_cnt(cont2);

        for(uint32_t i = 0; i < item_cnt; i++) {
            lv_obj_t *other_item_cont = lv_obj_get_child(cont2, i);
            
            // item_cont 안의 체크박스를 찾음 (첫 번째 자식이 체크박스)
            lv_obj_t *other_checkbox = lv_obj_get_child(other_item_cont, 0);
            
            // 현재 선택된 체크박스가 아닌 다른 체크박스들의 체크 상태를 해제
            if(other_checkbox != checkbox) {
                Serial.println("+++ radio_event_handler, i:"); 
                Serial.println(i);
                lv_obj_clear_state(other_checkbox, LV_STATE_CHECKED);
            }
            else {
              amount = i+1;
              Serial.println("+++ radio_event_handler, amount:"); 
              Serial.println(amount);
            }
        }
    } else {
        // 체크가 해제된 상태라면, 다시 체크 상태로 되돌림
        // (라디오 버튼처럼 항상 하나는 선택되어 있어야 하므로)
        Serial.println("+++ radio_event_handler else"); 
        lv_obj_add_state(checkbox, LV_STATE_CHECKED);
    }
}

void create_relay_settings(lv_obj_t *parent)
{
    // 라디오 버튼 그룹 컨테이너 생성
    //lv_obj_t * cont2 = lv_obj_create(lv_scr_act());
    //lv_obj_t * cont2 = lv_obj_create(parent);
    cont2 = lv_obj_create(parent);

    lv_obj_set_flex_flow(cont2, LV_FLEX_FLOW_ROW);
    lv_obj_set_size(cont2, 250, 100);
    lv_obj_align(cont2, LV_ALIGN_CENTER, 0, 0);
    
    // 컨테이너 정렬 설정
    lv_obj_set_style_flex_main_place(cont2, LV_FLEX_ALIGN_SPACE_EVENLY, 0);
    lv_obj_set_style_flex_cross_place(cont2, LV_FLEX_ALIGN_CENTER, 0);
    
    // 체크박스 스타일 설정
    static lv_style_t style_radio;
    lv_style_init(&style_radio);
    lv_style_set_radius(&style_radio, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&style_radio, 2);
    lv_style_set_border_color(&style_radio, lv_color_black());
    lv_style_set_bg_color(&style_radio, lv_color_white());
    lv_style_set_pad_all(&style_radio, 4);

    static lv_style_t style_radio_chk;
    lv_style_init(&style_radio_chk);
    lv_style_set_bg_color(&style_radio_chk, lv_color_black());
    lv_style_set_radius(&style_radio_chk, 3);
    lv_style_set_radius(&style_radio_chk, LV_RADIUS_CIRCLE);


    const char* labels[] = {"소", "중", "대"};
    
    for (unsigned char i = 0; i < 3; i++)
    {
        // 각 항목을 위한 컨테이너 생성
        lv_obj_t * item_cont = lv_obj_create(cont2);
        lv_obj_remove_style_all(item_cont);  // 컨테이너 스타일 제거
        lv_obj_set_size(item_cont, 40, 60);  // 적절한 크기 설정
        lv_obj_set_flex_flow(item_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(item_cont, 
                            LV_FLEX_ALIGN_CENTER,     // 수평 중앙 정렬
                            LV_FLEX_ALIGN_CENTER,     // 수직 중앙 정렬
                            LV_FLEX_ALIGN_CENTER);    // 교차축 중앙 정렬
        
        // 체크박스 생성
        lv_obj_t * checkbox = lv_checkbox_create(item_cont);

        lv_checkbox_set_text(checkbox, "");
        lv_obj_add_style(checkbox, &style_radio, LV_PART_INDICATOR);
        lv_obj_add_style(checkbox, &style_radio_chk, LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_add_event_cb(checkbox, radio_event_handler, LV_EVENT_CLICKED, NULL);
        //lv_obj_add_event_cb(checkbox, radio_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

        
        // 라벨 생성
        lv_obj_t * label = lv_label_create(item_cont);
        lv_label_set_text(label, labels[i]);
        
        // 첫 번째 체크박스 선택
        if (i == 0) {
            lv_obj_add_state(checkbox, LV_STATE_CHECKED);
        }
        lv_obj_set_style_anim_time(checkbox, 0, LV_STATE_PRESSED);  // 애니메이션 제거
        //lv_obj_set_style_anim_time(checkbox, 0, LV_STATE_CHECKED);  // 애니메이션 제거

    }
     
    // "전송" 버튼 생성
    //lv_obj_t* btn = lv_btn_create(lv_scr_act());
    lv_obj_t* btn = lv_btn_create(parent);

    lv_obj_set_size(btn, 200, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);

    // 버튼에 라벨 추가
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, "전송");
    lv_obj_center(label); 
    lv_obj_add_event_cb(btn, relay_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_anim_time(btn, 0, LV_STATE_PRESSED);  // 애니메이션 제거
 
}

void relay_event_handler(lv_event_t* e)
{
    Serial.printf("+++ relay_event_handler, amount: %d \n", amount);   
    BloodStatusMessage msg = {};
    msg.header.start_marker = 0xFF;
    msg.header.type = MSG_BLOOD_STATUS;
    Serial.println("+++ msg.header.type1:");
    Serial.println(msg.header.type);

    msg.header.timestamp = time(NULL);
    msg.header.seq_num = g_state.sequence_number++;
    msg.header.length = sizeof(BloodStatusMessage) - sizeof(MessageHeader);
 
    //strncpy(msg.amount, (char*)amount, sizeof(msg.amount));

    msg.amount = amount;
    Serial.println("+++ msg.amount: ");
    Serial.println(msg.amount);
  
    msg.checksum = calculate_checksum(&msg, sizeof(BloodStatusMessage) - 1);
    send_message(&msg, sizeof(BloodStatusMessage));

}

/////////////// 4. 배뇨 유도 ///////////////

void urination_sound_handler(lv_event_t* e) {
    SoundType newSound = URINATION;
    Serial.println("+++ in urination_sound_handler");

    xQueueSend(soundQueue, &newSound, 0);  // 비차단 방식으로 큐에 전송
}

void water_sound_handler(lv_event_t* e) {
    SoundType newSound = WATER;
    Serial.println("+++ in water_sound_handler");

    xQueueSend(soundQueue, &newSound, 0);  // 비차단 방식으로 큐에 전송
}

void urination_event_handler_1(lv_event_t *e)
{
    Serial.println("+++ BEFORE audio.requestStop(); in urination_event_handler_1"); 
                     // audio.requestStop();  // 재생 중지 요청

    //if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {  
      Serial.println("+++ BEFORE mp3play in urination_event_handler_1");       
      //mp3play("001");
    Serial.println("+++ 물소리");  
      //play_water_sound(); // one time play
      play_water_sound(REPEAT_INFINITE, 0);  // repeat infinite

    //  xSemaphoreGive(sdMutex);
    //}
    Serial.println("+++ AFTER 물소리 in urination_event_handler_1"); 
}

void urination_event_handler_2(lv_event_t *e)
{
    Serial.println("+++ BEFORE audio.requestStop(); in urination_event_handler_2"); 
                      // ???     audio.requestStop();  // 재생 중지 요청
    Serial.println("+++ 쉬소리");  
    //if (xSemaphoreTake(sdMutex, portMAX_DELAY) == pdTRUE) {        
      //mp3play("002");
      //play_urination_sound();     // one time play
      play_urination_sound(REPEAT_INFINITE, 0);     // repeat infinite

    //  xSemaphoreGive(sdMutex);
    //}
}


void urination_screen(lv_obj_t *parent)
{
        
    // "물 소리" 버튼 생성
    lv_obj_t* btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 200, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_anim_time(btn, 0, LV_STATE_PRESSED);  // 애니메이션 제거

    // 버튼에 라벨 추가
    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, "물 소리");
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    lv_obj_center(label);  

    //lv_obj_add_event_cb(btn, urination_event_handler_1, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn, water_sound_handler, LV_EVENT_CLICKED, NULL);

     // "쉬 소리" 버튼 생성
    lv_obj_t* btn2 = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn2, 200, 50);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_anim_time(btn2, 0, LV_STATE_PRESSED);  // 애니메이션 제거

    // 버튼에 라벨 추가
    lv_obj_t* label2 = lv_label_create(btn2);
    lv_label_set_text(label2, "쉬 소리");
    lv_obj_set_style_text_color(label2, lv_color_black(), 0);
    lv_obj_center(label2); 

    //lv_obj_add_event_cb(btn2, urination_event_handler_2, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn2, urination_sound_handler, LV_EVENT_CLICKED, NULL);

}


/////////////// 5. 단말 정보 보기 ///////////////

void terminal_screen(lv_obj_t *parent) {

    static lv_style_t style_terminal;
    lv_style_init(&style_terminal);
    lv_style_set_border_width(&style_terminal, 1);
    lv_style_set_border_color(&style_terminal, lv_color_hex(0x0019F4));
    lv_style_set_width(&style_terminal, 270);  // 폭을 120 px로 설정
    lv_style_set_height(&style_terminal, 25); // 높이를 60 px로 설정

    // Create labels for network information
    // Model name
    lv_obj_t* model_title = lv_label_create(parent);
    lv_label_set_text(model_title, "모델명:");
    lv_obj_set_pos(model_title, 25, 50);
    //lv_obj_align(model_title, LV_ALIGN_TOP_LEFT, 20, 50);
    
    model_label = lv_label_create(parent);
    lv_label_set_text(model_label, "Hygera Model 1");  // Will be set by update function
    lv_obj_set_pos(model_label, 150, 50);
    lv_obj_add_style(model_label, &style_terminal, 0);
    //lv_obj_align_to(model_label, model_title, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    
    // SW Version
    lv_obj_t* sw_ver_title = lv_label_create(parent);
    lv_label_set_text(sw_ver_title, "SW버전:");
    lv_obj_set_pos(sw_ver_title, 25, 80);
    //lv_obj_align(sw_ver_title, LV_ALIGN_TOP_LEFT, 20, 80);
    
    sw_ver_label = lv_label_create(parent);
    lv_label_set_text(sw_ver_label, "0.1");
    lv_obj_set_pos(sw_ver_label, 150, 80);
    lv_obj_add_style(sw_ver_label, &style_terminal, 0);
    //lv_obj_align_to(sw_ver_label, sw_ver_title, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    
    // Serial Number
    lv_obj_t* serial_title = lv_label_create(parent);
    lv_label_set_text(serial_title, "시리얼번호:");
    lv_obj_set_pos(serial_title, 25, 110);
    //lv_obj_align(serial_title, LV_ALIGN_TOP_LEFT, 20, 110);
    
    serial_label = lv_label_create(parent);
    lv_label_set_text(serial_label, "111");  // Will be set by update function
    lv_obj_set_pos(serial_label, 150, 110);
    lv_obj_add_style(serial_label, &style_terminal, 0);
    //lv_obj_align_to(serial_label, serial_title, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    
    // SSID
    lv_obj_t* ssid_title = lv_label_create(parent);
    lv_label_set_text(ssid_title, "SSID:");
    lv_obj_set_pos(ssid_title, 25, 140);
    //lv_obj_align(ssid_title, LV_ALIGN_TOP_LEFT, 20, 140);
    
    ssid_label = lv_label_create(parent);
	  lv_label_set_text(ssid_label, ssid_main);  // Will be set by update function
    lv_obj_set_pos(ssid_label, 150, 140);
    lv_obj_add_style(ssid_label, &style_terminal, 0);
    //lv_obj_align_to(ssid_label, ssid_title, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    
    // IP Address
    lv_obj_t* ip_title = lv_label_create(parent);
    lv_label_set_text(ip_title, "IP 주소:");
    lv_obj_set_pos(ip_title, 25, 170);
    //lv_obj_align(ip_title, LV_ALIGN_TOP_LEFT, 20, 170);
    
    ip_label = lv_label_create(parent);
    lv_label_set_text(ip_label, ip_addr_main);

    //lv_label_set_text(ip_label, WiFi.localIP().toString().c_str());
    //Serial.printf("WiFi.localIP().toString().c_str(): %s \n", WiFi.localIP().toString().c_str());

    lv_obj_set_pos(ip_label, 150, 170);
    lv_obj_add_style(ip_label, &style_terminal, 0);
    //lv_obj_align_to(ip_label, ip_title, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    
    // Gateway
    lv_obj_t* gateway_title = lv_label_create(parent);
    lv_label_set_text(gateway_title, "Gateway:");
    lv_obj_set_pos(gateway_title, 25, 200);
    //lv_obj_align(gateway_title, LV_ALIGN_TOP_LEFT, 20, 200);
    
    gateway_label = lv_label_create(parent);
    //Serial.printf("WiFi.gatewayIP().toString().c_str(): %s \n", WiFi.gatewayIP().toString().c_str());
    //lv_label_set_text(gateway_label, WiFi.gatewayIP().toString().c_str());
    lv_label_set_text(gateway_label, gateway_main);

    lv_obj_set_pos(gateway_label, 150, 200);
    lv_obj_add_style(gateway_label, &style_terminal, 0);
    //lv_obj_align_to(gateway_label, gateway_title, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
}

// Function to update network information
void update_network_info(const char* model, const char* serial, const char* ssid,
                        const char* ip, const char* gateway) {
    if (model) lv_label_set_text(model_label, model);
    if (serial) lv_label_set_text(serial_label, serial);
    if (ssid) lv_label_set_text(ssid_label, ssid);
    if (ip) lv_label_set_text(ip_label, ip);
    if (gateway) lv_label_set_text(gateway_label, gateway);
    
    // Example of updating the information
    /*
    update_network_info(
        "Device Model",  // 모델명
        "SN12345678",   // 시리얼번호
        "NetworkSSID",  // SSID
        "0.0.0.0",     // IP 주소
        "0.0.0.0"      // Gateway
    );
    */
}

/////////////// 6. 공장 초기화 ///////////////

/////////////////////////////////////////////////////////

// 확인 버튼 이벤트 처리 (factory_event_handler)
void msg_close_factory(lv_event_t * e) {
  Serial.println("+++ msg_close_factory \n");

  factory_menu_cb();
  //factory_screen(NULL);
}

void msg_close_handler_parent(lv_event_t * e) {
    
    Serial.println("+++ alert_msgbox del before \n");
    //initial_cb_1();
    //lvgl_create_app_ui();
    lvgl_update_app_ui();
    //cover_OPEN = 0;
}
 
void msg_close_handler_current(lv_event_t * e) {
    lv_obj_t * msgbox = lv_event_get_current_target(e);
    //lv_obj_t *alert = lv_obj_get_parent(msgbox);
    Serial.println("+++ msgbox close before");
    lv_msgbox_close(msgbox);
}

void factory_event_handler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);


    if (code == LV_EVENT_CLICKED) {

        Serial.println("+++ factory_event_handler, LV_EVENT_CLICKED");

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



            msg_label = lv_label_create(alert_msgbox);

            int caseA = check_password_requirements(entered_password);
  
            Serial.println("+++ 2, case:");
            Serial.println(caseA);

            if (caseA == 0)
             {lv_label_set_text(msg_label, "암호는 영문 대/소문자, 숫자,\n특수문자를 모두 포함해야 합니다.");}
            else if (caseA == 2)
             {lv_label_set_text(msg_label, "패스워드를 입력하세요.");}
            else if (caseA == 3)
             {lv_label_set_text(msg_label, "패스워드는 최소 8자이상이어야 합니다.");}

            else if (caseA == 1)
             {

              if (verify_password(entered_password)) {
                is_authenticated = true;
                //lv_obj_t* alert = lv_msgbox_create(NULL, "성공", "암호가 확인되었습니다.", buttons, true);
                //lv_obj_add_event_cb(alert, msg_close_handler_current, LV_EVENT_CLICKED, alert);
                lv_label_set_text(msg_label, "성공, 암호가 확인되었습니다.");
                send_factory_init();
                //lv_obj_center(alert);
              } else {
                  //lv_obj_t* alert = lv_msgbox_create(NULL, "실패", "잘못된 암호입니다.", buttons, true);
                  //lv_obj_add_event_cb(alert, msg_close_handler_current, LV_EVENT_CLICKED, alert);
                  lv_label_set_text(msg_label, "실패, 잘못된 암호입니다.");
                  //lv_obj_center(alert);
              }

             } 
 

            lv_obj_center(msg_label);

            // 6. 닫기 버튼 클릭 이벤트 핸들러 등록
            lv_obj_add_event_cb(close_btn_img, msg_close_factory, LV_EVENT_CLICKED, NULL);
           
            Serial.println("+++ 3"); 

            // 7. 전송 버튼 등록

            btn_hema = lv_btn_create(alert_msgbox);

            lv_obj_set_size(btn_hema, 50, 25);
            lv_obj_align(btn_hema, LV_ALIGN_BOTTOM_MID, 0, -5);

            // 버튼에 라벨 추가
            label_hema = lv_label_create(btn_hema);
            lv_label_set_text(label_hema, "확인");
            lv_obj_center(label_hema); 
            lv_obj_add_event_cb(btn_hema, msg_close_factory, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_anim_time(btn_hema, 0, LV_STATE_PRESSED);  // 애니메이션 제거      
        
    }  
}
