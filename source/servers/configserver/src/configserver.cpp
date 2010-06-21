/* Configuration server
 *
 * Copyright (c) 2010 Zoltan Kovacs
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
#include <yaosp/debug.h>

#include <configserver/configserver.hpp>
#include <configserver/loader.hpp>

int ConfigServer::run(int argc, char** argv) {
    if (argc != 2) {
        dbprintf("%s: invalid command line.\n", argv[0]);
        return EXIT_FAILURE;
    }

    Loader loader;
    if (!loader.loadFromFile(argv[1])) {
        dbprintf("%s: failed to load storage file: %s.", argv[0], argv[1]);
        return EXIT_FAILURE;
    }

    m_serverPort = new yutilpp::IPCPort();
    m_serverPort->createNew();
    m_serverPort->registerAsNamed("configserver");

    return EXIT_SUCCESS;
}
