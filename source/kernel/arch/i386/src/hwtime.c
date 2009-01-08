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

#define BCDTOBIN(val) ( ( ( val ) & 0x0f ) + ( ( val ) >> 4 ) * 10 )
#define BINTOBCD(val) ( ( ( ( val ) / 10 ) << 4 ) + ( val ) % 10 )

static spinlock_t datetime_lock = INIT_SPINLOCK;

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

    /* Seconds */
    outb( 0x00, RTCADDR );
    tm->sec = inb( RTCDATA );

    /* Minutes */
    outb( 0x02, RTCADDR );
    tm->min = inb( RTCDATA );

    /* Hours */
    outb( 0x04, RTCADDR );
    tm->hour = inb( RTCDATA );

    /* Day of month */
    outb( 0x07, RTCADDR );
    tm->mday = inb( RTCDATA );

    /* Month */
    outb( 0x08, RTCADDR );
    tm->mon = inb( RTCDATA );

    /* Year */
    outb( 0x09, RTCADDR );
    tm->year = inb( RTCDATA );

    /* TODO: wday, isDST */
    tm->isdst = 0;
    tm->wday = 0;
    tm->yday = 0;

    spinunlock_enable( &datetime_lock );

    if(!(settings & 0x04)){ /* See if we have to convert the values */
        tm->year = BCDTOBIN(tm->year);
        tm->year += 2000;
        tm->mon = BCDTOBIN(tm->mon);
        tm->mday = BCDTOBIN(tm->mday);
        tm->wday = 0;
        tm->hour = BCDTOBIN(tm->hour);
        tm->min = BCDTOBIN(tm->min);
        tm->sec = BCDTOBIN(tm->sec);
        tm->isdst = 0;
    }

    /* TODO: see other settings, for example 24-hour clock */
}

int sethwclock( const tm_t* tm ) {
    /* TODO */
    return -ENOSYS;
}
