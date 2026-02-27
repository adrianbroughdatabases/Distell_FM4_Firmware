#include "utility.h"

void i2c_wait_for_SSPIF() {
    while ((PIR1 & 0x08) == 0); // Wait for SSPIF
    PIR1 &= 0xF7; // Clear SSPIF
}

void i2cInit(void) {
    SSPCON1 = 0x28; // Set I2C mode
    SSPSTAT = 0;
    SSPCON2 = 0;
    SSPCON3 = 0;
    SSPADD = 12; // Approx 400KHz
}

void i2c_ack() {
    SSPCON2 |= 0x10; // Start ACK
    i2c_wait_for_SSPIF();
}

void i2c_nack() {
    SSPCON2 |= 0x20; // Set NACK ready
    SSPCON2 |= 0x30; // Start NACK
    i2c_wait_for_SSPIF();
}

void i2c_start() {
    SSPCON2 = 0x01;
    i2c_wait_for_SSPIF();
}

void i2c_restart() {
    SSPCON2 = 0x02;
    i2c_wait_for_SSPIF();
}

void i2c_stop() {
    SSPCON2 = 0x04;
    i2c_wait_for_SSPIF();
}

void i2c_put(u8 x) {
    SSPBUF = x;
    i2c_wait_for_SSPIF();
}

u8 i2c_get(void) {
    while (SSPSTAT & 0x04); // Wait for any existing transmission to end
    SSPCON2 = 0x08; // Start receive mode
    i2c_wait_for_SSPIF();
    return SSPBUF;
}

u8 readI2CEEPROM(u8 eepromNumber, u16 address, u8 *buffer, u16 length) {
    // First we send a (started) control byte (asserting write), then high address byte
    // then low address byte.
    // Then we send a (started) control byte (asserting read), then read as many bytes
    // as we want, with an acknowledge on all but the last.  Then we send a stop.
    u8 control = (eepromNumber * 2) | 0xA0;

    // Write the address to the device
    i2c_start();
    i2c_put(control);

    u8 retry = 50;
    while (1) {
        i2c_start();
        i2c_put(control);

        if (SSPCON2 & 0x40) {
            // No Ack
            --retry;
            if (retry == 0)
                return 0;
            pause(1);
        } else
            break;
    }

    u8 hiAddr = address >> 8;
    u8 loAddr = address & 0xFF;
    i2c_put(hiAddr);
    i2c_put(loAddr);

    // Now read right up to the penultimate byte
    control = control + 1;
    i2c_start();
    i2c_put(control);
    unsigned short i = 0;
    for (; i < length - 1; ++i) {
        buffer[i] = i2c_get();
        i2c_ack();
    }
    buffer[i] = i2c_get();
    i2c_nack();

    i2c_stop();
    return 1;
}

u8 writeI2CEEPROMPaged(u8 eepromNumber, u16 address, u8 *buffer, u8 length) {
    while (1) {
        u8 spaceInPage = 64 - (address & 0x3F);
        u8 toWrite = length;
        if (toWrite > spaceInPage)
            toWrite = spaceInPage;
        u8 rv = writeI2CEEPROM(eepromNumber, address, buffer, toWrite);
        if (!rv)
            return 0;

        length -= toWrite;
        if (length == 0)
            return 1;

        buffer += toWrite;
        address += toWrite;
    }
    return 0;
}

u8 writeI2CEEPROM(u8 eepromNumber, u16 address, u8 *buffer, u8 length) {
    // First we send a (started) control byte (asserting write), then high address byte
    // then low address byte.
    u8 control = (eepromNumber * 2) | 0xA0;

    // Write the address to the device
    u8 hiAddr = address >> 8;
    u8 loAddr = address & 0xFF;

    u8 retry = 50;
    while (1) {
        i2c_start();
        i2c_put(control);

        if (SSPCON2 & 0x40) {
            // No Ack
            --retry;
            if (retry == 0)
                return 0;
            pause(1);
        } else
            break;
    }

    i2c_put(hiAddr);
    i2c_put(loAddr);

    // Now write all the data
    unsigned short i = 0;
    for (; i < length; ++i) {
        i2c_put(buffer[i]);
    }
    i2c_stop();
    return 1;
}

void getRTCData(RTCDATA *data) {
    // Write the address to the device
    i2c_start();
    i2c_put(0xD0);
    i2c_put(0); // Address 0, include the seconds field
    i2c_restart();
    i2c_put(0xD1);
    u8 limit = sizeof (RTCDATA) - 1;
    u8 *dest = (u8 *) data;
    for (u8 i = 0; i < limit; ++i) {
        *dest++ = i2c_get();
        i2c_ack();
    }
    *dest++ = i2c_get();
    i2c_nack();
    i2c_stop();

    data->seconds = byteBCD2DEC(data->seconds & 0x7F);
    data->minutes = byteBCD2DEC(data->minutes);
    data->hours = byteBCD2DEC(data->hours);
    data->date = byteBCD2DEC(data->date);
    data->month = byteBCD2DEC(data->month);
    data->year = byteBCD2DEC(data->year);
}

void setRTCData(RTCDATA *data) {
    u8 buf[8];
    buf[0] = 0x30; // Seconds, also ensures clock starts
    buf[1] = byteDEC2BCD(data->minutes);
    buf[2] = byteDEC2BCD(data->hours);
    buf[3] = 0; // Don't use the Day part
    buf[4] = byteDEC2BCD(data->date);
    buf[5] = byteDEC2BCD(data->month);
    buf[6] = byteDEC2BCD(data->year);
    buf[7] = 0; // No square wave output

    // Write the address to the device
    i2c_start();
    i2c_put(0xD0);
    i2c_put(0); // Address 0, i.e. start from beginning
    for (u8 i = 0; i <= 7; ++i) {
        i2c_put(buf[i]);
    }
    i2c_stop();
}

