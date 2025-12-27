/*
 * Hi-Tag 2 Emulator - View Header
 */

#ifndef HITAG2_VIEW_H
#define HITAG2_VIEW_H

#include <gui/view.h>
#include <gui/view_dispatcher.h>

#include "hitag2_app.h"

/* View IDs */
typedef enum {
    AppViewMain,
    AppViewTokenList,
    AppViewTokenEdit,
    AppViewDebug,
    AppViewCount
} AppView;

/* Main view events */
typedef enum {
    Hitag2ViewMainEventExit,
    Hitag2ViewMainEventShowTokenList,
    Hitag2ViewMainEventReadToken,
    Hitag2ViewMainEventWriteToken,
    Hitag2ViewMainEventStartEmulation,
    Hitag2ViewMainEventStopEmulation,
    Hitag2ViewMainEventShowDebug,
} Hitag2ViewMainEvent;

/* Main view callback */
typedef void (*Hitag2ViewMainCallback)(Hitag2ViewMainEvent event, void* context);

/* Main view structure */
struct Hitag2ViewMain {
    View* view;
    App* app;
    Hitag2ViewMainCallback callback;
    void* callback_context;
};
typedef struct Hitag2ViewMain Hitag2ViewMain;

/* Token list view structure */
struct Hitag2ViewTokenList {
    View* view;
    App* app;
    uint8_t selected_item;
};
typedef struct Hitag2ViewTokenList Hitag2ViewTokenList;

/* Token edit view structure */
struct Hitag2ViewTokenEdit {
    View* view;
    App* app;
    int edit_mode;
    int byte_index;
    uint8_t edit_buffer[4];
    uint8_t cursor_pos;
};
typedef struct Hitag2ViewTokenEdit Hitag2ViewTokenEdit;

/* Debug view structure */
struct Hitag2ViewDebug {
    View* view;
    App* app;
    char log_buffer[512];
    uint16_t log_offset;
};
typedef struct Hitag2ViewDebug Hitag2ViewDebug;

/* Main view functions */
Hitag2ViewMain* hitag2_view_main_alloc(App* app);
void hitag2_view_main_free(Hitag2ViewMain* instance);
View* hitag2_view_main_get_view(Hitag2ViewMain* instance);
void hitag2_view_main_set_callback(Hitag2ViewMain* instance, Hitag2ViewMainCallback callback, void* context);

/* Token list view functions */
Hitag2ViewTokenList* hitag2_view_token_list_alloc(App* app);
void hitag2_view_token_list_free(Hitag2ViewTokenList* instance);
View* hitag2_view_token_list_get_view(Hitag2ViewTokenList* instance);

/* Token edit view functions */
Hitag2ViewTokenEdit* hitag2_view_token_edit_alloc(App* app);
void hitag2_view_token_edit_free(Hitag2ViewTokenEdit* instance);
View* hitag2_view_token_edit_get_view(Hitag2ViewTokenEdit* instance);

/* Debug view functions */
Hitag2ViewDebug* hitag2_view_debug_alloc(App* app);
void hitag2_view_debug_free(Hitag2ViewDebug* instance);
View* hitag2_view_debug_get_view(Hitag2ViewDebug* instance);
void hitag2_view_debug_add_log(Hitag2ViewDebug* instance, const char* message);

#endif // HITAG2_VIEW_H
