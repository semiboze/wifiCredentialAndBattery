#ifndef PRIVATE_H
#define PRIVATE_H

// WiFi接続情報
#define WIFI_SSID       "YourSSID"
#define WIFI_PASSWORD   "YourPassword"
#define BLY
NK_AUTH_TOKEN  "YourBlynkToken"  //Blynkの認証トークン
#define AUTH BLYNK_AUTH_TOKEN
#define HOSTNAME "LiFePO4SOC"
#define DeviceName "バッテリーモニター"
// Blynk AUTH TOKENと接続サーバー定義のマクロ
#define SERVER_IP_LOCAL IPAddress(192,168,0,81)
#define SERVER_IP_PUBLI IPAddress(1,1,1,1)
// SwitchBot用個人データ
const char* AirConDeviceId = "YourSwitchBotDeviceID";  // SwitchBot
const char* SwitchBotToken = "YourSwitchBotToken";                    // SwitchBot APIトークン
//GithubからグローバルIPアドレス取得用個人データ
#endif