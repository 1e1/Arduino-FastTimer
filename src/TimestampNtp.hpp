#pragma once

#include <Arduino.h>
#include <Udp.h>
#include <IPAddress.h>


class Ntp { 
public:
    virtual unsigned long time(void) const = 0;
    operator unsigned long() const {
        return this->time();
    }
};


template<typename T_udp>
class TimestampUnixNtp : public Ntp {

public:

    static constexpr unsigned int LOCAL_PORT = 6687; // "NTP" on DTMF
    static constexpr unsigned int NTP_PORT = 123;
    static constexpr unsigned long OFFSET_MON_JAN_1ST_1900_TO_UNIX_EPOCH = 2208988800UL; //  offset from 1900 to 1970 epoch
    static constexpr byte NTP_PACKET[48] PROGMEM =  {
            // LI = 11, alarm condition (clock not synchronized)
            // VN = 100, Version Number: currently 4
            // Mode = 011, client
            // Stratum = 0, unspecified
            // Poll Interval: 6 => 2**6 = 64 seconds
            // Precision: 16MHz Arduino is about 2**-24
            0b11100011, 0, 6, (byte) -24,
            // Root Delay: 29s -> target less than 1 minute (64s)
            0, 29, 0, 0,
            // Root Dispersion: 29s -> target less than 1 minute (64s)
            0, 29, 0, 0,
            // Reference Clock Identifier
            'K', 'I', 'S', 'S',
            //0
        };

    TimestampUnixNtp() : _udp(), _secondsSince1900(0) {}

    uint8_t begin(uint16_t port=0)
    {
        if (port == 0) {
            port = LOCAL_PORT;
        }

        return this->_udp.begin(port);
    }

    unsigned long time(void) const override
    {
        return this->getTimestampUnix();
    }

    unsigned long getTimestampUnix(const int offset = 0) const
    {
        return this->_secondsSince1900 + offset - OFFSET_MON_JAN_1ST_1900_TO_UNIX_EPOCH;
    }

    void request(const IPAddress serverIp, const uint16_t serverPort = NTP_PORT)
    {
        this->_udp.beginPacket(serverIp, serverPort);
        this->_sendPacket();
    }

    void request(const char* host, const uint16_t serverPort = NTP_PORT)
    {
        this->_udp.beginPacket(host, serverPort);
        this->_sendPacket();
    }

    bool listen(void)
    {
        if (!this->_hasResponse()) {
            return false;
        }

        this->_receivePacket();

        return true;
    }

protected:

    inline bool _hasResponse(void)
    {
        return this->_udp.parsePacket() >= sizeof(NTP_PACKET); 
    }

    void _sendPacket(void)
    {
        // NTP_PACKET lives in flash (PROGMEM); copy to a transient RAM buffer
        // before handing it to the UDP stack (48 bytes of stack, not of .data).
        byte packet[sizeof(NTP_PACKET)];
        memcpy_P(packet, NTP_PACKET, sizeof(NTP_PACKET));
        this->_udp.write(packet, sizeof(packet));
        this->_udp.endPacket();
    }

    void _receivePacket(void)
    {
        byte ntp_packet[sizeof(NTP_PACKET)];
        this->_udp.read(ntp_packet, sizeof(NTP_PACKET));
        // test if result is not null: as min date is MON_JAN_1ST_2024: ntp_packet[40] >= 233
        if (ntp_packet[40]) { //  && ntp_packet[41] && ntp_packet[42] && ntp_packet[43])
            this->_secondsSince1900 = (long(ntp_packet[40]) << 24)
                                    | (long(ntp_packet[41]) << 16)
                                    | (int(ntp_packet[42]) << 8)
                                    | (int(ntp_packet[43]) << 0)
                                    ;
        }
        this->_udp.flush();
    }

    T_udp _udp;
    unsigned long _secondsSince1900;

};

// Out-of-line definition: NTP_PACKET is odr-used (passed by address to
// memcpy_P), so pre-C++17 toolchains (e.g. the AVR core, gnu++11) need it.
template<typename T_udp>
constexpr byte TimestampUnixNtp<T_udp>::NTP_PACKET[];


template<typename T_udp>
class TimestampRFC3339Ntp : public TimestampUnixNtp<T_udp> {

public:

    static constexpr unsigned long OFFSET_UNIX_EPOCH_TO_MON_JAN_1ST_2024 = 1704067200UL; //  offset from 1970 to 2024 epoch

    TimestampRFC3339Ntp() : TimestampUnixNtp<T_udp>() {}

    // Backward compatible with 3.0.0: returns a String (allocates a copy).
    String getTimestampRFC3339(void) const
    {
        return this->_strRFC3339;
    }

