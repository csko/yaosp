/* Font loader and render engine
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

#include <dirent.h>
#include <string.h>
#include <yaosp/debug.h>

#include <fontmanager.h>

static FT_Library ft_library;

static void load_fonts( void ) {
    int i;
    DIR* dir;
    char path[ 128 ];
    struct dirent* entry;

    FT_Face face;
    FT_Error error;
    FT_CharMap char_map;

    dir = opendir( "/yaosp/system/fonts" );

    if ( dir == NULL ) {
        return;
    }

    while ( ( entry = readdir( dir ) ) != NULL ) {
        if ( ( strcmp( entry->d_name, "." ) == 0 ) ||
             ( strcmp( entry->d_name, ".." ) == 0 ) ) {
            continue;
        }

        snprintf( path, sizeof( path ), "/yaosp/system/fonts/%s", entry->d_name );

        dbprintf( "Loading font: %s\n", path );

        error = FT_New_Face( ft_library, path, 0, &face );

        if ( error != 0 ) {
            dbprintf( "Failed to open font: %s (%d)\n", path, error );
            continue;
        }

        for ( i = 0; i < face->num_charmaps; i++ ) {
            char_map = face->charmaps[ i ];

            if ( ( char_map->platform_id == 3 ) &&
                 ( char_map->encoding_id == 1 ) ) {
                goto found;
            }
        }

        FT_Done_Face( face );

        continue;

found:
        face->charmap = char_map;

        dbprintf( "Font: %s %s\n", face->family_name, face->style_name );
    }

    closedir( dir );
}

int init_font_manager( void ) {
    FT_Error error;

    error = FT_Init_FreeType( &ft_library );

    if ( error != 0 ) {
        dbprintf( "Failed to initialize freetype library\n" );
        return -1;
    }

    load_fonts();

    return 0;
}
