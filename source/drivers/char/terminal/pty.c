/* Terminal driver
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

#include <console.h>
#include <errno.h>
#include <time.h>
#include <mm/kmalloc.h>
#include <vfs/filesystem.h>
#include <lock/mutex.h>
#include <lock/condition.h>
#include <lock/semaphore.h>
#include <lib/hashtable.h>
#include <lib/string.h>
#include <lib/ctype.h>

#include "pty.h"

#define PTY_BUFSIZE 32768

static lock_id pty_lock;
static ino_t pty_inode_counter = 1;
static hashtable_t pty_node_table;

static pty_node_t root_inode = {
    .inode_number = PTY_ROOT_INODE,
    .mode = S_IFDIR | 0777
};

static inline bool pty_is_master( pty_node_t* node ) {
    return ( node->name[ 0 ] == 'p' );
}

static pty_node_t* pty_create_node( const char* name, int name_length, size_t buffer_size, mode_t mode ) {
    char tmp[ 32 ];
    pty_node_t* node;

    node = ( pty_node_t* )kmalloc( sizeof( pty_node_t ) );

    if ( node == NULL ) {
        goto error1;
    }

    memset( node, 0, sizeof( pty_node_t ) );

    if ( name_length == -1 ) {
        node->name = strdup( name );
    } else {
        node->name = strndup( name, name_length );
    }

    if ( node->name == NULL ) {
        goto error2;
    }

    node->buffer = ( uint8_t* )kmalloc( buffer_size );

    if ( node->buffer == NULL ) {
        goto error3;
    }

    snprintf( tmp, sizeof( tmp ), "%s lock", name );

    node->lock = mutex_create( tmp, MUTEX_NONE );

    if ( node->lock < 0 ) {
        goto error4;
    }

    snprintf( tmp, sizeof( tmp ), "%s read queue", name );

    node->read_queue = condition_create( tmp );

    if ( node->read_queue < 0 ) {
        goto error5;
    }

    snprintf( tmp,  sizeof( tmp ), "%s write queue", name );

    node->write_queue = condition_create( tmp );

    if ( node->write_queue < 0 ) {
        goto error6;
    }

    node->mode = mode;
    node->buffer_size = buffer_size;

    node->size = 0;
    node->read_position = 0;
    node->write_position = 0;

    node->read_requests = NULL;
    node->write_requests = NULL;

    return node;

error6:
    condition_destroy( node->read_queue );

error5:
    mutex_destroy( node->lock );

error4:
    kfree( node->buffer );

error3:
    kfree( node->name );

error2:
    kfree( node );

error1:
    return NULL;
}

static int pty_insert_node( pty_node_t* node ) {
    do {
        node->inode_number = pty_inode_counter++;

        if ( pty_inode_counter < 0 ) {
            pty_inode_counter = PTY_ROOT_INODE + 1;
        }
    } while ( hashtable_get( &pty_node_table, ( const void* )&node->inode_number ) != NULL );

    hashtable_add( &pty_node_table, ( hashitem_t* )node );

    return 0;
}

static int pty_mount( const char* device, uint32_t flags, void** fs_cookie, ino_t* root_inode_num ) {
    *root_inode_num = PTY_ROOT_INODE;
    root_inode.atime = root_inode.mtime = root_inode.ctime = time( NULL );

    return 0;
}

static int pty_unmount( void* fs_cookie ) {
    return 0;
}

static int pty_read_inode( void* fs_cookie, ino_t inode_num, void** _node ) {
    pty_node_t* node;

    if ( inode_num == PTY_ROOT_INODE ) {
        *_node = ( void* )&root_inode;
        return 0;
    }

    mutex_lock( pty_lock, LOCK_IGNORE_SIGNAL );

    node = ( pty_node_t* )hashtable_get( &pty_node_table, ( const void* )&inode_num );

    mutex_unlock( pty_lock );

    if ( node == NULL ) {
        return -ENOINO;
    }

    *_node = ( void* )node;

    return 0;
}

static int pty_write_inode( void* fs_cookie, void* node ) {
    return 0;
}

static int pty_lookup_helper( hashitem_t* item, void* _data ) {
    pty_node_t* node;
    pty_lookup_data_t* data;

    node = ( pty_node_t* )item;
    data = ( pty_lookup_data_t* )_data;

    if ( ( strlen( node->name ) == data->length ) &&
         ( strncmp( node->name, data->name, data->length ) == 0 ) ) {
        *data->inode_number = node->inode_number;

        return -1;
    }

    return 0;
}

static int pty_lookup_inode( void* fs_cookie, void* parent, const char* name, int name_length, ino_t* inode_num ) {
    int error;
    pty_lookup_data_t data;

    data.name = ( char* )name;
    data.length = name_length;
    data.inode_number = inode_num;

    mutex_lock( pty_lock, LOCK_IGNORE_SIGNAL );

    error = hashtable_iterate( &pty_node_table, pty_lookup_helper, ( void* )&data );

    mutex_unlock( pty_lock );

    if ( error == 0 ) {
        return -ENOENT;
    }

    return 0;
}

static int pty_open_directory( void** _cookie ) {
    pty_dir_cookie_t* cookie;

    cookie = ( pty_dir_cookie_t* )kmalloc( sizeof( pty_dir_cookie_t ) );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    cookie->current = 0;

    *_cookie = ( void* )cookie;

    return 0;
}

static int pty_open( void* fs_cookie, void* _node, int mode, void** file_cookie ) {
    pty_node_t* node;

    if ( _node == ( void* )&root_inode ) {
        return pty_open_directory( file_cookie );
    }

    node = ( pty_node_t* )_node;

    if ( pty_is_master( node ) ) {
        return -EINVAL;
    }

    return 0;
}

static int pty_close( void* fs_cookie, void* node, void* file_cookie ) {
    return 0;
}

static int pty_free_cookie( void* fs_cookie, void* node, void* file_cookie ) {
    if ( node == ( void* )&root_inode ) {
        kfree( file_cookie );
    }

    return 0;
}

static int pty_read( void* fs_cookie, void* _node, void* file_cookie, void* buffer, off_t pos, size_t size ) {
    int read;
    uint8_t* data;
    pty_node_t* node;
    select_request_t* request;

    if ( _node == ( void* )&root_inode ) {
        return -EINVAL;
    }

    read = 0;
    data = ( uint8_t* )buffer;
    node = ( pty_node_t* )_node;

    mutex_lock( node->lock, LOCK_IGNORE_SIGNAL );

    while ( node->size == 0 ) {
        condition_wait( node->read_queue, node->lock );
    }

    while ( ( size > 0 ) && ( node->size > 0 ) ) {
        *data++ = node->buffer[ node->read_position ];

        read++;
        size--;
        node->size--;
        node->read_position = ( node->read_position + 1 ) % node->buffer_size;
    }

    /* Notify write select listeners */

    request = node->write_requests;

    while ( request != NULL ) {
        request->ready = true;
        semaphore_unlock( request->sync, 1 );

        request = request->next;
    }

    mutex_unlock( node->lock );

    /* Tell possibly waiting writers that we have free space */

    condition_broadcast( node->write_queue );

    return read;
}

