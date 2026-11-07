/**
 * @file    AlcDisplay.cpp
 * @author  Tristan Wieczorek
 * @date    2026-10-31
 * @brief   Driver for the Display Controller IC500 from the ALC 8000/8500
 */

#include "AlcDisplay.h"

/**
 * Constructor for AlcDisplay
 * See Documentation for more info
 */
AlcDisplay::AlcDisplay(uint8_t _d0, uint8_t _d1, uint8_t _d2, uint8_t _d3, uint8_t _strobeA, uint8_t _strobeB, uint8_t _cmd, uint8_t _rst) :
  d0(_d0),
  d1(_d1),
  d2(_d2),
  d3(_d3),
  strobeA(_strobeA),
  strobeB(_strobeB),
  cmd(_cmd),
  rst(_rst)
{
  initPins();
}

/**
 * Reset the LCD Controller via Reset pin.
 * Reinitialize Library
 * Initialize Display
 */
void AlcDisplay::reset() {
  pinMode(rst,       OUTPUT);
  digitalWrite(rst,  LOW);
  delay(100);
  digitalWrite(rst,  HIGH);
  delay(100);
  setLeds(ledBuffer);
  clearMatrix();
  setContrast(contrastBuffer);
  //sendSegments(0xFFFFFFFFF); //Resend all Segments
  //setChannel(0xA);
  //setVoltage(NAN);
  //setCurrent(NAN);
  //setCapacity(NAN);

  //Clear all Digits and Segments
  uint8_t all[13];
  for (uint8_t i = 0; i < 13; i++) all[i] = 0x0;
  cmdFrame(CMD_SEGMENTS, 0x0, all, 9);
  for (uint8_t i = 0; i < 13; i++) all[i] = 0xA;
  cmdFrame(CMD_DIGITS, 0x0, all, 13);
  
  delay(10);
}



/*======================================== GPIO Control ========================================*/

/**
 * Initialises the Bus Pins
 */
void AlcDisplay::initPins()
{
  pinMode(d0,       OUTPUT);
  pinMode(d1,       OUTPUT);
  pinMode(d2,       OUTPUT);
  pinMode(d3,       OUTPUT);
  digitalWrite(d0,  LOW);
  digitalWrite(d1,  LOW);
  digitalWrite(d2,  LOW);
  digitalWrite(d3,  LOW);
  
  pinMode(strobeA,  INPUT_PULLUP);
  pinMode(strobeB,  INPUT_PULLUP);
  pinMode(cmd,      OUTPUT);
  digitalWrite(cmd, HIGH);
}

/**
 * Applys the binary data for the given nibble
 */
void AlcDisplay::setNibble(uint8_t _nibble)
{
  _nibble = _nibble & 0b00001111;       //reduce to 4bit
  digitalWrite(d0, _nibble & 0b0001);
  digitalWrite(d1, (_nibble & 0b0010) >> 1);
  digitalWrite(d2, (_nibble & 0b0100) >> 2);
  digitalWrite(d3, (_nibble & 0b1000) >> 3);
}

/**
 * Pulls one strobe and the cmd line low, depending on the given values
 */
void AlcDisplay::strobe(bool _highStrobe, bool _cmdStrobe)
{
  inStrobe = true;
  inHighStrobe = _highStrobe;
  
  digitalWrite(cmd, !_cmdStrobe); //Pull cmd low if sending command
  uint8_t strobePin = _highStrobe ? strobeB : strobeA;
  pinMode(strobePin,      OUTPUT);
  digitalWrite(strobePin, LOW);
}

/**
 * Checks if the reciever has Acknowleged the strobe
 */
bool AlcDisplay::strobeAckd()
{
  return inStrobe && !digitalRead(inHighStrobe ? strobeA : strobeB);
}

/**
 * Realeses the before done Strobe including the cmd line
 */
void AlcDisplay::releaseStrobe()
{
  pinMode(inHighStrobe ? strobeB : strobeA,  INPUT_PULLUP);
  digitalWrite(cmd, HIGH);
  inStrobe = false;
}

/**
 * Checks if the reciever is ready to read again
 */
