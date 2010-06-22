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
#include <errno.h>
#include <yaosp/debug.h>

#include <configserver/configserver.hpp>
#include <configserver/loader.hpp>

int ConfigServer::run(int argc, char** argv) {
    if (argc != 2) {
        dbprintf("%s: invalid command line.\n", argv[0]);
        return EXIT_FAILURE;
    }

    {
        Loader loader;
        if (!loader.loadFromFile(argv[1])) {
            dbprintf("%s: failed to load storage file: %s.", argv[0], argv[1]);
            return EXIT_FAILURE;
        }
        m_root = loader.getRoot();
    }

    m_serverPort = new yutilpp::IPCPort();
    m_serverPort->createNew();
    m_serverPort->registerAsNamed("configserver");

    while (1) {
        int ret;
        uint32_t code;

        ret = m_serverPort->receive(code, m_recvBuffer, sizeof(m_recvBuffer), 1000000);

        if (ret == -ETIME) {
            continue;
        } else if (ret < 0) {
            dbprintf("%s: failed to receive message: %d.\n", argv[0], ret);
            break;
        }

        switch (code) {
            case MSG_NODE_LIST_CHILDREN :
                handleListChildren(reinterpret_cast<msg_list_children_t*>(m_recvBuffer));
                break;
        }
    }

    return EXIT_SUCCESS;
}

int ConfigServer::handleListChildren(msg_list_children_t* msg) {
    Node* node = findNodeByPath(reinterpret_cast<char*>(msg + 1));
    dbprintf("node=%p\n", node);
    return 0;
}

Node* ConfigServer::findNodeByPath(const std::string& path) {
    Node* current = m_root;
    std::string::size_type pos;
    std::string::size_type start = 0;

    do {
        pos = path.find('/', start);
        std::string name = path.substr(start, pos - start);

        current = current->getChild(name);

        if (current == NULL) {
            return NULL;
        }

        start = pos + 1;
    } while (pos != std::string::npos);

    return current;
}
