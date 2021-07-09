
//2021/1/11 20:50 Firebaseへの接続成功！！！！
//ライブラリ一つでこんなに簡単に接続出来るなら、これからはM5StickCの方が良いってことになるなあ！！
//内蔵バッテリーでFirebase に値が飛ぶのは、ちょっと感動します！

//1/12 複数データの取り扱いに難儀をしたが、json形式での送り方がようやくわかった。
//どこの国のお方かわからないが、https://www.fernandok.com/2020/07/automacao-esp32-e-dht22.html　が大変役に立った。
//他サイトでは、いまいち理解出来なかった。コードの切り分け方も大変おしゃれである。今後の参考にしたい。

//Web上で複数デバイスを想定したオンオフ表示に成功。藤田学園にて

//#include <M5StickC.h>
//#include <WiFi.h>

#include <M5StickC.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <IOXhop_FirebaseESP32.h>  //ライブラリ https://github.com/ioxhop/IOXhop_FirebaseESP32　
#include "ArduinoJson.h"

#include <time.h>
//#define JST 3600*9
#define JST 0

WiFiClientSecure httpsClient;

int input1=36;
int input2=26;
int input3=0;

int sw=0;
int before_sw;
int beforeinput=99;

boolean swChange = false;
long swStartMills=0; //前回実行の時間を格納する。

long longBeforeconnect = 0 ; //６０秒ごとにWifiのコネクト状態を確認する

//===機械の設定====================================================  
//const char* NowLine="MC024"; //ターゲットとする機械番号（ハイフン等は入れない）
//String MachineNo = "LN034";  //機械番号を定数として入力しておく。
String MachineNo = "LNB035";  //
//String MachineNo = "MC031";  //
//String MachineNo = "GT999";  //
//String MachineNo = "GH002";  //
//String MachineNo = "MC028";  //
//String MachineNo = "MC037";  //
//String MachineNo = "EX124";  //
//String MachineNo = "MC037";  //
//福島工場
//String MachineNo = "MC010";  //
//String MachineNo = "MC011";  //
//String MachineNo = "MC009";  //
//String MachineNo = "MC013";  //
//===WiFi設定===================================================================
//#define WIFI_SSID "GlocalMe_88440" // ①
//#define WIFI_PASSWORD "85533446"

//#define WIFI_SSID "logitec54" // ①
//#define WIFI_PASSWORD "614G2546DH227"
//////
//const char* WIFI_SSID = "106F3F33EF29";
//const char* WIFI_PASSWORD = "fxfm5whgjnnfe";

//const char* WIFI_SSID = "nishio";
//const char* WIFI_PASSWORD = "0563522221";

const char* WIFI_SSID = "B_IoT";
const char* WIFI_PASSWORD = "wF7y82Az";

//福島工場
//const char* WIFI_SSID = "AP58278CC592A0";
//const char* WIFI_PASSWORD = "20418600809093";

//const char* WIFI_SSID = "B0C7456838C7_G";
//const char* WIFI_PASSWORD = "2QCJZGEGERRK";
//===Firebase==================================================================

//#define FIREBASE_DB_URL "https://n-iot-a25db.firebaseio.com/" // 
#define FIREBASE_DB_URL "https://ay-vue.firebaseio.com/" // 
//#define FIREBASE_DB_URL "https://iot-sandbox-ea132.firebaseio.com/" // 

String user_path = "SP_Status";
String user_path2 = "NishioMachineCT";

//====Slack===============================================================================

//2021/2/1 slackにちゃんと飛ばせることを確認した！

const char* host_slack="hooks.slack.com";
String AreaName="";
boolean Andon[2]={false,false}; //あんどん起動中は、true
long AndonTime[2]={0,0};   //前回のあんどん作動時間の記録


//slackの設定が変わったと思われる。-----------------------------------------------------------------------------------
//以下で、Incoming Webhookというものを、一つ一つに設定していくことが必要みたい。
//https://w1554440553-fih604877.slack.com/services/2143440998752?updated=1

//以下が参考になると思われる
//https://qiita.com/kshibata101/items/0e13c420080a993c5d16

//slack 呼び出しグループの指定 slack
const char* SlackWebhookURL = "https://hooks.slack.com/services/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";
//const char* SlackWebhookURL = "https://hooks.slack.com/services/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";
//↑以前は、この下側のアドレスで長らく問題なかったのだが・・・、2021/5/31久しぶりに使ってみたら、動かずにこの方法がわかった。
//----------------------------------------------------------------------------------------------------------------

