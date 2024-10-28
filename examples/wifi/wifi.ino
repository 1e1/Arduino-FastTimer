#include <WiFi.h>
#include <WiFiUdp.h>
#include <IPAddress.h>

#include "FastTimer.hpp"
#include "TimestampNtp.hpp"


#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif


unsigned int localPort = 3669;
unsigned int ntpPort = 123;

WiFiUDP udp;
IPAddress ntpIp;
TimestampNtp ntp;
FastTimer<FastTimer_precision_t::P_1s_4m> timer1s;


void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); 

    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(STASSID, STAPSK);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }


    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    ntpIp = WiFi.gatewayIP();
    Serial.print("NTP/gateway address: ");
    Serial.println(ntpIp);

    // start UDP
    udp.begin(localPort);
}

void loop()
{
    timer1s.update();

    if (timer1s.isPureTickMin()) {
        // ?.maintain()?
        TSNTP_REQUEST(ntp, udp, ntpIp, ntpPort);

        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("Led HIGH");
    } else {
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("Led LOW");
    }

    if (udp.parsePacket()) {
        ntp.readFrom(udp);
        udp.flush();

        Serial.print("Unix timestamp: ");
        Serial.println(ntp.getTimestampUnix());

        Serial.print("RFC3339 timestamp: ");
        Serial.println(ntp.getTimestampRFC3339());
    }

    delay(100);
}

void crash()
{
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