static int pty_do_write( pty_node_t* node, const void* buffer, size_t size ) {
    int written;
    uint8_t* data;
    select_request_t* request;

    written = 0;
    data = ( uint8_t* )buffer;

    while ( size > 0 ) {
        while ( node->size == node->buffer_size ) {
            condition_wait( node->write_queue, node->lock );
        }

        while ( ( node->size < node->buffer_size ) && ( size > 0 ) ) {
            node->buffer[ node->write_position ] = *data++;

            written++;
            node->size++;
            size--;
            node->write_position = ( node->write_position + 1 ) % node->buffer_size;
        }
    }

    /* Notify read select listeners */

    request = node->read_requests;

    while ( request != NULL ) {
        request->ready = true;
        semaphore_unlock( request->sync, 1 );

        request = request->next;
    }

    /* Tell possibly waiting readers that we have data */

    condition_broadcast( node->read_queue );

    return written;
}

static int pty_do_write_master( pty_node_t* master, const void* buffer, size_t size, bool echo_mode ) {
    size_t i;
    char* buf;
    struct termios* term_info;

    buf = ( char* )buffer;

    mutex_lock( master->lock, LOCK_IGNORE_SIGNAL );

    term_info = master->term_info;

    for ( i = 0; i < size; i++, buf++ ) {
        switch ( *buf ) {
            case '\n' :
                if ( ( term_info->c_oflag & ( OPOST | ONLCR ) ) == ( OPOST | ONLCR ) ) {
                    pty_do_write( master, "\r\n", 2 );
                } else {
                    pty_do_write( master, "\n", 1 );
                }

                break;

            case '\r' : {
                bool hascr;

                hascr = ( ( term_info->c_oflag & ( OPOST | ONLRET ) ) == 0 ) ||
                       ( ( term_info->c_oflag & ( OPOST | ONLRET ) ) == OPOST );

                if ( hascr ) {
                    if ( ( term_info->c_oflag & ( OPOST | OCRNL ) ) == ( OPOST | OCRNL ) ) {
                        pty_do_write( master, "\n", 1 );
                    } else {
                        pty_do_write( master, "\r", 1 );
                    }
                }

                break;
            }

            case '\b' :
                if ( ( echo_mode ) &&
                     ( term_info->c_lflag & ICANON ) ) {
                    pty_do_write( master, "\b \b", 3 );
                } else {
                    pty_do_write( master, "\b", 1 );
                }

                break;

            default :
                pty_do_write( master, buf, 1 );
                break;
        }
    }

    mutex_unlock( master->lock );

    return size;
}

