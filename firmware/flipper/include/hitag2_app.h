/*
 * Hi-Tag 2 Emulator - Flipper Zero Application Header
 */

#ifndef HITAG2_APP_H
#define HITAG2_APP_H

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <views/view.h>
#include <views/view_dispatcher.h>
#include <scenes/scene_manager.h>
#include <notification/notification.h>
#include <dialogs/dialogs.h>
#include <storage/storage.h>

#include "hitag2_view.h"
#include "hitag2_scene.h"

/* Maximum number of tokens */
#define MAX_TOKENS 10
#define TOKEN_SIZE 32

/* Token structure */
typedef struct {
    uint32_t uid;           // Page 0: Serial number
    uint32_t config;        // Page 1: Configuration
    uint8_t key[6];         // Pages 2-3: Secret key
    uint32_t user_data[4];  // Pages 4-7: User data
} Token;

/* Application state */
struct App {
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    
    // View pointers
    Hitag2ViewMain* view_main;
    Hitag2ViewTokenList* view_token_list;
    Hitag2ViewTokenEdit* view_token_edit;
    Hitag2ViewDebug* view_debug;
    
    // Application data
    Token token;
    Token token_list[MAX_TOKENS];
    int selected_token_index;
    bool emulation_active;
    
    // Communication
    SerialConnection* serial;
    bool connected;
    
    // UI state
    char status_text[64];
    uint8_t tick_counter;
};

/* Application entry point */
int32_t hitag2_app(void* p);

/* App lifecycle */
App* app_alloc(void);
void app_free(App* app);

/* Event handlers */
bool hitag2_app_custom_event_callback(void* context, uint32_t event);
bool hitag2_app_handle_back(void* context);
bool hitag2_app_handle_menu(void* context);

/* Resource management */
void hitag2_app_load_resources(App* app);
void hitag2_app_free_resources(App* app);

/* Token management */
bool hitag2_app_start_emulation(App* app);
bool hitag2_app_stop_emulation(App* app);
bool hitag2_app_load_token(App* app, Token* token);

/* Accessors */
Token* hitag2_app_get_token(App* app);
void hitag2_app_set_token(App* app, Token* token);
bool hitag2_app_is_connected(App* app);
void hitag2_app_set_connected(App* app, bool connected);
SerialConnection* hitag2_app_get_serial(App* app);
int hitag2_app_get_selected_index(App* app);
void hitag2_app_set_selected_index(App* app, int index);
Token* hitag2_app_get_list_token(App* app, int index);
void hitag2_app_set_list_token(App* app, int index, Token* token);
int hitag2_app_add_token(App* app, Token* token);
void hitag2_app_remove_token(App* app, int index);

/* Status update */
void hitag2_app_update_status(App* app);

#endif // HITAG2_APP_H
