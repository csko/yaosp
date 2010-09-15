/* yaosp configuration library
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

#include <string.h>
#include <yutil++/thread.hpp>

#include <yconfig++/connection.hpp>
#include <yconfig++/protocol.hpp>

namespace yconfigpp {

Connection::Connection(void) {
    m_serverPort = new yutilpp::IPCPort();
    m_replyPort = new yutilpp::IPCPort();
}

Connection::~Connection(void) {
    delete m_serverPort;
    delete m_replyPort;
}

bool Connection::init(void) {
    while (1) {
        if (m_serverPort->createFromNamed("configserver")) {
            break;
        }

        yutilpp::Thread::uSleep(100 * 1000);
    }

    if (!m_replyPort->createNew()) {
        return false;
    }

    return true;
}

bool Connection::getNumericValue(const std::string& path, const std::string& attr, uint64_t& value) {
    size_t size;
    msg_get_reply_t* reply;

    if (!getAttributeValue(path, attr, reply, size)) {
        return false;
    }

    if ((reply->error != 0) ||
        (reply->type != ATTR_NUMERIC)) {
        delete reply;
        return false;
    }

    value = *reinterpret_cast<uint64_t*>(reply + 1);

    delete reply;
    return true;
}

bool Connection::getAsciiValue(const std::string& path, const std::string& attr, std::string& value) {
    size_t size;
    msg_get_reply_t* reply;

    if (!getAttributeValue(path, attr, reply, size)) {
        return false;
    }

    if ((reply->error != 0) ||
        (reply->type != ATTR_ASCII)) {
        delete reply;
        return false;
    }

    value.append(reinterpret_cast<char*>(reply + 1), size - sizeof(msg_get_reply_t));

    delete reply;
    return true;
}

bool Connection::getBinaryValue(const std::string& path, const std::string& attr, uint8_t*& data, size_t& len) {
    size_t size;
    msg_get_reply_t* reply;

    data = NULL;
    len = 0;

    if (!getAttributeValue(path, attr, reply, size)) {
        return false;
    }

    if ((reply->error != 0) ||
        (reply->type != ATTR_BINARY)) {
        delete reply;
        return false;
    }

    len = size - sizeof(msg_get_reply_t);
    data = new uint8_t[len];

    memcpy(data, reinterpret_cast<void*>(reply + 1), len);

    return true;
}

bool Connection::listChildren(const std::string& path, std::vector<std::string>& children) {
    size_t size;
    uint8_t* data;
    uint32_t code;
    msg_list_children_t* request;
    msg_list_children_reply_t* reply;

    size = sizeof(msg_list_children_t) + path.size() + 1;
    data = new uint8_t[size];
    request = reinterpret_cast<msg_list_children_t*>(data);

    request->reply_port = m_replyPort->getId();
    memcpy( reinterpret_cast<void*>(request + 1), path.data(), path.size() + 1);

    m_serverPort->send(MSG_NODE_LIST_CHILDREN, data, size);
    delete[] data;
    m_replyPort->peek(code, size);
    data = new uint8_t[size];
    m_replyPort->receive(code, data, size);

    reply = reinterpret_cast<msg_list_children_reply_t*>(data);

    if (reply->error != 0) {
        delete[] data;
        return false;
    }

    uint8_t* tmp = data + sizeof(msg_list_children_reply_t);

    for (uint32_t i = 0; i < reply->count; i++) {
        std::string name = reinterpret_cast<char*>(tmp);
        children.push_back(name);
        tmp += name.size();
        tmp += 1;
    }

    delete[] data;

    return true;
}

bool Connection::getAttributeValue(const std::string& path, const std::string& attr,
                                   msg_get_reply_t*& reply, size_t& size) {
    uint8_t* tmp;
    uint8_t* data;
    uint32_t code;
    msg_get_attr_t* request;

    size = sizeof(msg_get_attr_t) + path.size() + 1 + attr.size() + 1;
    data = new uint8_t[size];
    request = reinterpret_cast<msg_get_attr_t*>(data);

    request->reply_port = m_replyPort->getId();
    tmp = data + sizeof(msg_get_attr_t);
    memcpy(tmp, path.data(), path.size() + 1);
    tmp += path.size() + 1;
    memcpy(tmp, attr.data(), attr.size() + 1);

    m_serverPort->send(MSG_GET_ATTRIBUTE_VALUE, data, size);
    delete[] data;
    m_replyPort->peek(code, size);
    data = new uint8_t[size];
    m_replyPort->receive(code, data, size);

    reply = reinterpret_cast<msg_get_reply_t*>(data);

    return true;
}

} /* namespace yconfigpp */
