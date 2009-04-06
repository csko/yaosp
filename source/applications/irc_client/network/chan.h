/* IRC client, Channel definition
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

#ifndef _NETWORK_CHAN_H_
#define _NETWORK_CHAN_H_
#include "irc.h"

#include <yutil/array.h>

typedef struct chan {
    char name[256]; /* with leading sign (e.g. '#') */
    array_t clients; /* client_t* */
    char topic[256];
    array_t bans; /* ban_t* */
    array_t invites; /* invite_t* */
    array_t exemptions; /* exemption_t* */
} chan_t;

chan_t* get_chan(const char* name);
int create_chan(const char* name);
int destroy_chan(const char* name);
int addclient_chan(const char* chan, const char* client, int mode);
int removeclient_chan(const char* chan, const char* client);

#endif /* _NETWORK_CHAN_H_ */
