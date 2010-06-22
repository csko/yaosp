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

#ifndef _CONFIGSERVER_NODE_HPP_
#define _CONFIGSERVER_NODE_HPP_

#include <map>
#include <string>
#include <vector>

#include <configserver/attribute.hpp>

class Node {
  public:
    Node(uint64_t fileOffset = 0);

    void addChild(const std::string& name, Node* child);
    void addAttribute(const std::string& name, Attribute* attribute);

    Node* getChild(const std::string& name);
    Attribute* getAttribute(const std::string& name);

    void getChildren(std::vector<Node*>& children);
    void getChildrenNames(std::vector<std::string>& childrenNames);
    uint64_t getFileOffset(void);

    typedef std::map<std::string, Node*> Map;
    typedef Map::const_iterator MapCIter;

  private:
    Map m_children;
    Attribute::Map m_attributes;

    uint64_t m_fileOffset;
}; /* class Node */

#endif /* _CONFIGSERVER_NODE_HPP_ */