static void pty_echo_one_character( pty_node_t* master, pty_node_t* slave, char c ) {
    struct termios* term_info;

    term_info = master->term_info;

    if ( term_info->c_lflag & ECHO ) {
        if ( ( c == '\r' ) && ( term_info->c_iflag & ICRNL ) ) {
            c = '\n';
        }

        if ( ( term_info->c_lflag & ECHOCTL ) &&
             ( ( c < ' ' ) || ( c == '\x7f' ) ) &&
             ( c != '\n' ) &&
             ( c != '\t' ) ) {
            char tmp[ 2 ];

            tmp[ 0 ] = '^';
            tmp[ 1 ] = ( c == '\x7f' ) ? '?' : ( c + '@' );

            pty_do_write_master( master, tmp, 2, true );
        } else {
            pty_do_write_master( master, &c, 1, true );
        }
    }
}

static int pty_do_write_slave( pty_node_t* slave, const void* buffer, size_t size ) {
    char c;
    size_t i;
    char* data;
    struct termios* term_info;

    data = ( char* )buffer;

    mutex_lock( slave->lock, LOCK_IGNORE_SIGNAL );

    term_info = slave->term_info;

    for ( i = 0; i < size; i++, data++ ) {
        c = *data;

        /* Convert uppercase characters to lowercase */

        if ( term_info->c_iflag & IUCLC ) {
            c = tolower( c );
        }

        /* Handle special characters first */

        if ( c == '\n' ) {
            /* Replace \n with \r ? */

            if ( term_info->c_iflag & INLCR ) {
                pty_do_write( slave, "\r", 1 );
            } else {
                pty_do_write( slave, "\n", 1 );
            }

            /* Echo it to the master? */

            if ( ( term_info->c_lflag & ECHO ) ||
                 ( term_info->c_lflag & ECHONL ) ) {
                pty_do_write_master( slave->partner, "\n", 1, true );
            }

            /* Skip the common echo part */

            goto no_echo;
        } else if ( c == '\r' ) {
            if ( ( term_info->c_iflag & IGNCR ) == 0 ) {
                /* Replace \r with \n ? */

                if ( term_info->c_iflag & ICRNL ) {
                    pty_do_write( slave, "\n", 1 );
                } else {
                    pty_do_write( slave, "\r", 1 );
                }
            }

            goto echo;
        } else if ( c == '\0' ) {
            goto write_and_echo;
        }

write_and_echo:
        pty_do_write( slave, &c, 1 );

echo:
        pty_echo_one_character( slave->partner, slave, c );

no_echo:
        ;
    }

    mutex_unlock( slave->lock );

    return size;
}

