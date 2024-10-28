#include <Arduino.h>
#include <Ethernet.h>

#include <FastTimerNtp.hpp>


// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
    0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};

unsigned int localPort = 3669;
unsigned int ntpPort = 123;

EthernetUDP udp;
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
    if (timer1s.update()) {

        if (timer1s.isTick8()) {
            Ethernet.maintain();
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