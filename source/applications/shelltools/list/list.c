/* list shell command
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

static char* argv0 = NULL;

static const char* units[] = { "b", "Kb", "Mb", "Gb", "Tb", "Pb" };

static int entry_compare( const void* p1, const void* p2 ) {
    struct dirent* entry1 = ( struct dirent* )p1;
    struct dirent* entry2 = ( struct dirent* )p2;

    return strcmp( entry1->d_name, entry2->d_name );
}

static int do_print_entry( char* path, struct stat* st ) {
    char* name;

    name = strrchr( path, '/' );

    if ( name == NULL ) {
        name = path;
    } else {
        name++;
    }

    if ( S_ISDIR( st->st_mode ) ) {
        printf( "directory %s\n", name );
    } else if ( S_ISLNK( st->st_mode ) ) {
        char link[ 256 ];

        readlink( path, link, sizeof( link ) );

        printf( "  symlink %s -> %s\n", name, link );
    } else {
        int unit = 0;
        char size[ 16 ];

        while ( ( st->st_size >= 1024 ) && ( unit < sizeof( units ) ) ) {
            st->st_size /= 1024;
            unit++;
        }

        snprintf( size, sizeof( size ), "%llu %2s", st->st_size, units[ unit ] );
        printf( "%9s %s\n", size, name );
    }

    return 0;
}

static int do_list_file( char* filename ) {
    struct stat st;

    if ( lstat( filename, &st ) != 0 ) {
        return -1;
    }

    do_print_entry( filename, &st );

    return 0;
}

static int do_list_directory( char* dirname ) {
    int i;
    int fd;
    int tmp;
    int act_count = 0;
    int max_count = 0;
    struct dirent* act;
    struct dirent* entry;
    struct dirent* entries = NULL;

    struct stat entry_stat;
    char full_path[ 256 ];

    fd = open( dirname, O_RDONLY );

    if ( fd < 0 ) {
        fprintf( stderr, "%s: cannot access %s: No such file or directory\n", argv0, dirname );
        free( entries );
        return -1;
    }

    do {
        entries = ( struct dirent* )realloc( entries, sizeof( struct dirent ) * ( max_count + 32 ) );
        act = entries + act_count;

        max_count += 32;

        tmp = getdents( fd, act, sizeof( struct dirent ) * 32 );

        if ( tmp < 0 ) {
            break;
        }

        act_count += tmp;
    } while ( act_count == 32 );

    close( fd );

    if ( act_count > 0 ) {
        int is_root;

        qsort( entries, act_count, sizeof( struct dirent ), entry_compare );

        is_root = ( strcmp( dirname, "/" ) == 0 );

        for ( i = 0; i < act_count; i++ ) {
            entry = &entries[ i ];

            if ( is_root ) {
                snprintf( full_path, sizeof( full_path ), "/%s", entry->d_name );
            } else {
                snprintf( full_path, sizeof( full_path ), "%s/%s", dirname, entry->d_name );
            }

            if ( lstat( full_path, &entry_stat ) != 0 ) {
                continue;
            }

            do_print_entry( full_path, &entry_stat );
        }
    }

    free( entries );

    return 0;
}

int main( int argc, char** argv ) {
    int i;
    int ret = EXIT_SUCCESS;

    if ( argc > 0 ) {
        argv0 = argv[ 0 ];
    }

    if ( argc > 1 ) {
        for ( i = 1; i < argc; i++ ) {
            struct stat st;

            if ( lstat( argv[ i ], &st ) != 0 ) {
                continue;
            }

            if ( S_ISDIR( st.st_mode ) ) {
                if ( do_list_directory( argv[ i ] ) < 0 ) {
                    ret = EXIT_FAILURE;
                }
            } else {
                if ( do_list_file( argv[ i ] ) < 0 ) {
                    ret = EXIT_FAILURE;
                }
            }
        }
    } else {
        if ( do_list_directory( "." ) < 0 ) {
            ret = EXIT_FAILURE;
        }
    }

    return ret;
}
