/*
 * Hi-Tag 2 Emulator - Token Edit View
 * Allows editing token fields (UID, key, etc.)
 */

#include "hitag2_view.h"
#include "hitag2_app.h"
#include <gui/elements.h>

/* Edit modes */
typedef enum {
    EDIT_MODE_UID,
    EDIT_MODE_KEY_BYTE,
    EDIT_MODE_CONFIG,
    EDIT_MODE_USER_DATA,
    EDIT_MODE_COUNT
} EditMode;

/* Edit mode names */
static const char* edit_mode_names[] = {
    "UID",
    "Key Byte",
    "Config",
    "User Data"
};

/* Draw callback */
static void hitag2_view_token_edit_draw(Canvas* canvas, void* context) {
    Hitag2ViewTokenEdit* instance = context;
    
    canvas_clear(canvas);
    
    // Draw title
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 0, 10, "Edit Token");
    
    // Draw current edit field
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 0, 25, edit_mode_names[instance->edit_mode]);
    
    // Draw value based on edit mode
    char value_str[32];
    
    switch (instance->edit_mode) {
        case EDIT_MODE_UID:
            snprintf(value_str, sizeof(value_str), "%08lX", (unsigned long)*(uint32_t*)instance->edit_buffer);
            break;
            
        case EDIT_MODE_KEY_BYTE:
            snprintf(value_str, sizeof(value_str), "Byte %d: %02X", 
                instance->byte_index, instance->edit_buffer[0]);
            break;
            
        case EDIT_MODE_CONFIG:
            snprintf(value_str, sizeof(value_str), "%08lX", (unsigned long)*(uint32_t*)instance->edit_buffer);
            break;
            
        case EDIT_MODE_USER_DATA:
            snprintf(value_str, sizeof(value_str), "%08lX", (unsigned long)*(uint32_t*)instance->edit_buffer);
            break;
            
        default:
            snprintf(value_str, sizeof(value_str), "???");
            break;
    }
    
    canvas_draw_str(canvas, 0, 40, value_str);
    
    // Draw hex keypad
    canvas_set_font(canvas, FontKeyboard);
    
    const char* hex_chars = "0123456789ABCDEF";
    uint8_t x = 10;
    uint8_t y = 55;
    
    for (int i = 0; i < 16; i++) {
        if (i == instance->cursor_pos) {
            canvas_draw_box(canvas, x - 2, y - 8, 7, 10);
            canvas_set_color(canvas, ColorBlack);
        }
        
        canvas_draw_char(canvas, x, y, hex_chars[i]);
        
        if (i == instance->cursor_pos) {
            canvas_set_color(canvas, ColorWhite);
        }
        
        x += 8;
        if (x > 120) {
            x = 10;
            y += 12;
        }
    }
    
    // Draw navigation hint
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 0, 72, "[OK] Save  [Back] Cancel");
}

/* Input callback */
static bool hitag2_view_token_edit_input(InputEvent* event, void* context) {
    Hitag2ViewTokenEdit* instance = context;
    
    if (event->type == InputTypeShort) {
        switch (event->key) {
            case InputKeyUp:
                if (instance->cursor_pos >= 4) {
                    instance->cursor_pos -= 4;
                    view_update(instance->view);
                }
                return true;
                
            case InputKeyDown:
                if (instance->cursor_pos < 12) {
                    instance->cursor_pos += 4;
                    view_update(instance->view);
                }
                return true;
                
            case InputKeyLeft:
                if (instance->cursor_pos > 0) {
                    instance->cursor_pos--;
                    view_update(instance->view);
                }
                return true;
                
            case InputKeyRight:
                if (instance->cursor_pos < 15) {
                    instance->cursor_pos++;
                    view_update(instance->view);
                }
                return true;
                
            case InputKeyOk:
                // Update value with current hex digit
                instance->edit_buffer[0] = instance->cursor_pos;
                return true;
                
            case InputKeyBack:
                // Cancel and return
                return true;
        }
    }
    
    return false;
}

/* Enter callback */
static void hitag2_view_token_edit_enter(void* context) {
    Hitag2ViewTokenEdit* instance = context;
    App* app = instance->app;
    
    // Load current UID for editing
    *(uint32_t*)instance->edit_buffer = app->token.uid;
    instance->edit_mode = EDIT_MODE_UID;
    instance->cursor_pos = 0;
    
    view_update(instance->view);
}

/* Exit callback */
static void hitag2_view_token_edit_exit(void* context) {
    UNUSED(context);
}

/* Allocate view */
Hitag2ViewTokenEdit* hitag2_view_token_edit_alloc(App* app) {
    Hitag2ViewTokenEdit* instance = malloc(sizeof(Hitag2ViewTokenEdit));
    
    instance->view = view_alloc();
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, hitag2_view_token_edit_draw);
    view_set_input_callback(instance->view, hitag2_view_token_edit_input);
    view_set_enter_callback(instance->view, hitag2_view_token_edit_enter);
    view_set_exit_callback(instance->view, hitag2_view_token_edit_exit);
    
    instance->app = app;
    instance->edit_mode = EDIT_MODE_UID;
    instance->byte_index = 0;
    instance->cursor_pos = 0;
    
    return instance;
}

/* Free view */
void hitag2_view_token_edit_free(Hitag2ViewTokenEdit* instance) {
    view_free(instance->view);
    free(instance);
}

/* Get view */
View* hitag2_view_token_edit_get_view(Hitag2ViewTokenEdit* instance) {
    return instance->view;
}