bool AlcDisplay::strobeReady()
{
  return !inStrobe && (digitalRead(strobeA) && digitalRead(strobeB));
}

/*======================================== Send Nibbles and Bytes ========================================*/

/**
 * Sends one nibble if display is ready and checks acknowlegement
 * returns true if transfer completed
 */
bool AlcDisplay::sendNibble(uint8_t _nibble, bool _highNibble, bool _cmd)
{
  if (inStrobe) return false; //Sending while sending
  setNibble(_nibble);

  uint32_t start = micros();
  while(!strobeReady())
  {
    if ((uint32_t)(micros() - start) >= ACK_TIMEOUT) return false; //Timeout Display not Ready
  }
  delayMicroseconds(READY_RESTROBE);

  strobe(_highNibble, _cmd);

  start = micros();
  while(!strobeAckd())
  {
    if ((uint32_t)(micros() - start) >= ACK_TIMEOUT) return false; //Timeout no Ack
  }
  delayMicroseconds(STROBE_HOLD);

  releaseStrobe();
  delayMicroseconds(DATA_HOLD);

  return true;
}

/**
 * Send Byte as Command
 * Commands are splitted like this ([LOWER][UPPER])
 * (The cmd pin is used)
 */
bool AlcDisplay::sendCmdByte(uint8_t _value)
{
  return sendNibble(_value & 0x0F, false, true)
      && sendNibble(_value >> 4,   true,  true);
}

/**
 * Send Byte as Data
 * Data is splitted like this ([UPPER][LOWER])
 */
bool AlcDisplay::sendDataByte(uint8_t _value)
{
  return sendNibble(_value >> 4,   true,  false)
      && sendNibble(_value & 0x0F, false, false);
}

/*======================================== Send Nibbles and Bytes ========================================*/

/**
 * Typical beginning of a Command. Selecting Header and Command
 */
bool AlcDisplay::beginCmd(uint8_t _cmd)
{
  return sendCmdByte(HEADER_COMMAND) && sendNibble(_cmd, false, false);
}

/**
 * Sending multiple low-Nibbles to a specified Adress
 */
bool AlcDisplay::cmdFrame(uint8_t _cmd, uint8_t _address, const uint8_t* _values, uint8_t _count) {
  if (!beginCmd(_cmd) || !sendCmdByte(_address)) return false;
  for (uint8_t i = 0; i < _count; i++) if (!sendNibble(_values[i], false, false)) return false;
  return true;
}

/**
 * Sending multiple Bytes to a specified Adress
 */
bool AlcDisplay::cmdByte(uint8_t _cmd, uint8_t _address, const uint8_t* _values, uint8_t _count) {
  if (!beginCmd(_cmd) || !sendCmdByte(_address)) return false;
  for (uint8_t i = 0; i < _count; i++) if (!sendDataByte(_values[i])) return false;
  return true;
}

/**
 * Change the mode
 * Is only used for Blinking Digits untill now.
 */
bool AlcDisplay::modeFrame(uint8_t _mode)
{
  modeBuffer = _mode;
  if (!sendCmdByte(HEADER_MODE)) return false;
  return sendNibble(_mode, backlightBuffer, false); //Nibble level controls Backlight
}

/*======================================== Application ========================================*/

/*==================== Custom Segments ====================*/

/**
 * Enable custom segments based on given mask
 */
bool AlcDisplay::setSegments(uint64_t _seg)
{
  uint64_t changedBits = (~segBuffer) & _seg;
  segBuffer |= _seg; //Set selected in Buffer
  
  return sendSegments(changedBits);
}

/**
 * Disable custom segments based on given mask
 */
bool AlcDisplay::resetSegments(uint64_t _seg)
{
  uint64_t changedBits = segBuffer & _seg;
  segBuffer &= ~_seg; //Clear selected in Buffer
  
  return sendSegments(changedBits);
}

/**
 * Send nibbles that contain changes for costom segments
 */
