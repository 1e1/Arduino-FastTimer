# Time managers

## Arduino Libraries

### FastTimer

Notify at regular intervals to distribute actions over time. 

Define a fixed duration of cycles that will trigger Ticks at regular intervals.
For example, `P_1s_4m` generates a Tick every 1 second and lets you define segment groups of up to 4 minutes. 

`isTick(FastTimer::CUT#)` retrieves a Tick of precise length. 
For example, `FastTimer::CUT64` retrieves Ticks of 4mn/64 ~ 4s.

setup:
```
FastTimer<FastTimer_precision_t::P_1s_4m> timer1s;
```

usage:
```
timer1s.update();

if (timer1s.isTick()) {
    Serial.println("tick...");
}
```

### TimestampNtp

An NTP client to obtain a Unix or RFC3339 timestamp.

setup:
```
IPAddress ntpIp(94, 23, 21, 189); // 0.fr.pool.ntp.org
WiFiUDP udp;
TimestampNtp ntp(udp);

udp.begin(3615); // free random local port
```

request:
```
ntp.request(ntpIp);
```

response:
```
ntp.listen();
```

usage:
```
Serial.print("Unix timestamp: ");
Serial.println(ntp.getTimestampUnix());

Serial.print("RFC3339 timestamp: ");
Serial.println(ntp.getTimestampRFC3339());
```

Notice: `getTimestampRFC3339()` is an expensive function


## compatibility
- Arduino avr boards
- ESP8266
- ESP32
