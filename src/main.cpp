// #define _BLYNK_VALID // Blynk有効有無デバッグ

#define DEBUG_MODE

#include "private.h"
// #include "history.h"
#include "config.h"

// #define wifiLed 2
#define BLYNK_TEMPLATE_ID "YourTemplateID"
#define BLYNK_TEMPLATE_NAME "YourTemplateName"

// ライブラリのインクルード
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <Adafruit_INA226.h> // <-- 1. ライブラリをインクルード
#include <INA226_WE.h>
#include <time.h>
#if defined(ESP8266)
  #include <ESP8266HTTPClient.h>
  #include <BlynkSimpleEsp8266.h>
  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266HTTPClient.h>
  #include <ESP8266WebServer.h>
  #include <WiFiClientSecureBearSSL.h>
  ESP8266WebServer server(80);
  const char* BOARD_TYPE = "ESP8266";
  const int SOLAR_VOLTAGE_PIN = A0;
  const int SOLAR_CURRENT_PIN = A0;
  const int I2C_SDA_PIN = 4;
  const int I2C_SCL_PIN = 5;
  #define LED_BUILTIN 2
#else
  #include <HTTPClient.h>
  #include <ESP32WebServer.h>
  #include <BlynkSimpleEsp32.h>
  #include <WiFi.h>
  // #include <ESPmDNS.h>
  #include <HTTPClient.h>
  #include <WiFiClientSecure.h>
  #define LED_BUILTIN 2
  ESP32WebServer server(80);
  const char* BOARD_TYPE = "ESP32";
  const int SOLAR_VOLTAGE_PIN = 35;
  const int SOLAR_CURRENT_PIN = 34;
#endif

// Virtual Pinの定義
#define VPIN_TIME           V10
#define VPIN_MAC            V50
#define VPIN_SOLAR_VOLTAGE  V1
#define VPIN_SOLAR_CURRENT  V1
#define VPIN_SOLAR_POWER    V2
#define VPIN_BATT_VOLTAGE   V3
#define VPIN_BATT_CURRENT   V4
#define VPIN_BATT_POWER     V5
// タイマー・遅延設定
const long TIMER_INTERVAL = 5000L;


// --- センサー設定 ---
const float R1 = 330000.0;
const float R2 = 10000.0;
const float ACS_ZERO_CURRENT_VOLTAGE = 1.65;
const float ACS_SENSITIVITY = 0.040;
const float SHUNT_RESISTANCE_OHMS = 0.00075;
const float MAX_EXPECTED_CURRENT_AMPS = 100.0;

// グローバル変数
BlynkTimer timer;
volatile bool periodTaskTrigger = false;
INA226_WE ina226 = INA226_WE(0x40);
unsigned long lastConnectionAttempt = 0;
const int connectionDelay = 50000;
int dispflag = 0;
char macStr[18];
char iPadd[24];
int timerId = -1;

// WiFi状態管理
enum WifiState {
  WIFI_STATE_DISCONNECTED,
  WIFI_STATE_CONNECTING,
  WIFI_STATE_CONNECTED,
  WIFI_STATE_AP_MODE
};
WifiState wifiState = WIFI_STATE_DISCONNECTED;
String st;
String content;
int statusCode;
unsigned long ledBlinkTimer = 0;
unsigned long blinkStartTime = 0;
int blinkCount = 0;
bool isBlinking = false;

// LEDステータス管理
enum LedStatus {
  LED_STATUS_WIFI_CONNECTING,      // WiFi接続待ち - 点灯
  LED_STATUS_AP_MODE,              // WiFi接続サーバー起動状態 - 早い点滅
  LED_STATUS_WIFI_CONNECTED,       // WiFi接続安定&Blynk接続待ち - ゆっくり点滅
  LED_STATUS_BLYNK_CONNECTED,      // BLYNK接続安定 - 消灯
  LED_STATUS_OTA_UPDATE,           // OTA更新中 - 非常に早い点滅
  LED_STATUS_NO_CONFIG,            // EEPROM設定なし - 2回点滅繰り返し
  LED_STATUS_WIFI_FAILED,          // WiFi接続失敗 - 3回点滅繰り返し
  LED_STATUS_BLYNK_RECONNECTING    // Blynk再接続中 - 1秒点灯→0.5秒消灯
};
LedStatus currentLedStatus = LED_STATUS_WIFI_CONNECTING;
unsigned long ledStatusTimer = 0;
int ledBlinkCounter = 0;
bool ledState = false;


