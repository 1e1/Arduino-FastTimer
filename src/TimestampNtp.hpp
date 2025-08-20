#pragma once

#include <Udp.h>
#include <IPAddress.h>


class TimestampUnixNtp {

public:

    static constexpr unsigned int NTP_PORT = 123;
    static constexpr unsigned long OFFSET_MON_JAN_1ST_1900_TO_UNIX_EPOCH = 2208988800UL; ///< Offset from 1900 to 1970 epoch.
    static constexpr unsigned long OFFSET_UNIX_EPOCH_TO_MON_JAN_1ST_2024 = 1704067200UL; ///< Offset from 1970 to 2024 epoch.
    static constexpr std::array<byte, 48> NTP_PACKET = {
            // LI = 11, alarm condition (FastTimer not synchronized)
            // VN = 100, Version Number: currently 4
            // VM = 011, client
            // Stratum = 0, unspecified
            // Poll Interval: 6 => 2**6 = 64 seconds
            // Precision: 16MHz Arduino is about 2**-24
            0b11100011, 0, 6, (byte) -24,
            // Root Delay: 29s -> target less than 1 minute (64s)
            0, 29, 0, 0,
            // Root Dispersion: 29s -> target less than 1 minute (64s)
            0, 29, 0, 0,
            // Reference FastTimer Identifier
            'K', 'I', 'S', 'S',
            //0
        };

    TimestampUnixNtp(UDP &udp) : _udp(udp), _secondsSince1900(0)
    {
    }

    unsigned long getTimestampUnix(const int offset = 0) const
    {
        return _secondsSince1900 + offset - OFFSET_MON_JAN_1ST_1900_TO_UNIX_EPOCH;
    }

    void request(const IPAddress serverIp, const uint16_t serverPort = NTP_PORT) const
    {
        _udp.beginPacket(serverIp, serverPort);
        _sendPacket();
    }

    void request(const char* host, const uint16_t serverPort = NTP_PORT) const
    {
        _udp.beginPacket(host, serverPort);
        _sendPacket();
    }

    const boolean listen(void)
    {
        if (!_hasResponse()) {
            return false;
        }

        _receivePacket();

        return true;
    }

protected:

    void _sendPacket(void) const
    {
        _udp.write(NTP_PACKET.data(), NTP_PACKET.size());
        _udp.endPacket();
    }

    boolean _hasResponse(void) const
    {
        return _udp.parsePacket() >= NTP_PACKET.size(); 
    }

    void _receivePacket(void)
    {
        byte ntp_packet[NTP_PACKET.size()];
        _udp.read(ntp_packet, NTP_PACKET.size());
        // test if result is not null: as min date is MON_JAN_1ST_2024: ntp_packet[40] >= 233
        if (ntp_packet[40]) { //  && ntp_packet[41] && ntp_packet[42] && ntp_packet[43])
            _secondsSince1900 = (long(ntp_packet[40]) << 24)
                                    | (long(ntp_packet[41]) << 16)
                                    | (int(ntp_packet[42]) << 8)
                                    | (int(ntp_packet[43]) << 0)
                                    ;
        }
        _udp.flush();
    }

    UDP &_udp;
    unsigned long _secondsSince1900;

};


class TimestampRFC3339Ntp : public TimestampUnixNtp {

public:

    TimestampRFC3339Ntp(UDP& udp) : TimestampUnixNtp(udp) {}

    const String getTimestampRFC3339(void) const
    {
        return _strRFC3339;
    }

    boolean listenSync(const int offset = 0)
    {
        if (!_hasResponse()) {
            return false;
        }

        _receivePacket();
        syncRFC3339(offset);

        return true;
    }
    
    void syncRFC3339(const int offset = 0)
    {
        // make Time
        // =========
        const uint32_t secondsSince2024 = getTimestampUnix(offset) - OFFSET_UNIX_EPOCH_TO_MON_JAN_1ST_2024;

        const uint32_t minutesSince2024 = secondsSince2024 / 60;
        {
            const uint8_t x = minutesSince2024;
            const uint8_t y = secondsSince2024;
            const uint8_t z = y - x * 60;
            _fillRFC3339(17, z);
        }
        // release secondsSince2024
        
        const uint32_t hoursSince2024 = minutesSince2024 / 60;
        {
            const uint8_t x = hoursSince2024;
            const uint8_t y = minutesSince2024;
            const uint8_t z = y - x * 60;
            _fillRFC3339(14, z);
        }
        // release minutesSince2024

        // limit to 65535 days = 179 years (*366)
        // max is MONDAY_2024_01_01 + 179 years => 2203
        const uint16_t daysSince2024 = hoursSince2024 / 24;
        {
            const uint8_t x = daysSince2024;
            const uint8_t y = hoursSince2024;
            const uint8_t z = y - x * 24;
            _fillRFC3339(11, z);
        }
        // release hoursSince2024


        // make Date
        // =========
        // minimal datetime is 2024-03-01 00:00:00
        const uint8_t nbLeapYear = /* 2024 is a leap year -> */ 1+ /* <- */ ( (daysSince2024 -31 -28) / (365*4 +1) );
        // limit to 255 years
        // max is MONDAY_2024_01_01 + 255 years => 2273
        const uint8_t yearsSince2024 = (daysSince2024 - nbLeapYear) / 365;
        {
            // max is 2099 due to 2-bytes completion
            _fillRFC3339(2, yearsSince2024 + 24);
        }

        const boolean isLeapYear = (yearsSince2024 & B11) == 0;
        // release yearsSince2024
        uint16_t dayOfPeriod = 1+ daysSince2024 - (yearsSince2024 * 365) - nbLeapYear;
        // release daysSince2024
        // release nbLeapYear
        {
            uint8_t month = 0;
            do {
                uint8_t monthSize = _MONTH_SIZES[month];
                ++month;
                if (isLeapYear && month == 1) {
                    ++monthSize;
                }
                if (dayOfPeriod > monthSize) {
                    dayOfPeriod = dayOfPeriod - monthSize;
                } else {
                    break;
                }

            } while(month < sizeof(_MONTH_SIZES));

            _fillRFC3339(5, month);
            _fillRFC3339(8, dayOfPeriod);
        }
    }

    protected:

    void _fillRFC3339(const uint8_t pos, const uint8_t value)
    {
        const uint8_t tens = value / 10;
        _strRFC3339[pos+0] = '0' + tens;
        _strRFC3339[pos+1] = '0' + value - (tens * 10);
    }

    static constexpr std::array<byte, 12> _MONTH_SIZES = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    char _strRFC3339[21] = "2024-01-01T00:00:00Z";

};

typedef TimestampRFC3339Ntp TimestampNtp;
