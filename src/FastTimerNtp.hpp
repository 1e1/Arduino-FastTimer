#pragma once

#include <Arduino.h>
#include <Udp.h>
#include <IPAddress.h>

#include "FastTimer.hpp"


// static const uint8_t NTP_IP[] = { 88, 191, 80, 53 }; // 0.fr.pool.ntp.org, prefer local NTP server like your gateway
static const unsigned int NTP_PORT = 123;
static const unsigned int NTP_PACKET_SIZE = 48;
static const unsigned long MONDAY_20240101_SINCE_19700101_IN_SECONDS = 1704067200UL;
static const unsigned long MONDAY_19700101_SINCE_19000101_IN_SECONDS = 2208988800UL;


template <FastTimer_precision_t p>
class FastTimerNtp : public FastTimer<p> {

    public:
    FastTimerNtp() : FastTimer<p>(), _secondsSince1900(0) {};
    const uint8_t update(void) override // call it once in the main loop()
    {
        return FastTimer<p>::update();
    }

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
        const char timestamp[] = "2000-00-00T00:00:00Z";
        const unsigned long secondsSince2024 = this->getTimestampUnix() - MONDAY_20240101_SINCE_19700101_IN_SECONDS;
        const unsigned long minutesSince2024 = secondsSince2024 / 60;
        const unsigned long hoursSince2024 = minutesSince2024 / 60;
        {
            const uint8_t minutes = minutesSince2024 - (hoursSince2024 * 60);
            this->_fillRFC3339(timestamp +14, minutes);
        }

        // limit to 65535 days = 179 years (*366)
        // max is MONDAY_2024_01_01 + 179 years => 2203
        const unsigned long daysSince2024 = hoursSince2024 / 24;
        {
            const uint8_t hours = hoursSince2024 - (daysSince2024 * 24);
            this->_fillRFC3339(timestamp +11, hours);
        }

        const uint8_t nbLeapYear = (daysSince2024 -31 -28) / (365*4 +1);
        // limit to 255 years
        // max is MONDAY_2024_01_01 + 255 years => 2273
        const uint8_t yearsSince2024 = (daysSince2024 - nbLeapYear) / 365;
        {
            // max is 2099 due to 2-bytes completion
            this->_fillRFC3339(timestamp +2, yearsSince2024);
        }

        unsigned int dayOfYear = daysSince2024 - (yearsSince2024 * 365) - nbLeapYear + 1;
        {
            uint8_t month = 0;
            const byte monthSizes[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
            
            if (yearsSince2024 & B11 == 0) {
                monthSizes[1] = 29;
            }
            while (month<sizeof(monthSizes) && dayOfYear > monthSizes[month]) {
                dayOfYear = dayOfYear - monthSizes[month];
                ++month;
            }

            this->_fillRFC3339(timestamp +5, month);
            this->_fillRFC3339(timestamp +8, dayOfYear);
        }

        return timestamp;
    }

    protected:

    void _fillRFC3339(const char *pattern, const uint8_t value)
    {
        const uint8_t tens = value / 10;
        pattern[0] = tens;
        pattern[1] = value - (tens * 10);
    }

    unsigned long _secondsSince1900;

};
