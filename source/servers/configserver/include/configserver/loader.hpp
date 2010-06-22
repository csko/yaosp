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

#ifndef _CONFIGSERVER_LOADER_HPP_
#define _CONFIGSERVER_LOADER_HPP_

#include <string>
#include <yutil++/storage/file.hpp>

#include <configserver/node.hpp>

class Loader {
  public:
    Loader(void);
    ~Loader(void);

    bool loadFromFile(const std::string& fileName);
    Node* getRoot(void);

  private:
    enum {
        TYPE_NODE = 1,
        TYPE_ATTRIBUTE = 2
    };

    bool readItemsAt(Node* parent, off_t offset);
    bool readItem(Node* parent);
    bool readNode(Node*& node);
    bool readAttribute(Attribute*& attribute);
    bool readNumericAttribute(Attribute*& attribute);
    bool readAsciiAttribute(Attribute*& attribute);
    bool readBoolAttribute(Attribute*& attribute);
    bool readBinaryAttribute(Attribute*& attribute);

  private:
    Node* m_root;
    yutilpp::storage::File* m_file;
}; /* class Loader */

#endif /* _CONFIGSERVER_LOADER_HPP_ */
