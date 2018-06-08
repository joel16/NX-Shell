#ifndef NX_SHELL_TOUCH_HELPER_H
#define NX_SHELL_TOUCH_HELPER_H

#include <switch.h>
#include <time.h>

typedef enum TouchState {
    TouchNone,
    TouchStart,
    TouchMoving,
    TouchEnded
} TouchState;

typedef enum TapType {
    TapNone,
    TapShort,
    TapLong
} TapType;

typedef struct TouchInfo {
    TouchState state;
    touchPosition firstTouch;
    touchPosition prevTouch;
    TapType tapType;
    time_t touchStart;
} TouchInfo;

void Touch_Init(TouchInfo * touchInfo);
void Touch_Process(TouchInfo * touchInfo);

#endif