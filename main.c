#include "utility.h"

__CONFIG(0x3942);
__CONFIG(0x1EFF);

METERSTATUS status;
METERINFO info;
SERVERMODESTATE serverMode = SM_OFF;

u8 bigBuf[220];
char *bigBufStr = (char *) bigBuf;

void __interrupt() isr(void) {
    if (CCP5IF == 1) {
        // A Capture event has happened
        handleCCP5Event();
        CCP5IF = 0;
        return;
    }

    if (RCIF == 1) {
        if (serverMode == SM_MONITOR)
            handleServerModeRX();
        else
            handleRXInterrupt();
        RCIF = 0;
        return;
    }

    if (TMR4IF == 1) {
        backlightTimerFired();
        TMR4IF = 0;
        return;
    }

#if 0
    if (TMR2IF == 1) // Timer 2 interrupt
    {
#asm
        clrwdt
#endasm
                TMR2IF = 0;
        return;
    }
#endif

}

void runTimer2() {
    PSA = 0; // Take prescaler away from the WDT
    CLRWDT();
    T2CON = 0x74;
    TMR2IE = 1; // Enable an interrupt on Timer 2
}

void initialiseHardware() {
    TRISA = 0xFF; // All of port A is input
    ANSELA = 0x0C; // A2 and A3 are analogue, others are digital

    TRISB = 0; // All of port B is output
    ANSELB = 0; // Port B is digital

    TRISC = 0xBF; // TRISC is all input except RC6 (DOUT)
    //	T0IE = 0;

    PEIE = 1; // Enable peripheral interrupts
    GIE = 1; // Enable global interrupts
    //	runTimer2();
    lcdInit();
    i2cInit();

}

void main() {
    char buf[20];
    u8 launchADMode = 0;

    // If we're running on the internal clock then make it a fast one
    //	if (OSCCON & 0x02)
    //		OSCCON = 0xFF;  //DEFAULT_OSCCON;
    //	OSCCON = 0x38;

    initialiseHardware();

    loadInfo();

    setBacklightTicks();
    backlight(BL_ON);


    loadStatus();
    loadSlotMap();

    if (status.checksum != 0xD158) {
        selectMeterType();
        status.checksum = 0xD158;
        status.curProduct = 0;
        status.numReadings = (status.meterType == 'T' ? 3 : 8);
        status.stored = 0;
        status.airValue = 0xFF;
        status.calibrationMode = 1;
        saveStatus();
    }

    if ((status.meterType == 'T') &&
            ((info.torryScale < 1) || (info.torryScale > 4))) {
        info.torryScale = 2;
        setTorryScale();
    }

    if (status.calibrationMode >= 2) {
        enterCalibrationMode();
    }

    lcdPad("Distell", LCD_CLEAR);
    switch (status.meterType) {
        case 'T':
            lcdPad("Torrymeter", LCD_LINE2);
            break;

        case 'P':
            if (status.fishFatmeter)
                lcdPad("FFM-692", LCD_LINE2);
            else
                lcdPad("MFM-1092", LCD_LINE2);
            break;

        case 'C':
            if (status.fishFatmeter)
                lcdPad("FFM-992", LCD_LINE2);
            else
                lcdPad("MFM-992", LCD_LINE2);
            break;
    }

    pause(2000);

    if (status.calibrationMode > 0) {
        lcdPad("CALIB MODE", LCD_CLEAR);
        lcdPad("ENABLED", LCD_LINE2);
        pause(2000);
    }

    if (status.demoMode == 1) {
        lcdPad("DEMO MODE", LCD_CLEAR);
        lcdPad("NOT FOR SALE", LCD_LINE2);
        pause(3000);
    }

    if (getButtonState() == BUTTON_NONE)
        ++launchADMode;

    lcdPad("v3.06", LCD_CLEAR);
    lcdPad(info.serialNumber, LCD_LINE2);
    pause(2000);

    if (getButtonState() == BUTTON_YES)
        ++launchADMode;

    showDateTime();
    pause(2000);

    rentalCheck();

    if (getButtonState() == BUTTON_NONE)
        ++launchADMode;

    if (launchADMode == 3)
        adMode();

    if (status.meterType != 'T')
        checkpadStartupTest();


    lcdPad("Stored data:", LCD_CLEAR);
    sprintf(buf, "%d reading%c", status.stored, (status.stored == 1 ? '\0' : 's'));
    lcdPad(buf, LCD_LINE2);
    pause(2000);

    while (1) {
        batteryCheck();
        showCalibrationName(status.curProduct, LCD_CLEAR);
        sprintf(buf, "SN=%d", status.numReadings);
        lcdPad(buf, LCD_LINE2);
        overlayYesNo();

        timedBacklight();
        enableServerMode();
        u8 bs = 0;
        do {
            bs = getButtonState();
            if (bs == BUTTON_NO) {
                // 'No' pressed
                disableServerMode();
                doChoicesMenu();
                waitForButtonRelease();
            } else if (bs == BUTTON_YES) {
                // 'Yes' pressed
                disableServerMode();
                while (startMeasuring());
                waitForButtonRelease();
            } else if (serverMode == SM_LAUNCH) {
                // Server mode command has been received
                runServerMode();
                enableServerMode(); // Reset the SM state machine ready for another command.
                bs = BUTTON_BOTH; // Force the display to be re-drawn
            }
        } while (bs == BUTTON_NONE);
    }
}


