# Supla-Sprinklers-for-KC868-A8
My own implementation of ESP32 firmware for Kincony KC868-A8 using SuplaDevice for my personal requirements.

Basic description:
* device controls:
  * pump
  * 7 valves
  * light relay (with switch)
  * extra button with action trigger for another light controled by another Supla device
  * temperature sensors (including RTC temperature sensor)
  * door sensor
  * 2 water level sensors
  * RTC module
  * SSH1106 OLED display
* ESP32, Supla Device, Adafruit, SSD1306/SH1106 libraries used
* works offline as well (RTC used)
* support for Wifi & LAN
* Supla configuration mode & OTA
* telnet log feature

Technical information:
* pump & 7 valve relays controled via pcf8574 onboard expander
* used another digital inputs (DI) for:
  * DS1820 1-wire sensors connected to input (nr 1)
  * LAN8720 reset pin (nr 2) - pin needs to by connected to specified place on board (see images folder)
  * light & extra external relays (nr 3 & 4)
* support for OLED display (DS1106 recomended, SSD1306), custom view for indicators & actions
* set short/long/both program for each valve separately (controled via app & device edit mode)
* schedule hour and autostart (controled via app & device edit mode)
* empty & refill tank actions working with sensors (+controled via app)
* valve with & without pump support (flags in relay constructors need to altered if required)
* cloud supla controls: pump, valves (7), water tank level indicator (3 levels), run now & schedule program action virtual relays (2x vr), 5x2 program setters (10x vr), all off killswitch (vr), light (extra relay is needed), timed light (for physical button connected to another device - timed light event), extra power switch (another extra relay needed), temperature sensors, door sensor, second light action handler, water level sensors (2), general measurment channel (sends codes for push messages cloud handler)


