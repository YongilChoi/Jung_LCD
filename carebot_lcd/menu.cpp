#include "common.h"
#include "lvgl_controller.h"
#include "serial_lcd.h"
#include "menu.h"
#include "event.h"
#include "audio.h"
#include "image_assets.h"

lv_obj_t *header_label;
lv_obj_t *header_label_back;

WiFiNetwork *network;
const char *password_wifi;
bool need_to_restore_motor = false;
//lv_timer_t *wifi_scan_timer = NULL;
bool wifi_screen_active = false;  // WiFi 화면 활성화 상태 추적

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

// 한글 폰트 파일을 배열로 포함 (예: NanumGothic.ttf를 변환한 경우)
//LV_FONT_DECLARE(nanum_gothic_16);
LV_FONT_DECLARE(nanum_gothic_18);

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

// 화면 전환 함수 개선
void transition_to_screen(lv_obj_t *screen) {
    logMessage("MENU", LOG_LEVEL_INFO, "화면 전환 시작");
    
    if (screen == NULL) {
        logMessage("MENU", LOG_LEVEL_ERROR, "화면 전환 실패: NULL 화면");
        return;
    }
    
    // 애니메이션 없이 직접 화면 전환으로 변경
    lv_scr_load(screen);
    
    // 화면 즉시 갱신을 위한 플러시 호출
    #ifdef CANVAS
        Arduino_GFX* gfx = get_gfx_instance();
        if (gfx) {
            gfx->flush();
            logMessage("MENU", LOG_LEVEL_INFO, "화면 전환 후 flush 호출");
        }
    #endif
    
    logMessage("MENU", LOG_LEVEL_INFO, "화면 전환 완료");
}

// 스타일 초기화
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
#if 1
// 키보드 이벤트 핸들러 수정
void keyboard_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * kb = lv_event_get_target(e);

    Serial.println("+++ in keyboard_event_cb");

    // 클릭 이벤트만 처리
    if (code != LV_EVENT_CLICKED) return;

    // 현재 입력된 패스워드 가져오기
    entered_password = lv_textarea_get_text(password_ta_2);
    Serial.printf("+++ 현재 패스워드: %s\n", entered_password);
    
    // 버튼 ID 가져오기
    uint16_t btn_id = lv_btnmatrix_get_selected_btn(kb);
    if(btn_id == LV_BTNMATRIX_BTN_NONE) return;
    
    const char * txt = lv_btnmatrix_get_btn_text(kb, btn_id);
    if(txt == NULL) return;
    
    Serial.printf("+++ 클릭된 버튼 텍스트: %s, ID: %d\n", txt, btn_id);
    
    // 특수 버튼 처리를 위한 플래그
    bool is_special_button = false;
    
    // 자체 모드 추적 변수 사용
    static bool is_uppercase_mode = false;
    
    // 대소문자 변환 처리 - 대소문자 구분없이 비교
    if(strcasecmp(txt, "ABC") == 0 || strcasecmp(txt, "abc") == 0) {
        Serial.println("+++ 대소문자 전환 시도");
        
        // 현재 내부 추적 상태를 기반으로 모드 전환
        is_uppercase_mode = !is_uppercase_mode;
        
        if(is_uppercase_mode) {
            Serial.println("+++ 대문자 모드로 전환");
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_UPPER);
        } else {
            Serial.println("+++ 소문자 모드로 전환");
            lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
        }
        
        // 키보드 갱신을 강제로 수행
        lv_obj_invalidate(kb);
        
        is_special_button = true;
    }
    // 특수문자 전환
    else if(strcmp(txt, "#+=") == 0) {
        Serial.println("+++ 특수문자 모드로 전환");
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_SPECIAL);
        is_special_button = true;
    }
    // 숫자/알파벳 모드 전환
    else if(strcmp(txt, "123") == 0) {
        Serial.println("+++ 기본 모드로 전환");
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
        is_uppercase_mode = false;
        is_special_button = true;
    }

    // 특수 버튼인 경우 텍스트 입력 방지
    if (is_special_button) {
        Serial.println("+++ 특수 버튼 처리 - 텍스트 입력 방지");
        lv_obj_t * ta = lv_keyboard_get_textarea(kb);
        // 추가 처리가 필요한 경우 여기에 코드 추가
    }
    
    Serial.printf("+++ is_special_button: %d, is_uppercase_mode: %d\n", 
                is_special_button, is_uppercase_mode);
}
#else
void keyboard_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * kb = lv_event_get_target(e);

    Serial.println("+++ in keyboard_event_cb");

    // 현재 입력된 패스워드 가져오기
    entered_password = lv_textarea_get_text(password_ta_2);
    Serial.printf("+++ in keyboard_event_cb password: %s \n", entered_password);
    
    // 버튼 ID 가져오기
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
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_UPPER);
		lv_obj_invalidate(kb);  // UI 갱신
    } 
    else if(strcmp(txt, "abc") == 0) {
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
		lv_obj_invalidate(kb);  // UI 갱신
    }
    else if(strcmp(txt, "#+=") == 0) {
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_SPECIAL);
        is_special_button = true;
		lv_obj_invalidate(kb);  // UI 갱신
    }
    else if(strcmp(txt, "123") == 0) {
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
        is_special_button = true;
		lv_obj_invalidate(kb);  // UI 갱신
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
}
#endif
void factory_screen(lv_obj_t *parent) {
    // 스타일 생성
    static lv_style_t style_ta;
    lv_style_init(&style_ta);
    lv_style_set_text_font(&style_ta, &lv_font_montserrat_16);
    lv_style_set_border_width(&style_ta, 2);
    lv_style_set_border_color(&style_ta, lv_color_hex(0x2196F3));
    
    // 제목 레이블 생성
    lv_obj_t *title2 = lv_label_create(parent);
    if (title2 == NULL) {
        Serial.println("+++ Failed to create title2 label");
        return;
    }

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
    if (password_ta_2 == NULL) {
        Serial.println("+++ Failed to create password_ta_2");
        return;
    }
    
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
    if (kb == NULL) {
        Serial.println("+++ Failed to create keyboard");
        return;
    }
    
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
    if (btn == NULL) {
        Serial.println("+++ Failed to create confirm button");
        return;
    }
    
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
    if (btn_label == NULL) {
        Serial.println("+++ Failed to create button label");
        return;
    }
    
    lv_label_set_text(btn_label, "확 인");
    lv_obj_center(btn_label);
    
    // 이벤트 핸들러 연결
    //lv_obj_add_event_cb(ta, password_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(btn, factory_event_handler, LV_EVENT_CLICKED, NULL);
}