bool AlcDisplay::sendSegments(uint64_t _changedBits)
{
  int startNibble = -1;
  int endNibble = -1;
  uint8_t nibbles[9];
  for (int n = 0; n < 9; n++)
  {
      nibbles[n] = (segBuffer >> (n * 4)) & 0x0F;
      uint64_t nibbleMask = (uint64_t)0xF << (n * 4);
      if ((_changedBits & nibbleMask) != 0)
      {
          if (startNibble == -1) startNibble = n;
          endNibble = n;
      }
  }
  
  return cmdFrame(CMD_SEGMENTS, startNibble, &nibbles[startNibble], (endNibble - startNibble + 1));
}



/*==================== LEDs, Backlight, Contrast ====================*/

/**
 * Send the complete Byte for all Channel LEDs
 */
bool AlcDisplay::setLeds(uint8_t _byte) { return beginCmd(CMD_LED) && sendCmdByte(0x00) && sendDataByte(_byte); }

/**
 * Control a spicific Channel LED
 */
bool AlcDisplay::setLed(uint8_t _led, alcLed _mode)
{
  if (_led > 3) return false;
  
  switch(_mode)
  {
    case LED_OFF:
        ledBuffer &= ~((1 << _led) | (1 << (4 + _led)));
        break;
    case LED_WAIT:
        ledBuffer = (ledBuffer | (1 << _led)) & ~(1 << (4 + _led));
        break;
    case LED_ERR:
        ledBuffer = (ledBuffer | (1 << (4 + _led))) & ~(1 << _led);
        break;
    case LED_SOLID:
        ledBuffer |= (1 << _led) | (1 << (4 + _led));
        break;
  };
  
  return setLeds(ledBuffer);
}

/**
 * enable/disable the LED Backlight
 */
bool AlcDisplay::setBacklight(bool _on)
{
  backlightBuffer = _on;
  return modeFrame(modeBuffer);
}

/**
 * Set the global contrast of the LCD
 */
bool AlcDisplay::setContrast(uint8_t _contrast)
{
  if (_contrast > 15) return false;
  contrastBuffer = _contrast;
  return beginCmd(CMD_CONFIG) && sendCmdByte(CONFIG_CONTRAST) && sendNibble(_contrast, false, false);
}



/*==================== Matrix / Text ====================*/

/**
 * Clear every Segment on the LCD including Matrix
 */
bool AlcDisplay::clearAll()
{
  segBuffer = 0;
  return beginCmd(CMD_CLEAR);
}

/**
 * Competely Clear the Matriy from any Pixel
 */
bool AlcDisplay::clearMatrix()
{
  return beginCmd(CMD_BANK5) && sendCmdByte(0x00) && sendNibble(0x0, false, false)   //Load Preset 0 (Clear)
                             && sendCmdByte(0x09) && sendNibble(0x0, false, false);  //Load Preset 0 (Clear)
}

/**
 * Write Text alignt to the 2x9 Grid
 */
bool AlcDisplay::writeText(uint8_t _start, const char* _text)
{
  if (!beginCmd(CMD_TEXT) || !sendCmdByte(_start)) return false;
  for (uint8_t i = 0; _text[i] && _start + i < 2 * LINE_LEN; i++)
    if (!sendDataByte((uint8_t)_text[i])) return false;
  return true;
}

/**
 * Start Writing at Specific X Position
 */
bool AlcDisplay::writeCharAt(uint8_t _position, const char* _text)
{
  //return beginCmd(CMD_TEXT_X) && sendCmdByte(_position) && sendDataByte((uint8_t)_char);
  if (!beginCmd(CMD_TEXT_X) || !sendCmdByte(_position)) return false;
  for (uint8_t i = 0; _text[i]; i++)
    if (!sendDataByte((uint8_t)_text[i])) return false;
  return true;
}



/*==================== Digit Control ====================*/

/**
 * Controls 4 Digits and the points. Everything else is must be set Externally.
 */
