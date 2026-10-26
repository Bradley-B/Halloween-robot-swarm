#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoNATS.h>
#include <ArduinoJson.h>

#include "frames.h"
#include "strip.h"
#include "matrix.h"

const char* WIFI_SSID = "";
const char* WIFI_PSK = "";

WiFiClient client;
NATS nats(
	&client,
	"192.168.0.105", 4223
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

    JsonDocument doc;
    deserializeJson(doc, msg.data);
    const char *letter = doc["letter"];
    writeLetterAll(letter);
}

void nats_on_connect() {
    nats.subscribe("echo", nats_echo_handler);
}

// put your setup code here, to run once:
void setup() {
    ledSetup();
    matrixSetup();
    setMonoFrameAll(&xFrame);

    connect_wifi();
    nats.on_connect = nats_on_connect;
    nats.connect();
    setMonoFrameAll(&circleFrame);
}

// put your main code here, to run repeatedly:
void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        setMonoFrameAll(&xFrame);
        connect_wifi();
        setMonoFrameAll(&circleFrame);
    }
    nats.process();
    ledLoop();
    yield();  // yield to let esp8266 run background wifi tasks
}