void menu(void) {     
    Serial.println("+++ in menu()");
    Serial.printf("+++ Motor_ON(%d) and need_to_restore_motor(%d)\n", motor_ON, need_to_restore_motor);
    if (motor_ON) {
        need_to_restore_motor = true;
        Serial.printf("+++ Motor is ON and need_to_restore_motor(%d)\n", need_to_restore_motor);
        Event_motor_OFF();
        // motor_ON 플래그는 Event_motor_OFF에서 처리됨
    }
    
    menu_ON = 1;
    initial_scr();
    
    transition_to_screen(main_screen);
}

void initial_scr(void) {
    logMessage("MENU", LOG_LEVEL_INFO, "메뉴 초기화 화면 생성 시작");
    
    // 스타일 초기화
    init_styles();    

    // 메인 스크린이 없으면 생성
    if (main_screen == NULL) {
        Serial.println("+++ BEFORE main_screen lv_obj_create");
        main_screen = lv_obj_create(NULL);
        if (main_screen == NULL) {
            logMessage("MENU", LOG_LEVEL_ERROR, "메인 스크린 생성 실패");
            return;
        }
        
        lv_obj_set_size(main_screen, 480, 272);  
        lv_obj_align(main_screen, LV_ALIGN_LEFT_MID, 0, 0);
    }
    
    // 화면 로드
    lv_scr_load(main_screen);
    
    // 헤더 레이블이 없으면 생성
    if (header_label == NULL) {
        header_label = lv_label_create(main_screen);
        if (header_label != NULL) {
            lv_label_set_text(header_label, "설정 메뉴");
            lv_obj_add_style(header_label, &style_header_18, 0);
            lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15);
            lv_obj_set_size(header_label, 480, 25);
            lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    // 뒤로가기 헤더가 없으면 생성
    if (header_label_back == NULL) {
        header_label_back = lv_imgbtn_create(main_screen); 
        if (header_label_back != NULL) {
            
            lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn	, NULL);
            lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
            lv_obj_set_size(header_label_back, 25, 25);
            
            
            // 클릭 이벤트 설정
            lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_1, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);
            lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);
            lv_obj_set_style_anim_time(header_label_back, 0, LV_STATE_PRESSED);
        }
    }
    
    // 메뉴 리스트가 없으면 생성
    if (menu_list == NULL) {
        menu_list = lv_list_create(main_screen);
        if (menu_list != NULL) {
            lv_obj_set_size(menu_list, 480, LV_PCT(90));
            lv_obj_align(menu_list, LV_ALIGN_BOTTOM_MID, 0, 15);
            lv_obj_add_style(menu_list, &style_list, 0);
            lv_obj_set_scrollbar_mode(menu_list, LV_SCROLLBAR_MODE_OFF);
            
            // 메뉴 항목 추가
            for (uint32_t i = 0; i < MENU_ITEM_COUNT; i++) {
                lv_obj_t *btn = lv_list_add_btn(menu_list, NULL, menu_items[i].name);
                if (btn != NULL) {
                    lv_obj_add_style(btn, &style_btn, 0);
                    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_add_event_cb(btn, menu_btn_event_cb, LV_EVENT_CLICKED, NULL);
                    lv_obj_set_style_anim_time(btn, 0, LV_STATE_PRESSED);
                }
            }
        }
    }
    
    logMessage("MENU", LOG_LEVEL_INFO, "메뉴 초기화 화면 생성 완료");
}

void show_menu_content(const char *title, const char *content) {
  
    lv_obj_t *popup = lv_obj_create(lv_scr_act());
    if (popup == NULL) {
        Serial.println("+++ Failed to create popup");
        return;
    }
    
    lv_obj_set_size(popup, LV_PCT(80), LV_PCT(60));
    lv_obj_center(popup);
    
    // 팝업 제목
    lv_obj_t *title_label = lv_label_create(popup);
    if (title_label != NULL) {
        lv_label_set_text(title_label, title);
        lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);
    }
    
    // 팝업 내용
    lv_obj_t *content_label = lv_label_create(popup);
    if (content_label != NULL) {
        lv_label_set_text(content_label, content);
        lv_obj_align(content_label, LV_ALIGN_CENTER, 0, 0);
    }
    
    // 닫기 버튼
    lv_obj_t *close_btn = lv_btn_create(popup);
    if (close_btn != NULL) {
        lv_obj_t *close_label = lv_label_create(close_btn);
        if (close_label != NULL) {
            lv_label_set_text(close_label, "Close");
            lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
            lv_obj_add_event_cb(close_btn, close_popup_cb, LV_EVENT_CLICKED, popup);
        }
    }
}

void close_popup_cb(lv_event_t *e) {
    //lv_obj_t *popup = lv_event_get_user_data(e);
    if (close_btn != NULL) {
        lv_obj_del(close_btn);
        close_btn = NULL;
    }
    //lv_event_get_user_data(e);
}

void wifi_ap_setup_cb(void) {
    logMessage("MENU", LOG_LEVEL_INFO, "WiFi AP 설정 화면 진입");
    
    // 메뉴 모드 설정
    menu_ON = 1;

    // wifi 등록 스크린 생성 - 한번만 생성하도록 수정
    if (wifi_ap_setup_screen == NULL) {
        wifi_ap_setup_screen = lv_obj_create(NULL);
        if (wifi_ap_setup_screen == NULL) {
            logMessage("MENU", LOG_LEVEL_ERROR, "WiFi AP 화면 생성 실패");
            return;
        }

        lv_obj_set_size(wifi_ap_setup_screen, 480, 272);
        lv_obj_align(wifi_ap_setup_screen, LV_ALIGN_LEFT_MID, 0, 0);
        
        // 헤더 레이블 생성
        header_label = lv_label_create(wifi_ap_setup_screen);
        if (header_label != NULL) {
            lv_label_set_text(header_label, "1. WiFi AP 등록");
            lv_obj_add_style(header_label, &style_header_18, 0);
            lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
            lv_obj_set_size(header_label, 480, 25);
            lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }

        // 뒤로가기 헤더 생성
        header_label_back = lv_imgbtn_create(wifi_ap_setup_screen);
        if (header_label_back != NULL) {
            lv_obj_set_size(header_label_back, 25, 25);
            lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn	, NULL);
            lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
            
            
            
            // 클릭 이벤트 설정
            lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_2, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);
            lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);
            lv_obj_set_style_anim_time(header_label_back, 0, LV_STATE_PRESSED);
        }

        // WiFi 정보 이미지 생성
        lv_obj_t * wifi_info = lv_img_create(wifi_ap_setup_screen);
        if (wifi_info != NULL) {
            lv_img_set_src(wifi_info, &img_qr);
            lv_obj_set_pos(wifi_info, 20, 50);
        } else {
            Serial.println("+++ Failed to create image object wifi_info");
        }

        // 안내 레이블 생성
        inform_label = lv_label_create(wifi_ap_setup_screen);
        if (inform_label != NULL) {
            lv_label_set_text(inform_label, " 스마트폰에서 WiFi 검색하여 \n \"Carebot\" 에 접속한 후에 \n \n 옆의 QR 코드를 촬영하거나 \n 스마트폰 웹브라우저에서 \n 10.10.0.1 에 접속하세요. ");
            lv_obj_add_style(inform_label, &style_header_18_2, 0);
            lv_obj_align(inform_label, LV_ALIGN_RIGHT_MID, -20, 10);   
        }
    }

    // 화면 로드
    lv_scr_load(wifi_ap_setup_screen);
    logMessage("MENU", LOG_LEVEL_INFO, "WiFi AP 설정 화면 로드 완료");
}

