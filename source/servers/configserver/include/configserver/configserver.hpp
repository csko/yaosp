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

#ifndef _CONFIGSERVER_CONFIGSERVER_HPP_
#define _CONFIGSERVER_CONFIGSERVER_HPP_

#include <yutil++/ipcport.hpp>
#include <yutil++/storage/file.hpp>
#include <yconfig++/protocol.hpp>

#include <configserver/node.hpp>

class ConfigServer {
  public:
    int run(int argc, char** argv);

  private:
    int handleGetAttributeValue(msg_get_attr_t* msg);
    int handleListChildren(msg_list_children_t* msg);

    Node* findNodeByPath(const std::string& path);

  private:
    Node* m_root;
    uint8_t m_recvBuffer[8192];
    yutilpp::IPCPort* m_serverPort;
    yutilpp::storage::File* m_storageFile;
}; /* class ConfigServer */

#endif /* _CONFIGSERVER_CONFIGSERVER_HPP_ */
