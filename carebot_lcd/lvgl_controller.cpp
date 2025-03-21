#include "lvgl_controller.h"
#include "display.h"
#include "touch.h"
#include "common.h"
#include "nanum_gothic_16.h"
#include "menu.h"
#include "event.h"
#include "image_assets.h"

// UI objects
lv_obj_t *main_screen = NULL;
lv_obj_t *panel0 = NULL;
lv_obj_t *hygera = NULL;
lv_obj_t *motor = NULL;
lv_obj_t *error = NULL;
lv_obj_t *feces = NULL;
lv_obj_t *connect = NULL;
lv_obj_t *diaper = NULL;
lv_obj_t *menuimg = NULL;
lv_obj_t *tank = NULL;
lv_obj_t *power = NULL;
lv_obj_t *wifi = NULL;
lv_obj_t *bat = NULL;
lv_obj_t *water_level_label = NULL;
lv_obj_t *datetime_label = NULL;

lv_obj_t* msg_top_img_check = NULL;
lv_obj_t* close_btn_img = NULL;
lv_obj_t* alert_msgbox = NULL;
lv_obj_t* msg_label = NULL;
lv_obj_t* btn_hema = NULL;
lv_obj_t* label_hema = NULL;

// UI status flags
int cover_OPEN = 0;
int motor_ON = 0;
int wifi_ON = 0;
int error_ON = 0;
int urine_ON = 0;
int connect_ON = 0;
int diaper_ON = 0;
int fulllevel_ON = 0;
int power_ON = 0;
int menu_ON = 0;
int water_level = 0;



// Font declarations
LV_FONT_DECLARE(nanum_gothic_16);

disp_size_t disp_size;

// Function prototypes for UI event handlers
static void img_click_event_cb_menuimg(lv_event_t * e);
static void update_time(lv_timer_t * timer);
void menu(void);
static void setup_time_display(lv_obj_t * parent);

// Timestamp display buffer
char timestamp_display[20] = "2025-03-18 12:00:00";

