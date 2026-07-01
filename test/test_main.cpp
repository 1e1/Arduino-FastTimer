// Native (host) unit tests for the pure logic of the library.
// Built with the mocks in test/mock/ — see test/run.sh.
//
// Covers the three non-obvious cores:
//   - FastTimer tick math (XOR sections, isTickByN vs isPureTickByN)
//   - ShortTimer8 elapsed-time accounting
//   - TimestampRFC3339Ntp date/time conversion (known Unix -> UTC vectors)

#include <cstdio>
#include <cstring>

#include "FastTimer.hpp"
#include "TimestampNtp.hpp"

// Backing storage for the mocked millis().
unsigned long g_fakeMillis = 0;

static int g_failures = 0;

#define CHECK(cond) do { \
    if (!(cond)) { \
        ++g_failures; \
        std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

#define CHECK_STR(got, want) do { \
    if (std::strcmp((got), (want)) != 0) { \
        ++g_failures; \
        std::printf("  FAIL %s:%d  got \"%s\" want \"%s\"\n", __FILE__, __LINE__, (got), (want)); \
    } \
} while (0)


// --- FastTimer ------------------------------------------------------------

// Step the timer so its 8-bit counter transitions (n-1) -> n, i.e. reproduce
// the XOR pattern of a single-unit increment landing on counter value n.
template <FastTimerPrecision P>
static void stepTo(FastTimer<P>& t, unsigned long n)
{
    const unsigned long unit = 1UL << static_cast<uint8_t>(P);
    g_fakeMillis = (n - 1) * unit;
    t.update();
    g_fakeMillis = n * unit;
    t.update();
}

static void test_fasttimer()
{
    std::printf("FastTimer\n");
    FastTimer<FastTimerPrecision::P_1s_4m> t;

    // counter 0 -> 1: finest tick only
    stepTo(t, 1);
    CHECK(t.isTick());
    CHECK(t.isTickBy256());
    CHECK(t.isPureTickBy256());
    CHECK(!t.isTickBy2());
    CHECK(!t.isPureTickBy16());

    // counter -> 16: a "pure 16" boundary
    stepTo(t, 16);
    CHECK(t.isTick());
    CHECK(t.isPureTickBy16());
    CHECK(t.isTickBy16());
    CHECK(!t.isPureTickBy8());
    CHECK(!t.isTickBy2());
    CHECK(t.getCachedMillis() == (16UL << 10));

    // counter -> 32: multiple of 32 => isTickBy16 stays true, but the boundary
    // "belongs" to a coarser tick so isPureTickBy16 is false (the ticks example).
    stepTo(t, 32);
    CHECK(t.isTickBy16());
    CHECK(!t.isPureTickBy16());
    CHECK(t.isPureTickBy8());

    // counter -> 128: coarsest boundary, all bits flip
    stepTo(t, 128);
    CHECK(t.isPureTickBy2());
    CHECK(t.isTickBy2());
    CHECK(t.isTickMax());

    // no advance => no tick
    t.update();
    CHECK(!t.isTick());
}


// --- ShortTimer8 ----------------------------------------------------------

static void test_shorttimer()
{
    std::printf("ShortTimer8\n");
    g_fakeMillis = 0;
    ShortTimer8<ShortTimerPrecision::P_seconds> t; // ctor: update() + reset()

    CHECK(t.getElapsedTime() == 0);
    CHECK(!t.hasChanged()); // same second

    g_fakeMillis = 1000;
    CHECK(t.hasChanged());
    CHECK(t.getElapsedTime() == 1);

    g_fakeMillis = 100000; // 100 s
    t.update();
    CHECK(t.getElapsedTime() == 100);
    // widened math: 100 * 1000 must not be truncated
    CHECK(t.getElapsedTimeInMillis() == 100000UL);

    t.reset();
    CHECK(t.getElapsedTime() == 0);
}


// --- TimestampRFC3339Ntp --------------------------------------------------

struct FakeUdp { };

template <typename U>
struct TestNtp : public TimestampRFC3339Ntp<U> {
    void setUnix(unsigned long unixSeconds) {
        this->_secondsSince1900 =
            unixSeconds + TimestampUnixNtp<U>::OFFSET_MON_JAN_1ST_1900_TO_UNIX_EPOCH;
    }
};

static void test_rfc3339()
{
    std::printf("TimestampRFC3339Ntp\n");
    struct { unsigned long unixTime; const char* expected; } vecs[] = {
        { 1709251200UL, "2024-03-01T00:00:00Z" },
        { 1709251201UL, "2024-03-01T00:00:01Z" },
        { 1709251260UL, "2024-03-01T00:01:00Z" },
        { 1709254800UL, "2024-03-01T01:00:00Z" },
        { 1735689599UL, "2024-12-31T23:59:59Z" },
        { 1735689600UL, "2025-01-01T00:00:00Z" },
        { 1803859199UL, "2027-02-28T23:59:59Z" },
        { 1803859200UL, "2027-03-01T00:00:00Z" },
        { 1835438400UL, "2028-02-29T12:00:00Z" },
        { 4102444799UL, "2099-12-31T23:59:59Z" },
    };

    for (auto& v : vecs) {
        TestNtp<FakeUdp> ntp;
        ntp.setUnix(v.unixTime);
        CHECK(ntp.getTimestampUnix() == v.unixTime);
        ntp.syncRFC3339();
        CHECK_STR(ntp.c_str(), v.expected);                        // zero-copy path
        CHECK_STR(ntp.getTimestampRFC3339().c_str(), v.expected);  // String path (3.0.0)
    }

    // offset argument shifts the rendered time (seconds)
    TestNtp<FakeUdp> ntp;
    ntp.setUnix(1709251200UL); // 2024-03-01T00:00:00Z
    ntp.syncRFC3339(65);       // +1mn05s
    CHECK_STR(ntp.c_str(), "2024-03-01T00:01:05Z");
}


int main()
{
    test_fasttimer();
    test_shorttimer();
    test_rfc3339();

    if (g_failures == 0) {
        std::printf("OK - all tests passed\n");
        return 0;
    }
    std::printf("FAILED - %d check(s)\n", g_failures);
    return 1;
}
