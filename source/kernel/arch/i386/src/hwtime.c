/* Date and time handling
 *
 * Copyright (c) 2009 Kornel Csernai
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
#include <arch/io.h>
#include <arch/spinlock.h>

#include <arch/hwtime.h>

#define BCDTOBIN(val) ( ( ( val ) & 0x0f ) + ( ( val ) >> 4 ) * 10 )
#define BINTOBCD(val) ( ( ( ( val ) / 10 ) << 4 ) + ( val ) % 10 )

static spinlock_t datetime_lock = INIT_SPINLOCK;

tm_t gethwclock() {

    uint8_t portdata;
    uint8_t settings;
    tm_t ret;

    spinlock_disable( &datetime_lock );

    /* See if RTC is updating right now */
    do{
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
    ret.sec = inb( RTCDATA );

    /* Minutes */
    outb( 0x02, RTCADDR );
    ret.min = inb( RTCDATA );

    /* Hours */
    outb( 0x04, RTCADDR );
    ret.hour = inb( RTCDATA );

    /* Day of month */
    outb( 0x07, RTCADDR );
    ret.mday = inb( RTCDATA );

    /* Month */
    outb( 0x08, RTCADDR );
    ret.mon = inb( RTCDATA );

    /* Year */
    outb( 0x09, RTCADDR );
    ret.year = inb( RTCDATA );

    /* TODO: wday, isDST */
    ret.isdst = 0;
    ret.wday = 0;

    spinunlock_enable( &datetime_lock );

    if(!(settings & 0x04)){ /* See if we have to convert the values */
        ret.year = BCDTOBIN(ret.year);
        ret.year += 2000;
        ret.mon = BCDTOBIN(ret.mon);
        ret.mday = BCDTOBIN(ret.mday);
        ret.wday = 0;
        ret.hour = BCDTOBIN(ret.hour);
        ret.min = BCDTOBIN(ret.min);
        ret.sec = BCDTOBIN(ret.sec);
        ret.isdst = 0;
        }

    /* TODO: see other settings, for example 24-hour clock */

    return ret;
}

int sethwclock(const tm_t* tm){
/* TODO */
    return 0;
}
