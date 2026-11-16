#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <ArduinoJson.h>
extern "C" {
#include <user_interface.h>
}
#include "config.h"
constexpr uint8_t BUTTON_PIN=D5, SERVO_PIN=D6;
LiquidCrystal_I2C lcd(0x27,16,2);
Servo lockServo;
ESP8266WebServer server(80);
enum class State { Idle, Waiting, Unlocked, Result };
State state=State::Idle;
unsigned long stateAt=0,lastEdge=0,lastWifiAttempt=0;
int lastRaw=HIGH,stable=HIGH;
String challengeId;
void screen(const char* first,const char* second=""){
 lcd.clear();lcd.print(first);lcd.setCursor(0,1);lcd.print(second);
}
void idle(){state=State::Idle;challengeId="";screen("SecureIT Ready","Press Button");}
bool auth(){if(server.header("X-Device-Key")==DEVICE_KEY)return true;server.send(401,"application/json","{\"error\":\"Unauthorized\"}");return false;}
void newChallenge(){
 if(WiFi.status()!=WL_CONNECTED){screen("WiFi offline","Try again later");state=State::Result;stateAt=millis();return;}
 char id[17];snprintf(id,sizeof(id),"%08x%08x",os_random(),os_random());challengeId=id;
 const int pin=100+os_random()%900;
 char number[8];snprintf(number,sizeof(number),"%d",pin);screen("Match this PIN:",number);
 state=State::Waiting;stateAt=millis();
 StaticJsonDocument<128> doc;doc["id"]=challengeId;doc["pin"]=pin;String payload;serializeJson(doc,payload);
 WiFiClient network;HTTPClient http;http.setTimeout(6000);
 http.begin(network,String(GATEWAY_URL)+"/api/device/challenge");http.addHeader("Content-Type","application/json");http.addHeader("X-Device-Key",DEVICE_KEY);
 int code=http.POST(payload);http.end();
 if(code!=200){challengeId="";screen("Gateway error","Press again");state=State::Result;stateAt=millis();}
}
void setup(){
 Serial.begin(115200);pinMode(BUTTON_PIN,INPUT); // External pull-up to 3.3 V; switch to GND.
 Wire.begin(D2,D1);lcd.init();lcd.backlight();lockServo.attach(SERVO_PIN);lockServo.write(LOCKED_ANGLE);
 WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(true);WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
 const char* headers[]={"X-Device-Key"};server.collectHeaders(headers,1);
 server.on("/decision",HTTP_POST,[]{
  if(!auth())return;
  if(state!=State::Waiting||server.arg("id")!=challengeId||millis()-stateAt>=CHALLENGE_MS){server.send(409,"application/json","{\"error\":\"Stale challenge\"}");return;}
  if(server.arg("grant")!="0"&&server.arg("grant")!="1"){server.send(400,"application/json","{}");return;}
  const bool grant=server.arg("grant")=="1";challengeId="";stateAt=millis();
  if(grant){lockServo.write(UNLOCKED_ANGLE);screen("Access Granted","Unlocking...");state=State::Unlocked;}
  else{screen("Access Denied");state=State::Result;}
  server.send(200,"application/json","{\"ok\":true}");
 });
 server.on("/status",HTTP_GET,[]{if(auth())server.send(200,"application/json","{\"online\":true}");});
 server.begin();idle();
}
void loop(){
 server.handleClient();const auto now=millis();
 const int raw=digitalRead(BUTTON_PIN);
 if(raw!=lastRaw){lastRaw=raw;lastEdge=now;}
 if(now-lastEdge>=DEBOUNCE_MS&&stable!=raw){stable=raw;if(stable==LOW&&state==State::Idle)newChallenge();}
 if(state==State::Waiting&&millis()-stateAt>=CHALLENGE_MS){challengeId="";screen("PIN expired");state=State::Result;stateAt=millis();}
 if(state==State::Unlocked&&millis()-stateAt>=UNLOCK_MS){lockServo.write(LOCKED_ANGLE);idle();}
 if(state==State::Result&&millis()-stateAt>=2000)idle();
 if(WiFi.status()!=WL_CONNECTED&&now-lastWifiAttempt>15000){lastWifiAttempt=now;WiFi.reconnect();}
 static bool reported=false;if(WiFi.status()==WL_CONNECTED&&!reported){Serial.print("Access board IP: ");Serial.println(WiFi.localIP());reported=true;}
 delay(1);
}
