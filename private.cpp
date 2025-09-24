#define BLYNK_AUTH_TOKEN  "J3p3OLfCU5HHfnHhRanNxUDNo3IXShHd"  //充電監視用テスト

#define AUTH BLYNK_AUTH_TOKEN
#define HOSTNAME "LiFePO4SOC"
#define DeviceName "バッテリーモニター"

// Blynk AUTH TOKENと接続サーバー定義のマクロ
#define SERVER_IP_LOCAL IPAddress(192,168,0,81)
#define SERVER_IP_PUBLI IPAddress(1,1,1,1)

// SwitchBot用個人データ
const char* AirConDeviceId = "01-202208061507-09823935";  // SwitchBotのデバイスID（霧ヶ峰）
const char* SwitchBotToken = "b6835e43fa54399951b40555ced267610171c88345e90253b96345d44da72cdbdcf81b881ceda71e44578f9450e28344";                    // SwitchBot APIトークン

//GithubからグローバルIPアドレス取得用個人データ
const char* myName = "semiboze";
const char* GIST_TOKEN = "1d72bba13ceac5e805953f1850b4019b";
