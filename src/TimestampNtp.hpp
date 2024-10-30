#pragma once

#include <Udp.h>
#include <IPAddress.h>


class TimestampNtp {

    public:

    static constexpr unsigned int NTP_PORT = 123;
    static constexpr unsigned int NTP_PACKET_SIZE = 48;
    static constexpr unsigned long MONDAY_20240101_SINCE_19700101_IN_SECONDS = 1704067200UL;
    static constexpr unsigned long MONDAY_19700101_SINCE_19000101_IN_SECONDS = 2208988800UL;

    TimestampNtp(UDP &udp) : _udp(udp), _secondsSince1900(0) {};

    /*
        udp.beginPacket(ip, FastTimerNtp::NTP_PORT);
        this->writeIn(udp);
        udp.endPacket();
    */
    void request(const IPAddress serverIp, const uint16_t serverPort = NTP_PORT)
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
        
        this->_udp.beginPacket(serverIp, serverPort);
        this->_udp.write(ntp_packet, NTP_PACKET_SIZE);
        this->_udp.endPacket();
    }

    /*
        if (udp.parsePacket())
        {
            this->readFrom(udp);
            // udp.flush();
        }
    */
    const boolean listen(void)
    {
        if (this->_udp.parsePacket() == 0) {
            return false;
        }

        byte ntp_packet[NTP_PACKET_SIZE];
        this->_udp.read(ntp_packet, NTP_PACKET_SIZE);
        this->_secondsSince1900 = long(ntp_packet[40]) << 24
                                | long(ntp_packet[41]) << 16
                                | int(ntp_packet[42]) << 8
                                | int(ntp_packet[43]) << 0
                                ;
        this->_udp.flush();

        return true;
    }

    const unsigned long getTimestampUnix(void) { return this->_secondsSince1900 - MONDAY_19700101_SINCE_19000101_IN_SECONDS; };
    
    const char* getTimestampRFC3339(void)
    {
        // Timestamp timestamp = {0};
        char* timestamp = "2000-00-00T00:00:00Z";
        const unsigned long secondsSince2024 = this->getTimestampUnix() - MONDAY_20240101_SINCE_19700101_IN_SECONDS;
        const unsigned long minutesSince2024 = secondsSince2024 / 60;
        const unsigned long hoursSince2024 = minutesSince2024 / 60;
        {
            const uint8_t minute = minutesSince2024 - (hoursSince2024 * 60);
            this->_fillRFC3339(timestamp +14, minute);
            // timestamp.minute = minute;
        }

        // limit to 65535 days = 179 years (*366)
        // max is MONDAY_2024_01_01 + 179 years => 2203
        const unsigned long daysSince2024 = hoursSince2024 / 24;
        {
            const uint8_t hour = hoursSince2024 - (daysSince2024 * 24);
            this->_fillRFC3339(timestamp +11, hour);
            // timestamp.hour = hour;
        }

        // minimal datetime is 2024-03-01 00:00:00
        const uint8_t nbLeapYear = (daysSince2024 -31 -28) / (365*4 +1);
        // limit to 255 years
        // max is MONDAY_2024_01_01 + 255 years => 2273
        const uint8_t yearsSince2024 = (daysSince2024 - (nbLeapYear+1)) / 365;
        {
            // max is 2099 due to 2-bytes completion
            this->_fillRFC3339(timestamp +2, yearsSince2024 + 24);
            // timestamp.yearSince24 = yearsSince2024;
        }

        unsigned int dayOfYear = daysSince2024 - (yearsSince2024 * 365) - nbLeapYear;
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

            this->_fillRFC3339(timestamp +5, month + 1);
            this->_fillRFC3339(timestamp +8, dayOfYear);
            // timestamp.month = month;
            // timestamp.day = dayOfYear;
        }

        // timestamp.isSync = 1;

        return timestamp;
    }

    protected:

    void _fillRFC3339(char *pattern, const uint8_t value)
    {
        const uint8_t tens = value / 10;
        pattern[0] = '0' + tens;
        pattern[1] = '0' + value - (tens * 10);
    }

    UDP &_udp;
    unsigned long _secondsSince1900;

};
