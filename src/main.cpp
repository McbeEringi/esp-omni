#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#define FSYS LittleFS
#define NAME "omni"
#define PASS "mcbeeringi"

#define WIFI_CFG "/wifi.txt"

#define RFEN 0
#define RFPH 1
#define RREN 2
#define RRPH 3
#define LREN 4
#define LRPH 5
#define LFEN 6
#define LFPH 7
#define NPDI 8



const IPAddress ip(192,168,1,1);
const IPAddress subnet(255,255,255,0);

static AsyncWebServer svr(80);
static AsyncWebSocket ws("/ws");

bool isAPmode=false;

void onWS(AsyncWebSocket *ws,AsyncWebSocketClient *cli,AwsEventType type,void *arg,uint8_t *data,size_t len){
}

void setup(){
	neopixelWrite(NPDI,16,0,0);
	FSYS.begin();
	pinMode(RFEN,OUTPUT);digitalWrite(RFEN,LOW);
	pinMode(RFPH,OUTPUT);digitalWrite(RFPH,LOW);
	pinMode(RREN,OUTPUT);digitalWrite(RREN,LOW);
	pinMode(RRPH,OUTPUT);digitalWrite(RRPH,LOW);
	pinMode(LREN,OUTPUT);digitalWrite(LREN,LOW);
	pinMode(LRPH,OUTPUT);digitalWrite(LRPH,LOW);
	pinMode(LFEN,OUTPUT);digitalWrite(LFEN,LOW);
	pinMode(LFPH,OUTPUT);digitalWrite(LFPH,LOW);

	delay(1000);

	neopixelWrite(NPDI,16,16,0);
	{
		String w=FSYS.open(WIFI_CFG).readString();
		w.trim();
		int n=w.indexOf('\n');
		WiFi.begin(w.substring(0,n).c_str(),w.substring(n+1).c_str());
	}
	WiFi.setAutoReconnect(true);
	for(uint8_t i=0;WiFi.status()!=WL_CONNECTED;i++){
		delay(500);
		if(20<i){
			isAPmode=true;
			WiFi.softAP(NAME,PASS);
			delay(100);// https://github.com/espressif/arduino-esp32/issues/985
			WiFi.softAPConfig(ip,ip,subnet);
			break;
		}
	}

	ArduinoOTA
		.setHostname(NAME).setPassword(PASS)
		.onStart([](){ws.enable(false);ws.textAll("OTA update started.");ws.closeAll();FSYS.end();})
		.onProgress([](unsigned int x,unsigned int a){neopixelWrite(NPDI,(a-x)*32/a,x*32/a,0);})
		.onError([](ota_error_t e){neopixelWrite(NPDI,64,0,64);ws.enable(true);FSYS.begin();})
		.begin();
	ws.onEvent(onWS);svr.addHandler(&ws);
	svr.onNotFound([](AsyncWebServerRequest *r){r->redirect("/");});
	svr.serveStatic("/",FSYS,"/public").setDefaultFile("index.html");
	svr.on("/wifi.txt",HTTP_GET,[](AsyncWebServerRequest *r){
		if(!r->authenticate(NAME,PASS))return r->requestAuthentication();
    r->send(FSYS,WIFI_CFG);
  });
	svr.begin();

}
void loop(){
	ArduinoOTA.handle();
	if(isAPmode)neopixelWrite(NPDI,0,16,16);// AP mode
	else if(WiFi.isConnected())neopixelWrite(NPDI,0,16,0);// STA connected
	else neopixelWrite(NPDI,16,16,0);// STA disconnected
	delay(10);
}
