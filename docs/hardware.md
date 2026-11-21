# Terminal hardware

The ATmega328P reads badges and keys. The ESP8266 handles the display, clock and event storage. [Project overview](../README.md) · [Interface gallery](interface.md)

## Hardware photos

[![Terminal circuit board and blue-and-red keypad joined by a flat ribbon cable](../media/terminal-board-and-keypad.png)](../media/terminal-board-and-keypad.png)

*Terminal board with its 4 × 4 keypad.*

[![Upright RFID antenna board beside the shielded ESP8266 module](../media/terminal-rfid-reader.png)](../media/terminal-rfid-reader.png)

*Close-up of the RFID reader and ESP8266 Wi-Fi module.*

## Front panel

[![Terminal front with the 20 × 4 LCD above a 4 × 4 keypad and badge target. A detail shows the card, plastic face and reader antenna in order.](../media/terminal-front.png)](../media/terminal-front.png)

When the display reads `Zeskanuj tag RFID`, the employee presents a badge to the marked area to the right of the keypad. The detail shows the antenna directly behind that area, parallel to the front. In the event menu, **4** and **6** move between entries, **A** saves the selection and **C** cancels. [Employee workflow](../libraries/EmployeeManager/EmployeeManager.cpp) · [LCD sequence](interface.md#terminal-display)

## Controller connections

[![Signal map from the keypad and SPI badge reader through the ATmega328P and serial link to the ESP8266, I²C display and clock, and SPI storage.](../media/controller-connections.svg)](../media/controller-connections.svg)

Two peripherals use SPI on separate processors. The ATmega328P selects the reader with **D9**. The ESP8266 selects the SD card with **GPIO16**. The arrow shows badge and keypad events travelling from the input controller to the ESP8266.

| Controller | Peripheral | Signals in the firmware |
| --- | --- | --- |
| ATmega328P | Matrix keypad | Group lines `A5`, `A4`, `A3`, `A2`. Sense lines `D4`, `D5`, `D6`, `D7`. |
| ATmega328P | RFID reader | SPI with chip select on `D9` and reset on `D8`. |
| ESP8266 | 20 × 4 character LCD | I²C address `0x3F`. |
| ESP8266 | DS3231 clock | I²C. The firmware can recover local time from the clock when network time is unavailable and the RTC has retained power. |
| ESP8266 | SD card | SPI with chip select on `GPIO16`. |

[Keypad and reader configuration](../libraries/MainController2/MainController2.cpp) · [LCD initialization](../libraries/Display145617/Display145617.cpp) · [SD and RTC initialization](../libraries/MainController145617/MainController145617.cpp) · [Clock update and recovery](../libraries/TaskUpdateRTC/TaskUpdateRTC.cpp)
