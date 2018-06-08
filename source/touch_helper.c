#include <stdlib.h>

#include "touch_helper.h"

#define TAP_MOVEMENT_GAP 20

void Touch_Init(TouchInfo * touchInfo) {
    touchInfo->state = TouchNone;
    touchInfo->tapType = TapNone;
}

void Touch_Process(TouchInfo * touchInfo) {
    touchPosition currentTouch;
    u32 touches = hidTouchCount();
    if (touches >= 1)
        hidTouchRead(&currentTouch, 0);

    // On touch start.
    if (touches == 1 && (touchInfo->state == TouchNone || touchInfo->state == TouchEnded)) {
        touchInfo->state = TouchStart;
        touchInfo->firstTouch = currentTouch;
        touchInfo->prevTouch = currentTouch;
        touchInfo->tapType = TapShort;
        touchInfo->touchStart = time(NULL);
    }
    // On touch moving.
    else if (touches >= 1 && touchInfo->state != TouchNone) {
        touchInfo->state = TouchMoving;
        touchInfo->prevTouch = currentTouch;

        if (touchInfo->tapType != TapNone && (abs(touchInfo->firstTouch.px - currentTouch.px) > TAP_MOVEMENT_GAP || abs(touchInfo->firstTouch.py - currentTouch.py) > TAP_MOVEMENT_GAP)) {
            touchInfo->tapType = TapNone;
        } else if (touchInfo->tapType == TapShort && time(NULL) - touchInfo->touchStart >= 2) {
            touchInfo->tapType = TapLong;
        }
    }
    // On touch end.
    else {
        touchInfo->state = (touchInfo->state == TouchMoving) ? TouchEnded : TouchNone;
    }
}