#include "utility.h"

static u16 samples[16]; // Measurements go here
static u8 sampleNumber;

void showSampleNumber() {
    char buf[16];
    sprintf(buf, "Sample %d", sampleNumber);
    lcdPad(buf, LCD_CLEAR);
}

u16 getCalibrationValue(u16 memoryLocation, u8 product) {
    u8 eepromNum = product >> 4;
    u16 address = (memoryLocation * 2) + ((product & 0x0F) * 2000);
    u16 value;
    readI2CEEPROM(eepromNum, address, (u8 *) & value, 2);
    return value;
}

u16 getAvgPulse() {
    u8 counts = 0;
    u32 total = 0;
    for (u8 i = 0; i < 32; ++i) {
        u16 pulse = readPulse();

        //		u8 buf[10];
        //		sprintf(buf, "p%d  ", pulse);
        //		lcd(buf, LCD_LINE2);

        if (pulse == 0) {
            i += 4;
        } else if (pulse > 12) {
            total += pulse;
            ++counts;
        }
    }

    if (counts == 0)
        return 0;
    
    total *= info.torryScale;
    total /= counts;
    if (total > 999)
        total = 999;
    return (u16)total;
}

void measureFreshness() {
    u8 gotData = 0;
    u16 cval = 0;

    lcdPad("Measuring...", LCD_CLEAR);

    while (getButtonState() == BUTTON_YES) {
        if (readAD(0) < 250) {
            lcdPad("No contact", LCD_LINE2);
        } else {
            // Got contact
            u16 total = 0;
            for (u8 i = 0; i < 8; ++i) {
                u16 pulse = getAvgPulse();
                total += pulse;
                pause(20);
            }
            if (getButtonState() == BUTTON_YES) {
                u16 avg = total / 8;
                cval = getCalibrationValue(avg, status.curProduct);
                u8 decimal = cval % 10;
                u8 major = (u8)(cval / 10);
                char buf[16];

                if (status.numReadings == 16) {
                    // Special case - don't show the data
                    lcdPad("Data OK", LCD_LINE2);
                } else {
                    //					sprintf(buf, "S%d= %d.%0d [%d]", sampleNumber, major, decimal, avg);
                    sprintf(buf, "S%d= %d.%0d", sampleNumber, major, decimal);
                    lcdPad(buf, LCD_LINE2);
                }
                gotData = 1;
            }
        }
        pause(50);
    }
    if (gotData) {
        // cval is the calibration value we have measured
        samples[sampleNumber - 1] = cval;
        ++sampleNumber;
    }
}

u16 getAvgAD() {
    u32 total = 0;
    for (u8 i = 0; i < 32; ++i) {
        u16 raw = readAD(0);
        total += raw;
    }
    total /= 32;
    if (total > 999)
        total = 999;
    return (u16)total;
}

#define QUEUELENGTH  4

void measureFat() {
    u16 queue[QUEUELENGTH];
    u8 qlen = 0;
    u8 gotData = 0;
    u16 cval = 0;

    u8 pct = '\0';
    if (status.curProduct < 64)
        pct = '%';

    lcdPad("Measuring...", LCD_LINE2);

    while (getButtonState() == BUTTON_YES) {
        u16 adval = getAvgAD();

        if (getButtonState() == BUTTON_YES) {
            for (u8 i = 0; i < QUEUELENGTH - 1; ++i)
                queue[i] = queue[i + 1];
            queue[QUEUELENGTH - 1] = adval;

            if (qlen < QUEUELENGTH) {
                ++qlen;
            } else {
                // We now have enough data to show numbers
                cval = getCalibrationValue(adval, status.curProduct);
                u8 decimal = cval % 10;
                u8 major = (u8)(cval / 10);
                char buf[16];

                if (cval == 0) {
                    // Special case
                    lcdPad("Out Of Range", LCD_LINE2);
                } else {
                    //					sprintf(buf, "S%d= %d.%0d [%d] %c", sampleNumber, major, decimal, adval, pct);
                    sprintf(buf, "S%d= %d.%0d%c", sampleNumber, major, decimal, pct);
                    lcdPad(buf, LCD_LINE2);
                    gotData = 1;
                }
            }
        }
        pause(250);
    }
    if (gotData) {
        // cval is the calibration value we have measured
        samples[sampleNumber - 1] = cval;
        ++sampleNumber;
    }
}

