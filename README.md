# Arduino-Smart-Relay-with-Display
A smart relay for the Arduino that allows programming of 10 time based alarms and one voltage based alarm (for overvoltage/ under voltage protection). Also uses DS1307 RTC to keep time, and allows the user to reset the time if the RTC loses time.

How to install and use:
1) Get PlatformIO
2) Make a blank Arduino Uno project (name it whatever you want)
3) Download main.cpp and platformio.ini from this repostiory
4) Replace the main.cpp file in the blank project with the downloaded one. Do the same for platformio.ini
5) Build, upload and use!

Bill of materials (approximate)
1) 1x Arduino Uno
2) 1x 1602 LCD display
3) 1x IIC I2C TWI SPI Serial Interface Board Module Port For Arduino LCD1602 Display (similar to https://rees52.com/arduino-compatible-modules/447-iic-i2c-twi-spi-serial-interface-board-module-port-for-arduino-lcd1602-display-aa134)
4) 6 suitably sized buttons
5) TinyRTC or similar DS1307 based realtime clock module
6) 5V relay module
7) Variable resistor for the voltage divider at the input to A0 (where voltage is measured)
8) Suitable resistors (e.g. 1k ohm) and capacitors (e.g. 100uF) to provide suitable current draws from the Arduino and do power smoothing, respectively
9) Cover for the entire system
