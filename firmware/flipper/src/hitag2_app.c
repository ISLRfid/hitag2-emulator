/*
 * Hi-Tag 2 Emulator - Flipper Zero Application
 * Application entry point and configuration
 * 
 * Target: Flipper Zero
 * Purpose: User interface for Hi-Tag 2 emulation
 */

#include "hitag2_app.h"
#include "hitag2_view.h"
#include "hitag2_scene.h"

/* Application entry point */
int32_t hitag2_app(void* p) {
    UNUSED(p);
    
    App* app = app_alloc();
    
    // Load resources
    hitag2_app_load_resources(app);
    
    // Register views
    view_dispatcher_add_view(app->view_dispatcher, AppViewMain, hitag2_view_main_get_view(app->view_main));
    view_dispatcher_add_view(app->view_dispatcher, AppViewTokenList, hitag2_view_token_list_get_view(app->view_token_list));
    view_dispatcher_add_view(app->view_dispatcher, AppViewTokenEdit, hitag2_view_token_edit_get_view(app->view_token_edit));
    view_dispatcher_add_view(app->view_dispatcher, AppViewDebug, hitag2_view_debug_get_view(app->view_debug));
    
    // Set initial scene
    view_dispatcher_set_next_view(app->view_dispatcher, AppViewMain);
    scene_manager_next_scene(app->scene_manager, AppSceneMain);
    
    // Run application
    view_dispatcher_run(app->view_dispatcher);
    
    // Cleanup
    hitag2_app_free_resources(app);
    app_free(app);
    
    return 0;
}

/* Application configuration */
static const AppEventHandlerPrototype event_handlers[] = {
    {.event = AppEventTypeBack, .callback = hitag2_app_handle_back},
    {.event = AppEventTypeMenu, .callback = hitag2_app_handle_menu},
};

/* Allocate application */
App* app_alloc(void) {
    App* app = malloc(sizeof(App));
    
    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&scene_handlers, app);
    view_dispatcher_attach_to_gui(
        app->view_dispatcher, ViewDispatcherTypeFullscreen, NULL);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, hitag2_app_custom_event_callback);
    
    // Allocate views
    app->view_main = hitag2_view_main_alloc(app);
    app->view_token_list = hitag2_view_token_list_alloc(app);
    app->view_token_edit = hitag2_view_token_edit_alloc(app);
    app->view_debug = hitag2_view_debug_alloc(app);
    
    app->serial = serial_alloc();
    app->connected = false;
    app->selected_token_index = -1;
    app->emulation_active = false;
    app->tick_counter = 0;
    
    // Initialize token
    memset(&app->token, 0, sizeof(Token));
    memset(app->token_list, 0, sizeof(Token) * MAX_TOKENS);
    
    // Load default token
    app->token.uid = 0xDEADBEEF;
    app->token.config = 0x00000000;
    app->token.key[0] = 0x12;
    app->token.key[1] = 0x34;
    app->token.key[2] = 0x56;
    app->token.key[3] = 0x78;
    app->token.key[4] = 0x9A;
    app->token.key[5] = 0xBC;
    
    return app;
}

/* Free application */
void app_free(App* app) {
    if (app->serial) {
        serial_free(app->serial);
    }
    hitag2_view_main_free(app->view_main);
    hitag2_view_token_list_free(app->view_token_list);
    hitag2_view_token_edit_free(app->view_token_edit);
    hitag2_view_debug_free(app->view_debug);
    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);
    free(app);
}

/* Custom event callback */
bool hitag2_app_custom_event_callback(void* context, uint32_t event) {
    App* app = context;
    UNUSED(app);
    UNUSED(event);
    // Handle custom events from views
    return false;
}

/* Back button handler */
bool hitag2_app_handle_back(void* context) {
    App* app = context;
    // Let scene manager handle back
    return scene_manager_handle_back(app->scene_manager);
}

/* Menu button handler */
bool hitag2_app_handle_menu(void* context) {
    UNUSED(context);
    // Show menu
    return false;
}

