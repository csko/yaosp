/* Date and time handling
 *
 * Copyright (c) 2009 Kornel Csernai
 * Copyright (c) 2009 Zoltan Kovacs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <errno.h>

#include <arch/io.h>
#include <arch/spinlock.h>
#include <arch/hwtime.h>
#include <lib/time.h>

#define BCDTOBIN(val) ( ( ( val ) & 0x0f ) + ( ( val ) >> 4 ) * 10 )
#define BINTOBCD(val) ( ( ( ( val ) / 10 ) << 4 ) + ( val ) % 10 )

static spinlock_t datetime_lock = INIT_SPINLOCK( "datetime" );

void gethwclock( tm_t* tm ) {
    uint8_t portdata;
    uint8_t settings;

    spinlock_disable( &datetime_lock );

    /* See if RTC is updating right now */

    do {
        outb( 0x0A, RTCADDR );
        portdata = inb( RTCDATA );
    } while ( portdata == 0x80 );

    /* Read the actual settings */

    outb ( 0x0B, RTCADDR );
    settings = inb( RTCDATA );

/*
    settings |= 0x02;  24-hour clock 
    settings |= 0x04;  Use binary mode instead of BCD 

     Update settings 
    outb ( 0x0B, RTCADDR );
    outb ( settings, RTCDATA );
*/

    /* Year */
    outb( 0x09, RTCADDR );
    tm->tm_year = inb( RTCDATA );

    /* Month */
    outb( 0x08, RTCADDR );
    tm->tm_mon = inb( RTCDATA ) - 1;

    /* Day of month */
    outb( 0x07, RTCADDR );
    tm->tm_mday = inb( RTCDATA );

    /* Day of week */
    outb( 0x06, RTCADDR );
    tm->tm_wday = inb( RTCDATA );

    /* Hours */
    outb( 0x04, RTCADDR );
    tm->tm_hour = inb( RTCDATA );

    /* Minutes */
    outb( 0x02, RTCADDR );
    tm->tm_min = inb( RTCDATA );

    /* Seconds */
    outb( 0x00, RTCADDR );
    tm->tm_sec = inb( RTCDATA );

    spinunlock_enable( &datetime_lock );

    if(!(settings & 0x04)){ /* See if we have to convert the values */
        tm->tm_year = BCDTOBIN(tm->tm_year);
        tm->tm_mon = BCDTOBIN(tm->tm_mon);
        tm->tm_mday = BCDTOBIN(tm->tm_mday);
        tm->tm_wday = BCDTOBIN(tm->tm_wday);
        tm->tm_hour = BCDTOBIN(tm->tm_hour);
        tm->tm_min = BCDTOBIN(tm->tm_min);
        tm->tm_sec = BCDTOBIN(tm->tm_sec);
    }
    tm->tm_wday--;
    tm->tm_year += 2000;

    /* TODO: isDST */
    tm->tm_isdst = -1;
    tm->tm_yday = ((tm->tm_year % 4 == 0) ? monthdays2[tm->tm_mon] : monthdays[tm->tm_mon]) + tm->tm_mday - 1;

    /* TODO: see other settings, for example 24-hour clock */
}

int sethwclock( const tm_t* tm ) {
    /* TODO */
    return -ENOSYS;
}
