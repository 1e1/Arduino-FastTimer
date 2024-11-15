#pragma once


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
        return getElapsedTime() * P;
    }

protected:
    uint8_t _referenceTime;
    uint8_t _cachedTime;
};
