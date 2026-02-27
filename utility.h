#if !defined(UTILITY_H_INCLUDED)
#define UTILITY_H_INCLUDED

#include <pic.h>
#include <stdio.h>
#include <string.h>
#include "constants.h"

extern SERVERMODESTATE serverMode;
extern METERSTATUS status;
extern METERINFO info;
extern u8 bigBuf[];
extern char *bigBufStr;
extern u16 backlightTickCount;

// I2C
void i2cInit(void);
u8 readI2CEEPROM(u8 eepromNumber, u16 address, u8 *buffer, u16 length);
u8 writeI2CEEPROM(u8 eepromNumber, u16 address, u8 *buffer, u8 length);
u8 writeI2CEEPROMPaged(u8 eepromNumber, u16 address, u8 *buffer, u8 length);
void getRTCData(RTCDATA *data);
void setRTCData(RTCDATA *data);

// LCD
void lcdWriteByte(unsigned char value, unsigned char data);
void lcdInit(void);
void lcd(const char *text, u8 options);
void lcdChar(u8 value);
void lcdClear(void);
void lcdHome(void);
void lcdPad(const char *text, const u8 options);

// Internal PIC EEPROM
u8 readInternalEeprom(u8 adr);
void writeInternalEeprom(u8 value, u8 address);
void loadStatus(void);
void saveStatus(void);

// Analogue
void handleCCP5Event(void);
u16 readAD(u8 channel);
u16 readPulse(void);

// Serial
void setBaudRate(BAUDRATE rate);
u16 receiveSerialBytes(u8 *data, u16 limit, u16 timeout);
void sendSerialString(const char *data);
void sendSerialBytes(const u8 *data, u16 length);
void sendSerialByte(const u8 data);
void handleRXInterrupt(void);
void handleT6Interrupt(void);

// Buttons
u8 getButtonState(void);
u8 waitForButtonPress(void);
u8 waitForButtonDown(void);
void waitForButtonRelease(void);

// Misc
void pause(u16 ticks);
u8 byteBCD2DEC(u8 bcd);
u8 byteDEC2BCD(u8 dec);
void appendVal(int value, u8 *buf, u8 decimal);
void showDateTime(void);
void overlayYesNo(void);
void getCalibrationName(u8 calibNumber, char *buf);
void showCalibrationName(u8 calibNumber, u8 options);
void batteryCheck(void);
u8 inputNumber(u8 start, u8 minVal, u8 maxVal, const char *prompt);

// Meter admin
void selectMeterType(void);
void setTorryScale(void);
void loadInfo(void);
void saveInfo(void);
void configureBacklight(void);

// Rental mode
void rentalCheck(void);
void showRentalEnd(void);
u8 acquireAndTestRentalCode(void);

// Server mode
void enableServerMode(void);
void disableServerMode(void);
void handleServerModeRX(void);
void runServerMode(void);

// Menu
void doChoicesMenu(void);
void sendSampleData(u16 dataset);
void addCalibration(void);
u8 getNextFilledSlot(u8 current, u8 minCal, u8 maxCal);
u8 calmap(u8 index);
void loadSlotMap(void);

// Take readings
u8 startMeasuring(void);
void adMode(void);
u16 getAvgPulse(void);
u16 getAvgAD(void);
u16 getCalibrationValue(u16 memoryLocation, u8 product);
void checkpadAutomation(void);
void checkpadStartupTest(void);

// Backlight
void backlight(BACKLIGHTSTATE state);
void timedBacklight(void);
void backlightTimerFired(void);
void setBacklightTicks(void);

// Calibration
void enterCalibrationMode(void);
void dumpCalibration(const u8 slot);

#endif