// SSL Certificate finngerprint for the host


const char* fingerprint = "fingerprint";

String message; //送信するチャンネルの指定
//String slack_channel_Name="ap_仕上ハンド";
String slack_channel_Name="webhook_test";
//String slack_channel_Name="hf_フレーム溶接";

//===プログラムで使う変数==================================================================

time_t nowTime;
String startTime = "999";

//WiFiMulti WiFiMulti;
int count = 1;  // ③

boolean ErrBool = false ;  //ERR1でtrurとする　○秒以下では処理を行わない、の為の実装
boolean RunBool = false ;  //RUN1でtrueとする　○秒以下では処理を行わない、の為の実装

boolean SetKKT = false ;  //BtnA １秒以上の長押しで!(否定演算子)で反転させる。

//=======================================================================================
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFC9F
//=======================================================================================
//slackへの送信

void slack_connect(int NowAreaNo){
    String channel;
    String username;
    String PostData;
    
    switch(NowAreaNo){
      case 0:
            channel = slack_channel_Name;
            username = "HPT-1";
            PostData="payload={\"channel\": \"" + channel + "\", \"username\": \"" + username + "\", \"text\": \"" + message + "\", \"icon_emoji\": \":space_invader:\"}";
            break;
      
      case 1:
            channel = slack_channel_Name;
            username = "HPT-2";
            PostData="payload={\"channel\": \"" + channel + "\", \"username\": \"" + username + "\", \"text\": \"" + message + "\", \"icon_emoji\": \":snail:\"}";
            break;
            
      case 2:
            channel = slack_channel_Name;
            username = "HPT-3";
            PostData="payload={\"channel\": \"" + channel + "\", \"username\": \"" + username + "\", \"text\": \"" + message + "\", \"icon_emoji\": \":tiger:\"}";
            break;
    }



    Serial.print("connecting to ");
    Serial.println(host_slack);

// create a secure connection using WiFiClientSecure
    WiFiClientSecure client;
    const int httpPort = 443;
    if (client.connect(host_slack, httpPort)) {

// verify the signature of the ssl certificate
    if (client.verify(fingerprint, host_slack)) {
      Serial.println("ssl cert matches");
    } else {
      Serial.println("ssl cert mismatch");
    }
    delay(10);

  Serial.println(PostData);
  client.print("POST ");
  client.print(SlackWebhookURL);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(host_slack);
  client.println("User-Agent: ArduinoIoT/1.0");
  client.println("Connection: close");
  client.println("Content-Type: application/x-www-form-urlencoded;");
  client.print("Content-Length: ");
  client.println(PostData.length());
  client.println();
  client.println(PostData);
  //このdelayはメチャクチャ大事　20190426---------------------------------------------
  //繰り返しの検証により、300以上じゃないと、slackに送信出来ていないことがあることが判明した。
  //時々slackに送信できていないのは、ここのDELAYかもしれないと、300から500に増量した。2019/05/09 5/17
  delay(500);
  //やっぱりもう一回トライしてみる。
  //delay(1);
  
  //--------------------------------------------------------------------------------
  // Read all the lines of the reply from server and print them to Serial for debugging
  /*確認処理、やめちゃおうか！？ 2019 05 17
    while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
  */
    Serial.println();
    Serial.println("closing connection"); 

    return; 
  }
}
//=======================================================================================
//Andon 作動

void Andon_ON(int AreaNo){

    //digitalWrite(output2,HIGH);
    switch(AreaNo){
      case 0:
        AreaName = "【未使用】";
        break;
      case 1:
        AreaName = "【★HF_ボディ★】";
        break;
      case 2:
        AreaName= MachineNo + "【❏wifi再接続❏】";
        break;
    }
    message= AreaName + "が君を呼んでいる！";
    //digitalWrite(output2,LOW);
    Andon[AreaNo]=true;
    AndonTime[AreaNo]=millis();

    delay(10);
    Serial.println("☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆☆");
    slack_connect(AreaNo);  
    
    delay(10);
           
    Andon[AreaNo]=true;
    //digitalWrite(output2,LOW);

}

//=======================================================================================
//
                


//=======================================================================================
//Firebaseへの送信

