#pragma once

#include <Udp.h>
#include <IPAddress.h>


class TimestampNtp {

    public:

    static constexpr unsigned int NTP_PORT = 123;
    static constexpr unsigned int NTP_PACKET_SIZE = 48;
    static constexpr unsigned long MONDAY_20240101_SINCE_19700101_IN_SECONDS = 1704067200UL;
    static constexpr unsigned long MONDAY_19700101_SINCE_19000101_IN_SECONDS = 2208988800UL;

    TimestampNtp(UDP &udp) : _udp(udp), _strRFC3339(strdup("2024-01-01T00:00:00Z")), _secondsSince1900(MONDAY_20240101_SINCE_19700101_IN_SECONDS) {};

    const unsigned long getTimestampUnix(void) { return this->_secondsSince1900 - MONDAY_19700101_SINCE_19000101_IN_SECONDS; }
    const char* getTimestampRFC3339(void) { return this->_strRFC3339; }

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

    const boolean listen(void)
    {
        if (!this->_hasResponse()) {
            return false;
        }

        this->_receivePacket();
        yield();
        this->_syncRFC3339();

        return true;
    }

    protected:

    void _sendPacket(void)
    {
        // http://tools.ietf.org/html/rfc1305
        byte ntp_packet[NTP_PACKET_SIZE] = {
            // LI = 11, alarm condition (FastTimer not synchronized)
            // VN = 100, Version Number: currently 4
            // VM = 011, client
            // Stratum = 0, unspecified
            // Poll Interval: 6 => 2**6 = 64 seconds
            // Precision: 16MHz Arduino is about 2**-24
            B11100011, 0, 6, (byte) -24,
            // Root Delay: 29s -> target less than 1 minute (64s)
            0, 29, 0, 0,
            // Root Dispersion: 29s -> target less than 1 minute (64s)
            0, 29, 0, 0,
            // Reference FastTimer Identifier
            'K', 'I', 'S', 'S',
            //0
        };
        
        this->_udp.write(ntp_packet, NTP_PACKET_SIZE);
        this->_udp.endPacket();
    }


    const boolean _hasResponse(void) { return this->_udp.parsePacket() >= NTP_PACKET_SIZE; }


    void _receivePacket(void)
    {
        byte ntp_packet[NTP_PACKET_SIZE];
        this->_udp.read(ntp_packet, NTP_PACKET_SIZE);
        this->_secondsSince1900 = long(ntp_packet[40]) << 24
                                | long(ntp_packet[41]) << 16
                                | int(ntp_packet[42]) << 8
                                | int(ntp_packet[43]) << 0
                                ;
        this->_udp.flush();
    }
    

    void _syncRFC3339(void)
    {
        const uint32_t secondsSince2024 = this->getTimestampUnix() - MONDAY_20240101_SINCE_19700101_IN_SECONDS;

        const uint32_t minutesSince2024 = secondsSince2024 / 60;
        {
            const uint8_t x = minutesSince2024;
            const uint8_t y = secondsSince2024;
            const uint8_t z = y - x * 60;
            this->_fillRFC3339(17, z);
        }
        
        const uint32_t hoursSince2024 = minutesSince2024 / 60;
        {
            const uint8_t x = hoursSince2024;
            const uint8_t y = minutesSince2024;
            const uint8_t z = y - x * 60;
            this->_fillRFC3339(14, z);
        }

        // limit to 65535 days = 179 years (*366)
        // max is MONDAY_2024_01_01 + 179 years => 2203
        const uint16_t daysSince2024 = hoursSince2024 / 24;
        {
            const uint8_t x = daysSince2024;
            const uint8_t y = hoursSince2024;
            const uint8_t z = y - x * 24;
            this->_fillRFC3339(11, z);
        }

        // minimal datetime is 2024-03-01 00:00:00
        const uint8_t nbLeapYear = (daysSince2024 -31 -28) / (365*4 +1);
        // limit to 255 years
        // max is MONDAY_2024_01_01 + 255 years => 2273
        const uint8_t yearsSince2024 = (daysSince2024 - (nbLeapYear+1)) / 365;
        {
            // max is 2099 due to 2-bytes completion
            this->_fillRFC3339(2, yearsSince2024 + 24);
        }

        uint16_t dayOfYear = daysSince2024 - (yearsSince2024 * 365) - nbLeapYear;
        {
            uint8_t month = 0;
            byte monthSizes[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
            
            if (yearsSince2024 & B11 == 0) {
                monthSizes[1] = 29;
            }
            while (month<sizeof(monthSizes) && dayOfYear > monthSizes[month]) {
                dayOfYear = dayOfYear - monthSizes[month];
                ++month;
            }

            this->_fillRFC3339(5, month + 1);
            this->_fillRFC3339(8, dayOfYear);
        }
    }

    void _fillRFC3339(const uint8_t p, const uint8_t value)
    {
        const uint8_t tens = value / 10;
        this->_strRFC3339[p+0] = '0' + tens;
        this->_strRFC3339[p+1] = '0' + value - (tens * 10);
    }

    UDP &_udp;
    char* _strRFC3339;
    unsigned long _secondsSince1900;

};
