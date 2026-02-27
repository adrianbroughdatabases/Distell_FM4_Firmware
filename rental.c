#include "utility.h"

u8 validateCode(u32 accumulator) {
    // The last thing that happens when constructing the code is that each of the
    // four bytes is XORed with a value.  The value starts as the low byte of the serial
    // number checksum, but we add the result of the XOR after each iteration.

    u8 key = (u8)(info.serialNumberChecksum);
    u32 decrypted = 0;
    u8 *src = (u8 *) & accumulator;
    u8 *dest = (u8 *) & decrypted;

    for (s8 i = 0; i <= 3; ++i) {
        u8 low = src[i];
        dest[i] = key ^ low;
        key = key + low;
    }

    // Now that we have decrypted the data (which will be gibberish if the serial checksum
    // in the meter is different to the one used to create the key), we can verify the built-in
    // checksum.
    // The checksum is held in the lowest four bits of the decrypted value, and is the sum
    // of all the other four-bit segments.
    u32 tmp = decrypted;
    key = 0;
    for (u8 i = 0; i <= 6; ++i) {
        tmp >>= 4;
        key = key + (tmp & 0x0F);
    }

    if ((key & 0x0F) != (decrypted & 0x0F)) {
        // The built-in checksum failed.

        return 0;
    }

    // The upper 12 bits of the code are the serial number checksum.
    // We need to make sure that it matches ours.
    tmp = decrypted >> 20;
    if (tmp != info.serialNumberChecksum) {
        return 0;
    }

    // The rental code is valid, so we can store the new numbers
    decrypted >>= 4;
    info.rentalDay = decrypted & 0x1F;
    decrypted >>= 5;
    info.rentalMonth = decrypted & 0x0F;
    decrypted >>= 4;
    info.rentalYear = decrypted & 0x1F;
    decrypted >>= 5;
    info.rentalMode = decrypted & 0x03;

    saveInfo();

    if (info.rentalMode == 1)
        showRentalEnd();
    return 1;
}

u32 getRentalCode() {
    u32 accumulator = 0;
    lcd("Enter code:", LCD_CLEAR);
    lcd("________", LCD_LINE2);

    u8 digits = 0;
    u8 dispChar = '0';
    u8 dispValue = 0;
    lcdWriteByte(0xC0, 0); // Select line 2
    while (digits < 8) {
        lcdChar(dispChar);
        lcdWriteByte(0x10, 0); // Cursor left

        if (waitForButtonDown() == BUTTON_NO) {
            // No was pressed - increment current digit
            ++dispValue;
            if (dispChar == '9')
                dispChar = 'A';
            else if (dispChar == 'F') {
                dispChar = '0';
                dispValue = 0;
            } else
                ++dispChar;
        } else {
            // Yes was pressed - move on to next digit
            accumulator <<= 4;
            accumulator += dispValue;
            ++digits;
            lcdChar(dispChar);
            dispChar = '0';
            dispValue = 0;
            pause(200);
        }
    }
    return accumulator;
}

u8 acquireAndTestRentalCode() {
    u32 accumulator = getRentalCode();

    if (validateCode(accumulator))
        return 1;

    lcdPad("Invalid code", LCD_CLEAR);
    pause(2000);
    return 0;
}

void rentalCheck() {
    while (1) {
        // Check if rental mode is enabled.
        if (info.rentalMode != 1) return;

        showRentalEnd();

        RTCDATA current;
        getRTCData(&current);
        u32 rentaldate = (u32)((info.rentalYear << 9) + (info.rentalMonth << 5) + info.rentalDay);
        u32 now = (u32)((current.year << 9) + (current.month << 5) + current.date);

        if (rentaldate >= now) return;

        // The rental has expired.  We need to enter the code.
        waitForButtonRelease();
        waitForButtonPress();

        // We keep acquiring and testing until we get something valid.
        while (!acquireAndTestRentalCode());

        // Even if we get something valid, they could have used
        // an old code so we check everything again.
    }
}

void showRentalEnd() {
    char buf[16];
    sprintf(buf, "Rental end:");
    lcdPad(buf, LCD_CLEAR);
    sprintf(buf, "%02d/%02d/%02d", info.rentalDay, info.rentalMonth, info.rentalYear);
    lcdPad(buf, LCD_LINE2);
    pause(2000);
}