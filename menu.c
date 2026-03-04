#include "utility.h"

u8 calibrationSlots[64];

u8 calmap(u8 index) {
    // This function maps the array index as follows:
    //   0  to 47:  identity mapping
    //   48 to 63:  return 0, because those slots are invalid
    //   64 to 79:  return 48 to 63

    if ((index >= 48) && (index < 63)) return 0;
    if (index >= 64)
        index -= 16;

    return calibrationSlots[index];
}

void loadSlotMap() {
    // Load the calibration map
    readI2CEEPROM(5, 32000, calibrationSlots, 64);
}

u8 getNextFilledSlot(u8 current, u8 minCal, u8 maxCal) {
    for (u8 i = current + 1; i < maxCal; ++i)
        if (calmap(i) == 1) return i;
    for (u8 i = minCal; i <= current; ++i)
        if (calmap(i) == 1) return i;
    return 255; // Indicates nothing found
}

void selectCalibration(u8 minCal, u8 maxCal) {
    // Calibration numbers >= 64 are mapped down to the 48 .. 63 range so that
    // they can be directly indexed to the calibration map
    u8 current = status.curProduct;

    if ((current < minCal) || (current > maxCal))
        current = minCal;

    if (calmap(current) != 1)
        current = getNextFilledSlot(current, minCal, maxCal);
    if (current == 255) {
        lcdPad("No setting", LCD_CLEAR);
        waitForButtonPress();
        return;
    }

    while (1) {
        showCalibrationName(current, LCD_CLEAR);
        overlayYesNo();
        if (waitForButtonPress() == BUTTON_YES) {
            status.curProduct = current;
            saveStatus();
            return;
        } else {
            current = getNextFilledSlot(current, minCal, maxCal);
            //			if (status.curProduct > maxCal)
            //				status.curProduct = minCal;
        }
    }
}

void selectSampleCount() {
    u8 maxReadings = 8;

    if (status.meterType == 'T')
        maxReadings = 16;

    status.numReadings = inputNumber(status.numReadings, 1, maxReadings, "Samples = ");
    saveStatus();
}

void selectDemoMode() {
    lcd("Y=Demo mode", LCD_CLEAR);
    lcd("N=Normal", LCD_LINE2);
    if (waitForButtonPress() == BUTTON_YES)
        status.demoMode = 1;
    else
        status.demoMode = 0;
    saveStatus();
}

void sendSampleData(u16 samplenumber) {
    u8 eepromNum = 3;
    u16 index = samplenumber;
    if (samplenumber >= 500) {
        eepromNum = 5;
        index = samplenumber - 500;
    }
    u8 id = 'E';
    if (status.meterType == 'T')
        id = 'D';


    DATARECORD rec;
    u16 offset = BYTESPERRECORD * index;

    if (!readI2CEEPROM(eepromNum, offset, (u8 *) & rec, sizeof (DATARECORD))) {
        lcd("I2C READ FAIL", LCD_CLEAR);
        pause(5000);
    }
    sprintf(bigBufStr, "%c,", id);

    char tmp[30];
    for (u8 i = 0; i < 16; ++i) {
        sprintf(tmp, "%u,", rec.samples[i]);
        strcat(bigBufStr, tmp);
    }
    sprintf(tmp, "%u,", rec.average);
    strcat(bigBufStr, tmp);

    // Fix for wrongly stored BDC month numbers.
    if (rec.data[5] > 12)
        rec.data[5] -= 6;

    for (u8 i = 0; i < 7; ++i) {
        sprintf(tmp, "%u,", rec.data[i]);
        strcat(bigBufStr, tmp);
    }
    sprintf(tmp, "%u,", samplenumber);
    strcat(bigBufStr, tmp);

    // Get the calibration name based on the slot number
    u8 caleeprom = rec.items.slotNumber >> 4;
    char calname[13];
    if (caleeprom <= 4) {
        calname[12] = 0;
        u16 idx = 32000 + ((rec.items.slotNumber & 0x0F) * BYTES_PER_CALIBRATION_ID);
        readI2CEEPROM(caleeprom, idx, (u8*) calname, 12);
    } else {
        strcpy(calname, "  UNKNOWN   ");
    }
    sprintf(tmp, "%s,%s,B\x0D\x0A", calname, info.serialNumber);
    strcat(bigBufStr, tmp);
    sendSerialString(bigBufStr);

}

void downloadSamples() {
    if (status.stored == 0) {
        lcdPad("No data", LCD_CLEAR);
        pause(2000);
        return;
    }

    lcdPad("Standard D/L", LCD_CLEAR);
    lcdPad("Ready ?", LCD_LINE2);
    overlayYesNo();

    if (waitForButtonPress() == BUTTON_NO) return;
    lcdPad("Downloading", LCD_CLEAR);

    setBaudRate(BAUD9600);
    for (u16 i = 0; i < status.stored; ++i)
        sendSampleData(i);
    sendSerialString("#\x0D");

    lcdPad("Erase ?", LCD_CLEAR);
    overlayYesNo();
    if (waitForButtonPress() == BUTTON_NO) return;
    lcdPad("Sure ?", LCD_CLEAR);
    overlayYesNo();
    if (waitForButtonPress() == BUTTON_NO) return;

    status.stored = 0;
    saveStatus();
}

