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

static hashtable_t family_table;

static void* family_key( hashitem_t* item ) {
    font_family_t* family;

    family = ( font_family_t* )item;

    return ( void* )family->name;
}

static uint32_t family_hash( const void* key ) {
    return hash_string(
        ( uint8_t* )key,
        strlen( ( const char* )key )
    );
}

static int family_compare( const void* key1, const void* key2 ) {
    return ( strcmp( ( const char* )key1, ( const char* )key2 ) == 0 );
}

static void* style_key( hashitem_t* item ) {
    font_style_t* style;

    style = ( font_style_t* )item;

    return ( void* )style->name;
}

static void* node_key( hashitem_t* item ) {
    font_node_t* node;

    node = ( font_node_t* )item;

    return ( void* )&node->properties;
}

static uint32_t node_hash( const void* key ) {
    return hash_number(
        ( uint8_t* )key,
        sizeof( font_properties_t )
    );
}

static int node_compare( const void* key1, const void* key2 ) {
    return ( memcmp( key1, key2, sizeof( font_properties_t ) ) == 0 );
}

static font_family_t* create_font_family( const char* name ) {
    int error;
    font_family_t* family;

    family = ( font_family_t* )malloc( sizeof( font_family_t ) );

    if ( family == NULL ) {
        goto error1;
    }

    family->name = strdup( name );

    if ( family->name == NULL ) {
        goto error2;
    }

    /* NOTE: We can use family_hash and family_compare because we use the name
             strings as keys for both font families and font styles. */

    error = init_hashtable(
        &family->styles,
        32,
        style_key,
        family_hash,
        family_compare
    );

    if ( error < 0 ) {
        goto error3;
    }

    return family;

error3:
    free( ( void* )family->name );

error2:
    free( family );

error1:
    return NULL;
}

static void insert_font_family( font_family_t* family ) {
    hashtable_add( &family_table, ( hashitem_t* )family );
}

static font_family_t* get_font_family( const char* name ) {
    return ( font_family_t* )hashtable_get( &family_table, ( const void* )name );
}

static font_style_t* create_font_style( const char* name, FT_Face face ) {
    int error;
    font_style_t* style;

    style = ( font_style_t* )malloc( sizeof( font_style_t ) );

    if ( style == NULL ) {
        goto error1;
    }

    style->name = strdup( name );

    if ( style->name == NULL ) {
        goto error2;
    }

    error = init_hashtable(
        &style->nodes,
        32,
        node_key,
        node_hash,
        node_compare
    );

    if ( error < 0 ) {
        goto error3;
    }

    return style;

error3:
    free( ( void* )style->name );

error2:
    free( style );

error1:
    return NULL;
}

static font_style_t* get_font_style( font_family_t* family, const char* name ) {
    return ( font_style_t* )hashtable_get( &family->styles, ( const void* )name );
}

static void insert_font_style( font_family_t* family, font_style_t* style ) {
    hashtable_add( &family->styles, ( hashitem_t* )style );
}

static void load_fonts( void ) {
    int i;
    DIR* dir;
    char path[ 128 ];
    struct dirent* entry;

    FT_Face face;
    FT_Error error;
    FT_CharMap char_map;

    font_family_t* family;
    font_style_t* style;

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

        family = get_font_family( face->family_name );

        if ( family == NULL ) {
            family = create_font_family( face->family_name );

            if ( family == NULL ) {
                /* TODO: clean up? */
                continue;
            }

            insert_font_family( family );
        }

        style = get_font_style( family, face->style_name );

        if ( style != NULL ) {
            /* TODO: clean up? */
            continue;
        }

        style = create_font_style( face->style_name, face );

        if ( style == NULL ) {
            /* TODO: clean up? */
            continue;
        }

        insert_font_style( family, style );
    }

    closedir( dir );
}

int init_font_manager( void ) {
    int result;
    FT_Error error;

    error = FT_Init_FreeType( &ft_library );

    if ( error != 0 ) {
        dbprintf( "Failed to initialize freetype library\n" );
        return -1;
    }

    result = init_hashtable(
        &family_table,
        64,
        family_key,
        family_hash,
        family_compare
    );

    if ( result < 0 ) {
        return result;
    }

    load_fonts();

    return 0;
}
