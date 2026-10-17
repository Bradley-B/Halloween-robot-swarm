#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoNATS.h>

#include "matrix.h"

const char* WIFI_SSID = "Internet";
const char* WIFI_PSK = "password";

WiFiClient client;
NATS nats(
	&client,
	"demo.nats.io", NATS_DEFAULT_PORT
);

void connect_wifi() {
	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PSK);
	while (WiFi.status() != WL_CONNECTED) {
		yield();
	}
}

void nats_echo_handler(NATS::msg msg) {
	nats.publish(msg.reply, msg.data);
}

void nats_on_connect() {
	nats.subscribe("echo", nats_echo_handler);
}

// put your setup code here, to run once:
void setup() {
  matrixSetup();
  connect_wifi();
	nats.on_connect = nats_on_connect;
	nats.connect();
}

// put your main code here, to run repeatedly:
void loop() {
  matrixLoop();
  if (WiFi.status() != WL_CONNECTED) connect_wifi();
	nats.process();
	yield(); // yield to let esp8266 run background wifi tasks
}