// 関数プロトタイプ宣言
// WiFi ネットワーク関連
bool fetchIPFromGist(int ipData[4]);// GitHub GistからグローバルIPアドレスを取得する
void createWebServer();// WiFi設定用のWebページを作成し、リクエストに対する処理を定義する
void manageWifiConnection();// WiFiとBlynkの接続状態を管理するメインの関数
void setupAP(void);// WiFi設定用のAPモードを開始し、設定用Webサーバーを起動する
void attemptConnectionFromEEPROM();// EEPROMからSSIDとPWを読み出しWiFi接続を試みる
void setupOTA();

// -- Blynk関連 --
void BlynkConnect();// Blynkサーバーへの接続処理を行う
void aliveReport();// 定期的に実行し生存時刻を報告する
void reconnectBlynk();// Blynkサーバーから切断された場合に再接続を試みる

// -- ハードウェア・センサー制御 --
void IRAM_ATTR onTimer();// 割り込みタイマーによって定期的に実行される関数
void updateLedStatus();// 現在の状態に応じて、本体LEDの点滅パターンを制御する
void setupSensors();//INA226読み出しポートの初期化

// -- ユーティリティ --
int DisplayTime(char *stringsTime, int *int_h, int *int_m, int *int_s);// NTPサーバーから取得した現在時刻を「時:分:秒」の文字列に整形する
void handleBlinking();
void parseMacAddress(const char* macStr, byte* mac);// MACアドレスの文字列をバイト配列に変換する

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // ピン設定
  pinMode(LED_BUILTIN, OUTPUT);
  
  setupSensors();
  // 初期状態設定
  digitalWrite(LED_BUILTIN, HIGH);
  
  // EEPROM初期化
  EEPROM.begin(512);
  
  // WiFi初期化
  WiFi.disconnect(true);
  delay(100);
  
  // 時刻設定
  configTime(JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
  
  setupOTA();

  // タイマー設定
  timer.setInterval(TIMER_INTERVAL, onTimer);
  
  // WiFi接続開始
  attemptConnectionFromEEPROM();
  
  DEBUG_PRINTLN("Setup completed");
}

void loop() {
  // 最優先処理
  ArduinoOTA.handle();
  handleBlinking();
  
  // LED状態更新を追加
  updateLedStatus();
  
  // WiFiとBlynk管理
  manageWifiConnection();
  
  // タイマー実行
  timer.run();
}
  
void setupOTA()
{
    // OTA設定
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    currentLedStatus = LED_STATUS_OTA_UPDATE;  // 追加
    DEBUG_PRINTLN("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINTF("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      DEBUG_PRINTLN("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      DEBUG_PRINTLN("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      DEBUG_PRINTLN("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      DEBUG_PRINTLN("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      DEBUG_PRINTLN("End Failed");
    }
  });
  ArduinoOTA.begin();
}
// WiFi管理関数
void manageWifiConnection() {
  switch (wifiState) {
    case WIFI_STATE_CONNECTING:
      currentLedStatus = LED_STATUS_WIFI_CONNECTING;  // 追加
      
      if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("\nWiFi Connected!");
        DEBUG_PRINT("IP Address: ");
        DEBUG_PRINTLN(WiFi.localIP());
        wifiState = WIFI_STATE_CONNECTED;
        currentLedStatus = LED_STATUS_WIFI_CONNECTED;  // 追加

        #if defined(_BLYNK_VALID)
        BlynkConnect();
        #endif
      } else if (millis() - lastConnectionAttempt > connectionDelay) {
        DEBUG_PRINTLN("Connection Failed. Starting AP mode.");
        WiFi.disconnect();
        currentLedStatus = LED_STATUS_WIFI_FAILED;  // 追加
        delay(2000);  // 失敗状態を2秒表示
        setupAP();
      }
      break;

    case WIFI_STATE_AP_MODE:
      currentLedStatus = LED_STATUS_AP_MODE;  // 追加
      server.handleClient();
      break;

    case WIFI_STATE_CONNECTED:
      if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("WiFi connection lost.");
        wifiState = WIFI_STATE_DISCONNECTED;
        lastConnectionAttempt = millis();
        currentLedStatus = LED_STATUS_WIFI_FAILED;  // 追加
        #if defined(_BLYNK_VALID)
        Blynk.disconnect();
        #endif
      } else {
        #if defined(_BLYNK_VALID)
        if (Blynk.connected()) {
          currentLedStatus = LED_STATUS_BLYNK_CONNECTED;  // 追加
          Blynk.run();
        } else {
          currentLedStatus = LED_STATUS_BLYNK_RECONNECTING;  // 追加
          reconnectBlynk();
        }
        #else
        currentLedStatus = LED_STATUS_WIFI_CONNECTED;  // Blynk無効時
        #endif

        if (periodTaskTrigger) {
          periodTaskTrigger = false;
          periodicTasks();
        }
      }
      break;

    case WIFI_STATE_DISCONNECTED:
      DEBUG_PRINTLN("WiFi is disconnected. Reconnecting...");
      WiFi.reconnect();
      wifiState = WIFI_STATE_CONNECTING;
      lastConnectionAttempt = millis();
      currentLedStatus = LED_STATUS_WIFI_CONNECTING;  // 追加
      break;
  }
}

