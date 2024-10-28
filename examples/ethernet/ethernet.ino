#include <Arduino.h>
#include <Ethernet.h>

#include "FastTimer.hpp"
#include "TimestampNtp.hpp"


// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
    0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};


#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif


unsigned int localPort = 3669;
unsigned int ntpPort = 123;

EthernetUDP udp;
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

    // start the Ethernet
    Ethernet.begin(mac);

    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
        crash();
    }
    if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("Ethernet cable is not connected.");
        crash();
    }


    Serial.print("IP address: ");
    Serial.println(Ethernet.localIP());

    // IPAddress dhcpAddress = Ethernet._dhcp->getDhcpServerIp();
    ntpIp = Ethernet.gatewayIP();
    Serial.print("NTP/gateway address: ");
    Serial.println(ntpIp);

    // start UDP
    udp.begin(localPort);
}

void loop()
{
    timer1s.update();

    if (timer1s.isPureTickMin()) {
        Ethernet.maintain();
        {   // send NTP packet
            udp.beginPacket(ntpIp, ntpPort);
            ntp.writeIn(udp);
            udp.endPacket();
        }

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