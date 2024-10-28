#include <Arduino.h>

#include <FastTimer.hpp>


bool isLedOn; 
FastTimer<FastTimer_precision_t::P_1s_4m> timer1s;


void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);    
    isLedOn = false;
}

void loop()
{
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