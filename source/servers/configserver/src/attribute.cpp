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

#include <configserver/attribute.hpp>

Attribute::Attribute(uint8_t type) : m_type(type) {
}

uint8_t Attribute::getType(void) {
    return m_type;
}

NumericAttribute::NumericAttribute(uint64_t value) : Attribute(ATTR_NUMERIC), m_value(value) {
}

uint32_t NumericAttribute::getSize(void) {
    return sizeof(uint64_t);
}

bool NumericAttribute::getData(yutilpp::storage::File* storageFile, uint8_t* data, size_t size) {
    if (size < getSize()) {
        return false;
    }

    *reinterpret_cast<uint64_t*>(data) = m_value;

    return true;
}

AsciiAttribute::AsciiAttribute(off_t offset, uint32_t size) : Attribute(ATTR_ASCII),
                                                              m_offset(offset), m_size(size) {
}

uint32_t AsciiAttribute::getSize(void) {
    return m_size;
}

bool AsciiAttribute::getData(yutilpp::storage::File* storageFile, uint8_t* data, size_t size) {
    if ((!storageFile->seek(m_offset)) ||
        (!storageFile->read(data, size))) {
        return false;
    }

    return true;
}

BoolAttribute::BoolAttribute(bool value) : Attribute(ATTR_BOOL),
                                           m_value(value) {
}

uint32_t BoolAttribute::getSize(void) {
    return sizeof(bool);
}

bool BoolAttribute::getData(yutilpp::storage::File* storageFile, uint8_t* data, size_t size) {
    if (size < getSize()) {
        return false;
    }

    *reinterpret_cast<bool*>(data) = m_value;

    return true;
}

BinaryAttribute::BinaryAttribute(off_t offset, uint32_t size) : Attribute(ATTR_BINARY),
                                                                m_offset(offset), m_size(size) {
}

uint32_t BinaryAttribute::getSize(void) {
    return m_size;
}

bool BinaryAttribute::getData(yutilpp::storage::File* storageFile, uint8_t* data, size_t size) {
    // todo
    return false;
}
