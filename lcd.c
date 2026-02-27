#include "utility.h"

void lcdWriteByte(u8 value, u8 data) {
    u8 nib1 = value >> 4;
    u8 nib2 = value & 0x0F;
    if (data) {
        nib1 = nib1 | 0x80; // Set register select to 'data'
        nib2 = nib2 | 0x80;
    }
    if (RB4) {
        nib1 = nib1 | 0x10;
        nib2 = nib2 | 0x10;
    }
    PORTB = nib1;
    RB5 = 1;
    RB5 = 0;
    PORTB = nib2;
    RB5 = 1;
    RB5 = 0;
    __delay_us(200);
    if (!data) {
        __delay_ms(5);
    }
}

void lcdChar(u8 value) {
    lcdWriteByte(value, 1);
}

void lcd(const char *text, u8 options) {
    if (options & LCD_CLEAR) {
        lcdClear();
    }
    if (options & LCD_HOME) {
        lcdHome();
    }
    if (options & LCD_LINE2) {
        lcdWriteByte(0xC0, 0); // Select line 2
    }
    while (*text) {
        lcdWriteByte(*text++, 1);
    }
}

void lcdClear() {
    lcdWriteByte(0x01, 0); // Clear (implicit home) display
}

void lcdHome() {
    lcdWriteByte(0x02, 0); // Home display
}

void lcdInit() {
    // 4-bit data goes in port B, bits 0 to 3
    // To write, set B6 = 0
    // To send data, set B7 = 1.  To send control, set B7 = 0
    // Set B5 = 1 when ready
    lcdWriteByte(0x28, 0); // Set 4 bit, 2 line mode
    lcdWriteByte(0x28, 0); // Set 4 bit, 2 line mode
    lcdWriteByte(0x06, 0); // Set entry mode
    lcdWriteByte(0x0C, 0); // Enable display
    pause(200);
    lcdClear();
}

void lcdPad(const char *text, const u8 options) {
    char actual[17]; // Up to 16 characters on a line
    u8 len = (u8)strlen(text);
    u8 screenWidth = 16;
    if (status.meterType == 'P')
        screenWidth = 12;

    if (len >= screenWidth) {
        lcd(text, options);
        return;
    }

    u8 gap = (screenWidth - len) / 2;
    u8 idx = 0;
    u8 i = 0;
    for (; i < screenWidth; ++i) {
        if (gap > 0) {
            actual[i] = ' ';
            --gap;
        } else if (len > 0) {
            actual[i] = text[idx];
            ++idx;
            --len;
        } else
            actual[i] = ' ';
    }
    actual[i] = '\0';
    lcd(actual, options);
}