#include <Arduino.h>
#include "WiFi.h"
#include "WebSocketsClient.h"
#include <ArduinoJson.h>

 const char *ssid = "samoanja";
 const char *password = "samoanja25";
 const char *serverIP = "192.168.1.205";

const int pinGlavnaLED = 4;
const int pinUgradjenaLED = 2;
const int nozicaFotoupornika = 36;

int vrednostFotoupornika;
int prethodnaVrednostFoto = 0;

bool glavnaLEDMoraSvetleti = false;
bool uAlarmu = false;
unsigned long vremeNaredbe = 0;
unsigned long prethodnoVremeTreptanja = 0;
bool stanjeUgradjeneLED = false;

WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    if (type == WStype_TEXT)
    {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, payload, length);
        if (error)
            return;

        const char *tip = doc["tipSporočila"];
        if (tip != nullptr && strcmp(tip, "LED") == 0)
        {
            int vrednost = doc["vrednost"];
            if (vrednost == 1)
            {
                glavnaLEDMoraSvetleti = true;
                vremeNaredbe = millis();
                uAlarmu = false;
                digitalWrite(pinGlavnaLED, HIGH);
                digitalWrite(pinUgradjenaLED, LOW);
            }
            else
            {
                glavnaLEDMoraSvetleti = false;
                uAlarmu = false;
                digitalWrite(pinGlavnaLED, LOW);
                digitalWrite(pinUgradjenaLED, LOW);

                DynamicJsonDocument statusDoc(128);
                statusDoc["tipSporočila"] = "status";
                statusDoc["stanje"] = "OK";
                String statusStr;
                serializeJson(statusDoc, statusStr);
                webSocket.sendTXT(statusStr);
            }
        }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    webSocket.begin(serverIP, 8811);
    webSocket.onEvent(webSocketEvent);

    pinMode(pinGlavnaLED, OUTPUT);
    pinMode(pinUgradjenaLED, OUTPUT);

    digitalWrite(pinGlavnaLED, LOW);
    digitalWrite(pinUgradjenaLED, LOW);
}

void loop()
{
    webSocket.loop();

    vrednostFotoupornika = analogRead(nozicaFotoupornika);

    if (glavnaLEDMoraSvetleti && !uAlarmu)
    {
        if (millis() - vremeNaredbe > 3000)
        {
            uAlarmu = true;
            glavnaLEDMoraSvetleti = false;

            digitalWrite(pinGlavnaLED, LOW);

            DynamicJsonDocument statusDoc(128);
            statusDoc["tipSporočila"] = "status";
            statusDoc["stanje"] = "ALARM";
            String statusStr;
            serializeJson(statusDoc, statusStr);
            webSocket.sendTXT(statusStr);
        }
    }

    if (uAlarmu)
    {
        if (millis() - prethodnoVremeTreptanja > 150)
        {
            prethodnoVremeTreptanja = millis();
            stanjeUgradjeneLED = !stanjeUgradjeneLED;
            digitalWrite(pinUgradjenaLED, stanjeUgradjeneLED ? HIGH : LOW);
        }
    }

    if (abs(vrednostFotoupornika - prethodnaVrednostFoto) > 20)
    {
        DynamicJsonDocument fotoDoc(128);
        fotoDoc["tipSporočila"] = "fotoupornik";
        fotoDoc["vrednost"] = vrednostFotoupornika;
        String fotoStr;
        serializeJson(fotoDoc, fotoStr);
        webSocket.sendTXT(fotoStr);
        prethodnaVrednostFoto = vrednostFotoupornika;
    }

    delay(10);
}