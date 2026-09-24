#pragma once
#ifndef DEFINITIONS_H
#define DEFINITIONS_H


#define MY_DEVICE_NAME     "KC868-A8 Sterownik nawodnienia"
#define MY_WIFI_NAME "SUPLA-KC868-A8"

// Definiowanie pinów dla chipu LAN8720 (KC868-A8)
#define ETH_ADDR        0
#define ETH_POWER_PIN  13
#define ETH_MDC_PIN    23
#define ETH_MDIO_PIN   18
#define ETH_TYPE       ETH_PHY_LAN8720
#define ETH_CLK_MODE   ETH_CLOCK_GPIO17_OUT

// Definicje I2C dla PCF8574
#define I2C_SDA 4
#define I2C_SCL 5

// krzycisk sprzetowy wejscia w konfiguracje
#define BUTTON_CFG_RELAY_GPIO 0

// wybierz ekran
// ************************************************************
//#define USE_SSD1306
#define USE_SH1106G
// ************************************************************
#define SCREEN_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 // Brak pinu resetu w większości ekranów I2C
#define OLED_OFF 0xAE
#define OLED_ON 0xAF
#define DIM_TIME 60000
#define OFF_TIME 240000
#define EDIT_MODE_EXIT_TIME 10000
#define SENSOR_PAGE_TIME 5000
#define DISPLAY_INTERVAL 250

// czas wyłączenia pompy przed zamknięciem zaworu
#define PUMP_ADVANCE_OFF_TIME_MS 3000   

#define DEBUG

#ifdef DEBUG
  #define MS_IN_MIN 1000
#else
  #define MS_IN_MIN 60000
#endif

#endif // DEFINITIONS_H