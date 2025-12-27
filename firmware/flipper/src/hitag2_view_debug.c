/*
 * Hi-Tag 2 Emulator - Debug View
 * Shows communication log and debug information
 */

#include "hitag2_view.h"
#include "hitag2_app.h"
#include <gui/elements.h>
#include <string.h>

/* Draw callback */
static void hitag2_view_debug_draw(Canvas* canvas, void* context) {
    Hitag2ViewDebug* instance = context;
    
    canvas_clear(canvas);
    
    // Draw title
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 0, 10, "Debug Log");
    
    // Draw connection status
    canvas_set_font(canvas, FontSecondary);
    if (instance->app->connected) {
        canvas_draw_str(canvas, 80, 10, "Connected");
    } else {
        canvas_set_color(canvas, ColorWarning);
        canvas_draw_str(canvas, 80, 10, "Disconnected");
        canvas_set_color(canvas, ColorBlack);
    }
    
    // Draw log buffer
    canvas_set_font(canvas, FontKeyboard);
    
    uint8_t y = 25;
    char* line_start = instance->log_buffer + instance->log_offset;
    char* p = line_start;
    
    while (*p && y < 70) {
        // Find end of line
        char* line_end = strchr(p, '\n');
        if (line_end == NULL) {
            line_end = p + strlen(p);
        }
        
        // Calculate line length
        size_t line_len = line_end - p;
        if (line_len > 20) {
            line_len = 20;
        }
        
        // Draw line
        if (line_len > 0) {
            char saved_char = *(p + line_len);
            *(p + line_len) = '\0';
            canvas_draw_str(canvas, 0, y, p);
            *(p + line_len) = saved_char;
        }
        
        y += 10;
        p = line_end + 1;
    }
    
    // Draw navigation hint
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 0, 72, "[Back] Return");
}

/* Input callback */
static bool hitag2_view_debug_input(InputEvent* event, void* context) {
    Hitag2ViewDebug* instance = context;
    
    if (event->type == InputTypeShort) {
        switch (event->key) {
            case InputKeyBack:
                // Return to main view
                return true;
        }
    }
    
    return false;
}

/* Enter callback */
static void hitag2_view_debug_enter(void* context) {
    Hitag2ViewDebug* instance = context;
    
    instance->log_offset = 0;
    strcpy(instance->log_buffer, "Hi-Tag 2 Debug Log\n");
    strcat(instance->log_buffer, "==================\n\n");
    strcat(instance->log_buffer, "Ready for debugging...\n");
    
    view_update(instance->view);
}

/* Exit callback */
static void hitag2_view_debug_exit(void* context) {
    UNUSED(context);
}

/* Add log message */
void hitag2_view_debug_add_log(Hitag2ViewDebug* instance, const char* message) {
    // Add message to log buffer
    size_t len = strlen(instance->log_buffer);
    size_t msg_len = strlen(message);
    
    if (len + msg_len + 2 < sizeof(instance->log_buffer)) {
        strcat(instance->log_buffer, message);
        strcat(instance->log_buffer, "\n");
    } else {
        // Buffer full, shift data
        memmove(instance->log_buffer, 
                instance->log_buffer + msg_len, 
                sizeof(instance->log_buffer) - msg_len - 2);
        instance->log_buffer[sizeof(instance->log_buffer) - msg_len - 2] = '\0';
        strcat(instance->log_buffer, message);
    }
    
    view_update(instance->view);
}

/* Allocate view */
Hitag2ViewDebug* hitag2_view_debug_alloc(App* app) {
    Hitag2ViewDebug* instance = malloc(sizeof(Hitag2ViewDebug));
    
    instance->view = view_alloc();
    view_set_context(instance->view, instance);
    view_set_draw_callback(instance->view, hitag2_view_debug_draw);
    view_set_input_callback(instance->view, hitag2_view_debug_input);
    view_set_enter_callback(instance->view, hitag2_view_debug_enter);
    view_set_exit_callback(instance->view, hitag2_view_debug_exit);
    
    instance->app = app;
    instance->log_buffer[0] = '\0';
    instance->log_offset = 0;
    
    return instance;
}

/* Free view */
void hitag2_view_debug_free(Hitag2ViewDebug* instance) {
    view_free(instance->view);
    free(instance);
}

/* Get view */
View* hitag2_view_debug_get_view(Hitag2ViewDebug* instance) {
    return instance->view;
}
