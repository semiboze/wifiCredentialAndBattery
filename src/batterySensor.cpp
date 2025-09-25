#include "config.h"       // これを必ず一番上に置きます
#include <Arduino.h>
#include "private.h"
#include <INA226_WE.h>

#include <Blynk/BlynkApi.h>
extern BlynkWifi Blynk;

void setupSensors() {
  #if defined(ARDUINO_ARCH_ESP8266)
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  #else
    Wire.begin();
  #endif
  
  if (!ina226.init()) {
    Serial.println("エラー: INA226デバイスに接続できませんでした。");
  } else {
    ina226.setResistorRange(SHUNT_RESISTANCE_OHMS, MAX_EXPECTED_CURRENT_AMPS);
    Serial.println("INA226デバイスに接続しました。");
  }
  
}
// =============================================================================
// 9. センサー測定関数
// =============================================================================
void measureSolar() {
  int solar_v_adc = analogRead(SOLAR_VOLTAGE_PIN);
  float pin_v = solar_v_adc * (3.3 / (strcmp(BOARD_TYPE, "ESP32") == 0 ? 4095.0 : 1023.0));
  float solar_voltage = pin_v * ((R1 + R2) / R2);

  int solar_c_adc = analogRead(SOLAR_CURRENT_PIN);
  float acs_v = solar_c_adc * (3.3 / (strcmp(BOARD_TYPE, "ESP32") == 0 ? 4095.0 : 1023.0));
  float solar_current = (acs_v - ACS_ZERO_CURRENT_VOLTAGE) / ACS_SENSITIVITY;
  if (solar_current < 0.2) solar_current = 0.0;
  
  float solar_power = solar_voltage * solar_current;

  Serial.printf("[SOLAR]   V: %.2f V | A: %.2f A | P: %.2f W\n", solar_voltage, solar_current, solar_power);

  Blynk.virtualWrite(VPIN_SOLAR_VOLTAGE, solar_voltage);
  Blynk.virtualWrite(VPIN_SOLAR_CURRENT, solar_current);
  Blynk.virtualWrite(VPIN_SOLAR_POWER, solar_power);
}

void measureBattery() {
  float battery_voltage = ina226.getBusVoltage_V();
  float battery_current = ina226.getCurrent_A();
  float battery_power = ina226.getBusPower();

  Serial.printf("[BATTERY] V: %.2f V | A: %.2f A | P: %.2f W\n", battery_voltage, battery_current, battery_power);

  Blynk.virtualWrite(VPIN_BATT_VOLTAGE, battery_voltage);
  Blynk.virtualWrite(VPIN_BATT_CURRENT, battery_current);
  Blynk.virtualWrite(VPIN_BATT_POWER, battery_power);
}
