/*
 * Hi-Tag 2 Emulator - Token List View
 * Displays list of stored tokens and allows selection
 */

#include "hitag2_view.h"
#include "hitag2_app.h"
#include <gui/elements.h>

/* Draw callback */
static void hitag2_view_token_list_draw(Canvas* canvas, void* context) {
    Hitag2ViewTokenList* instance = context;
    App* app = instance->app;
    
    canvas_clear(canvas);
    
    // Draw title
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 0, 10, "Select Token");
    
    // Draw token list
    canvas_set_font(canvas, FontSecondary);
    
    int display_count = 0;
    for (int i = 0; i < MAX_TOKENS && display_count < 5; i++) {
        if (app->token_list[i].uid != 0) {
            uint8_t y = 25 + (display_count * 12);
            
            // Highlight selected item
            if (i == instance->selected_item) {
                canvas_draw_box(canvas, 0, y - 10, 128, 12);
                canvas_set_color(canvas, ColorBlack);
            }
            
            // Draw token info
            char token_str[32];
            snprintf(token_str, sizeof(token_str), "#%d: %08lX", i + 1, (unsigned long)app->token_list[i].uid);
            canvas_draw_str(canvas, 5, y, token_str);
            
            if (i == instance->selected_item) {
                canvas_set_color(canvas, ColorWhite);
            }
            
            display_count++;
        }
    }
    
    // Draw navigation hint
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_str(canvas, 0, 72, "[OK] Select  [Back] Cancel");
}

/* Input callback */
static bool hitag2_view_token_list_input(InputEvent* event, void* context) {
    Hitag2ViewTokenList* instance = context;
    App* app = instance->app;
    
    if (event->type == InputTypeShort) {
        switch (event->key) {
            case InputKeyUp:
                if (instance->selected_item > 0) {
                    instance->selected_item--;
                    view_update(instance->view);
                }
                return true;
                
            case InputKeyDown:
                // Find next non-empty token
                for (int i = instance->selected_item + 1; i < MAX_TOKENS; i++) {
                    if (app->token_list[i].uid != 0) {
                        instance->selected_item = i;
                        view_update(instance->view);
                        break;
                    }
                }
                return true;
                
            case InputKeyOk:
                // Select token
                if (app->token_list[instance->selected_item].uid != 0) {
                    app->selected_token_index = instance->selected_item;
                    app->token = app->token_list[instance->selected_item];
                    hitag2_app_update_status(app);
                }
                return true;
                
            case InputKeyBack:
                // Cancel and return to main view
                return true;
        }
    }
    
    return false;
}

/* Enter callback */
static void hitag2_view_token_list_enter(void* context) {
    Hitag2ViewTokenList* instance = context;
    App* app = instance->app;
    
    // Find first token
    instance->selected_item = 0;
    for (int i = 0; i < MAX_TOKENS; i++) {
        if (app->token_list[i].uid != 0) {
            instance->selected_item = i;
            break;
        }
    }
    
    view_update(instance->view);
}

/* Exit callback */
static void hitag2_view_token_list_exit(void* context) {
    UNUSED(context);
}

/* Allocate view */
Hitag2ViewTokenList* hitag2_view_token_list_alloc(App* app) {
    Hitag2ViewTokenList* instance = malloc(sizeof(Hitag2ViewTokenList));
    
    instance->view = view_alloc();
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, hitag2_view_token_list_draw);
    view_set_input_callback(instance->view, hitag2_view_token_list_input);
    view_set_enter_callback(instance->view, hitag2_view_token_list_enter);
    view_set_exit_callback(instance->view, hitag2_view_token_list_exit);
    
    instance->app = app;
    instance->selected_item = 0;
    
    return instance;
}

/* Free view */
void hitag2_view_token_list_free(Hitag2ViewTokenList* instance) {
    view_free(instance->view);
    free(instance);
}

/* Get view */
View* hitag2_view_token_list_get_view(Hitag2ViewTokenList* instance) {
    return instance->view;
}