void setupAP(void) {
  WiFi.mode(WIFI_AP_STA);
  delay(100);

  int n = WiFi.scanNetworks();
  if (n == 0) {
    st = "<option value=''>利用可能なネットワークが見つかりません</option>";
  } else {
    st = "";
    for (int i = 0; i < n; ++i) {
      String ssid = WiFi.SSID(i);
      int rssi = WiFi.RSSI(i);
      bool isEncrypted;
      
      #if defined(ESP8266)
        isEncrypted = (WiFi.encryptionType(i) != ENC_TYPE_NONE);
      #elif defined(ESP32)
        isEncrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
      #endif
      
      st += "<option value='" + ssid + "'>" + ssid + " (" + String(rssi) + "dBm)";
      st += isEncrypted ? " 🔒" : " 🔓";
      st += "</option>";
    }
  }
  delay(100);

  WiFi.softAP("ESP_WiFi_Setup", "");
  createWebServer();
  server.begin();
  wifiState = WIFI_STATE_AP_MODE;
}

void attemptConnectionFromEEPROM() {
  DEBUG_PRINTLN("Reading EEPROM and attempting to connect...");
  String esid = "";
  for (int i = 0; i < 32; ++i) {
    char c = EEPROM.read(i);
    if (c != 0) esid += c;
  }
  String epass = "";
  for (int i = 32; i < 96; ++i) {
    char c = EEPROM.read(i);
    if (c != 0) epass += c;
  }

  esid.trim();
  epass.trim();

  if (esid.length() > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(esid.c_str(), epass.c_str());
    wifiState = WIFI_STATE_CONNECTING;
    lastConnectionAttempt = millis();
    currentLedStatus = LED_STATUS_WIFI_CONNECTING;  // 追加
    DEBUG_PRINT("Attempting to connect to SSID: ");
    DEBUG_PRINTLN(esid);
  } else {
    DEBUG_PRINTLN("No SSID in EEPROM, starting AP mode.");
    currentLedStatus = LED_STATUS_NO_CONFIG;  // 追加
    delay(3000);  // 設定なし状態を3秒表示
    setupAP();
  }
}