void sendToFirebase(String NowMachine,String NowStatus){
  digitalWrite(10,LOW); //内蔵LEDを点灯
  time_t t;
  t = time(NULL);
//  t = String(t) + "000"
  Serial.println("■■■■■■■■■■■■■■■■■■■■■■■");
  Serial.println(String(t) + "000");
  
  //Buffer para a criação dos jsons
  StaticJsonBuffer<150> jsonBufferSensor; 
  JsonObject& sensorData = jsonBufferSensor.createObject(); 
  
  sensorData["startTime"] = startTime ; //前回の停止時間と同じ（８行下で格納している）
  sensorData["endTime"] = String(t) + "000" ; //送信時点の時間
//  sensorData["endTime"] = String(nowTime) + "000" ; //センサー反応時点にさかのぼった時間 あかんよーわからんわー
                                                      //特にエラー赤色灯の時間が若干短くなってしまうが、堪忍してちょ

//  センサー反応時点の時刻を表示させたいが、ダメだ。不完全。
//  struct tm *tm;
//  static const char *wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};


//  t = time(NULL);
//  t = (String(nowTime) + "000").toInt();
//  tm = localtime(&t);
//  Serial.printf(" %04d/%02d/%02d(%s) %02d:%02d:%02d\n",
//        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
//        wd[tm->tm_wday],
//        tm->tm_hour, tm->tm_min, tm->tm_sec);
//   delay(3000);
//  
  sensorData["machine"] = NowMachine ;
  sensorData["status"] = NowStatus ;
  
  Firebase.push("/NishioMachineCT", sensorData);
//  int cunt=0; 
//  if (Firebase.failed()) {
//      cunt++;
//      delay(500);
//      M5.Lcd.print(".");
//      if(cunt%10==0) {
//        Serial.print("Firebaseが接続できなかったみたい。。。setting /number failed:");
//        Serial.println(Firebase.error()); 
//        Firebase.push("/NishioMachineCT", sensorData);
//
//        M5.Lcd.println("");
//        return;
//      }
//      
////      delay(10000);//１０秒待機 
//     
//  }

  startTime = String(t) +"000";
  jsonBufferSensor.clear();

  digitalWrite(10,LOW); //内蔵LEDを点灯
}

void LcdSet(String Content,int ContentColor){
    switch(ContentColor){
      case 0:
            M5.Lcd.fillScreen(TFT_BLACK);
            break;
      case 1:
            M5.Lcd.fillScreen(TFT_GREEN);
            break;
      case 2:
            M5.Lcd.fillScreen(TFT_RED);
            break;
      case 8:
            M5.Lcd.fillScreen(TFT_PINK);
            break;
      case 9:
            M5.Lcd.fillScreen(TFT_YELLOW);
            break;
    } 
  
//    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setTextColor(TFT_DARKGREEN); //文字色設定(背景は透明)(WHITE, BLACK, RED, GREEN, B
    M5.Lcd.setCursor(10, 1, 1);
    M5.Lcd.setTextSize(3);//文字の大きさを設定（1～7）
    M5.Lcd.print(MachineNo);
    M5.Lcd.setCursor(10, 30);
    M5.Lcd.setTextSize(5);//文字の大きさを設定（1～7）
    
    switch(ContentColor){
      case 0:
            M5.Lcd.setTextColor(TFT_CYAN); //文字色設定(背景は透明)(WHITE, BLACK, RED, GREEN, B
            break;
      case 1:
      case 2:
      case 8:
      case 9:
            M5.Lcd.setTextColor(TFT_BLACK);
            break;
    } 
    
//    M5.Lcd.setTextColor(ContentColor); //文字色設定(背景は透明)(WHITE, BLACK, RED, GREEN, B

    M5.Lcd.print(Content);   
    digitalWrite(10,HIGH);      
}