void uploadFailed(u8 code) {
    lcdPad("Upload fail", LCD_CLEAR);
    char buf[16];
    sprintf(buf, "code = %d", code);
    lcdPad(buf, LCD_LINE2);
    pause(4000);
}

void setCalibMap(u8 eepromNum, u8 dataset) {
    if (eepromNum > 2)
        eepromNum = 3;
    u8 index = (u8) ((eepromNum << 4) + (dataset & 0x0F));
    u8 buf = 1;
    // Write a 1 in the calibration map relating to this particular calibration location
    writeI2CEEPROM(5, 32000 + index, &buf, 1);
    calibrationSlots[index] = 1;
}

void addCalibration() {
    while (1) {
        setBaudRate(BAUD2400);
        lcdPad("Waiting data", LCD_CLEAR);
        u8 got = (u8) receiveSerialBytes(bigBuf, 18, 0);
        if (got != 18) {
            uploadFailed(1);
            return;
        }

        if (strncmp(bigBufStr, "namin", 5) != 0) {
            uploadFailed(2);
            return;
        }

        // We have the calibration name in bigBuf+5 now.
        char tmp[20];
        bigBuf[17] = 0;
        sprintf(tmp, "RX:%s\x0D", bigBuf + 5);
        sendSerialString(tmp);

        setBaudRate(BAUD9600);

        if (receiveSerialBytes((u8 *) tmp, 5, 0) != 5) {
            uploadFailed(20);
            return;
        }
        if ((tmp[0] != 'o') || (tmp[1] != 'k')) {
            uploadFailed(21);
            return;
        }
        u8 eepromNum = tmp[2];
        u8 dataset = tmp[3];
        u8 dfflag = tmp[4];

        lcdPad(bigBufStr + 5, LCD_CLEAR);

        u16 eepromaddr = 32000 + (dataset * BYTES_PER_CALIBRATION_ID);
        writeI2CEEPROM(eepromNum, eepromaddr, bigBuf + 5, 12);

        u16 nextSlot = 0;
        u8 progress = 255;

        while (1) {
            sprintf(tmp, "OK %d\x0D", nextSlot);
            sendSerialString(tmp);

            if (receiveSerialBytes(bigBuf, 45, 0) != 45) {
                uploadFailed(4);
                return;
            }
            if (bigBuf[0] != '*') {
                uploadFailed(5);
                return;
            }

            // Endian-swap
            for (u8 i = 1; i <= 44; i += 2) {
                u8 swap = bigBuf[i + 1];
                bigBuf[i + 1] = bigBuf[i];
                bigBuf[i] = swap;
            }

            u16 checksum = 0;
            for (u8 i = 3; i <= 42; ++i) {
                checksum += bigBuf[i];
            }

            UPLOADRECORD *ul = (UPLOADRECORD *) bigBuf;
            if (checksum != ul->checksum) {
                uploadFailed(6);
                return;
            }

            eepromaddr = (dataset * 2000) + (ul->location * 2);
            writeI2CEEPROMPaged(eepromNum, eepromaddr, (u8 *) (ul->data), 40);
            //			for (u8 i=0; i<20; ++i)
            //			{
            //				if (!writeI2CEEPROM(eeprom, eepromaddr, &ul->data[i], 2))
            //				{
            //					uploadFailed(99);
            //					return;
            //				}
            //				eepromaddr += 2;
            //			}

            u8 completed = (u8) (ul->location / 10);
            if (completed != progress) {
                progress = completed;
                if (dataset <= 64)
                    sprintf(tmp, "%d%% E%d S%d", progress, eepromNum, dataset);
                else
                    sprintf(tmp, "%d%%", progress);
                lcdPad(tmp, LCD_LINE2);
            }

            nextSlot = ul->location + 19;
            if (nextSlot >= 999) {
                setCalibMap(eepromNum, dataset);
                sendSerialString("ta\x0D");
                break;
            }
        }

        if (!dfflag)
            break;
    }
}

void setTime() {
    RTCDATA data;
    getRTCData(&data);
    data.minutes = inputNumber(data.minutes, 0, 59, "Mins = ");
    data.hours = inputNumber(data.hours, 0, 23, "Hours = ");
    data.date = inputNumber(data.date, 1, 31, "Day = ");
    data.month = inputNumber(data.month, 1, 12, "Month = ");
    data.year = inputNumber(data.year, 25, 45, "Year = 20");
    setRTCData(&data);
}