void createWebServer() {
  server.on("/", []() {
    IPAddress ip = WiFi.softAPIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    content = "<!DOCTYPE HTML>\r\n<html>";
    content += "<head><meta charset='UTF-8'><title>Wi-Fi Settings</title>";
    content += "<style>body{font-family:Arial,sans-serif;margin:20px;background-color:#f0f0f0;}";
    content += ".container{max-width:500px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
    content += "h2{color:#333;text-align:center;}";
    content += "select,input,button{width:100%;padding:10px;margin:8px 0;border:1px solid #ddd;border-radius:4px;box-sizing:border-box;}";
    content += "button{background-color:#4CAF50;color:white;cursor:pointer;}";
    content += "button:hover{background-color:#45a049;}";
    content += ".refresh-btn{background-color:#008CBA;}";
    content += ".refresh-btn:hover{background-color:#007B9A;}";
    content += "</style></head>";
    content += "<body><div class='container'>";
    content += "<h2>Wi-Fi設定</h2>";
    content += "<p>SoftAP IP: " + ipStr + "</p>";
    content += "<button class='refresh-btn' onclick='refreshNetworks()'>ネットワーク一覧を更新</button>";
    content += "<form method='get' action='setting'>";
    content += "<label for='ssid'>SSID:</label>";
    content += "<select name='ssid' id='ssid' onchange='updateSSID()'>";
    content += "<option value=''>-- ネットワークを選択 --</option>";
    content += st;
    content += "<option value='custom'>-- カスタム入力 --</option>";
    content += "</select>";
    content += "<input type='text' name='custom_ssid' id='custom_ssid' placeholder='カスタムSSIDを入力' style='display:none;'>";
    content += "<br><label for='pass'>パスワード:</label>";
    content += "<input type='password' name='pass' id='pass' placeholder='WiFiパスワードを入力'>";
    content += "<br><input type='submit' value='接続'>";
    content += "</form></div>";
    content += "<script>";
    content += "function updateSSID(){";
    content += "var select=document.getElementById('ssid');";
    content += "var customInput=document.getElementById('custom_ssid');";
    content += "if(select.value==='custom'){customInput.style.display='block';}";
    content += "else{customInput.style.display='none';customInput.value='';}}";
    content += "function refreshNetworks(){";
    content += "window.location.href='/refresh';";
    content += "}";
    content += "</script></body></html>";
    server.send(200, "text/html", content);
  });

  server.on("/refresh", []() {
    // ネットワークスキャンを再実行
    int n = WiFi.scanNetworks();
    if (n == 0) {
      st = "<option value=''>利用可能なネットワークが見つかりません</option>";
    } else {
      st = "";
      // 信号強度でソート（強い順）
      for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
          if (WiFi.RSSI(i) < WiFi.RSSI(j)) {
            // SSID交換は直接できないので、インデックスベースでソート
          }
        }
      }
      
      for (int i = 0; i < n; ++i) {
        String ssid = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);
        bool isEncrypted;
        
        #if defined(ESP8266)
          isEncrypted = (WiFi.encryptionType(i) != ENC_TYPE_NONE);
        #elif defined(ESP32)
          isEncrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        #endif
        
        st += "<option value='" + ssid + "'>" + ssid + " (" + String(rssi) + "dBm)";
        st += isEncrypted ? " 🔒" : " 🔓";
        st += "</option>";
      }
    }
    
    // リダイレクトしてメインページを再表示
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });

  server.on("/setting", []() {
    String qsid = server.arg("ssid");
    String customSsid = server.arg("custom_ssid");
    String qpass = server.arg("pass");
    
    // カスタムSSIDが入力されている場合はそちらを優先
    if (customSsid.length() > 0) {
      qsid = customSsid;
    }
    
    if (qsid.length() > 0 && qpass.length() > 0) {
      DEBUG_PRINTLN("Clearing EEPROM");
      for (int i = 0; i < 96; ++i) {
        EEPROM.write(i, 0);
      }
      DEBUG_PRINTLN("Writing new credentials to EEPROM:");
      DEBUG_PRINTLN(qsid);
      
      for (int i = 0; i < qsid.length(); ++i) {
        EEPROM.write(i, qsid[i]);
      }
      for (int i = 0; i < qpass.length(); ++i) {
        EEPROM.write(32 + i, qpass[i]);
      }
      EEPROM.commit();

      content = "<!DOCTYPE HTML><html><head><meta charset='UTF-8'></head><body>";
      content += "<h2>設定完了</h2><p>WiFi設定が保存されました。デバイスを再起動します...</p>";
      content += "<script>setTimeout(function(){alert('デバイスが再起動されます');},2000);</script>";
      content += "</body></html>";
      server.send(200, "text/html", content);
      delay(2000);
      ESP.restart();
    } else {
      content = "<!DOCTYPE HTML><html><head><meta charset='UTF-8'></head><body>";
      content += "<h2>エラー</h2><p>SSIDまたはパスワードが入力されていません。</p>";
      content += "<a href='/'>戻る</a></body></html>";
      server.send(400, "text/html", content);
    }
  });
}

// Blynk関連関数
void BlynkConnect() {
  #if defined(_BLYNK_VALID)
  IPAddress localIP = WiFi.localIP();
  IPAddress ServerIPAddr;
  String ipString = localIP.toString();
  String leftThreeChars = ipString.substring(0, 3);

  if(leftThreeChars.equals("192")) {
    ServerIPAddr = SERVER_IP_LOCAL;
  } else {
    int ipData[4];
    if(fetchIPFromGist(ipData)) {
      ServerIPAddr = IPAddress(ipData[0], ipData[1], ipData[2], ipData[3]);
      DEBUG_PRINTF("global ip add: %d:%d:%d:%d\n",ipData[0], ipData[1], ipData[2], ipData[3]);
    } else {
      ESP.restart();
    }
  }
  
  for(int i = 0; i < 5; i++) {
    Blynk.config(AUTH, ServerIPAddr, 8080);
    DEBUG_PRINTLN("Blynk.config(AUTH, ServerIPAddr, 8080) executed.");
    DEBUG_PRINTF("AUTH : %s",AUTH);
    DEBUG_PRINTF("     ServerIPAddr : %s\n",ServerIPAddr);

    delay(10);
    if(Blynk.connect()) {
      break;
    }
  }
  #endif
}

