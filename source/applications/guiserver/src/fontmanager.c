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
#include <yutil/array.h>

#include <fontmanager.h>
#include <splash.h>

static FT_Library ft_library;

static hashtable_t family_table;

static void* family_key( hashitem_t* item ) {
    font_family_t* family;

    family = ( font_family_t* )item;

    return ( void* )family->name;
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
    return do_hash_number(
        ( uint8_t* )key,
        sizeof( font_properties_t )
    );
}

static int node_compare( const void* key1, const void* key2 ) {
    return ( memcmp( key1, key2, sizeof( font_properties_t ) ) == 0 );
}

static font_family_t* create_font_family( const char* name ) {
    font_family_t* family;

    family = ( font_family_t* )malloc( sizeof( font_family_t ) );

    if ( family == NULL ) {
        goto error1;
    }

    family->name = strdup( name );

    if ( family->name == NULL ) {
        goto error2;
    }

    if ( init_hashtable( &family->styles,  32, style_key, hash_string, compare_string ) != 0 ) {
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
    font_style_t* style;

    style = ( font_style_t* )malloc( sizeof( font_style_t ) );

    if ( style == NULL ) {
        goto error1;
    }

    style->name = strdup( name );

    if ( style->name == NULL ) {
        goto error2;
    }

    if ( init_hashtable( &style->nodes, 32, node_key, node_hash, node_compare ) != 0 ) {
        goto error3;
    }

    if ( pthread_mutex_init( &style->mutex, NULL ) != 0 ) {
        goto error4;
    }

    style->face = face;
    style->glyph_count = face->num_glyphs;
    style->scalable = ( ( face->face_flags & FT_FACE_FLAG_SCALABLE ) != 0 );
    style->fixed_width = ( ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH ) != 0 );

    return style;

error4:
    destroy_hashtable( &style->nodes );

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

font_glyph_t* font_node_get_glyph( font_node_t* node, unsigned int character ) {
    int top;
    int left;
    int index;
    FT_Face face;
    FT_Error error;
    FT_GlyphSlot glyph;

    int raster_size;
    font_glyph_t* font_glyph;

    face = node->style->face;
    index = FT_Get_Char_Index( face, character );

    if ( ( index < 0 ) || ( index >= node->style->glyph_count ) ) {
        return NULL;
    }

    if ( node->glyph_table[ index ] != NULL ) {
        return node->glyph_table[ index];
    }

    if ( node->style->scalable ) {
        FT_Set_Char_Size( face, node->properties.point_size, node->properties.point_size, 96, 96 );
    } else {
        FT_Set_Pixel_Sizes( face, 0, ( node->properties.point_size * 96 / 72 ) / 64 );
    }

    FT_Set_Transform( face, NULL, NULL );

    error = FT_Load_Glyph( face, index, FT_LOAD_DEFAULT );

    if ( error != 0 ) {
        dbprintf( "font_node_get_glyph(): Unable to load glyph char=%u index=%d (error=%d)\n", character, index, error );
        return NULL;
    }

    glyph = face->glyph;

    if ( node->style->scalable ) {
        top = -( ( glyph->metrics.horiBearingY + 63 ) & -64 ) / 64;
        left = ( glyph->metrics.horiBearingX & -64 ) / 64;
    } else {
        top = -glyph->bitmap_top;
        left = glyph->bitmap_left;
    }

    if ( node->style->scalable ) {
        if ( node->properties.flags & FONT_SMOOTHED ) {
            error = FT_Render_Glyph( glyph, FT_RENDER_MODE_NORMAL );
        } else {
            error = FT_Render_Glyph( glyph, FT_RENDER_MODE_MONO );
        }

        if ( error != 0 ) {
          dbprintf( "font_node_get_glyph(): Failed to render glyph: 0x%X\n", error );
          return NULL;
        }
    }

    if ( ( glyph->bitmap.width < 0 ) ||
         ( glyph->bitmap.rows < 0 ) ||
         ( glyph->bitmap.pitch < 0 ) ) {
        dbprintf(
            "font_node_get_glyph(): Glyph got invalid size %dx%d (%d)\n",
            glyph->bitmap.width,
            glyph->bitmap.rows,
            glyph->bitmap.pitch
        );

        return NULL;
    }

    if ( glyph->bitmap.pixel_mode == ft_pixel_mode_grays ) {
        raster_size = glyph->bitmap.pitch * glyph->bitmap.rows;
    } else if ( glyph->bitmap.pixel_mode == ft_pixel_mode_mono ) {
        raster_size = glyph->bitmap.width * glyph->bitmap.rows;
    } else {
        dbprintf( "font_node_get_glyph(): Unknown pixel mode: %d\n", glyph->bitmap.pixel_mode );
        return NULL;
    }

    font_glyph = ( font_glyph_t* )malloc( sizeof( font_glyph_t ) + raster_size );

    if ( font_glyph == NULL ) {
        return NULL;
    }

    node->glyph_table[ index ] = font_glyph;

    font_glyph->raster = ( uint8_t* )( font_glyph + 1 );

    rect_init(
        &font_glyph->bounds,
        left,
        top,
        left + glyph->bitmap.width - 1,
        top + glyph->bitmap.rows - 1
    );

    if ( node->style->scalable ) {
        if ( node->style->fixed_width ) {
            font_glyph->advance.x = node->advance;
        } else {
            font_glyph->advance.x = ( glyph->metrics.horiAdvance + 32 ) / 64;
            font_glyph->advance.y = ( glyph->metrics.vertAdvance + 32 ) / 64;
        }
    } else {
        font_glyph->advance.x = glyph->bitmap.width;
    }

    if ( glyph->bitmap.pixel_mode == ft_pixel_mode_grays ) {
        font_glyph->bytes_per_line = glyph->bitmap.pitch;

        memcpy( font_glyph->raster, glyph->bitmap.buffer, raster_size );
    } else {
        int x;
        int y;
        int w;
        int h;

        font_glyph->bytes_per_line = glyph->bitmap.width;

        rect_bounds( &font_glyph->bounds, &w, &h );

        for ( y = 0; y < h; ++y ) {
            for ( x = 0; x < w; ++x ) {
                if ( glyph->bitmap.buffer[ x / 8 + y * glyph->bitmap.pitch ] & ( 1 << ( 7 - ( x % 8 ) ) ) ) {
                    font_glyph->raster[ x + y * font_glyph->bytes_per_line ] = 255;
                } else {
                    font_glyph->raster[ x + y * font_glyph->bytes_per_line ] = 0;
                }
            } // for x
        } // for y
    }

    return font_glyph;
}

font_node_t* create_font_node( font_style_t* style, font_properties_t* properties ) {
    FT_Size size;
    font_node_t* node;

    node = ( font_node_t* )malloc( sizeof( font_node_t ) );

    if ( node == NULL ) {
        goto error1;
    }

    node->style = style;
    memcpy( &node->properties, properties, sizeof( font_properties_t ) );

    if ( style->glyph_count > 0 ) {
        node->glyph_table = ( font_glyph_t** )malloc( sizeof( font_glyph_t* ) * style->glyph_count );

        if ( node->glyph_table == NULL ) {
            goto error2;
        }

        memset( node->glyph_table, 0, sizeof( font_glyph_t* ) * style->glyph_count );
    } else {
        node->glyph_table = NULL;
    }

    if ( style->scalable ) {
        FT_Set_Char_Size( style->face, properties->point_size, properties->point_size, 96, 96 );
    } else {
        FT_Set_Pixel_Sizes( style->face, 0, ( properties->point_size * 96 / 72 ) / 64 );
    }

    size = style->face->size;

    if ( size->metrics.descender > 0 ) {
        node->descender = -( size->metrics.descender + 63 ) / 64;
    } else {
        node->descender = ( size->metrics.descender + 63 ) / 64;
    }

    node->ascender = ( size->metrics.ascender + 63 ) / 64;
    node->line_gap = ( size->metrics.height + 63 ) / 64 - ( node->ascender - node->descender );
    node->advance = ( size->metrics.max_advance + 63 ) / 64;

    return node;

error2:
    free( node );

error1:
    return NULL;
}

int font_node_get_string_width( font_node_t* font, const char* text, int length ) {
    int width;
    font_glyph_t* glyph;

    width = 0;

    pthread_mutex_lock( &font->style->mutex );

    while ( length > 0 ) {
        int char_length = utf8_char_length( *text );

        if ( char_length > length ) {
            break;
        }

        glyph = font_node_get_glyph( font, utf8_to_unicode( text ) );

        text += char_length;
        length -= char_length;

        if ( glyph == NULL ) {
            continue;
        }

        width += glyph->advance.x;
    }

    pthread_mutex_unlock( &font->style->mutex );

    return width;
}

int font_node_get_height( font_node_t* font ) {
    return ( font->ascender - font->descender + font->line_gap );
}

font_node_t* font_manager_get( const char* family_name, const char* style_name, font_properties_t* properties ) {
    font_family_t* family;
    font_style_t* style;
    font_node_t* node;

    family = ( font_family_t* )hashtable_get( &family_table, ( const void* )family_name );

    if ( family == NULL ) {
        return NULL;
    }

    style = ( font_style_t* )hashtable_get( &family->styles, ( const void* )style_name );

    if ( style == NULL ) {
        return NULL;
    }

    pthread_mutex_lock( &style->mutex );

    node = ( font_node_t* )hashtable_get( &style->nodes, ( const void* )properties );

    if ( node == NULL ) {
        node = create_font_node( style, properties );

        if ( node != NULL ) {
            hashtable_add( &style->nodes, ( hashitem_t* )node );
        }
    }

    pthread_mutex_unlock( &style->mutex );

    return node;
}

int font_manager_load_fonts( void ) {
    int i;
    DIR* dir;
    struct dirent* entry;

    FT_Face face;
    FT_Error error;
    FT_CharMap char_map;

    font_family_t* family;
    font_style_t* style;

    int size;
    array_t font_files;

    if ( init_array( &font_files ) != 0 ) {
        return -1;
    }

    array_set_realloc_size( &font_files, 32 );

    dir = opendir( "/yaosp/system/fonts" );

    if ( dir == NULL ) {
        destroy_array( &font_files );
        return -1;
    }

    while ( ( entry = readdir( dir ) ) != NULL ) {
        char path[ 256 ];

        if ( ( strcmp( entry->d_name, "." ) == 0 ) ||
             ( strcmp( entry->d_name, ".." ) == 0 ) ) {
            continue;
        }

        snprintf( path, sizeof( path ), "/yaosp/system/fonts/%s", entry->d_name );

        array_add_item( &font_files, strdup( path ) );
    }

    closedir( dir );

    size = array_get_size( &font_files );
    splash_count_total += size;

    for ( i = 0; i < size; i++ ) {
        int j;
        char* path;

        path = ( char* )array_get_item( &font_files, i );

        error = FT_New_Face( ft_library, path, 0, &face );

        if ( error != 0 ) {
            dbprintf( "Failed to load font: %s (%d)\n", path, error );
            goto next_font;
        }

        for ( j = 0; j < face->num_charmaps; j++ ) {
            char_map = face->charmaps[ j ];

            if ( ( char_map->platform_id == 3 ) &&
                 ( char_map->encoding_id == 1 ) ) {
                goto found;
            }
        }

        FT_Done_Face( face );

        goto next_font;

found:
        face->charmap = char_map;

        family = get_font_family( face->family_name );

        if ( family == NULL ) {
            family = create_font_family( face->family_name );

            if ( family == NULL ) {
                /* TODO: clean up? */
                goto next_font;
            }

            insert_font_family( family );
        }

        style = get_font_style( family, face->style_name );

        if ( style != NULL ) {
            /* TODO: clean up? */
            goto next_font;
        }

        style = create_font_style( face->style_name, face );

        if ( style == NULL ) {
            /* TODO: clean up? */
            goto next_font;
        }

        insert_font_style( family, style );

    next_font:
        splash_inc_progress();
        free( path );
    }

    destroy_array( &font_files );

    return 0;
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
        hash_string,
        compare_string
    );

    if ( result < 0 ) {
        return result;
    }

    return 0;
}
