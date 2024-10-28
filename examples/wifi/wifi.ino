#include <WiFi.h>
#include <WiFiUdp.h>
#include <IPAddress.h>

#include <FastTimerNtp.hpp>


#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

#define LED_BUILTIN 2


unsigned int localPort = 3669;
unsigned int ntpPort = 123;

WiFiUDP udp;
FastTimerNtp<FastTimer_precision_t::P_1s_4m> timer1s;
IPAddress ntpIp;


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
    if (timer1s.update()) {

        if (timer1s.isTick8()) {
            // Ethernet.maintain();
            {   // send NTP packet
                udp.beginPacket(ntpIp, ntpPort);
                timer1s.writeIn(udp);
                udp.endPacket();
            }

            digitalWrite(LED_BUILTIN, HIGH);
            Serial.println("Led HIGH");
        } else {
            digitalWrite(LED_BUILTIN, LOW);
            Serial.println("Led LOW");
        }
    }

    if (udp.parsePacket()) {
        timer1s.readFrom(udp);
        udp.flush();
    }

    delay(100);
}

void crash()
{
    bool isLedOn; 
    while (true) {
        if (timer1s.update()) {
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