/*
 * Hi-Tag 2 Emulator - Scene Management
 */

#include "hitag2_scene.h"
#include "hitag2_app.h"
#include "hitag2_view.h"

/* Scene handlers */
static bool scene_on_event(void* context, SceneManagerEvent event) {
    App* app = context;
    
    switch (event.type) {
        case SceneManagerEventTypeBack:
            // Exit application on back button
            return false;
            
        case SceneManagerEventTypeCustom:
            switch (event.event) {
                case AppSceneMain:
                    // Handle main scene events
                    break;
            }
            break;
    }
    
    return true;
}

/* Main scene on enter */
static void scene_main_on_enter(void* context) {
    App* app = context;
    view_dispatcher_switch_to_view(app->view_dispatcher, AppViewMain);
}

/* Main scene on exit */
static void scene_main_on_exit(void* context) {
    UNUSED(context);
}

/* Scene definitions */
void (*const scene_on_enter_handlers[])(void*) = {
    scene_main_on_enter,
};

void (*const scene_on_exit_handlers[])(void*) = {
    scene_main_on_exit,
};

const SceneManagerHandlers scene_handlers = {
    .on_event = scene_on_event,
    .on_enter_handlers = scene_on_enter_handlers,
    .on_exit_handlers = scene_on_exit_handlers,
    .scene_num = AppSceneCount,
};
