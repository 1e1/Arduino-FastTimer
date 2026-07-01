#include <Arduino.h>

#include <FastTimer.hpp>


// P_1s_4m: 1 unit ~ 1s, full range ~ 4mn.
FastTimer<FastTimerPrecision::P_1s_4m> timer;


void setup()
{
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    Serial.println("*** START ***");
}

void loop()
{
    timer.update();

    // isTickByN() fires on its own boundary AND on every coarser one.
    // isPureTickByN() fires ONLY on its exact boundary.
    //
    // With P_1s_4m (1 unit ~ 1s), isTickBy16() fires every 16 units ~ 16s.
    // One boundary out of two (the multiples of 32 ~ 32s) is also a coarser
    // boundary, so there isTickBy16() stays true while isPureTickBy16() goes
    // false: that instant "belongs" to the coarser tick.
    //
    //   ~16s: pure 16 tick        (isPureTickBy16 true)
    //   ~32s: shared with coarser (isTickBy16 true, isPureTickBy16 false)
    //   ~48s: pure 16 tick        (isPureTickBy16 true)
    //   ~64s: shared with coarser ...

    if (timer.isPureTickBy16()) {
        Serial.println("pure 16s tick (this boundary only)");
    } else if (timer.isTickBy16()) {
        Serial.println("16s tick shared with a coarser one");
    }

    delay(100);
}