void setup() {
  setCpuFrequencyMhz(80);
  M5.begin(); 
  M5.Lcd.setRotation(1);
  M5.Lcd.setCursor(0, 0, 1);
  M5.Lcd.fillScreen(BLACK);
//  WiFiMulti.addAP(WIFI_SSID,WIFI_PASSWORD);
//  M5.Lcd.print("Connecting");
//  while(WiFiMulti.run() != WL_CONNECTED) {
//  while(WiFiMulti.run() != WL_CONNECTED) {
//    M5.Lcd.print(".");
//    delay(1000);
//  }
//==============================================================================
//Wifiに接続出来なくて苦労した。WiFi親機ルーターとの相性で接続出来ないことがあるらしい。
//以下で、WiFi切り離し＆再接続を行うことで、問題なくつながるようになる。以下ネット情報より
//https://msr-r.net/m5stickc-wifi-error/   
//↑これもやっぱり怪しいかも・・・ 

    int cnt=0;
    M5.Lcd.printf("Connecting to %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    while(WiFi.status() != WL_CONNECTED) {
      cnt++;
      delay(500);
      M5.Lcd.print(".");
      if(cnt%10==0) {
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        M5.Lcd.println("");
      }
      if(cnt>=30) {
        ESP.restart();
      }
    }
    M5.Lcd.printf("\nWiFi connected\n");
    
//==============================================================================
////  WiFi.disconnect(true);
////  delay(1000);
////
////  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//    Serial.print(".");
//  }
//  
//  M5.Lcd.println("");
//  M5.Lcd.println("Connected to");
//  M5.Lcd.println(WiFi.localIP());
//  delay(500);
//  M5.Lcd.qrcode("http://www.bishamon.co.jp");
  configTime( JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
   
  time_t t;
  t = time(NULL);
    
  pinMode(input1,INPUT);    //36  
  pinMode(input2,INPUT);    //26
//  pinMode(input3,INPUT);    //0
  pinMode(G0,INPUT);    //0
  
  pinMode(10,OUTPUT);       //10内蔵LED
  Firebase.begin(FIREBASE_DB_URL);   // ④

  longBeforeconnect = millis();
}

void loop() {
  M5.update();  // ⑤

  //コネクトしてから1分経過していたらコネクト確認処理を行う
  //前田先生から、slackでソースコードと解説をもらった。
  if(longBeforeconnect + 1000 <= millis()){    //1秒毎に変更
//    if(longBeforeconnect + 60000 <= millis()){
    //wifiのコネクト確認
    if( WiFi.status() != WL_CONNECTED) {
     //再接続
       
       WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
       //接続状態になるまで待つ
       while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
       }
//       Andon_ON(1);//wifi再接続のログをとる あかんわ。なんかSSLでひっかかるみたい。
     }else{
       Serial.println("Wi-Fi connect check OK"); 
       delay(1000);
     }
     longBeforeconnect = millis();
  }

  
  digitalWrite(10,HIGH);

//===============================================================
  if (M5.BtnA.wasPressed() ) {  // ⑥
//      2021/4/9 昨日の雅さんリクエストによる、計画停止を長押しで実装するために、wasPressed処理は一旦コメントアウトする
    
//    //M5.Lcd.clear; M5StickC では無効のコマンド
//    M5.Lcd.fillScreen(BLACK);
//    M5.Lcd.setCursor(10, 10); //文字表示の左上位置を設定
//    
//    M5.Lcd.setTextColor(RED); //文字色設定(背景は透明)(WHITE, BLACK, RED, GREEN, B
//    M5.Lcd.setTextSize(1);//文字の大きさを設定（1～7）
////    M5.Lcd.print("Hey Guys! \n\n We’re sending to Slack!");
//    M5.Lcd.print("Send to Slack!");
//    delay(1000);                   //500ms停止
////    Firebase.setInt("/button", count);  // ⑦
     Andon_ON(1);                 //slackへ送信
//
//    M5.Lcd.setTextColor(GREEN, RED); //文字色設定と背景色設定(WHITE, BLACK, RED, GREEN, BLUE, YELLOW...)
//    M5.Lcd.setCursor(10, 100); //文字表示の左上位置を設定
////    M5.Lcd.print("Hey Guys! \n\n Firebase!!");
//    M5.Lcd.print("Firebase!!");
////     M5.Lcd.fillScreen(RED);
//    sendToFirebase(MachineNo,"RUN1");
////    Firebase.push("/button", count);
//    delay(1000);
//    M5.Lcd.fillScreen(WHITE);
//    M5.Lcd.setTextColor(GREEN, RED);
//    M5.Lcd.setCursor(10, 10);
//    M5.Lcd.println(count +" Pushed");
//    delay(2000);   //画面表示を確認する為には、Delayが必要だよね。
//    count ++;  // ⑧
  }
//  ============================================================
  int btnA = M5.BtnA.pressedFor(1000); // ホームボタン
  if(btnA==1){
    if(SetKKT){
      SetKKT = false;
  
    }else{
      SetKKT = true;
  
    }
  }
//  Serial.print("btnAの状態は、");
//  Serial.println(SetKKT);
//  ============================================================
  if (M5.BtnA.wasReleased() ) {  // ⑥
//    M5.Lcd.clear;
    M5.Lcd.println("Released");

//    Firebase.setInt("/button", count);  // ⑦
    M5.Lcd.fillScreen(GREEN);
//    sendToFirebase(MachineNo,"2");
//    Firebase.push("/button", count);
    count ++;  // ⑧
  }
  if (M5.BtnB.wasPressed() ) {  // ⑥

    if(M5.BtnB.wasPressed()){ //再起動がめんどうなので、Bボタンで行えるようにした
      esp_restart();
    }
    M5.update();


    
    //M5.Lcd.clear; ←M5StickC では使えない

//============================================  カウントアップしたり、
//    M5.Lcd.println(count +" B-Pushed");
//
////    Firebase.setInt("/button", count);  // ⑦
//    M5.Lcd.fillScreen(GREEN);
//   
////    sendToFirebase("gt999","1");
//    sendToFirebase(MachineNo,"RUN2");
////    Firebase.push("/button", count);
//    count ++;  // ⑧
//============================================
  }
  if (M5.BtnB.wasReleased() ) {  // ⑥
//    M5.Lcd.clear;
    M5.Lcd.println("B-Released");
      
//    Firebase.setInt("/button", count);  // ⑦
    M5.Lcd.fillScreen(BLUE);
//    sendToFirebase("gt999","2");　　
//    Firebase.push("/button", count);
    count ++;  // ⑧
  }

    // 電源ボタンの状態取得（一度しか0以外のステータスは取得できない）
  int axpButton = M5.Axp.GetBtnPress();
  if ( axpButton == 1 ) {
    // 1秒以上電源ボタンを押している
    Serial.println("M5.Axp.GetBtnPress() == 1");
  }
  if ( axpButton == 2 ) {
    // 1秒未満電源ボタンを押して離した
    Serial.println("M5.Axp.GetBtnPress() == 2");
      // Display QRCode
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setRotation(0);
    M5.Lcd.setCursor(0, 0, 1);
    
    M5.Lcd.qrcode("http://www.ay-vue.firebaseapp.co.jp");
    // M5.Lcd.qrcode(const char *string, uint16_t x = 50, uint16_t y = 10, uint8_t width = 220, uint8_t version = 6);

  }

  boolean chkflag=false;
  int before_sw1_gyaku;
  int before_sw2_gyaku;
  int sw1_gyaku;
  int sw2_gyaku;
  long nowMillis;

   
  //chkflagがtrueなら、既にセンサー値を検出済なので処理を行わない=============================
//  if(chkflag){
      //
//  }else{
  int before_sw1=digitalRead(input1);
  int before_sw2=digitalRead(input2);
  int before_sw3=digitalRead(input3);     //段取り用物理SW

//  Serial.println("段取り専用　before_sw3" +before_sw3);
//  Serial.println(before_sw3);
if(SetKKT){//KKT計画停止モード の場合おは、before_swを８にセットする
//  Serial.print(SetKKT);
  before_sw=8;
}else{
  before_sw=0;  //KKT計画停止モードでは無い場合、before_swを０にセットする
  if(before_sw3==0){  //段取りSWが入っている場合
    before_sw=9;    //段取り(G0がインプット　GND導通)は、 before_swを９にセット

  }else{
        
    if (before_sw1==0){      //cdsセンサーの値読み取りを逆転させる（0<=>1)
        before_sw1_gyaku=1;
        
    }else{
        before_sw1_gyaku=0;

    }
    
    if (before_sw2==0){      //cdsセンサーの値読み取りを逆転させる（0<=>1)

        before_sw2_gyaku=1;
        
    }else{

        before_sw2_gyaku=0;

    }
    //  Serial.println(before_sw2_gyaku);
    before_sw=before_sw1_gyaku+before_sw2_gyaku*2;//各値を読み取って十進数化

    delay(300); //チャタリング対策300ミリsecをdelayさせることで、反応し続けていると判断出来る。
                //これ以上長いと間延びした感じになる。短いとフラッシュ点滅に反応してしまう。
  
    Serial.println(before_sw);
//    Serial.println(beforeinput);
  }
}
    M5.Lcd.setRotation(1);
    
    
    switch(before_sw){    //画面表示と内臓LEDのフラッシュ点滅
    
      case 0://停止中の場合
            LcdSet("Stop",before_sw);           
            break;       
      case 1://稼働中の場合
            LcdSet("RUN",before_sw);  
//            M5.Lcd.fillScreen(TFT_GREEN);
//            M5.Lcd.setTextColor(TFT_NAVY); //文字色設定(背景は透明)(WHITE, BLACK, RED, GREEN, B
//            M5.Lcd.print("RUN");  
//            digitalWrite(10,HIGH);
            break;
      case 2://異常中の場合
            LcdSet("ERR",before_sw);  
//            M5.Lcd.fillScreen(TFT_RED);
//            M5.Lcd.setTextColor(TFT_DARKGREY); //文字色設定(背景は透明)(WHITE, BLACK, RED, GREEN, B
//            M5.Lcd.print("ERR");  
//            digitalWrite(10,HIGH);
            break;
      case 8://計画停止の場合
            LcdSet("KKTS",before_sw);  
//            M5.Lcd.fillScreen(TFT_PINK);
//            M5.Lcd.setTextColor(BLACK); //文字色設定(背景は透明)(WHITE, BLACK, RED, GREEN, B
//            M5.Lcd.print("KKTS");  
            delay(1200);
            digitalWrite(10,LOW);
            delay(50);
            digitalWrite(10,HIGH);
            break;
      case 9://段取り中の場合
            LcdSet("DDR",before_sw);  
//            M5.Lcd.fillScreen(TFT_YELLOW);
//            M5.Lcd.setTextColor(BLACK); //文字色設定(背景は透明)(WHITE, BLACK, RED, GREEN, B
//            M5.Lcd.print("DDR");
        //    M5.Lcd.print("Hey Guys! \n\n We’re sending to Slack!");
        //    M5.Lcd.setTextFont(7); 多分コレは無効
            delay(1200);
            digitalWrite(10,LOW);
            delay(50);
            digitalWrite(10,HIGH);
            break;
    };

    nowMillis = millis()-swStartMills;
//    Serial.print("現在のミリ秒は、");
//    Serial.println(nowMillis);
//    Serial.print("現在の時刻は、");
//    Serial.println(time(NULL));
    delay(10);
    int sw1=digitalRead(input1);
    int sw2=digitalRead(input2);
    int sw3=digitalRead(input3);

if(SetKKT){
  sw=8;
}else{
  sw=0;
  if(before_sw3==0){
    sw=9;       //段取りは９として識別
             //この上５〜６行にて、計画停止が最優先、段取りがその次に優先、他は下記の通りに分岐となる。
  }else{  
    if (sw1==0){      //cdsセンサーの値読み取りを逆転させる（0<=>1)
        sw1_gyaku=1;
        
    }else{
        sw1_gyaku=0;
    }
    
    if (sw2==0){      //cdsセンサーの値読み取りを逆転させる（0<=>1)
        sw2_gyaku=1;
        
    }else{
        sw2_gyaku=0;
    }
    
    sw=sw1_gyaku+sw2_gyaku*2;
  
  }
}
  //  Serial.println(sw);
   
 //チャタリング対策
if (before_sw==sw){     
    chkflag=true;             //３００ミリsecの差を読み取り、センサー値を確定(チャタリングでは無いと判断)

}else{
    chkflag=false;
}
//} 
  //================================================================================

if(chkflag==true){          //チャタリングでは無い場合 =>ほとんどの場合は真となる。
  Serial.print("センサー起動　sw.....=、");
  Serial.println(sw);
  delay(10);
    //if(beforeinput!=sw && beforeinput!=99){  //値に変化がある、かつ、初回（９９）では無い場合に
//  if(beforeinput!=99){  //初期値９９ではない
//      Serial.print("99ではないよ！");
//      delay(5000);
    
//  if(beforeinput!=99){    //値が初期値（９９）ではなく、

    if(beforeinput!=sw){  //値に変化がある場合
    
      swChange=true;            //センサー値変化=>値変化変数swChangeを真とする
      swStartMills=millis();    //センサー値変化のmillis()時刻を記録
      time_t nowTime;
      nowTime = time(NULL);           //センサー値変化時点のRTC時刻を記録
    }
    
//    
//    Serial.print("beforeinputは、");
//    Serial.println(beforeinput);

    
    if(swChange==true){       //値変化変数swChangeが真の場合
      
            delay(10);
            switch (sw){
              case 0:
                     switch(beforeinput){
                        case 0:
                              break;
                        case 1:
                              //"RUN2"
//                              Serial.println("swは正常なのか？？？！");
                              delay(1200);
                              if(RunBool){
                                sendToFirebase(MachineNo,"RUN2"); 
                                RunBool = false;
                              };
                              break;
                        case 2:
                        case 3:
                              //"ERR2"
                              if(ErrBool){
                                sendToFirebase(MachineNo,"ERR2"); 
                                ErrBool = false;
                              };
                              break;
                        case 8:
                              sendToFirebase(MachineNo,"KKT2"); 
                              break;
                        case 9:
                              sendToFirebase(MachineNo,"DDR2"); 
                              break;
                        case 99:
                              //初期値（９９）の場合は何も処理をしない。beforeinputが最新ｓｗの値に更新される
//                              Serial.println("もしかしてこういうことか？");
                              delay(5000);
                              break;
                     
                        };
                        beforeinput=sw;   //次回サイクルに備えて、前回分として値を格納しておく。
                        swChange=false;   //値変化の処理が完了したので、値変化のフラッグを初期値（false）に戻す。
                     break;                   
              case 1:
                     Serial.println(nowMillis);
//                     delay(5000);
                     if(nowMillis>1000){       //１秒以上点灯している状態（寸動を除去する）         
                       switch(beforeinput){
                          case 0:
                                //""RUN1""
                                Serial.println("swは正常！");
//                                delay(1000);
                                delay(10);
                                
                                sendToFirebase(MachineNo,"RUN1"); //稼働中（cdsセンサー緑を検知）
                                RunBool = true;
                                break;
                          case 1:
                                break;
                          case 2:
                                //"ERR2" & "RUN1"
                                //"ERR2"
                                delay(10);
                                if(ErrBool){
                                  
                                  
                                  sendToFirebase(MachineNo,"ERR2"); 
                                  ErrBool = false;
                                };
                                delay(1200);
                                sendToFirebase(MachineNo,"RUN1");
                                RunBool = true;
                                
                                break;
                        case 8:
                              sendToFirebase(MachineNo,"KKT2"); 
                              delay(1200);//これぐらい待たないと、時間が同じになってしまって、JSで検出出来ない？
                              sendToFirebase(MachineNo,"RUN1"); //稼働中（cdsセンサー緑を検知）
                              RunBool = true;//2021/6/22追加。これが無いことで、RUNが入りっぱなしになっていた。
                              break;
                        case 9:
                              delay(3000);
//                              Serial.println("ここですよー！");
                              sendToFirebase(MachineNo,"DDR2"); 
                              delay(1200);//これぐらい待たないと、時間が同じになってしまって、JSで検出出来ない？
//                              Serial.println("1200ミリセック待ってから稼働中にー！");
                              
                              sendToFirebase(MachineNo,"RUN1"); //稼働中（cdsセンサー緑を検知）
                              RunBool = true;//2021/6/22追加。これが無いことで、RUNが入りっぱなしになっていた。
                              break;
                          case 99:
                                //初期値（９９）の場合は何も処理をしない。beforeinputが最新ｓｗの値に更新される
                                break;
                          };
                          beforeinput=sw;   //次回サイクルに備えて、前回分として値を格納しておく。
                          swChange=false;   //値変化の処理が完了したので、値変化のフラッグを初期値（false）に戻す。
                       };

                     break;
                     
              case 2:
                     if(nowMillis>2000){       //2秒以上点灯している状態（寸動や軽微なエラーを除去する）
                                                                
                       switch(beforeinput){
                          case 0:
                                //"ERR1"
                               sendToFirebase(MachineNo,"ERR1"); 
                               ErrBool = true;
                                
                               break;
                          case 1:
                                //"RUN2" & "ERR1"
                                if(RunBool){
                                  sendToFirebase(MachineNo,"RUN2");
                                  RunBool=false;
                                }
                                delay(1200);
                                
                                sendToFirebase(MachineNo,"ERR1");
                                ErrBool = true;
                                
                                break;
                          case 2:
                                break;
                          case 8:
                              sendToFirebase(MachineNo,"KKT2"); 
                              delay(1200);//これぐらい待たないと、時間が同じになってしまって、JSで検出出来ない？
                              sendToFirebase(MachineNo,"ERR1"); //稼働中（cdsセンサー緑を検知）
                              ErrBool = true;//2021/6/22追加。これが無いことで、ERRが入りっぱなしになっていた。
                              break;
                          case 9:
                              delay(3000);
//                              Serial.println("ここですよー！");
                              sendToFirebase(MachineNo,"DDR2"); 
                              delay(1200);//これぐらい待たないと、時間が同じになってしまって、JSで検出出来ない？
//                              Serial.println("1200ミリセック待ってから稼働中にー！");
                              
                              sendToFirebase(MachineNo,"ERR1"); //稼働中（cdsセンサー緑を検知）
                              ErrBool = true;//2021/6/22追加。これが無いことで、ERRが入りっぱなしになっていた。
                              break;
                              
                        case 99:
                              //初期値（９９）の場合は何も処理をしない。beforeinputが最新ｓｗの値に更新される
                              break;
                        };
                        beforeinput=sw;   //次回サイクルに備えて、前回分として値を格納しておく。
                        swChange=false;   //値変化の処理が完了したので、値変化のフラッグを初期値（false）に戻す。          
                        
                     };

                     break;
               case 8://計画停止
                     if(nowMillis>5000){       //５秒以上段取りSWがオンになっている状態（寸動や軽微なエラーを除去する）
                                             switch(beforeinput){
                          case 0:
                                //"ERR1"
                               sendToFirebase(MachineNo,"KKT1");
                               ErrBool = true;
                               delay(10);
                           
                               break;
                          case 1:
                                
                                if(RunBool){
                                  sendToFirebase(MachineNo,"RUN2");
                                  RunBool=false;
                                }
                                
                                delay(1200);
                                sendToFirebase(MachineNo,"KKT1");
                                delay(1);
                                
                                break;
                          case 2:

                                if(ErrBool){
                                  sendToFirebase(MachineNo,"ERR2");
                                  ErrBool=false;
                                }
                                delay(1200);
                                
                                sendToFirebase(MachineNo,"KKT1");
                                
                                
                                break;
                          case 8:

                                break;
                          case 9:
                                sendToFirebase(MachineNo,"DDR2");
                                delay(1200);
                                sendToFirebase(MachineNo,"ERR1");
                                ErrBool = true;
                                break;
                              
                        case 99:
                              //初期値（９９）の場合は何も処理をしない。beforeinputが最新ｓｗの値に更新される
                              break;
                        };

               
                      //計画停止中の処理

                                                          
                   
                      beforeinput=sw;   //次回サイクルに備えて、前回分として値を格納しておく。
                      swChange=false;   //値変化の処理が完了したので、値変化のフラッグを初期値（false）に戻す。   
                      break;
                      
               case 9://段取り
               
                     if(nowMillis>5000){       //５秒以上段取りSWがオンになっている状態（寸動や軽微なエラーを除去する）
                                             switch(beforeinput){
                          case 0:
                                //"ERR1"
                               sendToFirebase(MachineNo,"DDR1"); 
                               ErrBool = true;
                                
                               break;
                          case 1:
                                //"RUN2" & "ERR1"
                                if(RunBool){
                                  sendToFirebase(MachineNo,"RUN2");
                                  RunBool=false;
                                }
                                delay(1200);
                                
                                sendToFirebase(MachineNo,"DDR1");
                                ErrBool = true;
                                
                                break;
                          case 2:
                                //"RUN2" & "ERR1"
                                if(ErrBool){
                                  sendToFirebase(MachineNo,"ERR2");
                                  RunBool=false;
                                }
                                delay(1200);
                                
                                sendToFirebase(MachineNo,"DDR1");
                                ErrBool = true;
                                
                                break;
                          case 8:
                                sendToFirebase(MachineNo,"KKT2");
                                sendToFirebase(MachineNo,"DDR1");
                                break;
                          case 9:
                                break;
                              
                        case 99:
                              //初期値（９９）の場合は何も処理をしない。beforeinputが最新ｓｗの値に更新される
                              break;
                        };
                     
                        beforeinput=sw;   //次回サイクルに備えて、前回分として値を格納しておく。
                        swChange=false;   //値変化の処理が完了したので、値変化のフラッグを初期値（false）に戻す。
                      
                      }

                      break;
            }               

        }
    }

    
  }

}