// 타이머 안전하게 제거하는 함수
void safe_delete_wifi_timer() {
    if (wifi_scan_timer != NULL) {
        Serial.println("+++ 안전하게 WiFi 타이머 삭제");
        lv_timer_del(wifi_scan_timer);
        wifi_scan_timer = NULL;
    }
}
void initial_cb_1(void) {
    Serial.println("+++ initial_cb_1 initial before");

    // 우선 타이머 삭제
    safe_delete_wifi_timer();
    
    // 메뉴 모드 끄기
    menu_ON = 0;
    
    // wifi_screen 관련 플래그와 변수 초기화
    wifi_screen_active = false;
    
    // 애니메이션 없이 바로 화면 전환
    if (panel0 != NULL) {
        // 애니메이션 없이 바로 화면 로드
        lv_scr_load(panel0);
        
        // 화면 로드 후 UI 상태 업데이트
        lvgl_update_app_ui();
    } else {
        lvgl_create_app_ui();
    }
    
    Serial.printf("+++ need_to_restore_motor(%d) motor_On(%d)\n", need_to_restore_motor, motor_ON);
    if (need_to_restore_motor) {
      Serial.println("+++ Call Event_motor_ON()\n");
      Event_motor_ON();
    }

    Serial.println("+++ initial_cb_1 initial after");
}
void initial_cb_2(void) {
    Serial.println("+++ initial_cb_2 initial before");

    // 현재 재생 중인지 확인
    if (is_audio_playing()) stop_sound();

    // 메인 메뉴 화면으로 돌아가기
    if (main_screen != NULL) {
        lv_scr_load(main_screen);
        Serial.println("+++ Loaded main_screen");
    } else {
        Serial.println("+++ main_screen is NULL, creating new one");
        initial_scr();
    }
  
    Serial.println("+++ initial_cb_2 initial after");
}

void wifi_connect_with_password(lv_event_t *e) {
    menu_ON = 1;
    
    // 버튼과 연결된 네트워크 정보 가져오기
    lv_obj_t *connect_btn = lv_event_get_target(e);
    if (connect_btn == NULL) {
        Serial.println("+++ connect_btn is NULL");
        return;
    }
    
    network = (WiFiNetwork*)lv_obj_get_user_data(connect_btn);
    if (network == NULL) {
        Serial.println("+++ network user data is NULL");
        return;
    }

    // 패스워드 가져오기
    if (password_ta == NULL) {
        Serial.println("+++ password_ta is NULL");
        return;
    }
    
    password_wifi = lv_textarea_get_text(password_ta);
    if (password_wifi == NULL) {
        Serial.println("+++ password_wifi is NULL");
        return;
    }

    // WiFi 연결 정보 전송
    send_wifi_conn_info(network->ssid, password_wifi);

    // 대기 메시지 표시
    waiting_event_handler_2("WIFI 연결중...\n잠시후 확인 부탁 드립니다.");
}

// WiFi network selection event handler
void wifi_network_selected(lv_event_t *e) {
    Serial.println("+++ wifi_network_selected started");
    menu_ON = 1;

    // 스타일 초기화
    init_styles();

    // 선택된 네트워크 정보 가져오기
    lv_obj_t *btn = lv_event_get_target(e);
    if (btn == NULL) {
        Serial.println("+++ btn is NULL");
        return;
    }
    
    WiFiNetwork *selected_network = (WiFiNetwork*)lv_obj_get_user_data(btn);
    if (selected_network == NULL) {
        Serial.println("+++ selected_network is NULL");
        return;
    }

    // 패스워드 입력 화면 생성
    if (password_screen == NULL) {
        Serial.println("+++ BEFORE password_screen password_screen");
        password_screen = lv_obj_create(NULL);
        if (password_screen == NULL) {
            Serial.println("+++ Failed to create password_screen");
            return;
        }
    }
    
    lv_scr_load(password_screen);

    // 헤더 레이블 생성
    header_label = lv_label_create(password_screen);
    if (header_label != NULL) {
        lv_label_set_text_fmt(header_label, "Enter Password for %s", selected_network->ssid);
        lv_obj_add_style(header_label, &style_header_18, 0);
        lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
        lv_obj_set_size(header_label, 480, 25);
        lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT); 
    }

    // 패스워드 입력 필드 생성
    password_ta = lv_textarea_create(password_screen);
    if (password_ta != NULL) {
        lv_textarea_set_placeholder_text(password_ta, "Password");
        lv_textarea_set_password_mode(password_ta, true);
        lv_obj_set_width(password_ta, 200);
        lv_obj_set_height(password_ta, 30);
        lv_obj_set_pos(password_ta, 100, 70);
        lv_textarea_set_password_mode(password_ta, true);
        lv_textarea_set_one_line(password_ta, true);
    }

    // 가상 키보드 생성
    password_keyboard = lv_keyboard_create(password_screen);
    if (password_keyboard != NULL) {
        lv_keyboard_set_textarea(password_keyboard, password_ta);
        lv_obj_align(password_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
        setup_keyboard_with_special_chars(password_keyboard);
    }

    // 연결 버튼 생성
    lv_obj_t *connect_btn = lv_btn_create(password_screen);
    if (connect_btn != NULL) {
        lv_obj_t *connect_label = lv_label_create(connect_btn);
        if (connect_label != NULL) {
            lv_label_set_text(connect_label, "연결");
        }
        lv_obj_set_pos(connect_btn, 370, 70);

        // 네트워크 정보 저장
        lv_obj_set_user_data(connect_btn, selected_network);
        lv_obj_add_event_cb(connect_btn, wifi_connect_with_password, LV_EVENT_CLICKED, NULL);
    }

    // 뒤로가기 버튼 생성
    header_label_back = lv_imgbtn_create(password_screen);
    if (header_label_back != NULL) {
        lv_obj_set_size(header_label_back, 25, 25);
        lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn, NULL);
        lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);

        // 클릭 이벤트 설정
        lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_1, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);
    }
    
    Serial.println("+++ wifi_network_selected completed");
}