/* Load application resources */
void hitag2_app_load_resources(App* app) {
    UNUSED(app);
    // Load icons and other resources
    // This would load custom icons if needed
}

/* Free application resources */
void hitag2_app_free_resources(App* app) {
    UNUSED(app);
    // Free any allocated resources
}

/* Update status display */
void hitag2_app_update_status(App* app) {
    if (app->emulation_active) {
        snprintf(
            app->status_text, sizeof(app->status_text),
            "Hi-Tag 2 Emulator\n\nUID: %08lX\n\nEmulation: ACTIVE",
            (unsigned long)app->token.uid);
    } else {
        snprintf(
            app->status_text, sizeof(app->status_text),
            "Hi-Tag 2 Emulator\n\nUID: %08lX\n\nEmulation: Ready",
            (unsigned long)app->token.uid);
    }
}

/* Start emulation */
bool hitag2_app_start_emulation(App* app) {
    if (!app->connected) {
        return false;
    }
    
    // Send start command
    uint8_t response[8];
    uint8_t response_len = sizeof(response);
    
    if (serial_send_command(app->serial, CMD_START_EMULATE, NULL, 0, response, &response_len)) {
        app->emulation_active = true;
        hitag2_app_update_status(app);
        return true;
    }
    
    return false;
}

/* Stop emulation */
bool hitag2_app_stop_emulation(App* app) {
    if (!app->connected) {
        return false;
    }
    
    // Send stop command
    uint8_t response[8];
    uint8_t response_len = sizeof(response);
    
    if (serial_send_command(app->serial, CMD_STOP_EMULATE, NULL, 0, response, &response_len)) {
        app->emulation_active = false;
        hitag2_app_update_status(app);
        return true;
    }
    
    return false;
}

/* Load token to emulator */
bool hitag2_app_load_token(App* app, Token* token) {
    if (!app->connected) {
        return false;
    }
    
    // Send token data
    uint8_t response[8];
    uint8_t response_len = sizeof(response);
    
    if (serial_send_command(app->serial, CMD_LOAD_TOKEN, (uint8_t*)token, sizeof(Token), response, &response_len)) {
        app->token = *token;
        hitag2_app_update_status(app);
        return true;
    }
    
    return false;
}

/* Get current token */
Token* hitag2_app_get_token(App* app) {
    return &app->token;
}

/* Set current token */
void hitag2_app_set_token(App* app, Token* token) {
    app->token = *token;
    hitag2_app_update_status(app);
}

/* Get connection status */
bool hitag2_app_is_connected(App* app) {
    return app->connected;
}

/* Set connection status */
void hitag2_app_set_connected(App* app, bool connected) {
    app->connected = connected;
}

/* Get serial connection */
SerialConnection* hitag2_app_get_serial(App* app) {
    return app->serial;
}

/* Get selected token index */
int hitag2_app_get_selected_index(App* app) {
    return app->selected_token_index;
}

/* Set selected token index */
void hitag2_app_set_selected_index(App* app, int index) {
    app->selected_token_index = index;
}

/* Get token from list */
Token* hitag2_app_get_list_token(App* app, int index) {
    if (index >= 0 && index < MAX_TOKENS) {
        return &app->token_list[index];
    }
    return NULL;
}

/* Set token in list */
void hitag2_app_set_list_token(App* app, int index, Token* token) {
    if (index >= 0 && index < MAX_TOKENS) {
        app->token_list[index] = *token;
    }
}

/* Add token to list */
int hitag2_app_add_token(App* app, Token* token) {
    for (int i = 0; i < MAX_TOKENS; i++) {
        if (app->token_list[i].uid == 0) {
            app->token_list[i] = *token;
            return i;
        }
    }
    return -1;  // No space
}

/* Remove token from list */
void hitag2_app_remove_token(App* app, int index) {
    if (index >= 0 && index < MAX_TOKENS) {
        memset(&app->token_list[index], 0, sizeof(Token));
    }
}
