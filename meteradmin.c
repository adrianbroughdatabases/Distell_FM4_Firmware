#include "utility.h"

void selectMeterType() {
    waitForButtonRelease();
    lcd("Yes = TORRY", LCD_CLEAR);
    lcd("No  = FAT", LCD_LINE2);
    if (waitForButtonPress() == BUTTON_YES) {
        status.meterType = 'T';
        status.fishFatmeter = 0;
        setTorryScale();
        return;
    }

    lcd("Yes=Fish", LCD_CLEAR);
    lcd("No= Meat", LCD_LINE2);
    if (waitForButtonPress() == BUTTON_YES) {
        status.fishFatmeter = 1;
    } else {
        status.fishFatmeter = 0;
    }

    lcd("Yes=692/1092", LCD_CLEAR);
    lcd("No= 992", LCD_LINE2);
    if (waitForButtonPress() == BUTTON_YES) {
        status.meterType = 'P';
    } else {
        status.meterType = 'C';
    }
}

void setTorryScale() {
    info.torryScale = inputNumber(info.torryScale, 1, 4, "Scale = ");
    saveInfo();
}

void loadInfo() {
    // Get meter info
    readI2CEEPROM(5, 32640, (u8 *) & info, sizeof (info));
    info.serialNumber[8] = 0;

    // Since v3.0.6 signature the info struct has a signature so that we know
    // what data is stored there.  This is useful in firmware updates, since
    // a new firmware with old data can lead to uninitialised struct members
    // being interpreted as valid.
    
    u16 infoSig = info.infoSignature & 0xFFF0;      // Only check the top 12 bits
    if (infoSig != 0x9F10)
    {
        // v3.0.6 or later
        info.backlightSetting = 6;  // Set the backlight timer to 6 seconds by default
    }
}

void saveInfo() {
    // Store the info structure
    info.infoSignature = 0x9F10;
    if (!writeI2CEEPROM(5, 32640, (u8 *) & info, sizeof (info))) {
        lcd("WRITE FAILED", LCD_CLEAR);
        pause(4000);
    }
}
