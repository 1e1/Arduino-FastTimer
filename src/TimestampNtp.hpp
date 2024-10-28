#pragma once

#include <Udp.h>
#include <IPAddress.h>

#include "FastTimer.hpp"


// static const uint8_t NTP_IP[] = { 88, 191, 80, 53 }; // 0.fr.pool.ntp.org, prefer local NTP server like your gateway

#ifndef TSNTP_REQUEST
#define TSNTP_REQUEST(ntp, udp, ip, port) { \
    udp.beginPacket(ip, port);              \
    ntp.writeIn(udp);                       \ 
    udp.endPacket();                        \
}
#endif

#ifndef TSNTP_RESPONSE
#define TSNTP_RESPONSE(ntp, udp, ...) {     \
    /* ignore __VA_ARGS__ */                \
    if (udp.parsePacket()) {                \
        ntp.readFrom(udp);                  \
        udp.flush();                        \
    }                                       \
}
#endif


class TimestampNtp {

    public:

    static constexpr unsigned int NTP_PORT = 123;
    static constexpr unsigned int NTP_PACKET_SIZE = 48;
    static constexpr unsigned long MONDAY_20240101_SINCE_19700101_IN_SECONDS = 1704067200UL;
    static constexpr unsigned long MONDAY_19700101_SINCE_19000101_IN_SECONDS = 2208988800UL;

    /*
    struct Timestamp {
        unsigned int isSync: 1,
        unsigned int yearSince24: 5,    // +31 years max => 2055
        unsigned int month: 4,
        unsigned int day: 5,
        unsigned int hour: 5,
        unsigned int minute: 6,
        unsigned int seconde: 6,

        byte yearSince2000() const {
            return yearSince24 + 24;
        }

        String toRFC3339() const {
            const char text[] = "2000-00-00T00:00:00Z";
            const byte yy = yearSince2000();
            if (yy > 9) {
                text[2] = '0' + (yy / 10);
            }
            text[3] = '0' + (yy % 10);
        }
    };
    */

    TimestampNtp() : _secondsSince1900(0) {};

    /*
        udp.beginPacket(ip, FastTimerNtp::NTP_PORT);
        this->writeIn(udp);
        udp.endPacket();
    */
    size_t writeIn(UDP &udp)
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
        
        return udp.write(ntp_packet, NTP_PACKET_SIZE);
    }

    /*
        if (udp.parsePacket())
        {
            this->readFrom(udp);
            // udp.flush();
        }
    */
    void readFrom(UDP &udp)
    {
        byte ntp_packet[NTP_PACKET_SIZE];
        udp.read(ntp_packet, NTP_PACKET_SIZE);
        this->_secondsSince1900 = long(ntp_packet[40]) << 24
                                | long(ntp_packet[41]) << 16
                                | int(ntp_packet[42]) << 8
                                | int(ntp_packet[43]) << 0
                                ;
    }

    const unsigned long getTimestampUnix(void) { return this->_secondsSince1900 - MONDAY_19700101_SINCE_19000101_IN_SECONDS; };
    
    const char* getTimestampRFC3339(void)
    {
        // Timestamp timestamp = {0};
        char timestamp[] = "2000-00-00T00:00:00Z";
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

        const uint8_t nbLeapYear = (daysSince2024 -31 -28) / (365*4 +1);
        // limit to 255 years
        // max is MONDAY_2024_01_01 + 255 years => 2273
        const uint8_t yearsSince2024 = (daysSince2024 - nbLeapYear) / 365;
        {
            // max is 2099 due to 2-bytes completion
            this->_fillRFC3339(timestamp +2, yearsSince2024);
            // timestamp.yearSince24 = yearsSince2024;
        }

        unsigned int dayOfYear = daysSince2024 - (yearsSince2024 * 365) - nbLeapYear + 1;
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

            this->_fillRFC3339(timestamp +5, month);
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
        pattern[0] = tens;
        pattern[1] = value - (tens * 10);
    }

    unsigned long _secondsSince1900;

};
