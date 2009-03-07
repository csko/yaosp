/* i386 specific graphics driver implementation
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

#include <assert.h>
#include <yaosp/debug.h>

#include <graphicsdriver.h>

static void fill_rect_rgb32( bitmap_t* bitmap, rect_t* rect, uint32_t color ) {
    uint32_t h;
    uint32_t w;
    uint32_t* data;
    uint32_t padding;

    w = rect_width( rect );
    h = rect_height( rect );

    assert( bitmap->height >= h );
    assert( bitmap->width >= w );

    if ( ( h == 0 ) || ( w == 0 ) ) {
        return;
    }

    data = ( uint32_t* )bitmap->buffer + ( rect->top * bitmap->width + rect->left );
    padding = bitmap->width - rect_width( rect );

    __asm__ __volatile__(
        "1:\n"
        "movl %%ebx, %%ecx\n"
        "rep stosl\n"
        "addl %%esi, %%edi\n"
        "decl %%edx\n"
        "jnz 1b\n"
        :
        : "a" ( color ), "b" ( w ), "d" ( h ), "S" ( padding * 4 ), "D" ( data )
        : "ecx", "memory"
    );
}

int i386_fill_rect( bitmap_t* bitmap, rect_t* rect, color_t* color ) {
    switch ( bitmap->color_space ) {
        case CS_RGB32 :
            fill_rect_rgb32( bitmap, rect, color_to_uint32( color ) );
            break;

        default :
            dbprintf( "%s() Not supported for color space: %d\n", __FUNCTION__, bitmap->color_space );
            break;
    }

    return 0;
}
