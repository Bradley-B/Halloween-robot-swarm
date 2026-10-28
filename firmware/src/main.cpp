#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoNATS.h>
#include <ArduinoJson.h>
#include <string.h>

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
    if (deserializeJson(doc, msg.data)) return;
    const char *letter = doc["letter"].as<const char *>();
    if (letter) writeLetterAll(letter);
}

void nats_command_handler(NATS::msg msg) {
    JsonDocument doc;
    if (deserializeJson(doc, msg.data)) return;

    const char *target = doc["target"].as<const char *>();
    const char *command = doc["command"].as<const char *>();
    if (!target || strcmp(target, "all") != 0 || !command) return;

    if (strcmp(command, "matrix.set") == 0) {
        const JsonVariantConst matrix = doc["params"]["matrix"];
        MatrixTarget matrixTarget;
        if (matrix.is<const char *>()) {
            const char *name = matrix.as<const char *>();
            if (strcmp(name, "eyes") == 0) matrixTarget = MatrixTarget::Eyes;
            else if (strcmp(name, "mouth") == 0) matrixTarget = MatrixTarget::Mouth;
            else if (strcmp(name, "all") == 0) matrixTarget = MatrixTarget::All;
            else return;
        } else if (matrix.is<uint8_t>() && matrix.as<uint8_t>() >= 1 &&
                   matrix.as<uint8_t>() <= MATRIX_COUNT) {
            matrixTarget = static_cast<MatrixTarget>(matrix.as<uint8_t>());
        } else {
            return;
        }
        matrixSetAnimation(matrixTarget, doc["params"]["animation"].as<const char *>());
        return;
    }
    if (strcmp(command, "strip.set") != 0) return;

    const char *mode = doc["params"]["mode"].as<const char *>();
    const char *color = doc["params"]["color"].as<const char *>();
    const JsonVariantConst frameInterval = doc["params"]["frame_interval_ms"];
    const JsonVariantConst pulsePeriod = doc["params"]["pulse_period_ms"];
    if ((!frameInterval.isUnbound() && !frameInterval.is<uint32_t>()) ||
        (!pulsePeriod.isUnbound() && !pulsePeriod.is<uint32_t>())) return;

    ledSetState(mode, color,
                frameInterval | STRIP_DEFAULT_FRAME_INTERVAL_MS,
                pulsePeriod | STRIP_DEFAULT_PULSE_PERIOD_MS);
}

void nats_on_connect() {
    nats.subscribe("echo", nats_echo_handler);
    nats.subscribe("robots.commands", nats_command_handler);
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
    matrixLoop();
    yield();  // yield to let esp8266 run background wifi tasks
}
