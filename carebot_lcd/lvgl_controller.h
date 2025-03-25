#ifndef LVGL_CONTROLLER_H
#define LVGL_CONTROLLER_H

#include <Arduino.h>
#include <lvgl.h>

typedef enum {
    DISP_SMALL,
    DISP_MEDIUM,
    DISP_LARGE,
} disp_size_t;

// UI 상태 구조체 (각 요소의 현재 상태 저장)
typedef struct {
    bool connect_state;
    bool diaper_state;
    bool error_state;
    bool feces_state;
    bool wifi_state;
    int bat_state;  // 0: OFF, 1: ON, 2: LOW로 변경
    bool power_state;
    float water_level;

    bool initialized;
} UIState;

// Function declarations
void lvgl_init(void);
void updateUI();

void lvgl_create_app_ui(void);
void lvgl_update_app_ui(void);

extern char timestamp_display[20];

// External references to UI objects as needed
extern lv_obj_t *main_screen;
extern lv_obj_t *tank;
extern lv_obj_t *water_level_label;
extern lv_obj_t *datetime_label;

extern lv_obj_t *main_screen;
extern lv_obj_t *panel0;
extern lv_obj_t *hygera;
extern lv_obj_t *motor;
extern lv_obj_t *error;
extern lv_obj_t *feces;
extern lv_obj_t *connect;
extern lv_obj_t *diaper;
extern lv_obj_t *menuimg;
extern lv_obj_t *tank;
extern lv_obj_t *power;
extern lv_obj_t *wifi;
extern lv_obj_t *bat;
extern lv_obj_t *water_level_label;
extern lv_obj_t *datetime_label;
extern lv_obj_t* msg_top_img_check;
extern lv_obj_t* close_btn_img;
extern lv_obj_t* alert_msgbox;
extern lv_obj_t* msg_label;
extern lv_obj_t* btn_hema;
extern lv_obj_t* label_hema;

// UI status flags
extern int cover_OPEN;
extern int motor_ON;
extern int wifi_ON;
extern int error_ON;
extern int urine_ON;
extern int connect_ON;
extern int diaper_ON;
extern int fulllevel_ON;
extern int power_ON;
extern int menu_ON;
extern int water_level;

extern volatile UIState current_ui_state;
extern volatile bool ui_update_requested;


LV_IMG_DECLARE(img_bg);

LV_IMG_DECLARE(img_power_on);
LV_IMG_DECLARE(img_power_off);
LV_IMG_DECLARE(img_wifi_on);
LV_IMG_DECLARE(img_wifi_off);
LV_IMG_DECLARE(img_bat_on);
LV_IMG_DECLARE(img_bat_off);
LV_IMG_DECLARE(img_bat_low);


LV_IMG_DECLARE(img_hygera);

LV_IMG_DECLARE(img_motor_off);
//LV_IMG_DECLARE(img_motor_on);
LV_IMG_DECLARE(img_error_on);
LV_IMG_DECLARE(img_error_off);
LV_IMG_DECLARE(img_feces_on);
LV_IMG_DECLARE(img_feces_off);
LV_IMG_DECLARE(img_connect_on);
LV_IMG_DECLARE(img_connect_off);
LV_IMG_DECLARE(img_diaper_on);
LV_IMG_DECLARE(img_diaper_off);
LV_IMG_DECLARE(img_menu_off);

LV_IMG_DECLARE(img_msgbox_error);
LV_IMG_DECLARE(img_msgbox_check);
LV_IMG_DECLARE(img_close_btn);

LV_IMG_DECLARE(img_qr);
LV_IMG_DECLARE(img_speaker);


#endif // LVGL_CONTROLLER_H