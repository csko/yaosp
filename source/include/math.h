/* yaosp C library
 *
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

#ifndef _MATH_H_
#define _MATH_H_

#define HUGE_VAL \
  (__extension__                                                              \
   ((union { unsigned __l __attribute__((__mode__(__DI__))); double __d; })   \
    { __l: 0x7ff0000000000000ULL }).__d)

int finite( double x );

double sin( double x );
double cos( double x );
double log( double x );
double log10( double x );
double exp( double x );
double floor( double x );
double ceil( double x );
double frexp( double x, int* exp );
double fabs( double x );
double fmod( double x, double y );
double atan2( double y, double x );
double hypot( double x, double y );
double pow( double x, double y );
double ldexp( double x, int exp );
double scalbn( double x, int exp );
double modf( double x, double* iptr );

#endif // _MATH_H_