// create_wifi_selection_screen 수정
bool create_wifi_selection_screen() {
    Serial.println("+++ BEGIN in create_wifi_selection_screen:");
    
    // 메뉴 모드 설정
    menu_ON = 1;
    
    // 스타일 초기화
    init_styles();
   
    // 현재 스크린 정보 확인
    lv_obj_t *current_screen = lv_scr_act();
    Serial.printf("+++ 현재 스크린: %p\n", current_screen);
    
    // WiFi 화면 객체 상태 확인
    Serial.printf("+++ wifi_screen 상태: %p\n", wifi_screen);
    
    // 기존 WiFi 화면이 이미 있는 경우 내용만 갱신
    if (wifi_screen != NULL) {
        Serial.println("+++ 기존 wifi_screen 재사용");
        
        // 화면을 숨겨진 상태에서 복원
        lv_obj_clear_flag(wifi_screen, LV_OBJ_FLAG_HIDDEN);
        
        // 기존 내용 정리 (리스트만)
        if (wifi_list != NULL) {
            Serial.println("+++ 기존 wifi_list 숨기기");
            lv_obj_add_flag(wifi_list, LV_OBJ_FLAG_HIDDEN);
        }
        
        // 새 리스트 생성
        wifi_list = lv_list_create(wifi_screen);
    } else {
        Serial.println("+++ 새 wifi_screen 생성");
        
        // 새 화면 생성
        wifi_screen = lv_obj_create(NULL);
        if (wifi_screen == NULL) {
            Serial.println("+++ wifi_screen 생성 실패");
            return false;
        }
        
        lv_obj_set_size(wifi_screen, 480, 272);
        
        // 헤더 레이블 생성
        header_label = lv_label_create(wifi_screen);
        if (header_label != NULL) {
            lv_label_set_text(header_label, "WIFI 목록");
            lv_obj_add_style(header_label, &style_header_18, 0);
            lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
            lv_obj_set_size(header_label, 480, 25);
            lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }

        // 뒤로가기 버튼 생성
        header_label_back = lv_imgbtn_create(wifi_screen);
        if (header_label_back != NULL) {
            lv_obj_set_size(header_label_back, 25, 25);
            lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn, NULL);
            lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);

            // 클릭 이벤트 설정
            lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_1, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);
            lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);
        }
        
        // 리스트 생성
        wifi_list = lv_list_create(wifi_screen);
    }
    
    // WiFi 리스트 설정 (공통)
    if (wifi_list != NULL) {
        lv_obj_set_size(wifi_list, 320, 200);
        lv_obj_align(wifi_list, LV_ALIGN_CENTER, 0, 20);
        
        // WiFi 리스트 항목 추가
        if (network_count == 0) {
            lv_list_add_btn(wifi_list, NULL, "WiFi 리스트를 불러오는 중...");
        } else {
            for (int i = 0; i < network_count && i < MAX_WIFI_NETWORKS; i++) {
                if (available_networks[i].ssid[0] != '\0') {
                    lv_obj_t *btn = lv_list_add_btn(wifi_list, NULL, available_networks[i].ssid);
                    if (btn != NULL) {
                        lv_obj_set_user_data(btn, (void*)&available_networks[i]);
                        lv_obj_add_event_cb(btn, wifi_network_selected, LV_EVENT_CLICKED, NULL);
                    }
                }
            }
        }
    }
    
    // 화면 로드
    lv_scr_load(wifi_screen);
    
    Serial.println("+++ END in create_wifi_selection_screen:");
    return true;
}

void img_click_event_cb_wifi(lv_event_t *e) {
    Serial.println("+++ Begin img_click_event_cb_wifi");
    
    // 알림창이 이미 표시되어 있는지 확인
    if (alert_msgbox != NULL && !lv_obj_has_flag(alert_msgbox, LV_OBJ_FLAG_HIDDEN)) {
        Serial.println("+++ 이미 알림창이 표시 중입니다.");
        return;
    }
    
    // WiFi 스캔 요청
    Serial.println("+++ WiFi 스캔 요청");
    request_wifi_scan();
    
    // 대기 메시지 표시
    waiting_event_handler((char*)"WIFI 리스트 수신중...\n잠시만(약10초) 기다려 주세요.\n수신결과가 원활치 않은 경우\n잠시후 재시도 부탁 드립니다.");
    
    Serial.println("+++ End img_click_event_cb_wifi");
}


void alarm_setup_cb(void) {
    Serial.println("+++ Begin alarm_setup_cb");

    // 화면 생성 또는 재사용
    if (alarm_setup_screen == NULL) {
        alarm_setup_screen = lv_obj_create(NULL);
        if (alarm_setup_screen == NULL) {
            Serial.println("+++ Failed to create alarm_setup_screen");
            return;
        }

        lv_obj_set_size(alarm_setup_screen, 480, 272);
        lv_obj_add_style(alarm_setup_screen, &style_panel, 0);
        
        // 헤더 레이블 생성
        header_label = lv_label_create(alarm_setup_screen);
        if (header_label != NULL) {
            lv_label_set_text(header_label, "2. 알람 볼륨");
            lv_obj_add_style(header_label, &style_header_18, 0);
            lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
            lv_obj_set_size(header_label, 480, 25);
            lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
     
        // 뒤로가기 헤더 생성
        header_label_back = lv_imgbtn_create(alarm_setup_screen);
        if (header_label_back != NULL) {
            lv_obj_set_size(header_label_back, 25, 25);
            lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn, NULL);
            lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
            
            // 클릭 이벤트 설정
            lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_2, LV_EVENT_CLICKED, NULL);
            lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);
            lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);
            lv_obj_set_style_anim_time(header_label_back, 0, LV_STATE_PRESSED);
        }
    }

    // 화면 로드
    lv_scr_load(alarm_setup_screen);

    // 볼륨 컨트롤 생성
    create_volume_control(alarm_setup_screen);
   
    Serial.println("+++ End alarm_setup_cb");
}

