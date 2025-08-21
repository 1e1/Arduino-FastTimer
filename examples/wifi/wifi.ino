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

TimestampNtp<WiFiUDP> nts;
FastTimer<FastTimerPrecision::P_1s_4m> timer1s;


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
    nts.begin(LOCAL_PORT);

    Serial.println("*** START ***");
    Serial.flush();
}

void loop()
{
    timer1s.update();

    if (timer1s.isTickBy64()) {
        // ?.maintain()?
        nts.request(NTP_HOST);

        digitalWrite(LED_BUILTIN, LOW);
    }

    delay(100);

    if (nts.listenSync()) {
        Serial.print("Unix timestamp: ");
        Serial.println(nts.getTimestampUnix());

        Serial.print("RFC3339 timestamp: ");
        Serial.println(nts.getTimestampRFC3339());

        Serial.printf("[HW] Free heap: %d bytes\n", ESP.getFreeHeap());

        digitalWrite(LED_BUILTIN, HIGH);
    }

    delay(100);
}

void crash()
{
    Serial.println("*** CRASH ***");
    
    bool isLedOn; 
    while (true) {
        timer1s.update();

        if (timer1s.isTick()) {
            isLedOn = !isLedOn;
            if (isLedOn) {
                digitalWrite(LED_BUILTIN, HIGH);
            } else {
                digitalWrite(LED_BUILTIN, LOW);
            }
        }

        delay(100);
    }
}