    // Zero-copy alternative: points straight at the internal buffer, no
    // allocation. Valid until the next syncRFC3339() call.
    const char* c_str(void) const
    {
        return this->_strRFC3339;
    }

    bool listenSync(const int offset = 0)
    {
        if (!this->_hasResponse()) {
            return false;
        }

        this->_receivePacket();
        this->syncRFC3339(offset);

        return true;
    }
    
    // Fills the internal "YYYY-MM-DDThh:mm:ssZ" buffer from the last NTP sync.
    // Range limits (traded for a tiny, 16-bit, division-light implementation):
    //  - year is rendered as "20YY", so valid range is 2024..2099;
    //  - the leap-year rule is the simple 4-year cycle, so the non-leap
    //    centuries (2100, 2200, ...) are NOT handled;
    //  - internal counters cap the theoretical span (days on 16 bits,
    //    years on 8 bits) well beyond the 2099 display limit above.
    // `offset` is added (in seconds) before conversion, e.g. to compensate for
    // the elapsed time since the last network sync.
    void syncRFC3339(const int offset = 0)
    {
        // make Time
        // =========
        const uint32_t secondsSince2024 = this->getTimestampUnix(offset) - OFFSET_UNIX_EPOCH_TO_MON_JAN_1ST_2024;

        const uint32_t minutesSince2024 = secondsSince2024 / 60;
        {
            const uint8_t x = minutesSince2024;
            const uint8_t y = secondsSince2024;
            const uint8_t z = y - x * 60;
            this->_fillRFC3339(17, z);
        }
        // release secondsSince2024
        
        const uint32_t hoursSince2024 = minutesSince2024 / 60;
        {
            const uint8_t x = hoursSince2024;
            const uint8_t y = minutesSince2024;
            const uint8_t z = y - x * 60;
            this->_fillRFC3339(14, z);
        }
        // release minutesSince2024

        // limit to 65535 days = 179 years (*366)
        // max is MONDAY_2024_01_01 + 179 years => 2203
        const uint16_t daysSince2024 = hoursSince2024 / 24;
        {
            const uint8_t x = daysSince2024;
            const uint8_t y = hoursSince2024;
            const uint8_t z = y - x * 24;
            this->_fillRFC3339(11, z);
        }
        // release hoursSince2024


        // make Date
        // =========
        // Cheap civil conversion for the supported range: within 2024..2099
        // there is a leap year every 4 with no century exception, so a handful
        // of 16-bit divisions + a short month loop suffice (no 32-bit division,
        // unlike a generic days-from-epoch formula). Feb 29 is handled.
        static const uint8_t monthSizes[12] PROGMEM =
            { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

        const uint8_t block = daysSince2024 / 1461;              // 4-year blocks (leap first)
        uint16_t rem = daysSince2024 - uint16_t(block) * 1461;   // day within the block
        uint8_t yearInBlock;
        if (rem < 366) {
            yearInBlock = 0;                                     // the leap year of the block
        } else {
            rem -= 366;
            yearInBlock = 1 + rem / 365;
            rem -= uint16_t(yearInBlock - 1) * 365;
        }
        const uint8_t yearsSince2024 = block * 4 + yearInBlock;
        const bool isLeapYear = (yearInBlock == 0);

        uint16_t dayOfYear = rem;                                // 0-based
        uint8_t month = 0;
        while (month < 12) {
            uint8_t monthSize = pgm_read_byte(&monthSizes[month]);
            if (isLeapYear && month == 1) {                      // February
                ++monthSize;
            }
            if (dayOfYear < monthSize) {
                break;
            }
            dayOfYear -= monthSize;
            ++month;
        }

        // year rendered as "20YY": valid range 2024..2099
        this->_fillRFC3339(2, yearsSince2024 + 24);
        this->_fillRFC3339(5, month + 1);
        this->_fillRFC3339(8, dayOfYear + 1);
    }

    protected:

    void _fillRFC3339(const uint8_t pos, const uint8_t value)
    {
        const uint8_t tens = value / 10;
        this->_strRFC3339[pos+0] = '0' + tens;
        this->_strRFC3339[pos+1] = '0' + value - (tens * 10);
    }

    char _strRFC3339[21] = "2024-01-01T00:00:00Z";

};

//typedef TimestampRFC3339Ntp TimestampNtp;
template<typename T_udp>
using TimestampNtp = TimestampRFC3339Ntp<T_udp>;

/*
    TimestampNtp<WiFiUDP> nts;
    nts.begin();
    nts.request("pool.ntp.org");
*/