static int pty_write( void* fs_cookie, void* _node, void* file_cookie, const void* buffer, off_t pos, size_t size ) {
    pty_node_t* node;

    if ( _node == ( void* )&root_inode ) {
        return -EINVAL;
    }

    node = ( pty_node_t* )_node;

    if ( pty_is_master( node ) ) {
        return pty_do_write_slave( node->partner, buffer, size );
    } else {
        return pty_do_write_master( node->partner, buffer, size, false );
    }
}

static int pty_ioctl( void* fs_cookie, void* _node, void* file_cookie, int command, void* buffer, bool from_kernel ) {
    int error;
    pty_node_t* node;

    node = ( pty_node_t* )_node;

    mutex_lock( node->lock, LOCK_IGNORE_SIGNAL );

    switch ( command ) {
        case TCGETA :
            memcpy( buffer, node->term_info, sizeof( struct termios ) );
            error = 0;
            break;

        case TCSETA :
        case TCSETAW :
        case TCSETAF :
            memcpy( node->term_info, buffer, sizeof( struct termios ) );
            error = 0;
            break;

        case TIOCGWINSZ :
            memcpy( buffer, node->window_size, sizeof( struct winsize ) );
            error = 0;
            break;

        case TIOCSWINSZ :
            memcpy( node->window_size, buffer, sizeof( struct winsize ) );
            error = 0;
            break;

        default :
            kprintf( WARNING, "Terminal: Unknown pty ioctl: %x\n", command );
            error = -ENOSYS;
            break;
    }

    mutex_unlock( node->lock );

    return error;
}

static int pty_read_stat( void* fs_cookie, void* _node, struct stat* stat ) {
    pty_node_t* node;

    node = ( pty_node_t* )_node;

    stat->st_ino = node->inode_number;
    stat->st_size = 0;
    stat->st_mode = node->mode;
    stat->st_atime = node->atime;
    stat->st_mtime = node->mtime;
    stat->st_ctime = node->ctime;

    return 0;
}

static int pty_read_dir_helper( hashitem_t* item, void* _data ) {
    pty_node_t* node;
    pty_read_dir_data_t* data;

    node = ( pty_node_t* )item;
    data = ( pty_read_dir_data_t* )_data;

    if ( data->current == data->required ) {
        data->entry->inode_number = node->inode_number;

        strncpy( data->entry->name, node->name, NAME_MAX );
        data->entry->name[ NAME_MAX ] = 0;

        return -1;
    }

    data->current++;

    return 0;
}

static int pty_read_directory( void* fs_cookie, void* node, void* file_cookie, struct dirent* entry ) {
    int error;
    pty_dir_cookie_t* cookie;
    pty_read_dir_data_t data;

    if ( node != ( void* )&root_inode ) {
        return -EINVAL;
    }

    cookie = ( pty_dir_cookie_t* )file_cookie;

    data.current = 0;
    data.required = cookie->current;
    data.entry = entry;

    mutex_lock( pty_lock, LOCK_IGNORE_SIGNAL );

    error = hashtable_iterate( &pty_node_table, pty_read_dir_helper, ( void* )&data );

    mutex_unlock( pty_lock );

    if ( error == 0 ) {
        return 0;
    }

    cookie->current++;

    return 1;
}

static int pty_rewind_directory( void* fs_cookie, void* node, void* file_cookie ) {
    pty_dir_cookie_t* cookie;

    cookie = ( pty_dir_cookie_t* )file_cookie;

    cookie->current = 0;

    return 0;
}

