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

#include <yconfig++/protocol.hpp>

#include <configserver/loader.hpp>

Loader::Loader(yutilpp::storage::File* file) : m_file(file) {
    m_root = new Node();
}

Loader::~Loader(void) {
}

bool Loader::load(void) {
    readItemsAt(m_root, 0);
    return true;
}

Node* Loader::getRoot(void) {
    return m_root;
}

bool Loader::readItemsAt(Node* parent, off_t offset) {
    uint32_t itemCount;

    if (!m_file->seek(offset)) {
        return false;
    }

    if (m_file->read(&itemCount, 4) != 4) {
        return false;
    }

    for (uint32_t i = 0; i < itemCount; i++) {
        readItem(parent);
    }

    std::vector<Node*> children;
    parent->getChildren(children);

    for (std::vector<Node*>::const_iterator it = children.begin();
         it != children.end();
         ++it) {
        Node* child = *it;
        readItemsAt(child, child->getFileOffset());
    }

    return true;
}

bool Loader::readItem(Node* parent) {
    char* name;
    uint8_t type;
    int nameLength;

    if ((m_file->read(&type, 1) != 1) ||
        (m_file->read(&nameLength, 4) != 4)) {
        return false;
    }

    name = new char[nameLength + 1];

    if (m_file->read(name, nameLength) != nameLength) {
        return false;
    }

    name[nameLength] = 0;

    switch (type) {
        case TYPE_NODE : {
            Node* child;

            if (!readNode(child)) {
                return false;
            }

            parent->addChild(name, child);

            break;
        }

        case TYPE_ATTRIBUTE : {
            Attribute* attribute;

            if (!readAttribute(attribute)) {
                return false;
            }

            parent->addAttribute(name, attribute);

            break;
        }
    }

    return true;
}

bool Loader::readNode(Node*& node) {
    uint64_t fileOffset;

    if (m_file->read(&fileOffset, 8) != 8) {
        return false;
    }

    node = new Node(fileOffset);

    return true;
}

bool Loader::readAttribute(Attribute*& attribute) {
    uint8_t type;

    if (m_file->read(&type, 1) != 1) {
        return false;
    }

    switch (type) {
        case ATTR_NUMERIC : return readNumericAttribute(attribute);
        case ATTR_ASCII : return readAsciiAttribute(attribute);
        case ATTR_BOOL : return readBoolAttribute(attribute);
        case ATTR_BINARY : return readBinaryAttribute(attribute);
    }

    return false;
}

bool Loader::readNumericAttribute(Attribute*& attribute) {
    uint64_t value;

    if (m_file->read(&value, 8) != 8) {
        return false;
    }

    attribute = new NumericAttribute(value);

    return true;
}

bool Loader::readAsciiAttribute(Attribute*& attribute) {
    uint32_t length;
    off_t offset;

    if ((m_file->read(&length, 4) != 4) ||
        (m_file->read(&offset, 8) != 8)) {
        return false;
    }

    attribute = new AsciiAttribute(offset, length);

    return true;
}

bool Loader::readBoolAttribute(Attribute*& attribute) {
    bool value;

    if (m_file->read(&value, 1) != 1) {
        return false;
    }

    attribute = new BoolAttribute(value);

    return true;
}

bool Loader::readBinaryAttribute(Attribute*& attribute) {
    uint32_t length;
    off_t offset;

    if ((m_file->read(&length, 4) != 4) ||
        (m_file->read(&offset, 8) != 8)) {
        return false;
    }

    attribute = new BinaryAttribute(offset, length);

    return true;
}
