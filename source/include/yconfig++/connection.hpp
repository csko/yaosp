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

#ifndef _YCONFIGPP_CONNECTION_HPP_
#define _YCONFIGPP_CONNECTION_HPP_

#include <vector>
#include <string>
#include <yutil++/ipcport.hpp>

#include <yconfig++/protocol.hpp>

namespace yconfigpp {

class Connection {
  public:
    Connection(void);
    ~Connection(void);

    bool init(void);

    bool getNumericValue(const std::string& path, const std::string& attr, uint64_t& value);
    bool getAsciiValue(const std::string& path, const std::string& attr, std::string& value);
    bool getBinaryValue(const std::string& path, const std::string& attr, uint8_t*& data, size_t& len);

    bool listChildren(const std::string& path, std::vector<std::string>& children);

  private:
    bool getAttributeValue(const std::string& path, const std::string& attr, msg_get_reply_t*& reply, size_t& size);

  private:
    yutilpp::IPCPort* m_serverPort;
    yutilpp::IPCPort* m_replyPort;
}; /* class Connection */

} /* namespace yconfigpp */

#endif /* _YCONFIGPP_CONNECTION_HPP_ */
