/*
 * Hi-Tag 2 Emulator - Main View
 * Displays status and provides main menu controls
 */

#include "hitag2_view.h"
#include "hitag2_app.h"
#include <gui/elements.h>

/* Draw callback */
static void hitag2_view_main_draw(Canvas* canvas, void* context) {
    Hitag2ViewMain* instance = context;
    App* app = instance->app;
    
    // Clear canvas
    canvas_clear(canvas);
    
    // Draw title
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 0, 10, "Hi-Tag 2 Emulator");
    
    // Draw status
    canvas_set_font(canvas, FontSecondary);
    if (app->emulation_active) {
        canvas_draw_str(canvas, 0, 25, "Status: ACTIVE");
        canvas_draw_str(canvas, 0, 35, "RFID field detected");
    } else {
        canvas_draw_str(canvas, 0, 25, "Status: Ready");
        canvas_set_color(canvas, ColorWarning);
        canvas_draw_str(canvas, 0, 35, "Connect Arduino bridge");
        canvas_set_color(canvas, ColorBlack);
    }
    
    // Draw UID
    char uid_str[32];
    snprintf(uid_str, sizeof(uid_str), "UID: %08lX", (unsigned long)app->token.uid);
    canvas_draw_str(canvas, 0, 50, uid_str);
    
    // Draw token name
    if (app->selected_token_index >= 0) {
        char token_name[32];
        snprintf(token_name, sizeof(token_name), "Token: #%d", app->selected_token_index + 1);
        canvas_draw_str(canvas, 0, 60, token_name);
    }
    
    // Draw menu hint
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 0, 72, "[OK] Select  [R] Read");
}

/* Input callback */
static bool hitag2_view_main_input(InputEvent* event, void* context) {
    Hitag2ViewMain* instance = context;
    App* app = instance->app;
    
    if (event->type == InputTypeShort) {
        switch (event->key) {
            case InputKeyOk:
                // Show token list
                if (instance->callback) {
                    instance->callback(Hitag2ViewMainEventShowTokenList, instance->callback_context);
                }
                return true;
                
            case InputKeyBack:
                // Exit application
                if (instance->callback) {
                    instance->callback(Hitag2ViewMainEventExit, instance->callback_context);
                }
                return true;
                
            case InputKeyRight:
                // Read token from Arduino
                if (instance->callback) {
                    instance->callback(Hitag2ViewMainEventReadToken, instance->callback_context);
                }
                return true;
                
            case InputKeyLeft:
                // Write token to Arduino
                if (instance->callback) {
                    instance->callback(Hitag2ViewMainEventWriteToken, instance->callback_context);
                }
                return true;
                
            case InputKeyUp:
                // Toggle emulation
                if (app->emulation_active) {
                    if (instance->callback) {
                        instance->callback(Hitag2ViewMainEventStopEmulation, instance->callback_context);
                    }
                } else {
                    if (instance->callback) {
                        instance->callback(Hitag2ViewMainEventStartEmulation, instance->callback_context);
                    }
                }
                return true;
                
            case InputKeyDown:
                // Show debug view
                if (instance->callback) {
                    instance->callback(Hitag2ViewMainEventShowDebug, instance->callback_context);
                }
                return true;
        }
    }
    
    return false;
}

/* Enter callback */
static void hitag2_view_main_enter(void* context) {
    Hitag2ViewMain* instance = context;
    App* app = instance->app;
    
    // Update status
    hitag2_app_update_status(app);
}

/* Exit callback */
static void hitag2_view_main_exit(void* context) {
    UNUSED(context);
    // Cleanup
}

/* Allocate view */
Hitag2ViewMain* hitag2_view_main_alloc(App* app) {
    Hitag2ViewMain* instance = malloc(sizeof(Hitag2ViewMain));
    
    instance->view = view_alloc();
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, hitag2_view_main_draw);
    view_set_input_callback(instance->view, hitag2_view_main_input);
    view_set_enter_callback(instance->view, hitag2_view_main_enter);
    view_set_exit_callback(instance->view, hitag2_view_main_exit);
    
    instance->app = app;
    instance->callback = NULL;
    instance->callback_context = NULL;
    
    return instance;
}

/* Free view */
void hitag2_view_main_free(Hitag2ViewMain* instance) {
    view_free(instance->view);
    free(instance);
}

/* Get view */
View* hitag2_view_main_get_view(Hitag2ViewMain* instance) {
    return instance->view;
}

/* Set callback */
void hitag2_view_main_set_callback(Hitag2ViewMain* instance, Hitag2ViewMainCallback callback, void* context) {
    instance->callback = callback;
    instance->callback_context = context;
}
