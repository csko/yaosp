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

#include <configserver/node.hpp>

Node::Node(uint64_t fileOffset) : m_fileOffset(fileOffset) {
}

void Node::addChild(const std::string& name, Node* child) {
    m_children[name] = child;
}

void Node::addAttribute(const std::string& name, Attribute* attribute) {
    m_attributes[name] = attribute;
}

Node* Node::getChild(const std::string& name) {
    MapCIter it = m_children.find(name);

    if (it == m_children.end()) {
        return NULL;
    }

    return it->second;
}

Attribute* Node::getAttribute(const std::string& name) {
    Attribute::MapCIter it = m_attributes.find(name);

    if (it == m_attributes.end()) {
        return NULL;
    }

    return it->second;
}

void Node::getChildren(std::vector<Node*>& children) {
    for (MapCIter it = m_children.begin();
         it != m_children.end();
         ++it) {
        children.push_back(it->second);
    }
}

void Node::getChildrenNames(std::vector<std::string>& childrenNames) {
    for (MapCIter it = m_children.begin();
         it != m_children.end();
         ++it) {
        childrenNames.push_back(it->first);
    }
}

uint64_t Node::getFileOffset(void) {
    return m_fileOffset;
}