static int pty_create(
    void* fs_cookie,
    void* node,
    const char* name,
    int name_length,
    int mode,
    int permissions,
    ino_t* inode_number,
    void** file_cookie
) {
    int error;
    ino_t dummy;
    char* tty_name;
    pty_node_t* master;
    pty_node_t* slave;
    struct winsize* win_size;
    struct termios* term_info;

    /* Make sure this is a valid node */

    if ( ( name_length < 4 ) || ( strncmp( name, "pty", 3 ) != 0 ) ) {
        return -EINVAL;
    }

    /* Check if the node already exist */

    error = pty_lookup_inode( fs_cookie, node, name, name_length, &dummy );

    if ( error == 0 ) {
        return -EEXIST;
    }

    /* Create the master node */

    master = pty_create_node( name, name_length, PTY_BUFSIZE, S_IFCHR | 0777 );

    if ( master == NULL ) {
        return -ENOMEM;
    }

    /* Build the name of the slave node */

    tty_name = strdup( master->name );

    if ( tty_name == NULL ) {
        return -ENOMEM;
    }

    tty_name[ 0 ] = 't';

    /* Create the slave node */

    slave = pty_create_node( tty_name, -1, PTY_BUFSIZE, S_IFCHR | 0777 );

    kfree( tty_name );

    if ( slave == NULL ) {
        return -ENOMEM;
    }

    /* Make the partnership :) */

    master->partner = slave;
    slave->partner = master;

    /* Initialize terminal info */

    term_info = ( struct termios* )kmalloc( sizeof( struct termios ) );

    memset( term_info, 0, sizeof( struct termios ) );

    term_info->c_iflag = BRKINT | IGNPAR | ISTRIP | ICRNL | IXON;
    term_info->c_oflag = OPOST | ONLCR;
    term_info->c_cflag = CREAD | HUPCL | CLOCAL;
    term_info->c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | IEXTEN | ECHOCTL | ECHOKE;
    term_info->c_ispeed = B38400;
    term_info->c_ospeed = B38400;
    term_info->c_cc[ VINTR ] = '\x03';
    term_info->c_cc[ VQUIT ] = '\x1c';
    term_info->c_cc[ VERASE ] = '\x08';
    term_info->c_cc[ VKILL ] = '\x15';
    term_info->c_cc[ VEOF ] = '\x04';
    term_info->c_cc[ VSTART ] = '\x11';
    term_info->c_cc[ VSTOP ] = '\x13';
    term_info->c_cc[ VSUSP ] = '\x1a';
    term_info->c_cc[ VREPRINT ] = '\x12';
    term_info->c_cc[ VWERASE ] = '\x17';
    term_info->c_cc[ VLNEXT ] = '\x16';

    master->term_info = term_info;
    slave->term_info = term_info;

    /* Initialize window size */

    win_size = ( struct winsize* )kmalloc( sizeof( struct winsize ) );

    win_size->ws_row = 24;
    win_size->ws_col = 80;

    master->window_size = win_size;
    slave->window_size = win_size;

    /* Initialize other stuffs */

    master->atime = time( NULL );
    master->mtime = master->atime;
    master->ctime = master->atime;
    slave->atime = master->atime;
    slave->mtime = master->atime;
    slave->ctime = master->atime;

    /* Insert the nodes */

    mutex_lock( pty_lock, LOCK_IGNORE_SIGNAL );

    pty_insert_node( master );
    pty_insert_node( slave );

    mutex_unlock( pty_lock );

    *inode_number = master->inode_number;

    return 0;
}

static int pty_isatty( void* fs_cookie, void* node ) {
    if ( node == ( void* )&root_inode ) {
        return 0;
    }

    return 1;
}

