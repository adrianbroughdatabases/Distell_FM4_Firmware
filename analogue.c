#include "utility.h"

static u8 pulseStage = 0;
static u16 pulseStart = 0;
static u16 pulseEnd = 0;

static u8 pulseStartL = 0;
static u8 pulseStartH = 0;
static u8 pulseEndL = 0;
static u8 pulseEndH = 0;

u16 readAD(u8 channel) {
    if (channel == 0)
        ADCON0 = 0x09;
    else
        ADCON0 = 0x0D;
    //	CHS0 = (channel & 0x01);
    //	CHS1 = 1;
    ADCON1 = 0xE0;
    ADGO = 1;
    while (ADGO);

    u16 rv = ADRESL + (ADRESH * 0x100);
    ADCON1 = 0x86;
    ADCON0 = 0x80;
    return rv;
}

void handleCCP5Event() {
    CCP5IE = 0;
    if (pulseStage == 0) {
        pulseStage = 1;
        pulseStartH = CCPR5H;
        pulseStartL = CCPR5L;
        CCP5CON = 0x04;
        CCP5IE = 1;
    } else if (pulseStage == 1) {
        pulseStage = 2;
        pulseEndH = CCPR5H;
        pulseEndL = CCPR5L;
        CCP5CON = 0x00;
    }
}

u16 readPulse() {
    CCP5IE = 0;
    pulseStage = 0;
    T1CON = 0x00; // Stop Timer 1
    TMR1L = 0x00; // Clear Timer 1
    TMR1H = 0x00;
    TMR1IF = 0; // Clear the T1 overflow interrupt indicator
    //	T1IE = 0x01;			// Enable interrupts, use this as a 'no pulse' indicator
    T1CON = 0x11; // Start Timer 1, 1:2 prescaler
    CCP5CON = 0x05; // CCP5 Capture mode, rising edge
    CCP5IF = 0;
    CCP5IE = 1;

    //	u16 timer=0;
    while (pulseStage < 2) {
        //		++timer;
        //		if (timer > 2000)
        if (TMR1IF) {
            pulseStartL = 0;
            pulseStartH = 0;
            pulseEndL = 0;
            pulseEndH = 0;
            break;
        }
    };

    T1CON = 0x00; // Stop Timer 1
    //	T1IE = 0x00;

    pulseStart = pulseStartL + (pulseStartH * 0x100);
    pulseEnd = pulseEndL + (pulseEndH * 0x100);

    u16 rv = pulseEnd - pulseStart;
    if (rv > 1100)
    {
        // Anything above 1100 is out of range on a properly set-up board.
        return 0;
    }
    else
        return rv;
}

