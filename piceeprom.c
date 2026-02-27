#include "utility.h"

unsigned char readInternalEeprom(u8 adr) {
    EEADRL = adr;
    EECON1 = 0x01; // Set read from EEPROM
    return (EEDATL);
}

void writeInternalEeprom(u8 value, u8 address) {
    EEADRL = address;
    EEDATL = value;
    EECON1 = 0x04; // Write enable

    EECON2 = 0x55;
    EECON2 = 0xAA;

    WR = 1;
    WREN = 0;
    while (WR);
}

void loadStatus() {
    u8 i;
    u8 *dest = (u8 *) & status;
    for (i = 0; i<sizeof (status); ++i) {
        *dest = readInternalEeprom(i);
        ++dest;
    }
}

void saveStatus() {
    u8 i;
    u8 *dest = (u8 *) & status;
    for (i = 0; i<sizeof (status); ++i) {
        writeInternalEeprom(*dest, i);
        ++dest;
    }
}
