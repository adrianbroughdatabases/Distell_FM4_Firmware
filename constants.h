#if !defined(CONSTANTS_H_INCLUDED)
#define CONSTANTS_H_INCLUDED

//#define _XTAL_FREQ 20MHZ
//#define DEFAULT_OSCCON	0xFF
#define DEFAULT_OSCCON	0x38

// Number of bytes to store a single set of sample data
#define BYTESPERRECORD				64
// Number of bytes in a calibration identity
#define BYTES_PER_CALIBRATION_ID	32

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned long u32;
typedef signed long s32;

#define DEFAULT_PEIE	(1)		// Default value of peripheral interrupt enable
#define DEFAULT_GIE		(1)		// Default value of global interrupt enable

#define BUTTON_NONE		0
#define BUTTON_YES		1
#define BUTTON_NO		2
#define BUTTON_BOTH		3

#define LCD_HOME		1
#define LCD_CLEAR		2
#define LCD_LINE2		4

typedef struct {
    u8 curProduct;
    u8 numReadings;
    u8 meterType;
    u16 checksum;
    u16 stored;
    u8 airValue;
    u8 calibrationMode;
    u8 demoMode;
    u8 fishFatmeter;
} METERSTATUS;

typedef struct {
    char serialNumber[9];
    u16 serialNumberChecksum;
    u8 rentalMode;
    u8 rentalDay;
    u8 rentalMonth;
    u8 rentalYear;
    u8 torryScale;
    u16 infoSignature;
    u8 backlightSetting;    // Backlight timeout:  0 = 'off'.  0xFF = 'on'  Otherwise, timeout in seconds
} METERINFO;

typedef struct {
    u16 samples[16];
    u16 average;

    union {
        u8 data[7];

        struct {
            u8 sampleCount;
            u8 slotNumber;
            u8 mins;
            u8 hours;
            u8 date;
            u8 month;
            u8 year;
        } items;
    };
} DATARECORD;

typedef struct {
    u8 spacer;
    u16 location;
    u16 data[20];
    u16 checksum;
} UPLOADRECORD;

#define PIN_SER_OUT RC5
#define PIN_SER_IN RC4

typedef enum {
    BAUD2400 = 129, // 103
    BAUD4800 = 64, // 51
    BAUD9600 = 32, // 25
    BAUD19200 = 15 // 12
} BAUDRATE;

typedef enum {
    SM_OFF = 0,
    SM_MONITOR = 1,
    SM_LAUNCH = 2
} SERVERMODESTATE;

typedef struct {
    u8 seconds;
    u8 minutes;
    u8 hours;
    u8 dayofweek;
    u8 date;
    u8 month;
    u8 year;
    u8 control;
} RTCDATA;

typedef enum {
    BL_ON = 0,
    BL_OFF = 1
} BACKLIGHTSTATE;

#endif
