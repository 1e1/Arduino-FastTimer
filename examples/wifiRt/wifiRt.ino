#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <IPAddress.h>

#include <FastTimer.hpp>
#include <TimestampNtp.hpp>


#define STASSID "**** SSID ****"
#define STAPSK  "***password***"

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif


const unsigned int LOCAL_PORT = 3669;
const char* NTP_HOST = "2.europe.pool.ntp.org";

WiFiUDP udp;
TimestampNtp ntp(udp);
ShortTimer8<ShortTimer_precision_t::P_seconds> timer1s;


void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); 

    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    Serial.flush();


    WiFi.begin(STASSID, STAPSK);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }
    Serial.println();


    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.print("Gateway address: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("NTP host: ");
    Serial.println(NTP_HOST);

    // start UDP
    udp.begin(LOCAL_PORT);
    ntp.request(NTP_HOST);

    Serial.println("*** START ***");
    Serial.flush();
}

void loop()
{
    if (timer1s.hasChanged()) {
        const uint8_t delta = timer1s.getElapsedTime();

        Serial.print("(estimated) Unix timestamp: ");
        Serial.println(ntp.getTimestampUnix());

        ntp.syncRFC3339(delta);
        Serial.print("(estimated) RFC3339 timestamp: ");
        Serial.println(ntp.getTimestampRFC3339());

        // update every minute
        if (delta >= 60) {
            // ?.maintain()?
            ntp.request(NTP_HOST);

            digitalWrite(LED_BUILTIN, LOW);
        }
    }

    if (ntp.listen()) {
        timer1s.reset();

        Serial.print("(fresh) Unix timestamp: ");
        Serial.println(ntp.getTimestampUnix());

        ntp.syncRFC3339();
        Serial.print("(fresh) RFC3339 timestamp: ");
        Serial.println(ntp.getTimestampRFC3339());

        Serial.printf("[HW] Free heap: %d bytes\n", ESP.getFreeHeap());

        digitalWrite(LED_BUILTIN, HIGH);
    }

    delay(250);
}

void crash()
{
    Serial.println("*** CRASH ***");
    
    bool isLedOn; 
    while (true) {
        if (timer1s.hasChanged()) {
            isLedOn = !isLedOn;
            if (isLedOn) {
                digitalWrite(LED_BUILTIN, HIGH);
            } else {
                digitalWrite(LED_BUILTIN, LOW);
            }
        }

        delay(250);
    }
}