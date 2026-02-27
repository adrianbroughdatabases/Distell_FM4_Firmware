#include "utility.h"

u8 getButtonState() {
    u8 rv = BUTTON_NONE;

    if (RC1 == 0)
        rv |= BUTTON_NO;
    if (RC0 == 0)
        rv |= BUTTON_YES;
    return rv;
}

u8 waitForButtonPress() {
    timedBacklight();
    u8 rv = BUTTON_NONE;
    while (rv == BUTTON_NONE) {
        rv = getButtonState();
    };
    backlight(BL_ON);
    pause(100);
    waitForButtonRelease();
    return rv;
}

u8 waitForButtonDown() {
    timedBacklight();
    u8 rv = BUTTON_NONE;
    while (rv == BUTTON_NONE) {
        rv = getButtonState();
    };
    backlight(BL_ON);
    pause(600);
    return rv;
}

void waitForButtonRelease() {
    do {
        if (getButtonState() == BUTTON_NONE) {
            pause(100);
            return;
        }
    } while (1);
}