void storeAndSend() {
    DATARECORD rec;
    u32 avg = 0;
    for (u8 i = 0; i < status.numReadings; ++i) {
        avg += (samples[i] << 1) + 1;
    }
    rec.average = (u16)((avg / status.numReadings) >> 1);
    for (u8 i = 0; i < 16; ++i) {
        if (i < status.numReadings)
            rec.samples[i] = samples[i];
        else
            rec.samples[i] = 0;
    }
    rec.items.sampleCount = status.numReadings;
    rec.items.slotNumber = status.curProduct;
    RTCDATA now;
    getRTCData(&now);
    rec.items.mins = now.minutes;
    rec.items.hours = now.hours;
    rec.items.date = now.date;
    rec.items.month = now.month;
    rec.items.year = now.year;

    // We have our data record, so we can store it in EEPROM
    u16 eepromLoc = status.stored % 1000;

    if (status.stored > 1000) {
        lcdPad("MEMORY FULL", LCD_CLEAR);
        if (status.stored > 2000)
            status.stored -= 1000;
        pause(2000);
    }

    // Default to EEPROM 3 for samples 0 to 499
    u8 eepromNum = 3;
    u16 address = eepromLoc * BYTESPERRECORD;

    if (eepromLoc >= 500) {
        // samples 500 to 999 go into EEPROM 5
        eepromNum = 5;
        address = (eepromLoc - 500) * BYTESPERRECORD;
    }

    if (writeI2CEEPROM(eepromNum, address, (u8 *) & rec, sizeof (rec))) {
        // Data is stored; send it on the serial port.
        sendSampleData(eepromLoc);
        ++status.stored;
        saveStatus();
    } else {
        lcdPad("Internal", LCD_CLEAR);
        lcdPad("error", LCD_LINE2);
        pause(5000);
    }

    // Show average value of readings
    u8 decimal = rec.average % 10;
    u8 major = (u8)(rec.average / 10);
    u8 pct = '\0';
    if ((status.meterType != 'T') && (rec.items.slotNumber < 64))
        pct = '%';
    char buf[16];
    sprintf(buf, "Avg = %d.%0d%c", major, decimal, pct);
    lcdPad(buf, LCD_CLEAR);
}

u8 startMeasuring() {
    pause(250);
    //	waitForButtonRelease();
    sampleNumber = 1;
    backlight(BL_ON);
    while (sampleNumber <= status.numReadings) {
        showSampleNumber();

        u8 bs;
        do {
            bs = getButtonState();
        } while (bs == BUTTON_NONE);

        if (bs == BUTTON_YES) {
            if (status.meterType == 'T')
                measureFreshness();
            else
                measureFat();
        } else {
            waitForButtonRelease();
            return 0;
        }
    }

    // If we get here then we have completed a set of readings.
    storeAndSend();

    lcdPad("More?", LCD_LINE2);
    overlayYesNo();
    return (waitForButtonPress() == BUTTON_YES);

}

void adMode() {
    lcdPad("A/D mode", LCD_CLEAR);
    showCalibrationName(status.curProduct, LCD_LINE2);
    pause(2000);
    lcdClear();
    while (getButtonState() != BUTTON_NO) {
        char buf[40];

        u16 battery = readAD(1);
        sprintf(buf, "Batt = %d  ", battery);
        lcd(buf, LCD_HOME);
        u16 ad;
        if (status.meterType == 'T') {
            // Torrymeter
            ad = getAvgPulse();
        } else {
            ad = getAvgAD();
        }
        u16 cval = getCalibrationValue(ad, status.curProduct);
        sprintf(buf, "AD=%d V=%d        ", ad, cval);
        lcd(buf, LCD_LINE2);
        pause(150);
    }
}

void checkpadAutomation() {
    status.stored = 0;
    u16 tick = 0;
    u16 total = 0;

    lcd("Checkpad", LCD_CLEAR);
    while (1) {
        char buf[12];
        sprintf(buf, "Sample %d", total);
        lcd(buf, LCD_LINE2);
        RTCDATA current;
        getRTCData(&current);
        u16 latest = (current.seconds) + (current.minutes * 60) + (current.hours * 3600);
        if ((latest - tick) >= 10) {
            tick = latest;
            for (u8 i = 0; i < status.numReadings; ++i)
                samples[i] = getCalibrationValue(getAvgAD(), 64);
            storeAndSend();
            ++total;
            if (total > 180) {
                // One reading every ten seconds = 6 per minute.
                // 30 minutes = 180 readings.
                break;
            }
        }
        pause(500);
        if (getButtonState() != BUTTON_NONE)
            break;
    }

    saveStatus();
}

void checkpadStartupTest()
{
    lcdPad("Checkpad", LCD_CLEAR);
    waitForButtonRelease();
    waitForButtonPress();
    /*
    u8 buf[20];
    u8 ctr = 0;
    u16 total = 0;
    while (1)
    {
        total += getCalibrationValue(getAvgAD(), 64);
        ++ctr;
        if (ctr == 6)
        {
            u16 r1val = total / 6;
            u8 decimal = r1val % 10;
            u8 major = r1val / 10;
            sprintf(buf, "%d.%d", major, decimal);
            lcdPad(buf, LCD_LINE2);
            overlayYesNo();
            ctr = 0;
            total = 0;
        }
       pause(50);
       if (getButtonState() != BUTTON_NONE)
       {
           waitForButtonRelease();
           return;
       }
    }
    */
}