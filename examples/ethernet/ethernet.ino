#include <Arduino.h>
#include <Ethernet.h>
#include <IPAddress.h>

#include <FastTimer.hpp>
#include <TimestampNtp.hpp>


// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
    0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};


#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif


const unsigned int LOCAL_PORT = 3669;
const IPAddress NTP_IP = IPAddress(45, 138, 55, 62); // 2.europe.pool.ntp.org

TimestampNtp<EthernetUDP> nts;
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

    Serial.print("Gateway address: ");
    Serial.println(Ethernet.gatewayIP());

    Serial.print("NTP address: ");
    Serial.println(NTP_IP);

    // start UDP
    nts.begin(LOCAL_PORT);

    Serial.println("*** START ***");
    Serial.flush();
}

void loop()
{
    timer1s.update();

    if (timer1s.isPureTickMin()) {
        Ethernet.maintain();
        nts.request(NTP_IP);

        digitalWrite(LED_BUILTIN, LOW);
    }

    delay(100);

    if (nts.listen()) {
        Serial.print("Unix timestamp: ");
        Serial.println(nts.getTimestampUnix());

        Serial.print("RFC3339 timestamp: ");
        Serial.println(nts.getTimestampRFC3339());

        digitalWrite(LED_BUILTIN, HIGH);
    }
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