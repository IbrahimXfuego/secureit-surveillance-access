#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <DHT.h>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "config.h"
constexpr int PIR_PIN=13,RELAY_PIN=14,DHT_PIN=2,FLASH_PIN=4;
DHT dht(DHT_PIN,DHT11);
WebServer api(80);
SemaphoreHandle_t cameraLock;
bool motionEnabled=false;
unsigned long lastMotion=0,lastWifi=0;
float temperature=NAN,humidity=NAN;
bool authorized(){if(api.header("X-Device-Key")==DEVICE_KEY)return true;api.send(401,"application/json","{\"error\":\"Unauthorized\"}");return false;}
void relay(bool on){digitalWrite(RELAY_PIN,(on!=RELAY_ACTIVE_LOW)?HIGH:LOW);}
void initCamera(){
 camera_config_t c={};
 c.ledc_channel=LEDC_CHANNEL_0;c.ledc_timer=LEDC_TIMER_0;
 c.pin_d0=5;c.pin_d1=18;c.pin_d2=19;c.pin_d3=21;c.pin_d4=36;c.pin_d5=39;c.pin_d6=34;c.pin_d7=35;
 c.pin_xclk=0;c.pin_pclk=22;c.pin_vsync=25;c.pin_href=23;c.pin_sscb_sda=26;c.pin_sscb_scl=27;c.pin_pwdn=32;c.pin_reset=-1;
 c.xclk_freq_hz=20000000;c.pixel_format=PIXFORMAT_JPEG;c.jpeg_quality=12;
 c.frame_size=psramFound()?FRAMESIZE_VGA:FRAMESIZE_QVGA;c.fb_count=psramFound()?2:1;
 c.fb_location=psramFound()?CAMERA_FB_IN_PSRAM:CAMERA_FB_IN_DRAM;c.grab_mode=CAMERA_GRAB_LATEST;
 if(esp_camera_init(&c)!=ESP_OK){Serial.println("Camera initialization failed. Check power and camera ribbon.");while(true)delay(1000);}
}
esp_err_t streamHandler(httpd_req_t* req){
 httpd_resp_set_type(req,"multipart/x-mixed-replace;boundary=secureit");
 httpd_resp_set_hdr(req,"Cache-Control","no-store");
 while(true){
  if(xSemaphoreTake(cameraLock,pdMS_TO_TICKS(1000))!=pdTRUE){vTaskDelay(1);continue;}
  camera_fb_t* fb=esp_camera_fb_get();esp_err_t result=ESP_FAIL;
  if(fb){char head[128];int size=snprintf(head,sizeof(head),"\r\n--secureit\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",unsigned(fb->len));
   result=httpd_resp_send_chunk(req,head,size);if(result==ESP_OK)result=httpd_resp_send_chunk(req,reinterpret_cast<const char*>(fb->buf),fb->len);esp_camera_fb_return(fb);}
  xSemaphoreGive(cameraLock);if(result!=ESP_OK)break;vTaskDelay(pdMS_TO_TICKS(100));
 }
 return ESP_OK;
}
void startStream(){
 httpd_config_t c=HTTPD_DEFAULT_CONFIG();c.server_port=81;c.ctrl_port=32769;c.stack_size=8192;c.send_wait_timeout=2;c.recv_wait_timeout=2;c.max_open_sockets=2;c.lru_purge_enable=true;
 httpd_handle_t server=nullptr;
 if(httpd_start(&server,&c)==ESP_OK){httpd_uri_t route={};route.uri="/stream";route.method=HTTP_GET;route.handler=streamHandler;httpd_register_uri_handler(server,&route);}
 else Serial.println("Streaming server failed to start.");
}
void setup(){
 Serial.begin(115200);digitalWrite(RELAY_PIN,RELAY_ACTIVE_LOW?HIGH:LOW);pinMode(RELAY_PIN,OUTPUT);relay(false);pinMode(FLASH_PIN,OUTPUT);digitalWrite(FLASH_PIN,LOW);pinMode(PIR_PIN,INPUT);
 cameraLock=xSemaphoreCreateMutex();if(!cameraLock){while(true)delay(1000);}initCamera();dht.begin();
 WiFi.mode(WIFI_STA);WiFi.setSleep(false);WiFi.begin(WIFI_SSID,WIFI_PASSWORD);WiFi.setAutoReconnect(true);
 while(WiFi.status()!=WL_CONNECTED)delay(250);
 Serial.print("Camera IP: ");Serial.println(WiFi.localIP());
 const char* headers[]={"X-Device-Key"};api.collectHeaders(headers,1);
 api.on("/photo",HTTP_GET,[]{
  if(!authorized())return;
  if(xSemaphoreTake(cameraLock,pdMS_TO_TICKS(2000))!=pdTRUE){api.send(503,"application/json","{\"error\":\"Camera busy\"}");return;}
  const bool flash=api.arg("flash")=="1";
  if(flash){digitalWrite(FLASH_PIN,HIGH);delay(200);for(int i=0;i<2;i++){camera_fb_t* stale=esp_camera_fb_get();if(stale)esp_camera_fb_return(stale);}}
  camera_fb_t* fb=esp_camera_fb_get();
  if(fb){api.sendHeader("Cache-Control","no-store");api.setContentLength(fb->len);api.send(200,"image/jpeg","");WiFiClient out=api.client();size_t sent=0;unsigned long start=millis();while(sent<fb->len&&out.connected()&&millis()-start<5000){size_t n=out.write(fb->buf+sent,fb->len-sent);if(!n)break;sent+=n;delay(0);}esp_camera_fb_return(fb);}
  else api.send(503,"application/json","{\"error\":\"Capture failed\"}");
  digitalWrite(FLASH_PIN,LOW);xSemaphoreGive(cameraLock);
 });
 api.on("/relay",HTTP_POST,[]{if(!authorized())return;relay(api.arg("on")=="1");api.send(200,"application/json","{\"ok\":true}");});
 api.on("/motion",HTTP_POST,[]{if(!authorized())return;motionEnabled=api.arg("on")=="1";api.send(200,"application/json","{\"ok\":true}");});
 api.on("/temperature",HTTP_GET,[]{if(!authorized())return;if(isnan(temperature)||isnan(humidity)){api.send(503,"application/json","{\"error\":\"Sensor unavailable\"}");return;}api.send(200,"application/json",String("{\"temperature\":")+String(temperature,1)+",\"humidity\":"+String(humidity,1)+"}");});
 api.begin();startStream();
}
void loop(){
 api.handleClient();const unsigned long now=millis();
 static unsigned long lastDht=0;if(now-lastDht>=2500){lastDht=now;temperature=dht.readTemperature();humidity=dht.readHumidity();}
 if(motionEnabled&&now>30000&&digitalRead(PIR_PIN)==PIR_ACTIVE_LEVEL&&now-lastMotion>=10000&&WiFi.status()==WL_CONNECTED){
  lastMotion=now;WiFiClient net;HTTPClient http;http.setTimeout(2000);http.begin(net,String(GATEWAY_URL)+"/api/device/motion");http.addHeader("Content-Type","application/json");http.addHeader("X-Device-Key",DEVICE_KEY);http.POST("{}");http.end();
 }
 if(WiFi.status()!=WL_CONNECTED&&now-lastWifi>=15000){lastWifi=now;WiFi.reconnect();}
 delay(1);
}