void relay_info_cb(void) {
    Serial.println("+++ Begin relay_info_cb");

    // 화면 생성
    lv_obj_t * sub_screen = lv_obj_create(NULL);
    if (sub_screen == NULL) {
        Serial.println("+++ Failed to create sub_screen");
        return;
    }

    lv_obj_set_size(sub_screen, 480, 270);
    lv_obj_add_style(sub_screen, &style_panel, 0);
    lv_scr_load(sub_screen);

    // 헤더 레이블 생성
    header_label = lv_label_create(sub_screen);
    if (header_label != NULL) {
        lv_label_set_text(header_label, "3. 혈뇨 정보 전송");
        lv_obj_add_style(header_label, &style_header_18, 0);
        lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
        lv_obj_set_size(header_label, 480, 25);
        lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // 뒤로가기 헤더 생성
    header_label_back = lv_imgbtn_create(sub_screen);
    if (header_label_back != NULL) {
        lv_obj_set_size(header_label_back, 25, 25);
        lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn, NULL);
        lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
        
        // 클릭 이벤트 설정
        lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_2, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);
        lv_obj_set_style_anim_time(header_label_back, 0, LV_STATE_PRESSED);
    }

    // 릴레이 설정 생성
    create_relay_settings(sub_screen);
   
    Serial.println("+++ End relay_info_cb");
}

void urination_cb(void) {
    Serial.println("+++ Begin urination_cb");

    // 화면 생성
    lv_obj_t * sub_screen = lv_obj_create(NULL);
    if (sub_screen == NULL) {
        Serial.println("+++ Failed to create sub_screen");
        return;
    }

    lv_obj_set_size(sub_screen, 480, 270);
    lv_obj_add_style(sub_screen, &style_panel, 0);
    lv_scr_load(sub_screen);

    // 헤더 레이블 생성
    header_label = lv_label_create(sub_screen);
    if (header_label != NULL) {
        lv_label_set_text(header_label, "4. 배뇨 유도");
        lv_obj_add_style(header_label, &style_header_18, 0);
        lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
        lv_obj_set_size(header_label, 480, 25);
        lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // 뒤로가기 헤더 생성
    header_label_back = lv_imgbtn_create(sub_screen);
    if (header_label_back != NULL) {
        lv_obj_set_size(header_label_back, 25, 25);
        lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn, NULL);
        lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
        
        // 클릭 이벤트 설정
        lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_2, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);
        lv_obj_set_style_anim_time(header_label_back, 0, LV_STATE_PRESSED);
    }

    // 배뇨 유도 화면 생성
    urination_screen(sub_screen);
   
    Serial.println("+++ End urination_cb");
}

void terminal_info_cb(void) {
    Serial.println("+++ Begin terminal_info_cb");

    // 화면 생성
    lv_obj_t * sub_screen = lv_obj_create(NULL);
    if (sub_screen == NULL) {
        Serial.println("+++ Failed to create sub_screen");
        return;
    }

    lv_obj_set_size(sub_screen, 480, 270);
    lv_obj_add_style(sub_screen, &style_panel, 0);
    lv_scr_load(sub_screen);

    // 헤더 레이블 생성
    header_label = lv_label_create(sub_screen);
    if (header_label != NULL) {
        lv_label_set_text(header_label, "5. 단말 정보 보기");
        lv_obj_add_style(header_label, &style_header_18, 0);
        lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
        lv_obj_set_size(header_label, 480, 25);
        lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // 뒤로가기 헤더 생성
    header_label_back = lv_imgbtn_create(sub_screen);
    if (header_label_back != NULL) {
        lv_obj_set_size(header_label_back, 25, 25);
        lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn, NULL);
        lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
        
        // 클릭 이벤트 설정
        lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_2, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);
        lv_obj_set_style_anim_time(header_label_back, 0, LV_STATE_PRESSED);
    }

    // 단말 정보 화면 생성
    terminal_screen(sub_screen);
   
    Serial.println("+++ End terminal_info_cb");
}

