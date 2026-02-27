#include "utility.h"

void dumpCalibration(const u8 slot) {
    u8 eepromNum = slot >> 4;
    u16 base = (u16)((slot & 0x0F) * 2000);

    if ((eepromNum == 3) || (eepromNum > 4))
        return;

    // 50 cycles of 20 calibration numbers
    u16 cdata[20];
    char calName[13];
    getCalibrationName(slot, calName);
    sprintf(bigBufStr, "***S%s\x0D***N%s\x0D***X%d\x0D", info.serialNumber, calName, slot);
    sendSerialString(bigBufStr);
    for (u8 i = 0; i < 50; ++i) {
        u16 offset = (u16)(base + (40 * i));
        readI2CEEPROM(eepromNum, offset, (u8 *) cdata, sizeof (cdata));
        sprintf(bigBufStr, "%d: ", i);
        for (u8 j = 0; j < 20; ++j) {
            char buf[10];
            sprintf(buf, "%d ", cdata[j]);
            strcat(bigBufStr, buf);
        }
        strcat(bigBufStr, "\x0D");
        sendSerialString(bigBufStr);
    }
}

void runCalibrationProcess() {
    char buf[20];

    enableServerMode();

    // Air value, if appropriate
    if (status.meterType != 'T') {
        sprintf(buf, "AIR=%d\x0D", status.airValue);
        sendSerialString(buf);
    }

    status.curProduct = 64;
    showCalibrationName(status.curProduct, LCD_CLEAR);
    u16 ticker = 9999;
    while (1) {
        if (ticker >= 30) {
            sendSerialString("+++\x0D");
            ticker = 0;
        }
        ++ticker;

        // Show the current reading
        u16 adVal;
        if (status.meterType == 'T')
            adVal = getAvgPulse();
        else
            adVal = getAvgAD();
        u16 cdata = getCalibrationValue(adVal, status.curProduct);
        sprintf(buf, "V=%d AD=%d    ", cdata, adVal);
        lcd(buf, LCD_LINE2);

        u8 bs = getButtonState();
        if (bs == BUTTON_BOTH) {
            status.curProduct = getNextFilledSlot(status.curProduct, 0, 79);
            if (status.curProduct == 255)
                status.curProduct = 64;
            showCalibrationName(status.curProduct, LCD_CLEAR);
            waitForButtonRelease();
        } else if (bs == BUTTON_NO) {
            // Either exit or acknowledge, depending on how long it is held down for
            u16 counter = 0;
            while (getButtonState() == BUTTON_NO) {
                ++counter;
                pause(10);
            }
            if (counter > 400)
                return;
            sendSerialString("REMOTEACK\x0D");
            ticker = 0;
        } else if (bs == BUTTON_YES) {
            if (status.curProduct != 64)
                cdata = getCalibrationValue(adVal, 64);
            if ((status.meterType == 'T') && (readAD(0) < 200))
                lcd("   No contact   ", LCD_LINE2);
            else {
                sprintf(buf, "**%s:%d:%d\x0D", info.serialNumber, adVal, cdata);
                sendSerialString(buf);
                ticker = 0;
            }
        } else if (serverMode == SM_LAUNCH) {
            // Server mode command has been received
            runServerMode();
            if (status.calibrationMode <= 1)
                return;
            enableServerMode(); // Reset the SM state machine ready for another command.
            showCalibrationName(status.curProduct, LCD_CLEAR);
            ticker = 0;
        }

        pause(40);

    }
}

void enterCalibrationMode() {
    setBaudRate(BAUD9600);
    char buf[20];
    u16 ticker = 0;
    backlight(BL_ON);
    enableServerMode();
    while (1) {
        if (ticker == 0) {
            lcdPad("Calib Mode", LCD_CLEAR);
            lcdPad("Waiting...", LCD_LINE2);
            sprintf(buf, "IAM%c%s", status.meterType, info.serialNumber);
            strcat(buf, "\x0D");
            sendSerialString(buf);
            ticker = 10000;
        }

        --ticker;

        u8 bs = getButtonState();
        if (bs == BUTTON_NO) {
            // 'No' pressed
            disableServerMode();
            waitForButtonRelease();
            status.calibrationMode = 1;
            saveStatus();
            return;
        } else if (serverMode == SM_LAUNCH) {
            // Server mode command has been received
            runServerMode();

            if (status.calibrationMode >= 3) {
                runCalibrationProcess();
                status.calibrationMode = 2;
            }

            enableServerMode(); // Reset the SM state machine ready for another command.
        }
    }
}

