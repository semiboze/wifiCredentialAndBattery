#ifndef PRIVATE_H
#define PRIVATE_H
#include <Arduino.h> // <-- この行を追加！
// WiFi接続情報
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
extern const char* BLNK_AUTH_TOKEN;  //Blynkの認証トークン
extern const char* HOSTNAME;
extern const char* DeviceName;
// Blynk AUTH TOKENと接続サーバー定義のマクロ
extern IPAddress    SERVER_IP_LOCAL;
extern IPAddress    SERVER_IP_PUBLI;
// SwitchBot用個人データ
extern const char* AirConDeviceId;  // SwitchBot
extern const char* SwitchBotToken;     // SwitchBot APIトークン
//GithubからグローバルIPアドレス取得用個人データ
extern const char* myName;
extern const char* GIST_TOKEN;
#endif