void lvgl_init() {
    logMessage("LVGL", LOG_LEVEL_INFO, "Initializing LVGL");
    
    lv_init();
    
    // Determine display size based on resolution
    disp_size_t disp_size;
    if(LV_HOR_RES <= 320) disp_size = DISP_SMALL;
    else if(LV_HOR_RES < 720) disp_size = DISP_MEDIUM;
    else disp_size = DISP_LARGE;
    
    // Initialize fonts based on display size
    const lv_font_t *font_large;
    const lv_font_t *font_normal;
    
    if(disp_size == DISP_LARGE) {
        #if LV_FONT_MONTSERRAT_24
            font_large = &lv_font_montserrat_24;
        #else
            font_large = LV_FONT_DEFAULT;
            logMessage("LVGL", LOG_LEVEL_INFO, "LV_FONT_MONTSERRAT_24 is not enabled, using default font");
        #endif
        
        #if LV_FONT_MONTSERRAT_16
            font_normal = &lv_font_montserrat_16;
        #else
            font_normal = LV_FONT_DEFAULT;
            logMessage("LVGL", LOG_LEVEL_INFO, "LV_FONT_MONTSERRAT_16 is not enabled, using default font");
        #endif
    }
    else if(disp_size == DISP_MEDIUM) {
        #if LV_FONT_MONTSERRAT_20
            font_large = &lv_font_montserrat_20;
        #else
            font_large = LV_FONT_DEFAULT;
            logMessage("LVGL", LOG_LEVEL_INFO, "LV_FONT_MONTSERRAT_20 is not enabled, using default font");
        #endif
        
        #if LV_FONT_MONTSERRAT_14
            font_normal = &lv_font_montserrat_14;
        #else
            font_normal = LV_FONT_DEFAULT;
            logMessage("LVGL", LOG_LEVEL_INFO, "LV_FONT_MONTSERRAT_14 is not enabled, using default font");
        #endif
    }
    else { // DISP_SMALL
        #if LV_FONT_MONTSERRAT_18
            font_large = &lv_font_montserrat_18;
        #else
            font_large = LV_FONT_DEFAULT;
            logMessage("LVGL", LOG_LEVEL_INFO, "LV_FONT_MONTSERRAT_18 is not enabled, using default font");
        #endif
        
        #if LV_FONT_MONTSERRAT_12
            font_normal = &lv_font_montserrat_12;
        #else
            font_normal = LV_FONT_DEFAULT;
            logMessage("LVGL", LOG_LEVEL_INFO, "LV_FONT_MONTSERRAT_12 is not enabled, using default font");
        #endif
    }
    
    // Initialize theme
    #if LV_USE_THEME_DEFAULT
        lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), 
                             lv_palette_main(LV_PALETTE_RED), 
                             LV_THEME_DEFAULT_DARK, font_normal);
    #endif
    
    logMessage("LVGL", LOG_LEVEL_INFO, "LVGL initialized successfully");
}
// Time display update callback
static void update_time(lv_timer_t * timer) {
    if (datetime_label == NULL) {
        logMessage("LVGL", LOG_LEVEL_INFO, "LVGL", LOG_LEVEL_INFO, "datetime_label is NULL in update_time");
        return;
    }
    
    lv_label_set_text(datetime_label, timestamp_display);
    lv_obj_set_style_border_color(datetime_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(datetime_label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(datetime_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(datetime_label, 250, 25);
    lv_obj_align(datetime_label, LV_ALIGN_TOP_MID, -50, 25);
}

// Setup time display on screen
static void setup_time_display(lv_obj_t * parent) {
    logMessage("LVGL", LOG_LEVEL_INFO, "Setting up time display");
    
    // Create datetime label
    datetime_label = lv_label_create(parent);
    if (!datetime_label) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create datetime label");
        return;
    }
    
    lv_obj_set_style_text_font(datetime_label, &nanum_gothic_16, 0);
    
    // Initialize time display
    update_time(NULL);
    
    // Create timer to update time every second
    lv_timer_create(update_time, 1000, NULL);
    
    logMessage("LVGL", LOG_LEVEL_INFO, "Time display setup complete");
}

// Menu button click event handler
static void img_click_event_cb_menuimg(lv_event_t * e) {
    logMessage("LVGL", LOG_LEVEL_INFO, "Menu button clicked");
    menu();
}

// Update UI based on status flags
void lvgl_update_app_ui() {
    logMessage("LVGL", LOG_LEVEL_INFO, "UI 상태 업데이트 시작");
    
    // 메뉴에서 돌아왔을 때 홈 화면으로 전환하기
    if (menu_ON == 0 && lv_scr_act() != panel0) {
        // 홈 화면으로 전환
        transition_to_screen(panel0);
        logMessage("LVGL", LOG_LEVEL_INFO, "홈 화면으로 전환");
    }

    // Check and update motor status icon
    if (motor_ON) {
        //??? lv_img_set_src(motor, &img_motor_on);
    } else {
        lv_img_set_src(motor, &img_motor_off);
    }
    
    // Check and update error status icon
    if (error_ON) {
        lv_img_set_src(error, &img_error_on);
    } else {
        lv_img_set_src(error, &img_error_off);
    }
    
    // Check and update urine/feces detection icon
    if (urine_ON) {
        lv_img_set_src(feces, &img_feces_on);
    } else {
        lv_img_set_src(feces, &img_feces_off);
    }
    
    // Check and update connection status icon
    if (connect_ON) {
        lv_img_set_src(connect, &img_connect_on);
    } else {
        lv_img_set_src(connect, &img_connect_off);
    }
    
    // Check and update diaper status icon
    if (diaper_ON) {
        lv_img_set_src(diaper, &img_diaper_on);
    } else {
        lv_img_set_src(diaper, &img_diaper_off);
    }
    
    // Check and update power status icon
    if (power_ON) {
        lv_img_set_src(power, &img_power_on);
    } else {
        lv_img_set_src(power, &img_power_off);
    }
    
    // Check and update WiFi status icon
    if (wifi_ON) {
        lv_img_set_src(wifi, &img_wifi_on);
    } else {
        lv_img_set_src(wifi, &img_wifi_off);
    }
    
    // Update water level display
    lv_bar_set_value(tank, water_level, LV_ANIM_ON);
    
    // Update water level text
    char water_level_text[20];
    sprintf(water_level_text, "%d ml", water_level);
    lv_label_set_text(water_level_label, water_level_text);
    
    // Log update event
    logMessage("LVGL", LOG_LEVEL_INFO, "UI 상태 업데이트 완료");
}
// Create the main application UI
void lvgl_create_app_ui() {
    logMessage("LVGL", LOG_LEVEL_INFO, "Creating application UI");
    
    // Create main panel
    panel0 = lv_obj_create(NULL);
    if (!panel0) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create main panel");
        return;
    }
    
    lv_scr_load(panel0);
    menu_ON = 0;
    
    // Initialize BMP decoder
    lv_bmp_init();
    
    // Background image
    lv_obj_t * bgimg = lv_img_create(panel0);
    if (!bgimg) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create background image");
        return;
    }
    
    lv_img_set_src(bgimg, &img_bg);
    lv_obj_set_style_img_opa(bgimg, LV_OPA_COVER, 0);
    lv_obj_set_pos(bgimg, 0, 0);
    
    // Create Hygera logo
    hygera = lv_img_create(panel0);
    if (!hygera) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create Hygera logo");
        return;
    }
    
    lv_img_set_src(hygera, &img_hygera);
    lv_obj_set_pos(hygera, 10, 25);
    
    // Create motor status icon
    motor = lv_img_create(panel0);
    if (!motor) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create motor icon");
        return;
    }
    
    lv_img_set_src(motor, &img_motor_off);
    lv_obj_set_pos(motor, 30, 70);
    
    // Create motor label
    lv_obj_t *motor_label = lv_label_create(panel0);
    lv_obj_set_style_text_font(motor_label, &nanum_gothic_16, 0);
    lv_label_set_text(motor_label, "모터작동");
    lv_obj_set_style_border_color(motor_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(motor_label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(motor_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(motor_label, 70, 20);
    lv_obj_set_pos(motor_label, 20, 130);
    
    // Create error status icon
    error = lv_img_create(panel0);
    if (!error) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create error icon");
        return;
    }
    
    lv_img_set_src(error, &img_error_off);
    lv_obj_set_pos(error, 115, 70);
    
    // Create error label
    lv_obj_t *error_label = lv_label_create(panel0);
    lv_obj_set_style_text_font(error_label, &nanum_gothic_16, 0);
    lv_label_set_text(error_label, "작동오류");
    lv_obj_set_style_border_color(error_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(error_label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(error_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(error_label, 70, 20);
    lv_obj_set_pos(error_label, 105, 130);
    
    // Create feces detection icon
    feces = lv_img_create(panel0);
    if (!feces) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create feces icon");
        return;
    }
    
    lv_img_set_src(feces, &img_feces_off);
    lv_obj_set_pos(feces, 200, 70);
    
    // Create feces label
    lv_obj_t *feces_label = lv_label_create(panel0);
    lv_obj_set_style_text_font(feces_label, &nanum_gothic_16, 0);
    lv_label_set_text(feces_label, "배변감지");
    lv_obj_set_style_border_color(feces_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(feces_label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(feces_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(feces_label, 70, 20);
    lv_obj_set_pos(feces_label, 190, 130);
    
    // Create server connection icon
    connect = lv_img_create(panel0);
    if (!connect) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create connect icon");
        return;
    }
    
    lv_img_set_src(connect, &img_connect_off);
    lv_obj_set_pos(connect, 285, 70);
    
    // Create connection label
    lv_obj_t *connect_label = lv_label_create(panel0);
    lv_obj_set_style_text_font(connect_label, &nanum_gothic_16, 0);
    lv_label_set_text(connect_label, "서버연결");
    lv_obj_set_style_border_color(connect_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(connect_label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(connect_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(connect_label, 70, 20);
    lv_obj_set_pos(connect_label, 275, 130);
    
    // Create diaper status icon
    diaper = lv_img_create(panel0);
    if (!diaper) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create diaper icon");
        return;
    }
    
    lv_img_set_src(diaper, &img_diaper_off);
    lv_obj_set_pos(diaper, 100, 170);
    
    // Create diaper label
    lv_obj_t *diaper_label = lv_label_create(panel0);
    lv_obj_set_style_text_font(diaper_label, &nanum_gothic_16, 0);
    lv_label_set_text(diaper_label, "기저귀 체결");
    lv_obj_set_style_border_color(diaper_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(diaper_label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(diaper_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(diaper_label, 100, 20);
    lv_obj_set_pos(diaper_label, 70, 230);
    
    // Create menu icon with click event
    menuimg = lv_img_create(panel0);
    if (!menuimg) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create menu icon");
        return;
    }
    
    lv_img_set_src(menuimg, &img_menu_off);
    lv_obj_set_pos(menuimg, 220, 170);
    lv_obj_add_event_cb(menuimg, img_click_event_cb_menuimg, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(menuimg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_anim_time(menuimg, 0, LV_STATE_PRESSED);
    
    // Create menu label
    lv_obj_t *menuimg_label = lv_label_create(panel0);
    lv_obj_set_style_text_font(menuimg_label, &nanum_gothic_16, 0);
    lv_label_set_text(menuimg_label, "메뉴");
    lv_obj_set_style_border_color(menuimg_label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(menuimg_label, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(menuimg_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_size(menuimg_label, 70, 20);
    lv_obj_set_pos(menuimg_label, 210, 230);
    
    // Create water level bar
    tank = lv_bar_create(panel0);
    lv_obj_set_size(tank, 80, 175);
    lv_obj_set_pos(tank, 370, 75);
    lv_bar_set_range(tank, 0, 2200);
    lv_obj_set_style_radius(tank, 0, 0);
    lv_obj_set_style_radius(tank, 0, LV_PART_INDICATOR);
    lv_bar_set_mode(tank, LV_BAR_MODE_SYMMETRICAL);
    lv_obj_set_style_border_color(tank, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(tank, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(tank, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(tank, LV_OPA_100, 0);
    
    // Set initial water level
    water_level = 0;
    lv_bar_set_value(tank, water_level, LV_ANIM_ON);
    
    // Create water level label
    water_level_label = lv_label_create(panel0);
    char water_level_text[20];
    sprintf(water_level_text, "%d ml", water_level);
    lv_label_set_text(water_level_label, water_level_text);
    lv_obj_align_to(water_level_label, tank, LV_ALIGN_TOP_MID, 0, 10);
    
    // Style for water level label
    static lv_style_t style_label;
    lv_style_init(&style_label);
    lv_style_set_bg_opa(&style_label, LV_OPA_100);
    lv_obj_add_style(water_level_label, &style_label, 0);
    
    // Create power status icon
    power = lv_img_create(panel0);
    if (!power) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create power icon");
        return;
    }
    
    lv_img_set_src(power, &img_power_off);
    lv_obj_set_pos(power, 340, 25);
    
    // Create WiFi status icon with click event
    wifi = lv_img_create(panel0);
    if (!wifi) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create WiFi icon");
        return;
    }
    
    lv_img_set_src(wifi, &img_wifi_off);
    lv_obj_set_pos(wifi, 380, 21);
    lv_obj_set_style_pad_all(wifi, 5, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(wifi, img_click_event_cb_wifi, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(wifi, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_anim_time(wifi, 0, LV_STATE_PRESSED);
    
    // Create battery status icon
    bat = lv_img_create(panel0);
    if (!bat) {
        logMessage("LVGL", LOG_LEVEL_INFO, "Failed to create battery icon");
        return;
    }
    
    lv_img_set_src(bat, &img_bat_off);
    lv_obj_set_pos(bat, 420, 25);
    
    // Setup time display
    setup_time_display(panel0);
    
    logMessage("LVGL", LOG_LEVEL_INFO, "Application UI created successfully");
}