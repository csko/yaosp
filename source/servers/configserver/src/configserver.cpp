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
#include <string.h>
#include <yaosp/debug.h>

#include <configserver/configserver.hpp>
#include <configserver/loader.hpp>

int ConfigServer::run(int argc, char** argv) {
    if (argc != 2) {
        dbprintf("%s: invalid command line.\n", argv[0]);
        return EXIT_FAILURE;
    }

    m_storageFile = new yutilpp::storage::File(argv[1]);
    if (!m_storageFile->init()) {
        dbprintf("%s: failed to open storage file: %s.", argv[0], argv[1]);
        return EXIT_FAILURE;
    }

    {
        Loader loader(m_storageFile);

        if (!loader.load()) {
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
            case MSG_GET_ATTRIBUTE_VALUE :
                handleGetAttributeValue(reinterpret_cast<msg_get_attr_t*>(m_recvBuffer));
                break;

            case MSG_NODE_LIST_CHILDREN :
                handleListChildren(reinterpret_cast<msg_list_children_t*>(m_recvBuffer));
                break;

            default :
                dbprintf("ConfigServer::run(): invalid message: %x.\n", code);
                break;
        }
    }

    return EXIT_SUCCESS;
}

int ConfigServer::handleGetAttributeValue(msg_get_attr_t* msg) {
    uint8_t* tmp = reinterpret_cast<uint8_t*>(msg + 1);
    std::string path = reinterpret_cast<char*>(tmp);
    tmp += path.size() + 1;
    std::string attrib = reinterpret_cast<char*>(tmp);

    Node* node = findNodeByPath(path);
    if (node == NULL) {
        msg_get_reply_t err;
        err.error = -ENOENT;
        yutilpp::IPCPort::sendTo(msg->reply_port, 0, reinterpret_cast<char*>(&err), sizeof(err));
        return 0;
    }

    Attribute* attribute = node->getAttribute(attrib);
    if (attribute == NULL) {
        msg_get_reply_t err;
        err.error = -ENOENT;
        yutilpp::IPCPort::sendTo(msg->reply_port, 0, reinterpret_cast<char*>(&err), sizeof(err));
        return 0;
    }

    uint8_t* data;
    msg_get_reply_t* reply;
    size_t attrSize = attribute->getSize();
    size_t size = sizeof(msg_get_reply_t) + attrSize;
    data = new uint8_t[size];
    reply = reinterpret_cast<msg_get_reply_t*>(data);

    reply->error = 0;
    reply->type = attribute->getType();
    attribute->getData(m_storageFile, reinterpret_cast<uint8_t*>(reply + 1), attrSize);

    yutilpp::IPCPort::sendTo(msg->reply_port, 0, data, size);
    delete[] data;

    return 0;
}

int ConfigServer::handleListChildren(msg_list_children_t* msg) {
    Node* node = findNodeByPath(reinterpret_cast<char*>(msg + 1));

    if (node == NULL) {
        msg_list_children_reply_t err;
        err.error = -ENOENT;
        yutilpp::IPCPort::sendTo(msg->reply_port, 0, reinterpret_cast<char*>(&err), sizeof(err));
        return 0;
    }

    std::vector<std::string> children;
    node->getChildrenNames(children);

    size_t size = sizeof(msg_list_children_reply_t);
    for (std::vector<std::string>::const_iterator it = children.begin();
         it != children.end();
         ++it) {
        size += (*it).size();
        size += 1;
    }

    uint8_t* data = new uint8_t[size];
    msg_list_children_reply_t* reply = reinterpret_cast<msg_list_children_reply_t*>(data);

    reply->error = 0;
    reply->count = children.size();

    uint8_t* tmp = data + sizeof(msg_list_children_reply_t);

    for (std::vector<std::string>::const_iterator it = children.begin();
         it != children.end();
         ++it) {
        const std::string& name = *it;
        memcpy(tmp, name.data(), name.size() + 1);
    }

    yutilpp::IPCPort::sendTo(msg->reply_port, 0, data, size);
    delete[] data;

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
