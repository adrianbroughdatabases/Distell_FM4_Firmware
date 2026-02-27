#include "utility.h"

u8 byteBCD2DEC(u8 bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

u8 byteDEC2BCD(u8 dec) {
    return (u8)(((dec / 10) << 4) | (dec % 10));
}

void pause(u16 ticks) {
    for (u16 i = 0; i < ticks; ++i) {
        for (u8 j = 0; j < 200; ++j) {
            asm("NOP");
        }
    }
}

void pauseTick(u16 ticks) {
    T1CON = 0x30; // Turn off Timer 1
    TMR1L = 0x00;
    TMR1H = 0x00;

    T1CON = 0x31; // Start Timer 1
    while (1) {
        u16 tl = TMR1L;
        u16 th = TMR1H;

        u16 current = (th << 8) | tl;
        if (current > ticks)
            return;
    }
}

u8 *shortToText(short value, u8 *buf, u8 decimal) {
    u8 tmp[10];
    u8 length = 0;
    while (value > 0) {
        tmp[length++] = 48 + (value % 10);
        value = value / 10;
    }
    u8 src = length - 1;
    decimal = decimal - 1;
    for (u8 i = 0; i < length; ++i) {
        if (src == decimal)
            *buf++ = '.';
        *buf++ = tmp[src--];
    }
    *buf = 0;
    return buf;
}

u8 *byteToText(u8 value, u8 *buf, u8 decimal) {
    u8 tmp[10];
    u8 length = 0;
    while (value > 0) {
        tmp[length++] = 48 + (value % 10);
        value = value / 10;
    }
    u8 src = length - 1;
    decimal = decimal - 1;
    for (u8 i = 0; i < length; ++i) {
        if (src == decimal)
            *buf++ = '.';
        *buf++ = tmp[src--];
    }
    *buf = 0;
    return buf;
}

void appendVal(int value, u8 *buf, u8 decimal) {
    u8 *dest = buf;
    while (*dest != 0)
        ++dest;
    u8 tmp[12];
    u8 length = 0;
    while (value > 0) {
        tmp[length++] = 48 + (value % 10);
        value = value / 10;
    }
    while (length <= decimal)
        tmp[length++] = '0';
    u8 src = length - 1;
    decimal = decimal - 1;
    for (u8 i = 0; i < length; ++i) {
        if (src == decimal)
            *dest++ = '.';
        *dest++ = tmp[src--];
    }
    *dest = 0;
}

void showDateTime() {
    RTCDATA data;
    getRTCData(&data);
    char buf[16];
    sprintf(buf, "%02d/%02d/%02d", data.date, data.month, data.year);
    lcdPad(buf, LCD_CLEAR);
    sprintf(buf, "%02d:%02d", data.hours, data.minutes);
    lcdPad(buf, LCD_LINE2);
}

void overlayYesNo() {
    // Put Y in left box of row 2, N is right box of row 2
    // First, put the display cursor in row 2, position 1
    lcdWriteByte(0xC0, 0);
    lcdWriteByte('Y', 1);
    if (status.meterType == 'P') {
        // 12 character display
        lcdWriteByte(0xCB, 0);
    } else {
        lcdWriteByte(0xCF, 0);
    }
    lcdWriteByte('N', 1);
}

void getCalibrationName(u8 calibNumber, char *buf) {
    // There are 16 calibrations per EEPROM
    // so 0 - 15 are in EEPROM 0, 16 - 31 in EEPROM 1, etc.

    u8 eepromNum = calibNumber >> 4;
    buf[12] = 0;
    u16 idx = 32000 + ((calibNumber & 0x0F) * BYTES_PER_CALIBRATION_ID);
    readI2CEEPROM(eepromNum, idx, (u8*)buf, 12);
}

void showCalibrationName(u8 calibNumber, u8 options) {
    char buf[13];
    getCalibrationName(calibNumber, buf);
    lcdPad(buf, options);
    //	sprintf(buf, "%d  ", calibNumber);
    //	lcd(buf, LCD_HOME);
}

void batteryCheck() {
    u16 level = readAD(1);
    if (level < 600) {
        lcdPad("Battery low", LCD_CLEAR);
        pause(2000);
    }
}

u8 inputNumber(u8 start, u8 minVal, u8 maxVal, const char *prompt) {
    lcdClear();
    overlayYesNo();
    char buf[16];
    if (start < minVal)
        start = minVal;
    else if (start > maxVal)
        start = maxVal;
    while (1) {
        sprintf(buf, "%s%d", prompt, start);
        lcdPad(buf, LCD_HOME);
        if (waitForButtonDown() == BUTTON_YES) {
            waitForButtonRelease();
            return start;
        }
        ++start;
        if (start > maxVal)
            start = minVal;
    }
    return 0;
}
