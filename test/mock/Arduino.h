// Minimal host-side mock of Arduino.h — just enough to compile FastTimer.hpp
// and TimestampNtp.hpp natively for unit tests. Not for use on a device.
#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

typedef uint8_t byte;

// Flash-access shims: on a host there is no separate program memory.
#define PROGMEM
#define pgm_read_byte(addr) (*reinterpret_cast<const uint8_t*>(addr))
#define memcpy_P memcpy

// Minimal String: just enough to instantiate getTimestampRFC3339() natively.
// (The device uses Arduino's real String; here we only need c_str().)
class String {
public:
    String(const char* p) : _p(p) {}
    const char* c_str() const { return _p; }
private:
    const char* _p;
};

// Fake clock driven by the test. Set g_fakeMillis, then call update().
extern unsigned long g_fakeMillis;
inline unsigned long millis() { return g_fakeMillis; }
