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

#include <lib/string.h>
#include <lib/time.h>
#include <arch/pit.h>

#define APPEND( str ) \
        ret += strlen(str); \
        if(ret > max){ \
            return 0; \
        } \
        strcat(s, (const char*) str);

const char* month_names[ 12 ] = { "January", "February", "March",
                                  "April", "May", "June",
                                  "July", "August", "September",
                                  "October", "November", "December" };

const char* smonth_names[ 12 ] = { "Jan", "Feb", "Mar",
                                   "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep",
                                   "Oct", "Nov", "Dec" };

const char* day_names[ 7 ] = { "Sunday", "Monday",
                               "Tuesday", "Wednesday",
                               "Thursday", "Friday",
                               "Saturday" };

const char* sday_names[ 7 ] = { "Sun", "Mon", "Tue",
                                "Wed", "Thu", "Fri",
                                "Sat" };

const unsigned short int monthdays[13] =
{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

const unsigned short int monthdays2[13] =
{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 };

const unsigned short int sumofdays[ 60 ] =
{ 0, 366, 731, 1096, 1461, 1827, 2192, 2557, 2922, 3288, 3653, 4018, 4383, 4749,
  5114, 5479, 5844, 6210, 6575, 6940, 7305, 7671, 8036, 8401, 8766, 9132, 9497,
  9862, 10227, 10593, 10958, 11323, 11688, 12054, 12419, 12784, 13149, 13515,
  13880, 14245, 14610, 14976, 15341, 15706, 16071, 16437, 16802, 17167, 17532,
  17898, 18263, 18628, 18993, 19359, 19724, 20089, 20454, 20820, 21185, 21550 };

uint64_t timestamp(tm_t* time) {
    return daysdiff(time->year, time->mon, time->mday) * SECONDS_PER_DAY +
          time->hour * SECONDS_PER_HOUR + time->min * SECONDS_PER_MINUTE + time->sec;
}

tm_t gettime( uint64_t timeval ) {
    tm_t ret;
    int i;

    ret.sec = timeval % SECONDS_PER_MINUTE;
    timeval /= SECONDS_PER_MINUTE;
    ret.min = timeval % MINUTES_PER_HOUR;
    timeval /= MINUTES_PER_HOUR;
    ret.hour = timeval % HOURS_PER_DAY;
    timeval /= HOURS_PER_DAY;

    /* 1970(Epoch) is a leap year, and every 4th year too.
       2000 is also a leap year, because its divisible by 400.
       timeval now holds the difference of whole days.
       1 Jan 1970 was a Thursday. */

    ret.wday = (4 + timeval) % 7;

    ret.year = EPOCH;
    for(i=0; i<60; i++){
        if(sumofdays[i] > timeval){
            ret.year = EPOCH + i - 1;
            timeval -= sumofdays[i-1];
            break;
        }
    }
    ret.yday = (int) timeval;

    if(ret.year % 4 == 0){
        for(ret.mon = 0; ret.mon < 12; ret.mon++){
            if(monthdays2[ret.mon] > timeval){
                timeval -= monthdays2[--ret.mon];
                break;
            }
        }
    }else{
        for(ret.mon = 0; ret.mon < 12; ret.mon++){
            if(monthdays[ret.mon] > timeval){
                timeval -= monthdays[--ret.mon];
                break;
            }
        }
    }

    ret.mday = (int) timeval + 1;
    ret.isdst = -1;

    return ret;
}

int daysdiff(int year, int month, int day){
    int i, days = 0;

    for(i = EPOCH; i<year; i++){
        if(i % 4 == 0){ /* If it is a leap year */
            days += 366;
        }else{
            days += 365;
        }
    }

    if(year % 4 == 0){
        days += monthdays2[month];
    }else{
        days += monthdays[month];
    }

    days += day - 1;

    return days;
}

int dayofweek(int year, int month, int day){
    /* The UNIX Epoch(1 Jan 1970) was a Thursday */
    return (4 + daysdiff(year, month, day)) % 7;
}

uint64_t uptime( void ) {
    return get_system_time() - get_boot_time();
}

size_t strftime(char* s, size_t max, const char* format,
                       const tm_t* tm){
    size_t ret = 0;
    int state;
    int flags; 
    char tmp[max];

    s[0] = '\0'; /* Initialize s */
    max--; /* Reserve space for '\0' at the end */
    state = flags = 0;

    for( ; *format != '\0'; format++){
        switch(state){
            case 0:
                if(*format != '%'){
                    if(ret < max){
                        s[ret++] = *format;
                        s[ret] = '\0';
                    } else {
                        s[ret] = '\0';
                        return 0;
                    }
                    break;
                }
                state++;
                format++;
            case 1:
                if(*format == '%'){
                    if(ret < max){
                        s[ret++] = '%';
                        s[ret] = '\0';
                    } else {
                        s[ret] = '\0';
                        return 0;
                    }
                    state = flags = 0;
                    break;
                }
                /* TODO: locale-independent, we need locales first :)
                if(*format == 'E'){
                } */
                if(*format == 'O'){
                    flags |= STRFT_ALTFORM;
                    /* TODO: use this */
                }
                /* TODO: glibc modifiers: _-0^# */
                state++;
            case 2:
                switch(*format){
                    case 'a':
                        APPEND(sday_names[tm->wday]);
                        break;
                    case 'A':
                        APPEND(day_names[tm->wday]);
                        break;
                    case 'h':
                    case 'b':
                        APPEND(smonth_names[tm->mon]);
                        break;
                    case 'B':
                        APPEND(month_names[tm->mon]);
                        break;
                    case 'c':
                        /* The preferred date and time representation for the current locale. */
                        strftime(tmp, max-ret, "%a %b %e %H:%M:%S %Z %Y", tm);
                        APPEND(tmp);
                        break;
                    case 'C':
                        snprintf(tmp, max, "%d", tm->year / 100);
                        APPEND(tmp);
                        break;
                    case 'd':
                        snprintf(tmp, max, "%02d", tm->mday);
                        APPEND(tmp);
                        break;
                    case 'D':
                        strftime(tmp, max-ret, "%m/%d/%y", tm);
                        APPEND(tmp);
                        break;
                    case 'e':
                        snprintf(tmp, max, "%2d", tm->mday);
                        APPEND(tmp);
                        break;
                    case 'F':
                        strftime(tmp, max-ret, "%Y-%m-%d", tm);
                        APPEND(tmp);
                        break;
                    case 'G':
/* The ISO 8601 year with century as a decimal number.  The 4-digit year corresponding to the ISO  week  number
   (see  %V).  This has the same format and value as %y, except that if the ISO week number belongs to the
   previous or next year, that year is used instead. (TZ)*/
                        break;
                    case 'g':
                        /* Like %G, but without century, that is, with a 2-digit year (00-99). (TZ) */
                        break;
                    case 'H':
                        snprintf(tmp, max, "%02d", tm->hour);
                        APPEND(tmp);
                        break;
                    case 'I':
                        snprintf(tmp, max, "%02d", tm->hour % 12);
                        APPEND(tmp);
                        break;
                    case 'j':
                        /* Day of the year, 001-366 */
                        snprintf(tmp, max, "%03d", tm->yday + 1);
                        APPEND(tmp);
                        break;
                    case 'k':
                        snprintf(tmp, max, "%2d", tm->hour);
                        APPEND(tmp);
                        break;
                    case 'l':
                        snprintf(tmp, max, "%2d", tm->hour % 12);
                        APPEND(tmp);
                        break;
                    case 'm':
                        snprintf(tmp, max, "%02d", tm->mon + 1);
                        APPEND(tmp);
                        break;
                    case 'M':
                        snprintf(tmp, max, "%02d", tm->min);
                        APPEND(tmp);
                        break;
                    case 'n':
                        APPEND("\n");
                        break;
                    case 'p':
                        if(tm->hour >= 12){
                            APPEND("PM");
                        }else{
                            APPEND("AM");
                        }
                        break;
                    case 'P':
                        if(tm->hour >= 12){
                            APPEND("pm");
                        }else{
                            APPEND("am");
                        }
                        break;
                    case 'r':
                        strftime(tmp, max-ret, "%I:%M:%S %p", tm);
                        APPEND(tmp);
                        break;
                    case 'R':
                        strftime(tmp, max-ret, "%H:%M", tm);
                        APPEND(tmp);
                        break;
                    case 's':
                        snprintf(tmp, max, "%d", timestamp((tm_t*) tm));
                        APPEND(tmp);
                        break;
                    case 'S':
                        snprintf(tmp, max, "%02d", tm->sec);
                        APPEND(tmp);
                        break;
                    case 't':
                        APPEND("\t");
                        break;
                    case 'T':
                        strftime(tmp, max-ret, "%H:%M:%S", tm);
                        APPEND(tmp);
                        break;
                    case 'u':
                        /* Day of the week, 1-7, Monday=1 */
                        snprintf(tmp, max, "%d", 1 + (tm->wday + 6 ) % 7);
                        APPEND(tmp);
                        break;
                    case 'U':
/* The week number of the current year as a decimal number, range 00 to 53, starting with the first  Sunday  as
   the first day of week 01.*/
                        break;
                    case 'V':
/*  The  ISO  8601:1988 week number of the current year as a decimal number, range 01 to 53, where week 1 is the
    first week that has at least 4 days in the current year, and with Monday as the first day of the week.   See
    also %U and %W. */
                        break;
                    case 'w':
                        /* Day of the week, 0-6, Sunday=0 */
                        snprintf(tmp, max, "%d", tm->wday);
                        APPEND(tmp);
                        break;
                    case 'W':
/* The  week  number of the current year as a decimal number, range 00 to 53, starting with the first Monday as
   the first day of week 01. */
                        break;
                    case 'x':
                        /* The preferred date representation for the current locale without the time. */
                        strftime(tmp, max-ret, "%a %b %e %Z %Y", tm);
                        APPEND(tmp);
                        break;
                    case 'X':
                        /* The preferred time representation for the current locale without the date. */
                        strftime(tmp, max-ret, "%H:%M:%S", tm);
                        APPEND(tmp);
                        break;
                    case 'y':
                        /* The year as a decimal number without a century (range 00 to 99). */
                        snprintf(tmp, max, "%02d", tm->year % 100);
                        APPEND(tmp);
                        break;
                    case 'Y':
                        /* The year as a decimal number including the century. */
                        snprintf(tmp, max, "%d", tm->year);
                        APPEND(tmp);
                        break;
                    case 'z':
/* The  time-zone  as  hour  offset   from   GMT.    Required   to   emit   RFC 822-conformant   dates   (using
   "%a, %d %b %Y %H:%M:%S %z"). (GNU) */
                        break;
                    case 'Z':
                        /* The time zone or name or abbreviation. */
                        break;
                    default:
                        break;
                }
            state = flags = 0;
        }
    }

    s[ret] = '\0';

    return ret;
}
