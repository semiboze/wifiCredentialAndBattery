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
