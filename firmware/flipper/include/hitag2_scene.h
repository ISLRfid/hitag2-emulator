/*
 * Hi-Tag 2 Emulator - Scene Management Header
 */

#ifndef HITAG2_SCENE_H
#define HITAG2_SCENE_H

#include <scene_manager.h>

/* Scene IDs */
typedef enum {
    AppSceneMain,
    AppSceneTokenList,
    AppSceneTokenEdit,
    AppSceneDebug,
    AppSceneCount
} AppScene;

/* Scene handlers */
extern const SceneManagerHandlers scene_handlers;

#endif // HITAG2_SCENE_H
