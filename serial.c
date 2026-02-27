#include "utility.h"

static u8 *rxDest = 0;
static u16 rxRemaining = 0;
static u16 rxBytesRead = 0;
static u8 lastBRG = 0;

void handleRXInterrupt() {
    if (!RCIE) return;

    if (rxRemaining) {
        if (FERR) {
            u8 dummy = RCREG;
        }
        else {
            *rxDest = RCREG;
            ++rxDest;
            --rxRemaining;
            ++rxBytesRead;
            RCIE = (rxRemaining > 0);
        }
    }
}

void setBaudRate(BAUDRATE rate) {
    TXIE = 0; // No TX interrupts
    RCIE = 0; // No RX interrupts

    // Allow any current TX to complete
    while ((TXSTA & 0x02) == 0);

    // We need to allow any current inbound data to arrive
    // and be trashed, otherwise we will receive it under the
    // new baud rate

    u8 timer;
    for (timer = 0; timer < lastBRG; ++timer) {
        if (RCIF) {
            // Something has been received, start counting from zero again
            RCIF = 0;
            u8 dummy = RCREG;
            timer = 0;
        }
        pause(2);
    }
    u8 dummy = RCREG;

    // Finish any current RX operation
    while ((BAUDCON & 0x40) == 0);

    RCSTA = 0; // Reset the UART

    /*
            if (OSCCON & 0x02)
            {
                    // Running off internal oscillator
                    //OSCCON = DEFAULT_OSCCON;
                    short transform = rate * 4;
                    transform = transform / 5;
                    SPBRG = transform;
            }
            else*/
    //{

    SPBRG = rate;
    lastBRG = SPBRG;

    //}


    //	u8 clear = RCREG;	// Read any data that is in the RX buffer
    //	clear = RCREG;		// And again.
    BAUDCON = 0x00; // Non-inverted data, 8 bit clock gen
    TXSTA = 0x20; // TXEN = 1, BRGH = 0
    RCSTA = 0x90; // SPEN = 1, CREN = 1
}

void sendSerialString(const char *c) {
    while (*c != 0) {
        sendSerialByte(*c);
        ++c;
    }
}

void sendSerialBytes(const u8 *c, u16 length) {
    while (length > 0) {
        sendSerialByte(*c);
        ++c;
        --length;
    }
}

void sendSerialByte(const u8 c) {
    while ((TXSTA & 0x02) == 0);
    TXREG = c;
}

u16 receiveSerialBytes(u8 *data, u16 limit, u16 timeout) {
    RCIE = 0;

    // Check for overrun, and clear the problem if there is one
    if (RCSTA & 0x02) {
        // Overrun error, clear status
        RCSTA = 0x00;
        RCSTA = 0x90;
    }

    // Receive by interrupts
    rxDest = data;
    rxBytesRead = 0;
    rxRemaining = limit;
    RCIE = 1;

    u16 lastRxRemaining = rxRemaining;
    u16 timer = 0;
    u8 autoStage = 0;

    while (rxRemaining > 0) {
        if (lastRxRemaining == rxRemaining) {
            ++timer;
            if ((timeout > 0) && (timer >= timeout)) {
                rxRemaining = 0;
            }
        } else {
            if (timeout == 0) {
                if (autoStage == 0) {
                    autoStage = 1;
                } else {
                    timeout = timer * 2;
                    //					u8 buf[20];
                    //					sprintf(buf, "timeout=%u\r\n", timeout);
                    //					sendSerialString(buf);
                }
            }

            lastRxRemaining = rxRemaining;
            timer = 0;
        }
    };

    RCIE = 0;

    return rxBytesRead;
}
