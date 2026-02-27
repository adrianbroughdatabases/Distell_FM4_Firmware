#include "utility.h"

static u8 smState = 0;
static u8 smCmd = 0;

void enableServerMode() {
    setBaudRate(BAUD9600);

    smState = 0;
    serverMode = SM_MONITOR;

    // Check for overrun first, and clear the problem if there is one
    //	if (RCSTA & 0x02)
    //	{
    //		// Overrun error, clear status
    //		RCSTA = 0x00;
    //		RCSTA = 0x90;
    //	}

    RCIE = 1; // Enable interrupts on RX serial data
}

void disableServerMode() {
    serverMode = SM_OFF;
    RCIE = 0;
    smState = 0;
}

void handleServerModeRX() {
    if (!RCIE) return;
    if (serverMode != SM_MONITOR) return;

    // Server mode is entered by receiving '=' then ']' in succession.
    if (RCREG == '=') {
        smState = 1;
        return;
    }
    if (smState == 1) {
        if (RCREG == ']') {
            smState = 2;
        } else {
            smState = 0;
        }
        return;
    }
    if (smState == 2) {
        RCIE = 0; // No more interrupts on RX serial data
        smState = 0;
        smCmd = RCREG;
        serverMode = SM_LAUNCH;
    }
}

void runServerMode() {
    // We are now in server mode.
    switch (smCmd) {
        case '?':
        {
            // Return serial number
            sendSerialBytes((u8*)info.serialNumber, 8);
            sendSerialByte(13);
            break;
        }

        case 'D':
        {
            // Set the date
            u8 buf[5];
            if (receiveSerialBytes(buf, 5, 30000) == 5) {
                RTCDATA cdata;
                cdata.minutes = buf[0];
                cdata.hours = buf[1];
                cdata.date = buf[2];
                cdata.month = buf[3];
                cdata.year = buf[4];

                setRTCData(&cdata);

                sendSerialString("k9\x0D");
                showDateTime();
                pause(1000);
            }
            break;
        }

        case 'S':
        {
            // Set serial number
            if (receiveSerialBytes((u8*)info.serialNumber, 8, 30000) == 8) {
                info.serialNumberChecksum = 0;
                for (u8 i = 0; i <= 7; ++i) {
                    info.serialNumberChecksum += (info.serialNumber[i] - 32);
                }
                info.serialNumberChecksum <<= 3;
                info.serialNumberChecksum |= info.serialNumber[7];
                info.serialNumber[8] = '\0';
                saveInfo();
                sendSerialString("ka\x0D");
                lcdPad(info.serialNumber, LCD_CLEAR);
                pause(1000);
            }
            break;
        }

        case 'R':
        {
            // Rental mode command
            u8 buf[4];
            if (receiveSerialBytes(buf, 4, 30000) == 4) {
                info.rentalMode = buf[0];
                info.rentalYear = buf[1];
                info.rentalMonth = buf[2];
                info.rentalDay = buf[3];
                saveInfo();
                sendSerialString("kr\x0D");
                showRentalEnd();
            }
            break;
        }

        case 'K':
        {
            // Reset meter state.
            // We can do this by invalidating the checksum of the status data
            status.checksum = 0x0000;
            saveStatus();
            // Clear the calibration map
            u8 buf = 0;
            for (u8 i = 0; i < 64; ++i)
                writeI2CEEPROM(5, 32000 + i, &buf, 1);
            sendSerialString("kk\x0D");
            lcd("Restart reqd", LCD_CLEAR);
            pause(1000);
            break;
        }

        case 'Z':
        {
            // Set Torry scale
            sendSerialString("kz\x0D");
            setTorryScale();
            break;
        }

        case 'P':
        {
            // Ping
            sendSerialString("kp\x0D");
            break;
        }

        case 'U':
        {
            // Upload calibration
            sendSerialString("ku\x0D");
            addCalibration();
            break;
        }

        case 'X':
        {
            // Dump research-1 then go to full calibration mode
            sendSerialString("kx\x0D");
            dumpCalibration(64);
            if (status.calibrationMode >= 2)
                status.calibrationMode = 3;
            break;
        }

        case 'Y':
        {
            // Exit calibration mode
            if (status.calibrationMode > 1)
                status.calibrationMode = 1;
            sendSerialString("ky\x0D");
            break;
        }

        case 'M':
        {
            // Full calibration dump
            sendSerialString("km\x0D");
            u8 prev = 255;
            u8 toSend = 0;
            for (u8 i = 0; i <= 79; ++i) {
                if (calmap(i) == 1) {
                    char buf[12];
                    sprintf(buf, "Sending %d", i);
                    lcd(buf, LCD_CLEAR);
                    dumpCalibration(i);
                }
            }
            sendSerialString("#\x0D");
            break;
        }

        case 'E':
        {
            // Set demo mode
            u8 buf[1];
            if (receiveSerialBytes(buf, 1, 30000) == 1) {
                status.demoMode = (buf[0] != 0);
                saveStatus();
                sendSerialString("ke\x0D");
            }
            break;
        }

    }

    smCmd = 0;
    serverMode = SM_OFF;
}