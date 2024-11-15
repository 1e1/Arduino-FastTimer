#include <Arduino.h>

#include <FastTimer.hpp>


bool isLedOn; 
FastTimer<FastTimerPrecision::P_1s_4m> timer1s;


void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);    
    isLedOn = false;
}

void loop()
{
    timer1s.update();

    if (timer1s.isTick()) {
        isLedOn = !isLedOn;
        if (isLedOn) {
            digitalWrite(LED_BUILTIN, LOW);
        } else {
            digitalWrite(LED_BUILTIN, HIGH);
        }
    }

    delay(100);
}