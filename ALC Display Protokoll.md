# ALC Display

## Hardware Ebene

Steuerpins PF0-2 sind pull-up und in in ruhe HIGH. Werden sie von einer der Seiten gegen Masse gezogen (open drain) ist das ein Auslösen.

Die Datenpins PD4-7 werden nur vom sender beschrieben und stellen das aktuelle Nibble Binär da.

| Pin | Funktion | Bedeutung                   |
| --- | -------- | --------------------------- |
| PD4 | Bit 1    | Binar Nibble                |
| PD5 | Bit 2    | Binar Nibble                |
| PD6 | Bit 3    | Binar Nibble                |
| PD7 | Bit 4    | Binar Nibble                |
| PF0 | StrobeA  | markiert ein unteres Nibble |
| PF1 | CMD      | markiert eine Steuerbyte    |
| PF2 | StrobeB  | markiert ein oberes Nibble  |

##### Vorgegen beim senden eines Nibbles:

1. Nibble (Binärdaten) werden angelegt.

2. CMD wird gemeinsam mit einem der beiden Strobes angelegt (low).

3. Es wird auf das Acknowlegment des Displays gewartet, dies wird durch eine negative Flanke ausgehen vom Display auf dem anderen Strobe erkannt.

4. Eigener Strobe und ggf. CMD wird wieder realeased und der PullUp des Strobes angelegt.

5. Ab jetzt kann schon das nächste Nibble angelegt werden, das Display ist mit der verarbeitung beschäftigt. Der nächste Strobe darf allerdings erst ausgelöst werden wenn das Display seinerseits den Strobe aufgehoben hat.

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
