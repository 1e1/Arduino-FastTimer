#pragma once

#include <Arduino.h>


/** TIME **/
//==>  >> 0: 1 unit of embedTime is 0.001s
//-->  const unsigned long maxTime        = 4294967295; // = 49d 17h 02m 47s
//-->  const unsigned int maxTime         = 65535;      // = 65s
//==>  >> 10: 1 unit of embedTime is 1.024s
//-->  const uint8_t maxEmbedTime         = 255;        // = 4mn 21s 120ms
//==>  >> 12: 1 unit of embedTime is 4.096s
//-->  const unsigned int maxEmbedTime    = 65535;      // = 3d 02h 33mn 51s 360ms
//     const unsigned int moduloEmbedTime = 63281;      // = 3d 00h 00mn 00s 000ms
//-->  const uint8_t maxEmbedTime         = 255;        // = 17mn 24s 480ms
//     const uint8_t moduloEmbedTime      = 219;        // = 15mn 00s 000ms
//==>  >> 14: 1 unit of embedTime is 16.384s
//-->  const uint8_t maxEmbedTime         = 255;        // = 1h 09mn 37s 920ms
//     const uint8_t moduloEmbedTime      = 219;        // = 1h 00mn 00s 000ms
//==>  >> 16: 1 unit of embedTime is 65.536s
//-->  const uint8_t maxEmbedTime         = 255;        // = 4h 38mn 31s 680ms
//     const uint8_t moduloEmbedTime      = 219;        // = 4h 00mn 00s 000ms



enum FastTimer_precision_t : uint8_t {
    P_1s_4m=10,     // update()==true every  1s, max time  4m
    P_4s_15m=12,    // update()==true every  4s, max time 15m
    P_16s_1h=14,    // update()==true every 16s, max time  1h
    P_65s_4h=16,    // update()==true every 65s, max time  4h
};


template <FastTimer_precision_t p>
class FastTimer {

    public:
    
    FastTimer() : _section(0), _cachedTime(-1) {};

    virtual const uint8_t update(void) // call it once in the main loop()
    {
        const uint8_t previousTime = this->_cachedTime;
        this->_cachedTime = byte(millis() >> p);
        this->_section = this->_cachedTime ^ previousTime;
        return this->_section;
    }

    // inline
    __attribute__((always_inline)) inline const boolean isTick2  (void) const { return this->isTick(0x02); };
    __attribute__((always_inline)) inline const boolean isTick4  (void) const { return this->isTick(0x04); };
    __attribute__((always_inline)) inline const boolean isTick8  (void) const { return this->isTick(0x08); };
    __attribute__((always_inline)) inline const boolean isTick16 (void) const { return this->isTick(0x10); };
    __attribute__((always_inline)) inline const boolean isTick32 (void) const { return this->isTick(0x20); };
    __attribute__((always_inline)) inline const boolean isTick64 (void) const { return this->isTick(0x40); };
    __attribute__((always_inline)) inline const boolean isTick128(void) const { return this->isTick(0x80); };
    
    __attribute__((always_inline)) inline const boolean isTick(const byte section) const { return this->_section & section; };
    __attribute__((always_inline)) inline const uint8_t getCachedTime(void) const { return this->_cachedTime; };

    protected:

    uint8_t _section;
    uint8_t _cachedTime;
};
