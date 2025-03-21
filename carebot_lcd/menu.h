// menu.h
#ifndef MENU_H
#define MENU_H

#define LENGTH_TIMESTAMP  32
#define MAX_WIFI_NETWORKS 10

typedef struct {
    char ssid[33];  // Max SSID length
    int32_t rssi;
    bool is_encrypted;
} WiFiNetwork;

extern WiFiNetwork available_networks[MAX_WIFI_NETWORKS];

extern char* ssid_main;
extern char* ip_addr_main;
extern char* gateway_main;

// Forward declarations for screen callbacks
void main_screen_create(void);
void main_screen_destroy(void);
void menu_screen_create(void);
void menu_screen_destroy(void);
void settings_screen_create(void);
void settings_screen_destroy(void);

void create_wifi_selection_screen();

// 메뉴 콜백 함수 선언
void wifi_ap_setup_cb(void);
void wifi_ap_setup_cb_2(void);
void alarm_setup_cb(void);
void relay_info_cb(void);
void urination_cb(void);
void terminal_info_cb(void);
void factory_menu_cb(void);
void close_popup_cb(lv_event_t *e);


void display_init(void);
void task_delay(uint32_t ms);

void screen_manager_init(void);
//void screen_switch(screen_id_t new_screen);
void destroy_current_screen(lv_timer_t *timer);
void create_new_screen(lv_timer_t *timer);
void settings_screen_destroy(void);

void initial_scr(void);
void initial_cb_1(lv_event_t * e);
void initial_cb_2(lv_event_t * e);

void msg_close_handler_parent(lv_event_t * e);

void create_volume_control(lv_obj_t *parent);
void create_relay_settings(lv_obj_t *parent);
void radio_event_handler(lv_event_t *e);
void relay_event_handler(lv_event_t *e);
void urination_screen(lv_obj_t *parent);
void terminal_screen(lv_obj_t *parent);
void factory_screen(lv_obj_t *parent);
void urination_sound_handler(lv_event_t* e);
void breathing_sound_handler(lv_event_t* e);
void urination_event_handler_1(lv_event_t *e);
void urination_event_handler_2(lv_event_t *e);

void img_click_event_cb_wifi(lv_event_t *e);
void transition_to_screen(lv_obj_t *screen);


#endif //MENU_H
