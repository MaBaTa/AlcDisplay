
#include "AlcDisplay.h"
AlcDisplay display(22 /*Bit0*/, 23 /*Bit1*/, 24 /*Bit2*/, 25 /*Bit3*/, 26 /*strobeA*/, 28 /*strobeB*/, 27 /*cmd*/, 29 /*reset*/);

void setup() {
  Serial.begin(115200);
  Serial.println(F("AlcDisplay"));
  display.reset();

  //Welcome
  display.writeText(0, "Welcome        >< ");
  delay(1000);
  display.clearMatrix();

  display.writeText(0, "Header:");
  display.writeText(9, "!!");
  delay(1000);
  display.writeText(0, "Command:");
  display.writeText(9, "00!");
  delay(1000);
  display.writeText(0, "Adress: ");
  display.writeText(9, "000 !!");
  delay(1000);
  display.writeText(0, "Value:  ");
  display.writeText(9, "000 00 !!");
  delay(2000);
  display.clearMatrix();

  //LEDs
  display.writeText(0, "Ch. LEDs");
  display.writeText(9, "EFE 00 XX");
  for (uint8_t x = 0; x < 4; x++) { display.setLed(x, LED_SOLID); delay(1000); }
  for (uint8_t x = 0; x < 2; x++) { display.setLed(x, LED_WAIT); }
  for (uint8_t x = 2; x < 4; x++) { display.setLed(x, LED_ERR); }
  delay(4000);
  for (uint8_t x = 0; x < 4; x++) { display.setLed(x, LED_OFF); }
  display.clearMatrix();


  //Backlight
  display.writeText(0, "Backlight");
  display.writeText(9, "DFE Xx");
  delay(1000);
  display.setBacklight(false);
  delay(500);
  display.setBacklight(true);
  display.clearMatrix();

  //Contrast
  display.writeText(0, "Contrast:");
  display.writeText(9, "EF9 00 XX");
  char cstr[9];
  for (uint8_t x = 0; x < 16; x++)
  {
    sprintf(cstr, "EF9 00 %02d", x);
    display.writeText(9, cstr);
    display.setContrast(x);
    delay(200);
  }
  delay(1000);
  display.clearMatrix();

  //Segments
  display.writeText(0, "Segments:");
  display.writeText(9, "EF3 0X 0X");
  for (uint8_t x = 1; x < 36; x++) { display.setSegments((uint64_t)1<<x); delay(200); }
  delay(1000);
  display.clearAll();

  //Channel
  display.writeText(0, "Channel:");
  display.writeText(9, "EF2 0C XX");
  for (uint8_t x = 0; x < 9; x++) { display.setChannel(x); delay(500); }
  display.setChannel(0xA);
  delay(1000);
  display.clearAll();

  //Nubers
  display.writeText(0, "Numbers:");
  display.writeText(9, "EF2 0X XX");
  display.setVoltage(14.2);
  for (float x = 0.00005; x < 5000; x *= 2) { display.setCurrent(x); delay(500); }
  display.setCurrent(-0.0006471);
  display.setCapacity(3.141);
  delay(2000);
  display.setVoltage(NAN);
  delay(500);
  display.setCurrent(NAN);
  delay(500);
  display.setCapacity(NAN);
  
  
  //display.setVoltage(0);
  //display.setSegments(FUNC_REFRESH | CELL_DISCHARGE | CELL_BODY);
  //display.resetSegments(FUNC_REFRESH);


  //Text
  //display.writeText(0, "Hello          >< ");


  /*for (uint8_t x = 0; x < 15; x++)
  {
    //display.writeCharAt(x, "X");
    display.beginCmd(CMD_BANK5) && display.sendCmdByte(0x00) && display.sendNibble(x, false, false);
    delay(1000);
  }*/


  //BLINK
  //display.modeFrame(MODE_BLINK);
  //display.beginCmd(CMD_DIGITS) && display.sendCmdByte(DIGITS_CURRENT) && display.sendNibble(0x7, false, false);
  //display.cmdFrame(CMD_DIGITS, DIGITS_CURRENT, v, 3);
  //display.modeFrame(MODE_NORMAL);
}



bool handle(const String& cmd, const String& arg) {
  if (cmd == "u")    return display.setVoltage((uint16_t)arg.toInt());
  //if (cmd == "i")    return lcd.setCurrent((uint16_t)arg.toInt())  && lcd.refresh();
  //if (cmd == "c")    return lcd.setCapacity((uint16_t)arg.toInt()) && lcd.refresh();
  //if (cmd == "ch")   return lcd.setChannel((uint8_t)arg.toInt())   && lcd.refresh();
  if (cmd == "seg") return display.setSegments((uint16_t)arg.toInt());
  //if (cmd == "led") return display.setLeds((uint16_t)arg.toInt(), LED_SOLID);
  if (cmd == "rst") display.reset();
  Serial.println(F("unbekannt"));
  return true;
}



void loop() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  int sp = line.indexOf(' ');
  String cmd = (sp < 0) ? line : line.substring(0, sp);
  String arg = (sp < 0) ? String("") : line.substring(sp + 1);
  cmd.toLowerCase();
  arg.trim();

  if (handle(cmd, arg)) { Serial.println(F("ok")); }
}
