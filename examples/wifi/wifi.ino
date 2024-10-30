#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <IPAddress.h>

#include <FastTimer.hpp>
#include <TimestampNtp.hpp>


#ifndef STASSID
#define STASSID "**** SSID ****"
#define STAPSK  "***password***"
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif


const unsigned int localPort = 3669;
const uint8_t NTP_IP[] = { 94, 23, 21, 189 };

WiFiUDP udp;
IPAddress ntpIp;
TimestampNtp ntp(udp);
FastTimer<FastTimer_precision_t::P_1s_4m> timer1s;


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

    ntpIp = WiFi.gatewayIP();
    Serial.print("Gateway address: ");
    Serial.println(ntpIp);

    ntpIp = IPAddress(NTP_IP);
    Serial.print("NTP address: ");
    Serial.println(ntpIp);

    // start UDP
    udp.begin(localPort);

    Serial.println("*** START ***");
    Serial.flush();
}

void loop()
{
    timer1s.update();

    if (timer1s.isPureTickMin()) {
        // ?.maintain()?
        ntp.request(ntpIp);

        digitalWrite(LED_BUILTIN, LOW);
    }

    delay(100);

    if (ntp.listen()) {
        Serial.print("Unix timestamp: ");
        Serial.println(ntp.getTimestampUnix());

        Serial.print("RFC3339 timestamp: ");
        Serial.println(ntp.getTimestampRFC3339());

        digitalWrite(LED_BUILTIN, HIGH);
    }
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