#include <stdlib.h>
#include <stdio.h>

#include "touch_helper.h"

#define TAP_MOVEMENT_GAP 20
#define LONG_TAP_PERIOD 1.5


void Touch_Init(TouchInfo *touchInfo) {
    touchInfo->state = TouchNone;
    touchInfo->tapType = TapNone;
}

void Touch_Process(TouchInfo *touchInfo) {
    touchPosition currentTouch;
    u32 touches = hidTouchCount();
    
    if (touches >= 1)
        hidTouchRead(&currentTouch, 0);
        
    u64 current_time;
    timeGetCurrentTime(TimeType_UserSystemClock, &current_time);
    
    // On touch start.
    if (touches == 1 && (touchInfo->state == TouchNone || touchInfo->state == TouchEnded)) {
        touchInfo->state = TouchStart;
        touchInfo->firstTouch = currentTouch;
        touchInfo->prevTouch = currentTouch;
        touchInfo->tapType = TapShort;
        touchInfo->touchStart = current_time;
    }
    // On touch moving.
    else if (touches >= 1 && touchInfo->state != TouchNone) {
        touchInfo->state = TouchMoving;
        touchInfo->prevTouch = currentTouch;

        if (touchInfo->tapType != TapNone && (abs(touchInfo->firstTouch.px - currentTouch.px) > TAP_MOVEMENT_GAP || abs(touchInfo->firstTouch.py - currentTouch.py) > TAP_MOVEMENT_GAP))
            touchInfo->tapType = TapNone;
        else if (touchInfo->tapType == TapShort && current_time - touchInfo->touchStart >= LONG_TAP_PERIOD)
            touchInfo->tapType = TapLong;
    }
    // On touch end.
    else
        touchInfo->state = (touchInfo->state == TouchMoving) ? TouchEnded : TouchNone;
}