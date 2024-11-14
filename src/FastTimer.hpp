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



enum FastTimer_precision_t : byte {
    P_1s_4m =10,    // update()==true every  1s, max time  4m
    P_4s_15m=12,    // update()==true every  4s, max time 15m
    P_16s_1h=14,    // update()==true every 16s, max time  1h
    P_65s_4h=16,    // update()==true every 65s, max time  4h
};

enum ShortTimer_precision_t : uint16_t {
    P_millis =1,
    P_seconds=1000,    
    P_minutes=60000,   
};


template <const FastTimer_precision_t p>
class FastTimer {

    public:

    enum TickIndex : byte {
        TI_CUT256 = 0,
        TI_CUT128 = 1,
        TI_CUT64  = 2,
        TI_CUT32  = 3,
        TI_CUT16  = 4,
        TI_CUT8   = 5,
        TI_CUT4   = 6,
        TI_CUT2   = 7,
    };

    enum PureTickIndex : byte {
        PTI_CUT256 = 1,
        PTI_CUT128 = 3,
        PTI_CUT64  = 7,
        PTI_CUT32  = 15,
        PTI_CUT16  = 31,
        PTI_CUT8   = 63,
        PTI_CUT4   = 127,
        PTI_CUT2   = 255,
    };
    
    FastTimer() : _section(0), _cachedTime(-1) {};

    virtual void update(void) // call it once in the main loop()
    {
        const byte previousTime = this->_cachedTime;
        this->_cachedTime = byte(millis() >> p);
        this->_section = this->_cachedTime ^ previousTime;
    }

    // inline
    __attribute__((always_inline)) inline const boolean isPureTickMin  (void) const { return this->isPureTickBy2();    };
    __attribute__((always_inline)) inline const boolean isPureTickBy2  (void) const { return this->isPureTick(PTI_CUT2);   };
    __attribute__((always_inline)) inline const boolean isPureTickBy4  (void) const { return this->isPureTick(PTI_CUT4);   };
    __attribute__((always_inline)) inline const boolean isPureTickBy8  (void) const { return this->isPureTick(PTI_CUT8);   };
    __attribute__((always_inline)) inline const boolean isPureTickBy16 (void) const { return this->isPureTick(PTI_CUT16);  };
    __attribute__((always_inline)) inline const boolean isPureTickBy32 (void) const { return this->isPureTick(PTI_CUT32);  };
    __attribute__((always_inline)) inline const boolean isPureTickBy64 (void) const { return this->isPureTick(PTI_CUT64);  };
    __attribute__((always_inline)) inline const boolean isPureTickBy128(void) const { return this->isPureTick(PTI_CUT128); };
    __attribute__((always_inline)) inline const boolean isPureTickBy256(void) const { return this->isPureTick(PTI_CUT256); };
    __attribute__((always_inline)) inline const boolean isPureTickMax  (void) const { return this->isPureTickBy256();  };
    __attribute__((always_inline)) inline const boolean isPureTick(const TickIndex index) const { return this->_section & (0x1<<index); };

    __attribute__((always_inline)) inline const boolean isTickMin  (void) const { return this->isTickBy2();    };
    __attribute__((always_inline)) inline const boolean isTickBy2  (void) const { return this->isTick(TI_CUT2);   };
    __attribute__((always_inline)) inline const boolean isTickBy4  (void) const { return this->isTick(TI_CUT4);   };
    __attribute__((always_inline)) inline const boolean isTickBy8  (void) const { return this->isTick(TI_CUT8);   };
    __attribute__((always_inline)) inline const boolean isTickBy16 (void) const { return this->isTick(TI_CUT16);  };
    __attribute__((always_inline)) inline const boolean isTickBy32 (void) const { return this->isTick(TI_CUT32);  };
    __attribute__((always_inline)) inline const boolean isTickBy64 (void) const { return this->isTick(TI_CUT64);  };
    __attribute__((always_inline)) inline const boolean isTickBy128(void) const { return this->isTick(TI_CUT128); };
    __attribute__((always_inline)) inline const boolean isTickBy256(void) const { return this->isTick(TI_CUT256); };
    __attribute__((always_inline)) inline const boolean isTickMax  (void) const { return this->isTickBy256();  };
    __attribute__((always_inline)) inline const boolean isTick(const TickIndex index) const { return this->_section >> index; };
    __attribute__((always_inline)) inline const boolean isTick(void) const { return this->_section; };

    __attribute__((always_inline)) inline unsigned long getCachedMillis(void) const { return this->_cachedTime << p; };

    protected:

    byte _section;
    byte _cachedTime;
};


template <const ShortTimer_precision_t p>
class ShortTimer8 {

    public:
    

    ShortTimer8() {
        this->update();
        this->reset();
    };

    constexpr uint16_t getPrecisionInMillis(void) const
    {
        return p;
    }

    const boolean hasChanged(void) // call it once in the main loop()
    {
        const uint8_t previousTime = this->_cachedTime;
        this->update();

        return this->_cachedTime ^ previousTime;
    }

    void update(void)
    {
        this->_cachedTime = byte(millis() / p);
    }

    void reset(void)
    {
        this->_referenceTime = this->_cachedTime;
    }

    const uint8_t getElapsedTime(void) const
    {
        return this->_cachedTime - this->_referenceTime;
    }

    const unsigned long getElapsedTimeInMillis(void) const
    {
        return this->getElapsedTime() * p;
    }

    protected:

    uint8_t _referenceTime;
    uint8_t _cachedTime;
};
