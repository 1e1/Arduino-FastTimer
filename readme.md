# Fasttimer - Arduino Library

Notify at regular intervals to distribute actions over time. 
Can be extended with an NTP client to obtain a Unix or RFC3339 timestamp.


## compatibility
- Arduino avr boards
- ESP8266
- ESP32


## Issues - Workarounds

As the IPAddress class is not the same between AVR and ESP (different lwip), the UDP client cannot be abstracted. 
You must manage the UDP client outside this library. 

Choose into your ino file: 
- `EthernetUDP udp;`
- `WiFiUDP udp;`

request: rarely
```
udpClient.beginPacket(ntpIp, ntpPort);
fastTimer.writeIn(udpClient);
udpClient.endPacket();
```

response: often
```
if (udpClient.parsePacket()) {
    fastTimer.readFrom(udpClient);
    udpClient.flush();
}
```