void factory_menu_cb(void) {
    Serial.println("+++ Begin factory_menu_cb");

    // 화면 생성
    lv_obj_t * sub_screen = lv_obj_create(NULL);
    if (sub_screen == NULL) {
        Serial.println("+++ Failed to create sub_screen");
        return;
    }

    lv_obj_set_size(sub_screen, 480, 270);
    lv_obj_add_style(sub_screen, &style_panel, 0);
    lv_scr_load(sub_screen);

    // 헤더 레이블 생성
    header_label = lv_label_create(sub_screen);
    if (header_label != NULL) {
        lv_label_set_text(header_label, "6. 공장 초기화");
        lv_obj_add_style(header_label, &style_header_18, 0);
        lv_obj_align(header_label, LV_ALIGN_TOP_MID, 0, 15); 
        lv_obj_set_size(header_label, 480, 25);
        lv_obj_set_style_text_align(header_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // 뒤로가기 헤더 생성
    header_label_back = lv_imgbtn_create(sub_screen);
    if (header_label_back != NULL) {
        lv_obj_set_size(header_label_back, 25, 25);
        lv_imgbtn_set_src(header_label_back, LV_IMGBTN_STATE_RELEASED, NULL, &img_backbtn, NULL);
        lv_obj_align(header_label_back, LV_ALIGN_TOP_LEFT, 0, 15);
        
        // 클릭 이벤트 설정
        lv_obj_add_flag(header_label_back, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(header_label_back, (lv_event_cb_t)&initial_cb_2, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_bg_opa(header_label_back, LV_OPA_50, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(header_label_back, lv_color_hex(0x808080), LV_STATE_PRESSED);
        lv_obj_set_style_anim_time(header_label_back, 0, LV_STATE_PRESSED);
    }

    // 공장초기화 화면 생성
    factory_screen(sub_screen);
   
    Serial.println("+++ End factory_menu_cb");
}

void slider_event_cb(lv_event_t *e) {
    // 슬라이더 이벤트 처리
    lv_obj_t *slider_recv = lv_event_get_target(e);
    if (slider_recv == NULL) {
        Serial.println("+++ slider_recv is NULL");
        return;
    }
    
    volume_value = lv_slider_get_value(slider_recv);
    
    // 볼륨 값 레이블 업데이트
    if (volume_label == NULL) {
        Serial.println("+++ volume_label is NULL");
    } else {
        char buf[8];
        lv_snprintf(buf, sizeof(buf), "%d", volume_value);
        lv_label_set_text(volume_label, buf);
    }

    Serial.printf("+++ 볼륨값 : %d \n", volume_value);
    
    // 오디오 볼륨 설정
    audio_set_volume(volume_value);
}

void create_volume_control(lv_obj_t *parent) {
    if (parent == NULL) {
        Serial.println("+++ parent is NULL in create_volume_control");
        return;
    }
    
    // 스피커 아이콘 생성
    speaker = lv_img_create(parent);
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
    if (volume_label == NULL) {
        volume_label = lv_label_create(parent);
        if (volume_label == NULL) {
            Serial.println("+++ Failed to create volume_label");
            return;
        }
        
        char buf[10];
        sprintf(buf, "%ld", volume_value);
        lv_label_set_text(volume_label, buf);
        lv_obj_set_pos(volume_label, 360, 150);
        lv_obj_move_foreground(volume_label);
    }
    
    // 세로 슬라이더 생성
    if (slider == NULL) {
        slider = lv_slider_create(parent);
        if (slider == NULL) {
            Serial.println("+++ Failed to create slider");
            return;
        }
        
        lv_obj_set_size(slider, 15, 120);
        lv_obj_set_pos(slider, 400, 100);
        
        // 세로 방향으로 설정
        lv_slider_set_mode(slider, LV_SLIDER_MODE_SYMMETRICAL);
        lv_slider_set_range(slider, 0, 10);
        
        // 저장된 volume_value 값으로 슬라이더 초기값 설정
        lv_slider_set_value(slider, volume_value, LV_ANIM_OFF);
        
        // 스타일 초기화
        lv_style_init(&style_indicator);
        lv_style_init(&style_knob);

        // 스타일 속성 설정
        lv_style_set_bg_color(&style_indicator, lv_color_hex(0x0000FF));
        lv_style_set_bg_color(&style_knob, lv_color_hex(0x000000));

        // 슬라이더 스타일 적용
        lv_obj_set_style_bg_color(slider, lv_color_hex(0x808080), 0);
        lv_obj_add_style(slider, &style_indicator, LV_PART_INDICATOR);
        lv_obj_add_style(slider, &style_knob, LV_PART_KNOB);
        
        // 이벤트 핸들러 설정
        lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // 오디오 볼륨 설정 적용
    audio_set_volume(volume_value);
    
    Serial.printf("+++ 초기 볼륨값 적용: %d \n", volume_value);
}

void radio_event_handler(lv_event_t *e) {
    Serial.println("+++ radio_event_handler");

    // 이벤트가 발생한 체크박스 객체 가져오기
    lv_obj_t *checkbox = lv_event_get_target(e);
    if (checkbox == NULL) {
        Serial.println("+++ checkbox is NULL");
        return;
    }
        
    // 체크박스의 부모 컨테이너 가져오기
    lv_obj_t *item_cont = lv_obj_get_parent(checkbox);
    if (item_cont == NULL) {
        Serial.println("+++ item_cont is NULL");
        return;
    }
    
    // item_cont의 부모 컨테이너 가져오기
    lv_obj_t *cont2 = lv_obj_get_parent(item_cont);
    if (cont2 == NULL) {
        Serial.println("+++ cont2 is NULL");
        return;
    }
    
    // 현재 체크박스의 상태 확인
    bool is_checked = lv_obj_has_state(checkbox, LV_STATE_CHECKED);
    
    // 체크된 상태라면
    if (is_checked) {
        // cont2의 모든 자식 컨테이너(item_cont)를 순회
        uint32_t item_cnt = lv_obj_get_child_cnt(cont2);

        for (uint32_t i = 0; i < item_cnt; i++) {
            lv_obj_t *other_item_cont = lv_obj_get_child(cont2, i);
            if (other_item_cont == NULL) continue;
            
            // item_cont 안의 체크박스를 찾음 (첫 번째 자식이 체크박스)
            lv_obj_t *other_checkbox = lv_obj_get_child(other_item_cont, 0);
            if (other_checkbox == NULL) continue;
            
            // 현재 선택된 체크박스가 아닌 다른 체크박스들의 체크 상태를 해제
            if (other_checkbox != checkbox) {
                Serial.println("+++ radio_event_handler, i:"); 
                Serial.println(i);
                lv_obj_clear_state(other_checkbox, LV_STATE_CHECKED);
            } else {
                amount = i + 1;
                Serial.println("+++ radio_event_handler, amount:"); 
                Serial.println(amount);
            }
        }
    } else {
        // 체크가 해제된 상태라면, 다시 체크 상태로 되돌림
        Serial.println("+++ radio_event_handler else"); 
        lv_obj_add_state(checkbox, LV_STATE_CHECKED);
    }
}

void create_relay_settings(lv_obj_t *parent) {
    if (parent == NULL) {
        Serial.println("+++ parent is NULL in create_relay_settings");
        return;
    }
    
    // 라디오 버튼 그룹 컨테이너 생성
    cont2 = lv_obj_create(parent);
    if (cont2 == NULL) {
        Serial.println("+++ Failed to create cont2");
        return;
    }

    lv_obj_set_flex_flow(cont2, LV_FLEX_FLOW_ROW);
    lv_obj_set_size(cont2, 250, 100);
    lv_obj_align(cont2, LV_ALIGN_CENTER, 0, 0);
    
    // 컨테이너 정렬 설정
    lv_obj_set_style_flex_main_place(cont2, LV_FLEX_ALIGN_SPACE_EVENLY, 0);
    lv_obj_set_style_flex_cross_place(cont2, LV_FLEX_ALIGN_CENTER, 0);
    
    // 체크박스 스타일 설정
    static lv_style_t style_checkbox;
    lv_style_init(&style_checkbox);
    lv_style_set_border_width(&style_checkbox, 2);
    lv_style_set_border_color(&style_checkbox, lv_color_black());
    lv_style_set_bg_color(&style_checkbox, lv_color_white());

    // 체크 표시 스타일 설정
    static lv_style_t style_checkbox_checked;
    lv_style_init(&style_checkbox_checked);
    lv_style_set_text_color(&style_checkbox_checked, lv_color_black());  // 체크 표시 색상
    lv_style_set_text_font(&style_checkbox_checked, &lv_font_montserrat_16); // 체크 표시 글꼴 크기

    const char* labels[] = {"소", "중", "대"};
    
    for (unsigned char i = 0; i < 3; i++) {
        // 각 항목을 위한 컨테이너 생성
        lv_obj_t * item_cont = lv_obj_create(cont2);
        if (item_cont == NULL) {
            Serial.println("+++ Failed to create item_cont");
            continue;
        }
        
        lv_obj_remove_style_all(item_cont);  // 컨테이너 스타일 제거
        lv_obj_set_size(item_cont, 40, 60);  // 적절한 크기 설정
        lv_obj_set_flex_flow(item_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(item_cont, 
                            LV_FLEX_ALIGN_CENTER,     // 수평 중앙 정렬
                            LV_FLEX_ALIGN_CENTER,     // 수직 중앙 정렬
                            LV_FLEX_ALIGN_CENTER);    // 교차축 중앙 정렬
        
        // 체크박스 생성
        lv_obj_t * checkbox = lv_checkbox_create(item_cont);
        if (checkbox == NULL) {
            Serial.println("+++ Failed to create checkbox");
            continue;
        }

        // 체크박스 설정
        lv_checkbox_set_text(checkbox, "");
        
        // 체크박스 스타일 적용
        lv_obj_add_style(checkbox, &style_checkbox, LV_PART_INDICATOR);
        lv_obj_add_style(checkbox, &style_checkbox_checked, LV_PART_INDICATOR | LV_STATE_CHECKED);
        
        // 체크박스 크기 조정
        lv_obj_set_style_pad_all(checkbox, 0, 0);  // 패딩 제거
                
        // 이벤트 핸들러 등록
        lv_obj_add_event_cb(checkbox, radio_event_handler, LV_EVENT_CLICKED, NULL);
        
        // 라벨 생성
        lv_obj_t * label = lv_label_create(item_cont);
        if (label == NULL) {
            Serial.println("+++ Failed to create label");
            continue;
        }
        
        lv_label_set_text(label, labels[i]);
        
        // 첫 번째 체크박스 선택
        if (i == 0) {
            lv_obj_add_state(checkbox, LV_STATE_CHECKED);
            amount = 1; // 초기값 설정
        }
        lv_obj_set_style_anim_time(checkbox, 0, LV_STATE_PRESSED);  // 애니메이션 제거
    }
     
    // "전송" 버튼 생성
    lv_obj_t* btn = lv_btn_create(parent);
    if (btn == NULL) {
        Serial.println("+++ Failed to create send button");
        return;
    }

    lv_obj_set_size(btn, 200, 50);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);

    // 버튼에 라벨 추가
    lv_obj_t* label = lv_label_create(btn);
    if (label == NULL) {
        Serial.println("+++ Failed to create button label");
        return;
    }
    
    lv_label_set_text(label, "전송");
    lv_obj_center(label); 
    lv_obj_add_event_cb(btn, relay_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_anim_time(btn, 0, LV_STATE_PRESSED);  // 애니메이션 제거
}
void relay_event_handler(lv_event_t* e) {
    Serial.printf("+++ relay_event_handler, amount: %d \n", amount);
    
    // 혈뇨 상태 전송 함수 호출
    send_blood_status(amount);
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

void urination_event_handler_1(lv_event_t *e) {
    Serial.println("+++ BEFORE audio.requestStop(); in urination_event_handler_1"); 
                     // audio.requestStop();  // 재생 중지 요청

    Serial.println("+++ BEFORE mp3play in urination_event_handler_1");       
    Serial.println("+++ 물소리");  
    
    // 물소리 재생
    play_water_sound(REPEAT_INFINITE, 0);  // repeat infinite
    
    Serial.println("+++ AFTER 물소리 in urination_event_handler_1"); 
}

void urination_event_handler_2(lv_event_t *e) {
    Serial.println("+++ BEFORE audio.requestStop(); in urination_event_handler_2"); 
    Serial.println("+++ 쉬소리");  
    
    // 배뇨 소리 재생
    play_urination_sound(REPEAT_INFINITE, 0);     // repeat infinite
}

void urination_screen(lv_obj_t *parent) {
    if (parent == NULL) {
        Serial.println("+++ parent is NULL in urination_screen");
        return;
    }
        
    // "물 소리" 버튼 생성
    lv_obj_t* btn = lv_btn_create(parent);
    if (btn == NULL) {
        Serial.println("+++ Failed to create water sound button");
        return;
    }
    
    lv_obj_set_size(btn, 200, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_anim_time(btn, 0, LV_STATE_PRESSED);  // 애니메이션 제거

    // 버튼에 라벨 추가
    lv_obj_t* label = lv_label_create(btn);
    if (label == NULL) {
        Serial.println("+++ Failed to create button label");
        return;
    }
    
    lv_label_set_text(label, "물 소리");
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    lv_obj_center(label);  

    lv_obj_add_event_cb(btn, water_sound_handler, LV_EVENT_CLICKED, NULL);

    // "쉬 소리" 버튼 생성
    lv_obj_t* btn2 = lv_btn_create(parent);
    if (btn2 == NULL) {
        Serial.println("+++ Failed to create urination sound button");
        return;
    }
    
    lv_obj_set_size(btn2, 200, 50);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 50);
    lv_obj_set_style_anim_time(btn2, 0, LV_STATE_PRESSED);  // 애니메이션 제거

    // 버튼에 라벨 추가
    lv_obj_t* label2 = lv_label_create(btn2);
    if (label2 == NULL) {
        Serial.println("+++ Failed to create button label");
        return;
    }
    
    lv_label_set_text(label2, "쉬 소리");
    lv_obj_set_style_text_color(label2, lv_color_black(), 0);
    lv_obj_center(label2); 

    lv_obj_add_event_cb(btn2, urination_sound_handler, LV_EVENT_CLICKED, NULL);
}

/////////////// 5. 단말 정보 보기 ///////////////

void terminal_screen(lv_obj_t *parent) {
    if (parent == NULL) {
        Serial.println("+++ parent is NULL in terminal_screen");
        return;
    }

    static lv_style_t style_terminal;
    lv_style_init(&style_terminal);
    lv_style_set_border_width(&style_terminal, 1);
    lv_style_set_border_color(&style_terminal, lv_color_hex(0x0019F4));
    lv_style_set_width(&style_terminal, 270);  // 폭을 120 px로 설정
    lv_style_set_height(&style_terminal, 25); // 높이를 60 px로 설정

    // Create labels for network information
    // Model name
    lv_obj_t* model_title = lv_label_create(parent);
    if (model_title != NULL) {
        lv_label_set_text(model_title, "모델명:");
        lv_obj_set_pos(model_title, 25, 50);
    }
    
    model_label = lv_label_create(parent);
    if (model_label != NULL) {
        Serial.printf("Model Name : %s\n", lcd_state.model_name);
        lv_label_set_text(model_label, lcd_state.model_name);  // Will be set by update function
        lv_obj_set_pos(model_label, 150, 50);
        lv_obj_add_style(model_label, &style_terminal, 0);
    }
    
    // SW Version
    lv_obj_t* sw_ver_title = lv_label_create(parent);
    if (sw_ver_title != NULL) {
        lv_label_set_text(sw_ver_title, "SW버전:");
        lv_obj_set_pos(sw_ver_title, 25, 80);
    }
    
    sw_ver_label = lv_label_create(parent);
    if (sw_ver_label != NULL) {
        lv_label_set_text(sw_ver_label, lcd_state.sw_version);
        lv_obj_set_pos(sw_ver_label, 150, 80);
        lv_obj_add_style(sw_ver_label, &style_terminal, 0);
    }
    
    // Serial Number
    lv_obj_t* serial_title = lv_label_create(parent);
    if (serial_title != NULL) {
        lv_label_set_text(serial_title, "시리얼번호:");
        lv_obj_set_pos(serial_title, 25, 110);
    }
    
    serial_label = lv_label_create(parent);
    if (serial_label != NULL) {
        lv_label_set_text(serial_label, lcd_state.serial_num);  // Will be set by update function
        lv_obj_set_pos(serial_label, 150, 110);
        lv_obj_add_style(serial_label, &style_terminal, 0);
    }
    
    // SSID
    lv_obj_t* ssid_title = lv_label_create(parent);
    if (ssid_title != NULL) {
        lv_label_set_text(ssid_title, "SSID:");
        lv_obj_set_pos(ssid_title, 25, 140);
    }
    
    ssid_label = lv_label_create(parent);
    if (ssid_label != NULL) {
        lv_label_set_text(ssid_label, lcd_state.ssid);  // Will be set by update function
        lv_obj_set_pos(ssid_label, 150, 140);
        lv_obj_add_style(ssid_label, &style_terminal, 0);
    }
    
    // IP Address
    lv_obj_t* ip_title = lv_label_create(parent);
    if (ip_title != NULL) {
        lv_label_set_text(ip_title, "IP 주소:");
        lv_obj_set_pos(ip_title, 25, 170);
    }
    
    ip_label = lv_label_create(parent);
    if (ip_label != NULL) {
        lv_label_set_text(ip_label, lcd_state.ip_addr);
        lv_obj_set_pos(ip_label, 150, 170);
        lv_obj_add_style(ip_label, &style_terminal, 0);
    }
    
    // Gateway
    lv_obj_t* gateway_title = lv_label_create(parent);
    if (gateway_title != NULL) {
        lv_label_set_text(gateway_title, "Gateway:");
        lv_obj_set_pos(gateway_title, 25, 200);
    }
    
    gateway_label = lv_label_create(parent);
    if (gateway_label != NULL) {
        lv_label_set_text(gateway_label, lcd_state.gateway);
        lv_obj_set_pos(gateway_label, 150, 200);
        lv_obj_add_style(gateway_label, &style_terminal, 0);
    }
}

// Function to update network information
void update_network_info(const char* model, const char* serial, const char* ssid,
                        const char* ip, const char* gateway) {
    if (model && model_label) lv_label_set_text(model_label, model);
    if (serial && serial_label) lv_label_set_text(serial_label, serial);
    if (ssid && ssid_label) lv_label_set_text(ssid_label, ssid);
    if (ip && ip_label) lv_label_set_text(ip_label, ip);
    if (gateway && gateway_label) lv_label_set_text(gateway_label, gateway);
}

/////////////// 6. 공장 초기화 ///////////////

/////////////////////////////////////////////////////////

// 확인 버튼 이벤트 처리 (factory_event_handler)
void msg_close_factory(lv_event_t * e) {
    Serial.println("+++ msg_close_factory \n");

    factory_menu_cb();
}

void msg_close_handler_parent(lv_event_t * e) {
    Serial.println("+++ alert_msgbox del before \n");
    
    // alert_msgbox가 NULL인지 확인
    if (alert_msgbox != NULL) {
        lv_obj_del(alert_msgbox);
        alert_msgbox = NULL;
    }
    
    // 홈 화면으로 돌아가기
    lvgl_create_app_ui();
    //delay(5);
    //lvgl_update_app_ui();
}
 
void msg_close_handler_current(lv_event_t * e) {
    lv_obj_t * msgbox = lv_event_get_current_target(e);
    if (msgbox == NULL) {
        Serial.println("+++ msgbox is NULL in msg_close_handler_current");
        return;
    }
    
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
        if (msg_top_img_check == NULL) {
            Serial.println("+++ Failed to create msg_top_img_check");
            return;
        }
        
        lv_img_set_src(msg_top_img_check, &img_msgbox_check);

        // 2. 닫기 버튼 이미지 준비
        close_btn_img = lv_img_create(lv_scr_act());
        if (close_btn_img == NULL) {
            Serial.println("+++ Failed to create close_btn_img");
            return;
        }
        
        lv_img_set_src(close_btn_img, &img_close_btn);
        lv_obj_add_flag(close_btn_img, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_anim_time(close_btn_img, 0, LV_STATE_PRESSED);

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

        // 5. 닫기 버튼을 alert 메시지 박스의 우측 상단에 추가
        lv_obj_set_parent(close_btn_img, alert_msgbox);
        lv_obj_align(close_btn_img, LV_ALIGN_TOP_RIGHT, 10, -10);

        // 메시지 라벨 생성
        msg_label = lv_label_create(alert_msgbox);
        if (msg_label == NULL) {
            Serial.println("+++ Failed to create msg_label");
            return;
        }

        // 패스워드 유효성 검사
        int caseA = check_password_requirements(entered_password);
  
        Serial.println("+++ 2, case:");
        Serial.println(caseA);

        if (caseA == 0) {
            lv_label_set_text(msg_label, "암호는 영문 대/소문자, 숫자,\n특수문자를 모두 포함해야 합니다.");
        } else if (caseA == 2) {
            lv_label_set_text(msg_label, "패스워드를 입력하세요.");
        } else if (caseA == 3) {
            lv_label_set_text(msg_label, "패스워드는 최소 8자이상이어야 합니다.");
        } else if (caseA == 1) {
            if (verify_password(entered_password)) {
                is_authenticated = true;
                lv_label_set_text(msg_label, "성공, 암호가 확인되었습니다.");
                send_factory_init();
            } else {
                lv_label_set_text(msg_label, "실패, 잘못된 암호입니다.");
            }
        } 

        lv_obj_center(msg_label);

        // 6. 닫기 버튼 클릭 이벤트 핸들러 등록
        lv_obj_add_event_cb(close_btn_img, msg_close_factory, LV_EVENT_CLICKED, NULL);
           
        Serial.println("+++ 3"); 

        // 7. 확인 버튼 등록
        btn_hema = lv_btn_create(alert_msgbox);
        if (btn_hema == NULL) {
            Serial.println("+++ Failed to create btn_hema");
            return;
        }
        
        lv_obj_set_size(btn_hema, 50, 25);
        lv_obj_align(btn_hema, LV_ALIGN_BOTTOM_MID, 0, -5);

        // 버튼에 라벨 추가
        label_hema = lv_label_create(btn_hema);
        if (label_hema == NULL) {
            Serial.println("+++ Failed to create label_hema");
            return;
        }
        
        lv_label_set_text(label_hema, "확인");
        lv_obj_center(label_hema); 
        lv_obj_add_event_cb(btn_hema, msg_close_factory, LV_EVENT_CLICKED, NULL);
        lv_obj_set_style_anim_time(btn_hema, 0, LV_STATE_PRESSED);
    }  
}