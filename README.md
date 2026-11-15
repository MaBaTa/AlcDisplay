# AlcDisplay

Library and Documentation for the Display of the ALC 8500/8000/3000 Charge Controller from ELV

![](pictures/Display%20Test.png)

*Bootsplash of the display controller (source: me)*

This project consists of the reverse engeneering of the Protocol used to control the ALC *(**A**kku **L**ade **C**enter eng. Battery Charging Center)* Display. It was the preparation for my [ALC Firmware](https://github.com/MaBaTa/ALC-Firmware) development Project. Although it would be easier to replace the screen, i wanted to be able to control the original display, since it fits the style of the device and in this way anybody can use the alternative firmware without replacing it.

All references pertain to the **ALC 8500 2 Expert**, however the display is identical in the **ALC 8000** and **ALC 3000 PC**.

## Protocol

### Hardware Level:

The comunication between the main µC and the display controler consists of 7 pins:

| DisplayPin  | ATMEGA128 Pin | Function | Description            |
| ----------- | ------------- | -------- | ---------------------- |
| P3.0        | PD4           | Bit 1    | Binary Nibble          |
| P3.1        | PD5           | Bit 2    | Binary Nibble          |
| P3.2        | PD6           | Bit 3    | Binary Nibble          |
| P3.3        | PD7           | Bit 4    | Binary Nibble          |
| P4.0 & P1.0 | PF0           | StrobeA  | Indicates lower Nibble |
| P4.1 & P1.1 | PF1           | CMD      | Indicates Controlbyte  |
| P4.2 & P1.2 | PF2           | StrobeB  | Indicates upper Nibble |

The data-pins P3.X are exclusively controlled externally, so they can be driven in Push/Pull mode. They are representing a nibble in paralell.

For the control-pins P4.X, the two strobes are pulled high and driven low by both sides (pull up/open drain). The CMD line is normally high and, like the data-pins, only being read by the display.

Additionaly the display can be reset by pulling down the reset-pin.

*This is controlled be the `GPIO Control` section of the Library.*

### Procedure when sending a nibble: *`sendNibble()`*

1. Nibbles (binary data) are applied.

2. One strobe is asserted low (depending on upper or lower Nibble) optionally in conjunction with CMD line.

3. Waiting for an acknowledgment from the display; this is detected via a negative-edge originating from the display, on the other strobe line.

4. The own strobe and, if applicable, CMD are released again, and the strobe's pull-up is reapplied.

5. From this point on, the next Nibble can already be applied; the display is busy processing the last one. However, the next strobe may not be triggered until the display has finished processing and therefore realising it's strobe.

**Here an Example:**

![](pictures/Protocol%20Sending%20Nibbles.png)

### Frames:

Based on this fundamental sending of nibbles, data frames are being constructed.

If a complete Byte is required it is split into a high and low nibble. However, the order depends on whether a data byte or a command byte is being sent. For commands the lower 4 Bits are sent first `sendCmdByte()`, and for data the upper are first `sendDataByte()`. This could be another factor for the display to seperate data from commands.

#### Building a Frame:

A typical frame has this structure:

```
[HEADER] [CMD]  [ADDRESS]-[VAULE](-[VAULE])   ([ADDRESS]-[VAULE])...
CmdByte  Data   CmdByte   Data
8Bit     4Bit   8Bit      4/8Bit
```

- Header: Only 2 Types: `0xEF` for almost everything and `0xDF` changing mode.

- Cmd: This decides witch Register is accessed (Digit, Text, LED) for normal commands. When changing mode it decides the mode.

- Address: This specifies where to start if you do not want to overwrite the entire register.

- Value: New Nibble to be Written in this Register on this Address. If writing Multiple each one takes the next higher Adress.

---

## Ansteuerung

##### LEDs

Die LEDs welche den Kanalstatus anzeigen werden mit Kommando `0xE` und dem 1. Byte angesprochen, also Offset `0x00`. Das untere Nibble aktiviert das langsame(warten) und das obere das schnelle(error) Blinken der Jeweiligen LED. Werden beide Bits einer LED gesetzt, so ist sie statisch an(aktiv).

`0b10100010` entspricht als einem dauerhaften leuchten bei Kanal 2 und einem Schnellen Blinken auf Kanal 4.

Ansteuerbar ist diese Funktionalität direkt binär über `bool setLeds(uint8_t _byte);` oder mit internem buffer der bisherigen Änderungen via `setLeds(uint8_t _led, alcLed _mode);`.

x

| Aktion                  | Code                  |
| ----------------------- | --------------------- |
| Änndern der Kapazität   | E F 2 8 <XXX Cap>     |
| Ändern der Ch. Nummer   | E F 2 C 0 <Nr.>       |
| Ändern des Akku Symbols | E F 3 0 <Cell Type>   |
| Kapazität Cousor        | E F 2 8 0 <Pos.>      |
| Text Schreiben          | E F 4 <Pos.> 0 <Text> |