void reconnectBlynk() {
  #if defined(_BLYNK_VALID)
  if (Blynk.connect()) {
    DEBUG_PRINTLN("Reconnected to Blynk!");
  } else {
    DEBUG_PRINTLN("Reconnection to Blynk failed.");
  }
  delay(5000);
  #endif
}

// Blynkコールバック関数
#if defined(_BLYNK_VALID)
BLYNK_CONNECTED() {
  Blynk.syncAll();
  delay(1);
  aliveReport();
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  IPAddress ipaddr = WiFi.localIP();
  sprintf(iPadd, "%d.%d.%d.%d", ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
  
  Blynk.setProperty(VPIN_MAC, "offLabel", DeviceName);
  Blynk.setProperty(VPIN_TIME, "offLabel", String("V") + FirmwareVersion);
  dispflag = 0;
  Blynk.virtualWrite(VPIN_MAC, 0);
  delay(1);
}

BLYNK_WRITE(VPIN_MAC) {
  DEBUG_PRINTLN("VPIN_MAC pushed.")
  if (timerId != -1) {
    timer.deleteTimer(timerId);
  }
  
  switch (dispflag) {
    case 0:
      Blynk.setProperty(VPIN_MAC, "offLabel", DeviceName);
      break;
    case 1:
      Blynk.setProperty(VPIN_MAC, "offLabel", String("V") + FirmwareVersion);
      break;
    case 2:
      Blynk.setProperty(VPIN_MAC, "offLabel", iPadd);
      break;
    case 3:
      Blynk.setProperty(VPIN_MAC, "offLabel", macStr);
      break;
  }
  if(dispflag >= 3){
    dispflag = 0;
  }else{
    dispflag ++;
  }

  timerId = timer.setTimeout(5000, []() {
    dispflag = 0;
    Blynk.setProperty(VPIN_MAC, "offLabel", DeviceName);
    delay(1);
  });
}

#endif

// タイマー関数
void IRAM_ATTR onTimer() {
  periodTaskTrigger = true;
}

// 生存報告関数
void aliveReport() {
  #if defined(_BLYNK_VALID)
  char str[30];
  if(!Blynk.connected()) {
    return;
  }
  
    int int_h, int_m, int_s;
    int sec = DisplayTime(str, &int_h, &int_m, &int_s);
    
    Blynk.setProperty(VPIN_TIME, "offLabel", str);
    DEBUG_PRINTLN("VPIN_TIME writed.")

    delay(1);
    
    if (sec % 30 < TIMER_INTERVAL / 1000) {  
      String colorText;
      sec >= 30 ? colorText = "#00FFFF" : colorText = "#FFA500";
      Blynk.setProperty(VPIN_TIME, "offBackColor", colorText.c_str());
      delay(1);
    }
  #endif
}

void handleBlinking() {
  if (!isBlinking) {
    return;
  }

  if (millis() - blinkStartTime >= 100) {
    blinkStartTime = millis();
    blinkCount--;
    if (blinkCount <= 0) {
      isBlinking = false;
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
  }
}

// 時刻表示関数
int DisplayTime(char *stringsTime, int *int_h, int *int_m, int *int_s) {
  struct tm tminfo;
  
  if (!getLocalTime(&tminfo)) {
    sprintf(stringsTime, "00:00:00");
    *int_h = 0; *int_m = 0; *int_s = 0;
    return 0;
  }
  
  *int_h = tminfo.tm_hour;
  *int_m = tminfo.tm_min;
  *int_s = tminfo.tm_sec;
  sprintf(stringsTime, "%02d:%02d:%02d", tminfo.tm_hour, tminfo.tm_min, tminfo.tm_sec);
  return tminfo.tm_sec;
}

bool fetchIPFromGist(int ipData[4]) {
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN("WiFi not connected.");
    return false;
  }

  #if defined(ESP8266)
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure());
  client->setInsecure();
  HTTPClient http;
  String url = "https://gist.githubusercontent.com/" + String(myName) + "/" + GIST_TOKEN + "/raw/global_ip.txt";
  http.begin(*client, url.c_str());
  #else
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url = "https://gist.githubusercontent.com/" + String(myName) + "/" + GIST_TOKEN + "/raw/global_ip.txt";
  http.begin(client, url.c_str());
  #endif

  int httpCode = http.GET();
  if (httpCode != 200) {
    DEBUG_PRINTF("HTTP GET failed: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }

  String ip = http.getString();
  ip.trim();
  DEBUG_PRINTF("Fetched IP: %s\n", ip.c_str());

  int start = 0;
  for (int i = 0; i < 4; i++) {
    int end = ip.indexOf('.', start);
    String part;

    if (i < 3) {
      if (end == -1) {
        DEBUG_PRINTLN("Invalid IP format (dot missing).");
        http.end();
        return false;
      }
      part = ip.substring(start, end);
      start = end + 1;
    } else {
      part = ip.substring(start);
    }

    ipData[i] = part.toInt();
    if (ipData[i] < 0 || ipData[i] > 255) {
      DEBUG_PRINTF("Invalid IP segment: %d\n", ipData[i]);
      http.end();
      return false;
    }
  }

  http.end();
  return true;
}

void parseMacAddress(const char* macStr, byte* mac) {
  sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}
// LED制御関数
void updateLedStatus() {
  unsigned long currentTime = millis();
  
  switch (currentLedStatus) {
    case LED_STATUS_WIFI_CONNECTING:
      // WiFi接続待ち - 常時点灯
      digitalWrite(LED_BUILTIN, LOW);  // LOW = 点灯
      break;
      
    case LED_STATUS_AP_MODE:
      // WiFi接続サーバー起動状態 - 早い点滅（100ms間隔）
      if (currentTime - ledStatusTimer > 300) {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
        ledStatusTimer = currentTime;
      }
      break;
      
    case LED_STATUS_WIFI_CONNECTED:
      // WiFi接続安定&Blynk接続待ち - ゆっくり点滅（1秒間隔）
      if (currentTime - ledStatusTimer > 1000) {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
        ledStatusTimer = currentTime;
      }
      break;
      
    case LED_STATUS_BLYNK_CONNECTED:
      // BLYNK接続安定 - 消灯
      digitalWrite(LED_BUILTIN, HIGH);  // HIGH = 消灯
      break;
      
    case LED_STATUS_OTA_UPDATE:
      // OTA更新中 - 非常に早い点滅（50ms間隔）
      if (currentTime - ledStatusTimer > 50) {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
        ledStatusTimer = currentTime;
      }
      break;
      
    case LED_STATUS_NO_CONFIG:
      // EEPROM設定なし - 2回点滅繰り返し
      if (currentTime - ledStatusTimer > 200) {
        if (ledBlinkCounter < 4) {  // 2回点滅 = 4回の状態変化
          ledState = !ledState;
          digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
          ledBlinkCounter++;
        } else if (ledBlinkCounter < 20) {  // 1秒間休止
          digitalWrite(LED_BUILTIN, HIGH);
          ledBlinkCounter++;
        } else {
          ledBlinkCounter = 0;  // リセットして繰り返し
        }
        ledStatusTimer = currentTime;
      }
      break;
      
    case LED_STATUS_WIFI_FAILED:
      // WiFi接続失敗 - 3回点滅繰り返し
      if (currentTime - ledStatusTimer > 200) {
        if (ledBlinkCounter < 6) {  // 3回点滅 = 6回の状態変化
          ledState = !ledState;
          digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
          ledBlinkCounter++;
        } else if (ledBlinkCounter < 20) {  // 1秒間休止
          digitalWrite(LED_BUILTIN, HIGH);
          ledBlinkCounter++;
        } else {
          ledBlinkCounter = 0;  // リセットして繰り返し
        }
        ledStatusTimer = currentTime;
      }
      break;
      
    case LED_STATUS_BLYNK_RECONNECTING:
      // Blynk再接続中 - 1秒点灯→0.5秒消灯
      if (ledState && (currentTime - ledStatusTimer > 1000)) {
        ledState = false;
        digitalWrite(LED_BUILTIN, HIGH);
        ledStatusTimer = currentTime;
      } else if (!ledState && (currentTime - ledStatusTimer > 500)) {
        ledState = true;
        digitalWrite(LED_BUILTIN, LOW);
        ledStatusTimer = currentTime;
      }
      break;
  }
}
// =============================================================================
// 8. 定期実行タスク (5秒ごとに実行)
// =============================================================================
void periodicTasks() {
  Serial.println("\n----- 定期タスク実行 -----");
  measureSolar();
  measureBattery();
  // ここに元の`aliveReport`の他の処理（時刻表示更新など）を移植
  aliveReport();
}