bool AlcDisplay::setDigits(uint8_t _adr, float _value, alcSegment _basePoint, bool _leadingZeros)
{
  if (isnan(_value) || signbit(_value)) return false;

  resetSegments(_basePoint | _basePoint<<1 | _basePoint<<2);
  
  uint8_t digits[4];
  long temp;

  if (_value<10) //3. point
  {
    setSegments(_basePoint<<2);
    temp = (long)(abs(_value) * 1000.0 + 0.005); 
  }
  else if (_value<100) //2. point
  {
    setSegments(_basePoint<<1);
    temp = (long)(abs(_value) * 100.0 + 0.0005); 
  }
  else if (_value<1000) //1. point
  {
    setSegments(_basePoint);
    temp = (long)(abs(_value) * 10.0 + 0.00005); 
  }
  else // >1000 no point
  {
    temp = (long)(abs(_value) + 0.000005); 
  }

  
  for (uint8_t i = 0; i < 4; i++)
  {
    digits[i] = temp % 10;
    temp /= 10;
  }

  if (!_leadingZeros)
    for (uint8_t i = 3; i > 0 && digits[i] == 0; i--) digits[i] = 0xA;
    
  return cmdFrame(CMD_DIGITS, _adr, digits, 4);
}

/**
 * Disables 4 Digits and Points entirely.
 */
bool AlcDisplay::blankDigits(uint8_t _adr, alcSegment _basePoint)
{
  uint8_t digits[4] = { 0xA, 0xA, 0xA, 0xA};
  return cmdFrame(CMD_DIGITS, _adr, digits, 4) && resetSegments(_basePoint | _basePoint<<1 | _basePoint<<2);;
}

/**
 * Displays a Value as a Voltage.
 */
bool AlcDisplay::setVoltage(float _voltage)
{
  bool ok = true;
  if (isnan(_voltage))
  {
    ok &= resetSegments(UINT_U_U | UINT_U_V);
    ok &= blankDigits(DIGITS_VOLTAGE, UINT_U_DOT);
    return true;
  }

  if (signbit(_voltage)) _voltage *= -1;

  return setSegments(UINT_U_U | UINT_U_V)
      && setDigits(DIGITS_VOLTAGE, _voltage, UINT_U_DOT)
      && ok;
}

/**
 * Displays a Value as a Current.
 */
bool AlcDisplay::setCurrent(float _current)
{
  bool ok = true;
  if (isnan(_current))
  {
    ok &= resetSegments(UINT_I_I | UINT_I_NEG | UINT_I_M | UINT_I_A);
    ok &= blankDigits(DIGITS_CURRENT, UINT_I_DOT);
    return true;
  }

  if (signbit(_current)) { ok &= setSegments(UINT_I_NEG); _current *= -1;}
  else                   ok &= resetSegments(UINT_I_NEG);

  if (_current < 1.0 && _current > -1.0)
  {
    ok &= setSegments(UINT_I_M);
    _current *= 1000;
  }
  else ok &= resetSegments(UINT_I_M);

  return setSegments(UINT_I_I | UINT_I_A)
      && setDigits(DIGITS_CURRENT, _current, UINT_I_DOT)
      && ok;
}

/**
 * Displays a Value as a Capacity.
 */
bool AlcDisplay::setCapacity(float _capacity)
{
  bool ok = true;
  if (isnan(_capacity))
  {
    ok &= resetSegments(UINT_C_C | UINT_C_M | UINT_C_AH);
    ok &= blankDigits(DIGITS_CAPACITY, UINT_C_DOT);
    return true;
  }

  if (signbit(_capacity)) _capacity *= -1;

  if (_capacity < 1.0 && _capacity > -1.0)
  {
    ok &= setSegments(UINT_C_M);
    _capacity *= 1000;
  }
  else ok &= resetSegments(UINT_C_M);

  return setSegments(UINT_C_C | UINT_C_AH)
      && setDigits(DIGITS_CAPACITY, _capacity, UINT_C_DOT)
      && ok;
}

/**
 * Sets the Channel Digit
 * 0xA/10 Disables it
 */
bool AlcDisplay::setChannel(uint8_t _ch)
{
  if(_ch >= 0xA)
  {
    if (!resetSegments(CHANNEL_FRAME)) return false;
    _ch = 0xA;
  }
  else if (!setSegments(CHANNEL_FRAME)) return false;

  return beginCmd(CMD_DIGITS) && sendCmdByte(DIGIT_CHANNEL) && sendNibble(_ch, false, false);
}
