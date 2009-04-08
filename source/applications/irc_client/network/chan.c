/* IRC client, Channels
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
#include <stdlib.h>
#include "irc.h"
#include "chan.h"

chan_t* get_chan(const char* name){
    int i;
    int count;
    chan_t* chan;

    count = array_get_size(&chan_list);

    for ( i = 0; i < count; i++ ) {
        chan = ( chan_t* )array_get_item( &chan_list, i );

        if ( strcmp(chan->name , name ) == 0 ) {
            return chan;
        }
    }

    return NULL;

}

int create_chan(const char* name){
    if(get_chan(name) == NULL){

        chan_t* chan = (chan_t*) malloc(sizeof(chan_t));
        if(chan == NULL){
            return 2;
        }

        strncpy(chan->name, name, 256);
        chan->topic[0] = '\0';
        init_array(&chan->nicks);
        init_array(&chan->bans);
        init_array(&chan->invites);
        init_array(&chan->exemptions);

        array_add_item(&chan_list, (void*) chan);
        return 0;
    }else{
        return 1;
    }
}

int destroy_chan(const char* name){
    chan_t* chan;

    chan = get_chan( name );

    if(chan == NULL){
        return 1;
    }else{
        array_remove_item(&chan_list, (void*) chan);
        free(chan);
        return 0;
    }
}

int addnick_chan(const char* chan, const char* nick, int mode){
    chan_t* channel;

    channel  = get_chan(chan);

    if(nick_in_channel(channel, nick)){
        return 1;
    }

    // TODO: add item
    return 0;
}

int removenick_chan(const char* chan, const char* nick){
    chan_t* channel;

    channel = get_chan(chan);

    if(!nick_in_channel(channel, nick)){
        return 1;
    }

    // TODO: remove item
    return 0;
}

int nick_in_channel(chan_t* chan, const char* nick){
    return 0;
}
