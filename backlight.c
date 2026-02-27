#include "utility.h"

static u16 ticks;
u16 backlightTickCount;

void setBacklightTicks()
{
    // The multiplier here is based on trial and error
    backlightTickCount = (u16)(info.backlightSetting * 19.3);
}

void timedBacklight()
{
    if (info.backlightSetting == 0)
    {
        RB4 = 1;    // Backlight off
    }
    else if (info.backlightSetting == 0xFF)
    {
        RB4 = 0;    // Backlight on
    }
    else
    {
        ticks = backlightTickCount;
        PR4 = 0xFF;
        RB4 = 0; // Backlight on
        // Use timer 4 for backlight off
        TMR4 = 0;
        T4CON = 0x7F;
        TMR4IE = 1;
    }
}

void backlightTimerFired() {
    --ticks;
    if (ticks == 0) {
        TMR4IE = 0;
        T4CON = 0;
        RB4 = 1; // Turn off the backlight
    }
}

void backlight(BACKLIGHTSTATE state) {
    if (info.backlightSetting == 0)
    {
        state = BL_OFF;
    }
    else if (info.backlightSetting == 0xFF)
    {
        state = BL_ON;
    }
    
    T4CON = 0; // Disable Timer 4, so the backlight timer doesn't fire
    RB4 = state;
}
