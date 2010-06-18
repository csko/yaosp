/* Bitmap resizing functions
 *
 * Copyright (c) 2010 Attila Magyar
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

#include <ygui/bitmap.h>

#define __expect(foo,bar) __builtin_expect((long)(foo),bar)
#define __unlikely(foo) __expect((foo),0)

#define BLUE(c)  ( (c)  & 0xFF              )
#define GREEN(c) ( ((c) & 0xFF00)     >>  8 )
#define RED(c)   ( ((c) & 0xFF0000)   >> 16 )
#define ALPHA(c) ( ((c) & 0xFF000000) >> 24 )

static int bitmap_resize_nearest( bitmap_t* src, bitmap_t* dst ) {
    int src_w = src->width;
    int src_h = src->height;

    double scale_x = src_w / ( double )dst->width;
    double scale_y = src_h / ( double )dst->height;

    int x, y, dx, dy;
    double fx, fy;

    uint32_t* dst_data = ( uint32_t* )dst->data;
    uint32_t* src_data = ( uint32_t* )src->data;
    uint32_t* src_row;

    for ( y = 0, fy = 0.5; y < dst->height; ++y, fy += scale_y ) {
        dy = ( int )fy;
        if ( __unlikely( dy >= src_h ) ) { dy = src_h -1; }

        src_row = &src_data[ dy * src_w ];

        for ( x = 0, fx = 0.5; x < dst->width; ++x, fx += scale_x ) {
            dx = ( int )fx;
            if ( __unlikely( dx >= src_w ) ) { dx = src_w -1; }

            *dst_data++ = src_row[ dx ];
        }
    }

    return 0;
}

static int bitmap_resize_bilinear( bitmap_t* src, bitmap_t* dst ) {
    int src_w = src->width;
    int src_h = src->height;

    double scale_x = src_w / (double)dst->width;
    double scale_y = src_h / (double)dst->height;
    double ix, iy, fx, fy, neg_x, neg_y, ixy, nxy, xny, ynx;
    int x, y, lo_x, lo_y;

    uint32_t *dst_data = ( uint32_t* )dst->data;
    uint32_t *src_data = ( uint32_t* )src->data;

    uint32_t q11, q12, q21, q22;
    uint32_t r, g, b, a;

    for ( fy = y = 0; y < dst->height; ++y, fy += scale_y ) {
        lo_y = (int) fy;
        iy = fy - lo_y;
        neg_y = 1 - iy;

        uint32_t* tmp_y = src_data + lo_y * src_w;
        uint32_t* tmp_y2;
        if ( lo_y + 1 < src_h ) { tmp_y2 = tmp_y + src_w; } else {  tmp_y2 = tmp_y; }

        for ( fx = x = 0; x < dst->width; ++x, fx += scale_x ) {
            lo_x = (int) fx;

            uint32_t* tmp = tmp_y + lo_x;
            uint32_t* tmp2 = tmp_y2 + lo_x;

            q11 = *tmp;
            q12 = *tmp2; // loX hiY = Q12

            if ( lo_x + 1 >= src_w ) { q21 = q11; q22 = q12; } else { q21 = *(tmp+1); q22 = *(tmp2+1); } // hiX hiY = Q22,  hiX loY = Q21

            ix = fx - lo_x;
            neg_x = 1 - ix;

            ixy = ix * iy;
            nxy = neg_x * neg_y;
            xny = ix * neg_y ;
            ynx = iy * neg_x;

            r = RED(q11)      * nxy + RED(q21)    * xny+ RED(q12)      * ynx+ RED(q22)    * ixy;
            g = GREEN(q11) * nxy + GREEN(q21)  * xny+ GREEN(q12) * ynx+ GREEN(q22)* ixy;
            b = BLUE(q11)    * nxy + BLUE(q21)    * xny+ BLUE(q12)    * ynx+ BLUE(q22)  * ixy;
            a = ALPHA(q11)  * nxy + ALPHA(q21)  * xny+ ALPHA(q12) * ynx+ ALPHA(q22) * ixy;

            *dst_data++ = (a << 24) | (r << 16) | (g << 8) | b ;
        }
    }

    return 0;
}

static inline double cube_kernel( double d ) {
    d = d >= 0 ? d : -d;

    double b = d*d;

    if ( d < 1 ) {
        return b*d - 2*b +1;
    }

    if ( d < 2 ) {
        return -b*d + 5*b - 8*d + 4;
    }

    return 0;
}

static int bitmap_resize_bicubic( bitmap_t* src, bitmap_t* dst ) {
    int src_w = src->width;
    int src_h = src->height;

    double scale_x = src_w / (double)dst->width;
    double scale_y = src_h / (double)dst->height;

    uint32_t* dst_data = ( uint32_t* )dst->data;
    uint32_t* src_data = ( uint32_t* )src->data;

    double cx, cy, w;
    int x, y, i, j, u, v;
    double r, g, b, a, rr, gg, bb, aa;
    uint8_t pa, pb, pg, pr;

    for ( y = 0; y < dst->height; ++y ) {
        cy  = y * scale_y;

        for ( x = 0; x < dst->width; ++x ) {
            cx = x * scale_x;
            rr = gg = bb = aa = 0.0;
            for ( j = 0; j <= 3; ++j ) {
                v = (int)cy - 1 + j;
                if ( v < 0 ) v = 0;
                if ( v >= src_h ) v = src_h -1;

                r = g = b = a = 0.0;
                for ( i = 0; i <= 3; ++i ) {
                    u = (int)cx -1 +i;
                    if ( u < 0 ) u = 0;
                    if ( u >= src_w ) u = src_w -1;

                    //assert( v * src_w + u < src_w * src_h );

                    w = cube_kernel( cx - u );
                    uint32_t argb = src_data[ v * src_w + u ];

                    r += RED( argb ) * w;
                    g += GREEN( argb ) * w;
                    b += BLUE( argb ) * w;
                    a += ALPHA( argb ) * w;

                }

                w = cube_kernel( cy - v );

                rr += r * w;
                gg += g * w;
                bb += b * w;
                aa +=  a * w;
            }

            pa = (uint32_t)aa > 255 ? ~(uint32_t)aa : (uint32_t)aa;
            pr = (uint32_t)rr > 255 ? ~(uint32_t)rr : (uint32_t)rr;
            pg = (uint32_t)gg > 255 ? ~(uint32_t)gg : (uint32_t)gg;
            pb = (uint32_t)bb > 255 ? ~(uint32_t)bb : (uint32_t)bb;

            *dst_data++ =  (pa << 24) | (pr << 16) | (pg << 8) | pb ;
        }
    }

    return 0;
}

bitmap_t* bitmap_resize( bitmap_t* src, int width, int height, resize_algorithm_t algorithm ) {
    bitmap_t* dest;

    if ( ( src == NULL ) ||
         ( width <= 0 ) ||
         ( height <= 0 ) ) {
        return NULL;
    }

    if ( ( src->width == width ) &&
         ( src->height == height ) ) {
        bitmap_inc_ref( src );
        return src;
    }

    dest = bitmap_create( width, height, CS_RGB32 );

    if ( dest == NULL ) {
        return NULL;
    }

    switch ( algorithm ) {
        case NEAREST :
            bitmap_resize_nearest( src, dest );
            break;

        case BILINEAR :
            bitmap_resize_bilinear( src, dest );
            break;

        case BICUBIC :
            bitmap_resize_bicubic( src, dest );
            break;
    }

    return dest;
}