void SetBacklight() {
    lcdPad("Backlight", LCD_CLEAR);
    u8 loopBl = 1;
    char buf[12];
    u8 timeSecs = info.backlightSetting;
    do {
        switch (timeSecs) {
            case 0:
            {
                lcdPad("Off", LCD_LINE2);
                break;
            }
                
            case 0xFF:
            {
                lcdPad("On", LCD_LINE2);
                break;
            };
            
            default:
            {
                sprintf(buf, "%d sec", timeSecs);
                lcdPad(buf, LCD_LINE2);
                break;
            };
        }
        overlayYesNo();
        
        if (waitForButtonPress() == BUTTON_YES)
        {
            loopBl = 0;
            info.backlightSetting = timeSecs;
            saveInfo();
            setBacklightTicks();
            
            timedBacklight();
        }
        else 
        {
            if (timeSecs == 0xFF)   // Always on
            {
                // Cycle round to 3 seconds.
                // If this is set to zero then we include the option to turn the
                // backlight off completely.
                timeSecs = 3;
            }
            else if (timeSecs >= 60)
            {
                timeSecs = 0xFF;
            }
            else if (timeSecs >= 30)
            {
                timeSecs += 5;
            }
            else
            {
                timeSecs += 3;
            }
        }
    } while (loopBl);
}

void doChoicesMenu() {
    backlight(BL_ON);
    lcdPad("Choices Menu", LCD_CLEAR);
    waitForButtonRelease();

    lcdPad("Product", LCD_LINE2);
    overlayYesNo();
    if (waitForButtonPress() == BUTTON_YES) {
        selectCalibration(0, 47);
        return;
    }

    lcdPad("Research", LCD_LINE2);
    overlayYesNo();
    if (waitForButtonPress() == BUTTON_YES) {
        selectCalibration(64, 79);
        return;
    }

    lcdPad("Samples", LCD_LINE2);
    overlayYesNo();
    if (waitForButtonPress() == BUTTON_YES) {
        selectSampleCount();
        return;
    }

    lcdPad("Download", LCD_LINE2);
    overlayYesNo();
    if (waitForButtonPress() == BUTTON_YES) {
        downloadSamples();
        return;
    }

    if (status.meterType == 'P')
        lcdPad("Add Prdt", LCD_LINE2);
    else
        lcdPad("Add Product", LCD_LINE2);
    overlayYesNo();
    if (waitForButtonPress() == BUTTON_YES) {
        addCalibration();
        return;
    }

    if (info.rentalMode == 1) {
        if (status.meterType == 'P')
            lcdPad("Rntl Key", LCD_LINE2);
        else
            lcdPad("Rental Key", LCD_LINE2);
        overlayYesNo();
        if (waitForButtonPress() == BUTTON_YES) {
            acquireAndTestRentalCode();
            return;
        }
    } else {
        lcdPad("Set Time", LCD_LINE2);
        overlayYesNo();
        if (waitForButtonPress() == BUTTON_YES) {
            setTime();
            return;
        }
    }

    lcdPad("Send Ref", LCD_LINE2);
    overlayYesNo();
    if (waitForButtonPress() == BUTTON_YES) {
        lcdPad("Sending...", LCD_CLEAR);
        dumpCalibration(64);
        return;
    }

    if (status.meterType == 'P')
        lcdPad("Bklight", LCD_LINE2);
    else
        lcdPad("Backlight", LCD_LINE2);
    overlayYesNo();
    if (waitForButtonPress() == BUTTON_YES) {
        SetBacklight();
        return;
    }

    lcdPad("Clear", LCD_LINE2);
    overlayYesNo();
    if (waitForButtonPress() == BUTTON_YES) {
        lcdPad("Sure ?", LCD_CLEAR);
        overlayYesNo();
        if (waitForButtonPress() == BUTTON_NO) return;
        status.stored = 0;
        saveStatus();
        return;
    }

    // A non-zero calibration mode means that we're still in the production phase
    // so we can do certain admin tasks.
    if (status.calibrationMode > 0) {

        lcdPad("Cal.Mode", LCD_LINE2);
        overlayYesNo();
        if (waitForButtonPress() == BUTTON_YES) {
            status.calibrationMode = 2;
            saveStatus();
            enterCalibrationMode();
            return;
        }

        lcdPad("Set Type", LCD_LINE2);
        overlayYesNo();
        if (waitForButtonPress() == BUTTON_YES) {
            selectMeterType();
            saveStatus();
            return;
        }

        lcdPad("Demo", LCD_LINE2);
        overlayYesNo();
        if (waitForButtonPress() == BUTTON_YES) {
            selectDemoMode();
            saveStatus();
            return;
        }

        if (status.meterType == 'T') {
            lcdPad("Scale", LCD_LINE2);
            overlayYesNo();
            if (waitForButtonPress() == BUTTON_YES) {
                setTorryScale();
                return;
            }
        } else {
            lcdPad("Checkpad", LCD_LINE2);
            overlayYesNo();
            if (waitForButtonPress() == BUTTON_YES) {
                checkpadAutomation();
                return;
            }
        }

        lcdPad("Finalise", LCD_LINE2);
        overlayYesNo();
        if (waitForButtonPress() == BUTTON_YES) {
            status.calibrationMode = 0;
            saveStatus();
            return;
        }
    }

}
