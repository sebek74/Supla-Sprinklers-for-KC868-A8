# Supla-Sprinklers-for-KC868-A8
My own implementation of ESP32 firmware for Kincony KC868-A8 using SuplaDevice

Basic desctiption:
* Supla Device library used
* works offline (registration & datetime synchronization at start is needed)
* support for pump & 7 valves via pcf8574
* 8 inputs (via pcf8574): pump switch (hold=all off, physical button), edit mode switch (physical button), edit value switch (physical button), door sensor, 2 light switches (physical buttons), 2 tank water levels
* uses another digital inputs (DI) for Dallas ds1820 (pin1), and 2 external 3.3V relays (pin3,4) (light + extra device in future)
* support for OLED display (DS1106 recomended, SSD1306), all actions, settings & sensor indicators
* set short/long/both program for each valve separately (controled via app & device edit mode)
* schedule hour and autostart (controled via app & device edit mode)
* empty & refill tank actions working with sensors (+controled via app)
* valve with & without pump support (flags in relay constructors need to altered if required)
* cloud supla controls: pump, valves (7), water tank level indicator (3 levels), run now & schedule program action virtual relays (2x vr), 5x2 program setters (10x vr), all off killswitch (vr), light (extra relay is needed), timed light (for physical button connected to another device - timed light event), extra power switch (another extra relay needed), temperature sensors, door sensor, second light action handler, water level sensors (2), general measurment channel (sends codes for push messages cloud handler)
* Supla configuration mode & OTA
* support for Wifi & LAN
