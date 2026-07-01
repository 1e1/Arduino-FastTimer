#pragma once

#include <Arduino.h>


/** HOW FASTTIMER WORKS **
 *
 * FastTimer keeps a single 8-bit counter derived from millis():
 *     _cachedTime = (uint8_t)(millis() >> P)
 * P is the precision (a right shift), so it sets the duration of "1 unit":
 *     P=10 -> unit = 2^10 ms = 1.024s   (256 units ~ 4mn : P_1s_4m)
 *     P=12 -> unit = 2^12 ms = 4.096s   (256 units ~ 15mn: P_4s_15m)
 *     P=14 -> unit = 2^14 ms = 16.384s  (256 units ~ 1h  : P_16s_1h)
 *     P=16 -> unit = 2^16 ms = 65.536s  (256 units ~ 4h  : P_65s_4h)
 *
 * update() records which bits of the counter flipped since the last call:
 *     _section = _cachedTime ^ previousTime
 * The lowest bit flips every unit, the next every 2 units, ... the highest
 * every 128 units. So each bit is a tick source of a different period.
 *
 *   isTick()        -> true if anything ticked this update (any bit flipped)
 *   isTickByN()     -> true for the "full range / N" tick OR any coarser one
 *                      (tests _section >> index; a coarse tick also lights
 *                       up the finer ones, e.g. isTickBy64 ~ every 4mn/64)
 *   isPureTickByN() -> true ONLY for that exact boundary (_section == mask),
 *                      without the coarser overlap
 *
 * Call update() often enough (loop period < 1 unit) or ticks may be skipped.
 * Cost: 2 bytes of RAM, no division, no allocation.
 */



enum class FastTimerPrecision : uint8_t {
    P_1s_4m = 10,   // update() == true every 1s, max time 4m
    P_4s_15m = 12,  // update() == true every 4s, max time 15m
    P_16s_1h = 14,  // update() == true every 16s, max time 1h
    P_65s_4h = 16,  // update() == true every 65s, max time 4h
};

enum class ShortTimerPrecision : uint16_t {
    P_millis = 1,
    P_seconds = 1000,
    P_minutes = 60000,
};


template <FastTimerPrecision P>
class FastTimer {
public:
    enum TickIndex : uint8_t {
        TI_CUT256 = 0,
        TI_CUT128 = 1,
        TI_CUT64 = 2,
        TI_CUT32 = 3,
        TI_CUT16 = 4,
        TI_CUT8 = 5,
        TI_CUT4 = 6,
        TI_CUT2 = 7,
    };

    enum PureTickMask : uint8_t {
        PTM_CUT256 = 1,
        PTM_CUT128 = 3,
        PTM_CUT64 = 7,
        PTM_CUT32 = 15,
        PTM_CUT16 = 31,
        PTM_CUT8 = 63,
        PTM_CUT4 = 127,
        PTM_CUT2 = 255,
    };

    FastTimer() : _section(0), _cachedTime(-1) {}

    void update() {
        const uint8_t previousTime = _cachedTime;
        _cachedTime = static_cast<uint8_t>(millis() >> static_cast<uint8_t>(P));
        _section = _cachedTime ^ previousTime;
    }

    inline bool isPureTickMin() const { return isPureTickBy2(); }
    inline bool isPureTickBy2() const { return isPureTick(PTM_CUT2); }
    inline bool isPureTickBy4() const { return isPureTick(PTM_CUT4); }
    inline bool isPureTickBy8() const { return isPureTick(PTM_CUT8); }
    inline bool isPureTickBy16() const { return isPureTick(PTM_CUT16); }
    inline bool isPureTickBy32() const { return isPureTick(PTM_CUT32); }
    inline bool isPureTickBy64() const { return isPureTick(PTM_CUT64); }
    inline bool isPureTickBy128() const { return isPureTick(PTM_CUT128); }
    inline bool isPureTickBy256() const { return isPureTick(PTM_CUT256); }
    inline bool isPureTickMax() const { return isPureTickBy256(); }
    inline bool isPureTick(PureTickMask mask) const { return _section == mask; }

    inline bool isTickMin() const { return isTickBy2(); }
    inline bool isTickBy2() const { return isTick(TI_CUT2); }
    inline bool isTickBy4() const { return isTick(TI_CUT4); }
    inline bool isTickBy8() const { return isTick(TI_CUT8); }
    inline bool isTickBy16() const { return isTick(TI_CUT16); }
    inline bool isTickBy32() const { return isTick(TI_CUT32); }
    inline bool isTickBy64() const { return isTick(TI_CUT64); }
    inline bool isTickBy128() const { return isTick(TI_CUT128); }
    inline bool isTickBy256() const { return isTick(TI_CUT256); }
    inline bool isTickMax() const { return isTickBy256(); }
    inline bool isTick(TickIndex index) const { return _section >> index; }
    inline bool isTick() const { return _section; }

    inline unsigned long getCachedMillis() const { return static_cast<unsigned long>(_cachedTime) << static_cast<uint8_t>(P); }

protected:
    uint8_t _section;
    uint8_t _cachedTime;
};


template <ShortTimerPrecision P>
class ShortTimer8 {
public:
    ShortTimer8() {
        update();
        reset();
    }

    constexpr ShortTimerPrecision getPrecisionInMillis() const {
        return P;
    }

    bool hasChanged() {
        const uint8_t previousTime = _cachedTime;
        update();
        return _cachedTime != previousTime;
    }

    void update() {
        _cachedTime = byte(millis() / static_cast<uint16_t>(P));
    }

    void reset() {
        _referenceTime = _cachedTime;
    }

    uint8_t getElapsedTime() const {
        return _cachedTime - _referenceTime;
    }

    unsigned long getElapsedTimeInMillis() const {
        return static_cast<unsigned long>(getElapsedTime()) * static_cast<uint16_t>(P);
    }

protected:
    uint8_t _referenceTime;
    uint8_t _cachedTime;
};
