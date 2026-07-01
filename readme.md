# Time managers

[![compile-examples](https://github.com/1e1/Arduino-FastTimer/actions/workflows/compile-examples.yml/badge.svg)](https://github.com/1e1/Arduino-FastTimer/actions/workflows/compile-examples.yml)

## Arduino Libraries

### FastTimer (approximative time)

Notify at regular intervals to distribute actions over time. 

Define a fixed duration of cycles that will trigger Ticks at regular intervals.
For example, `P_1s_4m` generates a Tick every 1 second and lets you define segment groups of up to 4 minutes. 

#### How it works

FastTimer holds a single 8-bit counter, `_cachedTime = (millis() >> P)`. The
precision `P` sets the duration of one unit, and 256 units make the full range:

| precision   | 1 unit  | full range |
|-------------|---------|------------|
| `P_1s_4m`   | ~1.024s | ~4 min     |
| `P_4s_15m`  | ~4.1s   | ~15 min    |
| `P_16s_1h`  | ~16.4s  | ~1 h       |
| `P_65s_4h`  | ~65.5s  | ~4 h       |

Each `update()` records which bits of the counter flipped since the last call.
The lowest bit flips every unit, the next every 2 units, ..., the highest every
128 units — so each bit is a Tick source of a different period.

- `isTick()` — something ticked this update (any bit flipped).
- `isTickByN()` — the `full range / N` Tick **or any coarser one**. For example
  `isTickBy64()` fires ~every 4mn/64 ~ 4s with `P_1s_4m`; a coarser tick also
  lights up the finer ones.
- `isPureTickByN()` — **only** that exact boundary, without the coarser overlap.

Call `update()` often enough (loop period shorter than one unit) or Ticks may be
skipped. Cost: 2 bytes of RAM, no division, no allocation.

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

if (timer1s.isTickBy64()) {
    Serial.println("...every 4s");
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
TimestampNtp<WiFiUDP> nts;

nts.begin(); // OR force a port: nts.begin(3615);

```

request:
```
nts.request(NTP_IP);
```

response TimestampUnixNtp:
```
if (nts.listen()) {
    Serial.println(nts.getTimestampUnix());
    /*
    nts.syncRFC3339();
    Serial.println(nts.getTimestampRFC3339());
    */
};
```

response TimestampRFC3339Ntp:
```
if (nts.listenSync()) {
    // Serial.println(nts.getTimestampUnix());
    Serial.println(nts.getTimestampRFC3339());
};
```

#### by Host

setup
```
const char* NTP_HOST = "2.europe.pool.ntp.org";
TimestampNtp<WiFiUDP> nts;

nts.begin(); // OR force a port: nts.begin(3615);
```

request:
```
nts.request(NTP_HOST);
```

response TimestampUnixNtp:
```
if (nts.listen()) {
    Serial.println(nts.getTimestampUnix());
    /*
    nts.syncRFC3339(offset);
    Serial.println(nts.getTimestampRFC3339());
    */
};
```

response TimestampRFC3339Ntp:
```
if (nts.listenSync(offset)) {
    // Serial.println(nts.getTimestampUnix());
    Serial.println(nts.getTimestampRFC3339());
};
```

Notice: `getTimestampRFC3339()` returns a `String` (it allocates a copy). To
avoid the allocation, use `c_str()`, which points straight at the internal
buffer (valid until the next `syncRFC3339()`):
```
Serial.println(nts.c_str());
```

Tips: you can inject myShortTimer.getElapsedTimeInMillis() as offset of myNtp.syncRFC3339(offset), so that you have precision to the second (or minute), whereas network synchronisation is to the minute (or hour). 


## compatibility

`FastTimer` and `ShortTimer8` run everywhere:
- Arduino avr boards
- ESP8266
- ESP32

`TimestampNtp` targets 32-bit cores (ESP8266, ESP32). On 8-bit AVR (16-bit
`int`) the NTP timestamp assembly overflows: use it on ESP, or feed the raw
packet bytes into a `long` yourself. `getTimestampRFC3339()` also assumes the
year stays within `2024..2099`.
