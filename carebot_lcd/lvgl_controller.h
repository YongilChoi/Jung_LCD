#ifndef LVGL_CONTROLLER_H
#define LVGL_CONTROLLER_H

#include <Arduino.h>
#include <lvgl.h>
#include "image_assets.h"

typedef enum {
    DISP_SMALL,
    DISP_MEDIUM,
    DISP_LARGE,
} disp_size_t;

// Function declarations
void lvgl_init(void);
void lvgl_create_app_ui(void);
void lvgl_update_app_ui(void);

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



#endif // LVGL_CONTROLLER_H