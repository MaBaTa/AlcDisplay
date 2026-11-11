/**
 * @file    AlcDisplay.h
 * @author  Tristan Wieczorek
 * @date    2026-10-31
 * @brief   Driver for the Display Controller IC500 from the ALC 8000/8500
 */

#ifndef _ALCDISPLAY_H_
#define _ALCDISPLAY_H_
#include <Arduino.h>


#define READY_RESTROBE  10        //Ready => Strobe
#define STROBE_HOLD     6         //Ack => Release
#define DATA_HOLD       12        //Release => new Nibble
#define ACK_TIMEOUT     1000000UL
#define DEFAULT_LEADING_ZEROS false



#define HEADER_COMMAND  0xFE
#define HEADER_MODE     0xFD

#define MODE_NORMAL   0x8  // normale Anzeige-Ebene (oberes Nibble)
#define MODE_BLINK    0xB  // Blink-Masken-Ebene (oberes Nibble)

#define DIGIT_BLANK     0xA    // Ziffer aus

#define LINE_LEN    9

enum alcCmd
{
  CMD_INIT     = 0x0,
  CMD_DIGITS   = 0x2,  // 7-Segment-Ziffern
  CMD_SEGMENTS = 0x3,  // LCD Sondersegmente
  CMD_TEXT     = 0x4,  // Text im 2x9-Raster
  CMD_BANK5    = 0x5,  // Text Presets
  CMD_TEXT_X   = 0x6,  // frei positionierter Text
  CMD_CLEAR    = 0x7,  // Matrix loeschen (Display ist danach ~3 ms beschaeftigt)
  CMD_BANK8    = 0x8,  // Unbekannt
  CMD_CONFIG   = 0x9,  // Kontrast (Konfiguration)
  CMD_BANKC    = 0xC,  // Unbekannt, Gefährlich
  CMD_BANKD    = 0xD,  // Unbekannt, Gefährlich
  CMD_LED      = 0xE   // LEDs
};

enum alcAdr // Adress Offsets für die Funktionen
{
  DIGITS_VOLTAGE  = 0x00,
  DIGITS_CURRENT  = 0x04,
  DIGITS_CAPACITY = 0x08,
  DIGIT_CHANNEL   = 0x0C,
  SEG_CELLTYPE    = 0x00,
  SEG_FUNCTION    = 0x02,
  CONFIG_CONTRAST = 0x00    //Einige andere Adressen ändern auch den Kontrast
};

enum alcSegment
{
  CELL_NC        = 1<<1,  //1
  CELL_NIMH      = 1<<2,
  CELL_LIION     = 1<<3,
  CHANNEL_FRAME  = 1<<4,
  CELL_DISCHARGE = 1<<5,
  CELL_PB        = 1<<6,
  CELL_RAM       = 1<<7,
  FUNC_ACTIVATOR = 1<<8,
  FUNC_MAINTAIN  = 1<<9,
  FUNC_FORMING   = 1<<10,
  FUNC_REFRESH   = 1<<11,
  FUNC_CYCLE     = 1<<12,
  FUNC_TEST      = 1<<13,
  FUNC_DISCHARGE = 1<<14,
  FUNC_CHARGE    = 1<<15,
  UINT_C_DOT     = (uint64_t)1<<16,  //16, 17, 18
  UINT_C_M       = (uint64_t)1<<19,
  UINT_I_DOT     = (uint64_t)1<<20,  //20, 21, 22
  UINT_I_A       = (uint64_t)1<<23,  
  UINT_U_DOT     = (uint64_t)1<<24,  //24, 25, 26
  UINT_U_V       = (uint64_t)1<<27,
  UINT_C_C       = (uint64_t)1<<28,
  UINT_I_I       = (uint64_t)1<<29,
  UINT_U_U       = (uint64_t)1<<30,
  UINT_I_M       = (uint64_t)1<<31,
  UINT_C_AH      = (uint64_t)1<<32,
  CELL_BODY      = (uint64_t)1<<33,
  CELL_CHARGE    = (uint64_t)1<<34,
  UINT_I_NEG     = (uint64_t)1<<35
};

enum alcLed
{
  LED_OFF,
  LED_WAIT,
  LED_ERR,
  LED_SOLID
};



class AlcDisplay
{
  public:
    // Constructor
    AlcDisplay(uint8_t _d0, uint8_t _d1, uint8_t _d2, uint8_t _d3, uint8_t _strobeA, uint8_t _strobeB, uint8_t _cmd, uint8_t _rst);
    AlcDisplay(); //For faster direct Control with native PD4,PD5,PD6,PD7,PF0,PF2,PF1,PF3 for the ALC Controller

    // Basic
    void reset();
    
    // Application
    bool setSegments(uint64_t _seg);
    bool resetSegments(uint64_t _seg);
    
    bool setVoltage(float _voltage);
    bool setCurrent(float _current);
    bool setCapacity(float _capacity);
    bool setChannel(uint8_t _ch);
    
    
    bool setLed(uint8_t _led, alcLed _mode = LED_SOLID);
    bool setBacklight(bool _on);
    bool setContrast(uint8_t _contrast);

    bool clearAll();
    bool clearMatrix();
    bool writeText(uint8_t _start, const char* _text);
    bool writeCharAt(uint8_t _position, const char* _text);
    
  private:
    
    // GPIO Control
    uint8_t d0, d1, d2, d3, strobeA, strobeB, cmd, rst;
    bool inStrobe = false, inHighStrobe = false, native = false; // >>sould only be accessed by GPIO Control<<<
    
    void initPins();
    void setNibble(uint8_t _nibble);
    void strobe(bool _highStrobe, bool _cmdStrobe);
    bool strobeAckd();
    void releaseStrobe();
    bool strobeReady();

    // Send Nibbles and Bytes
    bool sendNibble(uint8_t _nibble, bool _highNibble, bool _cmd);
    bool sendCmdByte(uint8_t _value);       //Send Byte as Command ([LOWER][UPPER])
    bool sendDataByte(uint8_t _value);      //Send Byte as Data    ([UPPER][LOWER])

    // Commands / Frames
    bool beginCmd(uint8_t _cmd);
    bool cmdFrame(uint8_t _cmd, uint8_t _address, const uint8_t* _values, uint8_t _count);
    bool cmdByte(uint8_t _cmd, uint8_t _address, const uint8_t* _values, uint8_t _count);
    bool modeFrame(uint8_t _mode);

    // Application
    uint8_t ledBuffer = 0x00;
    uint64_t segBuffer = 0;
    bool backlightBuffer = true;
    uint8_t contrastBuffer = 12;
    uint8_t modeBuffer = 8;

    bool setLeds(uint8_t _byte);
    bool setDigits(uint8_t _adr, float _value, alcSegment _basePoint, bool _leadingZeros = DEFAULT_LEADING_ZEROS);
    bool blankDigits(uint8_t _adr, alcSegment _basePoint);
    bool sendSegments(uint64_t _changedBits);
    
};
#endif
