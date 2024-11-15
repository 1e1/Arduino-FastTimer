# Time managers

## Arduino Libraries

### FastTimer (approximative time)

Notify at regular intervals to distribute actions over time. 

Define a fixed duration of cycles that will trigger Ticks at regular intervals.
For example, `P_1s_4m` generates a Tick every 1 second and lets you define segment groups of up to 4 minutes. 

`isTick(FastTimer::CUT#)` retrieves a Tick of precise length. 
For example, `FastTimer::CUT64` retrieves Ticks of 4mn/64 ~ 4s.

setup:
```
FastTimer<FastTimerPrecision::P_1s_4m> timer1s;
```

usage:
```
timer1s.update();

if (timer1s.isTick()) {
    Serial.println("tick...");
}
```

### FastTimer with real time = ShortTimer8

Notify at regular intervals to distribute actions over time. 

In this configuration there is no dynamic subsection. 
The single interval is defined in the setup.

setup:
```
ShortTimer8<ShortTimerPrecision::P_seconds> timer1s;
```

usage:
```
if (timer1s.hasChanged()) {
    Serial.println("+1s");
}
```

Although there is no sub-section, there is a counter that can be reset. 
This is a way of grouping sections. 
You can define your own group size (up to 255). 

Example with minutes and seconds:
setup:
```
ShortTimer8<ShortTimerPrecision::P_seconds> timer1s;
```

loop:
```
if (timer1s.hasChanged()) {
    Serial.println("+1s");

    if (timer1s.getElapsedTime() == 60) {
        timer1s.reset();
        Serial.println("= 1mn");
    }
}
```



### TimestampNtp

An NTP client to obtain a Unix or RFC3339 timestamp.

#### by IP

setup:
```
const IPAddress NTP_IP(192, 168, 1, 1);
WiFiUDP udp;
TimestampNtp ntp(udp);

udp.begin(3615); // free random local port
```

request:
```
ntp.request(NTP_IP);
```

response TimestampUnixNtp:
```
if (ntp.listen()) {
    Serial.println(ntp.getTimestampUnix());
    /*
    ntp.syncRFC3339();
    Serial.println(ntp.getTimestampRFC3339());
    */
};
```

response TimestampRFC3339Ntp:
```
if (ntp.listenSync()) {
    // Serial.println(ntp.getTimestampUnix());
    Serial.println(ntp.getTimestampRFC3339());
};
```

#### by Host

setup
```
const char* NTP_HOST = "2.europe.pool.ntp.org";
WiFiUDP udp;
TimestampNtp ntp(udp);

udp.begin(3615); // free random local port
```

request:
```
ntp.request(NTP_HOST);
```

response TimestampUnixNtp:
```
if (ntp.listen()) {
    Serial.println(ntp.getTimestampUnix());
    /*
    ntp.syncRFC3339(offset);
    Serial.println(ntp.getTimestampRFC3339());
    */
};
```

response TimestampRFC3339Ntp:
```
if (ntp.listenSync(offset)) {
    // Serial.println(ntp.getTimestampUnix());
    Serial.println(ntp.getTimestampRFC3339());
};
```

Notice: `getTimestampRFC3339()` is an expensive.

Tips: you can inject myShortTimer.getElapsedTimeInMillis() as offset of myNtp.syncRFC3339(offset), so that you have precision to the second (or minute), whereas network synchronisation is to the minute (or hour). 


## compatibility
- Arduino avr boards
- ESP8266
- ESP32
