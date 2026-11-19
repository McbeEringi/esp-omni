#include <Arduino.h>
#include <driver/ledc.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
// #include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#define FSYS LittleFS
#define NAME "omni"
#define PASS "mcbeeringi"

#define PUB_DIR "/public"
#define PVT_DIR "/private"
#define WIFI_CFG "/private/wifi.txt"

#define NUM_WHEEL 4
#define FOR(X) for(uint8_t i=0;i<X;++i)
#define FOR_WHEEL() FOR(NUM_WHEEL)


const uint8_t pin[NUM_WHEEL][2]={
	// EN PH
	{1,0},// RF
	{2,3},// RR
	{5,4},// LR
	{6,7} // LF
};
float v[NUM_WHEEL]={0};

#define NPDI 8

const IPAddress ip(192,168,1,1);
const IPAddress subnet(255,255,255,0);

static AsyncWebServer svr(80);
static AsyncWebSocketMessageHandler wsh;
static AsyncWebSocket ws("/ws",wsh.eventHandler());

bool isAPmode=false;

void setup(){
	neopixelWrite(NPDI,16,0,0);
	FSYS.begin();

	FOR_WHEEL(){
		ledcSetup(i,1000,LEDC_TIMER_14_BIT);ledcAttachPin(pin[i][0],i);
		pinMode(pin[i][1],OUTPUT);digitalWrite(pin[i][1],LOW);
	}

	delay(1000);

	neopixelWrite(NPDI,16,16,0);
	{
		String w=FSYS.open(WIFI_CFG).readString();
		w.trim();
		int n=w.indexOf('\n');
		WiFi.begin(w.substring(0,n).c_str(),w.substring(n+1).c_str());
		WiFi.setAutoReconnect(true);
		for(uint8_t i=0;WiFi.status()!=WL_CONNECTED;++i){
			delay(500);
			if(20<i){
				isAPmode=true;
				WiFi.softAP(NAME,PASS);
				delay(100);// https://github.com/espressif/arduino-esp32/issues/985
				WiFi.softAPConfig(ip,ip,subnet);
				break;
			}
		}
	}

	// ArduinoOTA
	// 	.setHostname(NAME).setPassword(PASS)
	// 	.onStart([](){ws.enable(false);ws.textAll("OTA update started.");ws.closeAll();FSYS.end();})
	// 	.onProgress([](unsigned int x,unsigned int a){neopixelWrite(NPDI,(a-x)*32/a,x*32/a,0);})
	// 	.onError([](ota_error_t e){neopixelWrite(NPDI,64,0,64);ws.enable(true);FSYS.begin();})
	// 	.begin();
	MDNS.begin(NAME);
	wsh.onMessage([](AsyncWebSocket *svr,AsyncWebSocketClient *cli,const uint8_t *w,size_t l){
		FOR_WHEEL()v[i]=*(float*)(w+4*i);
		// svr->binaryAll(w,l);
	});
	wsh.onConnect([](AsyncWebSocket *ws,AsyncWebSocketClient *cli){FOR_WHEEL()v[i]=0;});
	wsh.onDisconnect([](AsyncWebSocket *ws,uint32_t cli){FOR_WHEEL()v[i]=0;});
	svr.addHandler(&ws);
	svr.onNotFound([](AsyncWebServerRequest *r){r->redirect(PUB_DIR);});
	svr.serveStatic(PUB_DIR,FSYS,PUB_DIR).setDefaultFile("index.html");
	svr.serveStatic(PVT_DIR,FSYS,PVT_DIR).setDefaultFile("index.html").setAuthentication(NAME,PASS,AsyncAuthType::AUTH_BASIC);
	svr.begin();

}
void loop(){
	// ArduinoOTA.handle();

	if(isAPmode)neopixelWrite(NPDI,0,16,16);// AP mode
	else if(WiFi.isConnected())neopixelWrite(NPDI,0,16,0);// STA connected
	else neopixelWrite(NPDI,16,16,0);// STA disconnected

	FOR_WHEEL(){
		ledcWrite(i,fabs(v[i])*(1<<14));
		digitalWrite(pin[i][1],0<v[i]?HIGH:LOW);
	}

	delay(10);
}
