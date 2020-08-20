# Telnet to Serial

This project was derived from Examples->ESP8266WiFi->WiFiTelnetToSerial.

Code is running on an ESP-01.

I use this project to attach to headless Raspberry Pi's and Odroid SBC's when I need to troubleshoot and I can't ssh into them.
I swapped only the TX pin. This ensures that ESP-01 bootup characters do not get injected into the Serial TX which would honk up the login.

## Pin connections

I use jumper wires to connect the ESP-01 to the SBC serial port.

| ESP-01 | RPi | Func |
| :-----: | :---: | :----: |
| 1 | 6 | GND |
| 3 | 8 | TX |
| 7 | 10 | RX |
| 8 | 1 | 3v3 |


It might help to switch to character mode once you have stared up the telnet program.

^]

telnet> mode character

...proceed with telnet session

^]

telnet> quit
