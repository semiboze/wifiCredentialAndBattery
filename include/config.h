
#define BLYNK_TEMPLATE_ID "YourTemplateID"
#define BLYNK_TEMPLATE_NAME "YourTemplateName"

#if defined(ESP8266)
  #include <ESP8266HTTPClient.h>
  #include <BlynkSimpleEsp8266.h>
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266HTTPClient.h>
  #include <ESP8266WebServer.h>
  #include <WiFiClientSecureBearSSL.h>
  // const char* BOARD_TYPE = "ESP8266";
  // const int SOLAR_VOLTAGE_PIN = A0;
  // const int SOLAR_CURRENT_PIN = A0;
  // const int I2C_SDA_PIN = 4;
  // const int I2C_SCL_PIN = 5;
#else
  #include <HTTPClient.h>
  #include <ESP32WebServer.h>
  #include <BlynkSimpleEsp32.h>
  #include <WiFi.h>
  // #include <ESPmDNS.h>
  #include <HTTPClient.h>
  #include <WiFiClientSecure.h>
  #define LED_BUILTIN 2
  // const char* BOARD_TYPE = "ESP32";
  // const int SOLAR_VOLTAGE_PIN = 35;
  // const int SOLAR_CURRENT_PIN = 34;
#endif

#ifdef DEBUG_MODE
  #define DEBUG_PRINT(...)     Serial.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...)   Serial.print(__VA_ARGS__)
  #define DEBUG_PRINTF(fmt, ...)  Serial.printf(fmt, ##__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTLN(...)
  #define DEBUG_PRINTF(...)
#endif

#define JST     3600* 9
#include <INA226_WE.h>
extern INA226_WE ina226;

// Virtual Pinの定義
#define VPIN_TIME           V10
#define VPIN_MAC            V50
#define VPIN_SOLAR_VOLTAGE  V1
#define VPIN_SOLAR_CURRENT  V2
#define VPIN_SOLAR_POWER    V3
#define VPIN_BATT_VOLTAGE   V4
#define VPIN_BATT_CURRENT   V5
#define VPIN_BATT_POWER     V6
// タイマー・遅延設定
#define TIMER_INTERVAL  5000L

// --- センサー設定 ---
#define  R1  330000.0
#define R2  10000.0
#define ACS_ZERO_CURRENT_VOLTAGE  1.65
#define ACS_SENSITIVITY  0.040
#define SHUNT_RESISTANCE_OHMS  0.00075
#define MAX_EXPECTED_CURRENT_AMPS  100.0

#if defined(ESP8266)
  #define BOARD_TYPE  "ESP8266"
  #define SOLAR_VOLTAGE_PIN  A0
  #define SOLAR_CURRENT_PIN  A0
  #define I2C_SDA_PIN  4
  #define I2C_SCL_PIN  5
#else
 #define BOARD_TYPE  "ESP32"
  #define SOLAR_VOLTAGE_PIN  35
#define SOLAR_CURRENT_PIN  34
#endif