static int pty_add_select_request( void* fs_cookie, void* _node, void* file_cookie, select_request_t* request ) {
    pty_node_t* node;

    node = ( pty_node_t* )_node;

    mutex_lock( node->lock, LOCK_IGNORE_SIGNAL );

    switch ( ( int )request->type ) {
        case SELECT_READ :
            request->next = node->read_requests;
            node->read_requests = request;

            if ( node->size > 0 ) {
                request->ready = true;
                semaphore_unlock( request->sync, 1 );
            }

            break;

        case SELECT_WRITE :
            request->next = node->write_requests;
            node->write_requests = request;

            if ( node->size < node->buffer_size ) {
                request->ready = true;
                semaphore_unlock( request->sync, 1 );
            }

            break;
    }

    mutex_unlock( node->lock );

    return 0;
}

static int pty_remove_select_request( void* fs_cookie, void* _node, void* file_cookie, select_request_t* request ) {
    pty_node_t* node;

    node = ( pty_node_t* )_node;

    mutex_lock( node->lock, LOCK_IGNORE_SIGNAL );

    switch ( ( int )request->type ) {
        case SELECT_READ : {
            select_request_t* tmp = node->read_requests;
            select_request_t* prev = NULL;

            while ( tmp != NULL ) {
                if ( tmp == request ) {
                    if ( prev == NULL ) {
                        node->read_requests = request->next;
                    } else {
                        prev->next = request->next;
                    }

                    break;
                }

                tmp = tmp->next;
            }

            break;
        }

        case SELECT_WRITE : {
            select_request_t* tmp = node->write_requests;
            select_request_t* prev = NULL;

            while ( tmp != NULL ) {
                if ( tmp == request ) {
                    if ( prev == NULL ) {
                        node->write_requests = request->next;
                    } else {
                        prev->next = request->next;
                    }

                    break;
                }

                tmp = tmp->next;
            }

            break;
        }
    }

    mutex_unlock( node->lock );

    return 0;
}

static filesystem_calls_t pty_calls = {
    .probe = NULL,
    .mount = pty_mount,
    .unmount = pty_unmount,
    .read_inode = pty_read_inode,
    .write_inode = pty_write_inode,
    .lookup_inode = pty_lookup_inode,
    .open = pty_open,
    .close = pty_close,
    .free_cookie = pty_free_cookie,
    .read = pty_read,
    .write = pty_write,
    .ioctl = pty_ioctl,
    .read_stat = pty_read_stat,
    .write_stat = NULL,
    .read_directory = pty_read_directory,
    .rewind_directory = pty_rewind_directory,
    .create = pty_create,
    .unlink = NULL,
    .mkdir = NULL,
    .rmdir = NULL,
    .isatty = pty_isatty,
    .symlink = NULL,
    .readlink = NULL,
    .set_flags = NULL,
    .add_select_request = pty_add_select_request,
    .remove_select_request = pty_remove_select_request
};

static void* pty_key( hashitem_t* item ) {
    pty_node_t* node;

    node = ( pty_node_t* )item;

    return ( void* )&node->inode_number;
}

static uint32_t pty_hash( const void* key ) {
    return hash_number( ( uint8_t* )key, sizeof( ino_t ) );
}

static bool pty_compare( const void* key1, const void* key2 ) {
    ino_t* inode_num_1;
    ino_t* inode_num_2;

    inode_num_1 = ( ino_t* )key1;
    inode_num_2 = ( ino_t* )key2;

    return ( *inode_num_1 == *inode_num_2 );
}

int init_pty_filesystem( void ) {
    int error;

    error = init_hashtable(
        &pty_node_table,
        32,
        pty_key,
        pty_hash,
        pty_compare
    );

    if ( error < 0 ) {
        goto error1;
    }

    pty_lock = mutex_create( "PTY mutex", MUTEX_NONE );

    if ( pty_lock < 0 ) {
        goto error2;
    }

    error = register_filesystem( "pty", &pty_calls );

    if ( error < 0 ) {
        goto error3;
    }

    return 0;

error3:
    mutex_destroy( pty_lock );

error2:
    destroy_hashtable( &pty_node_table );

error1:
    return error;
}
