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

#include <configserver/attribute.hpp>

Attribute::Attribute(uint8_t type) : m_type(type) {
}

NumericAttribute::NumericAttribute(uint64_t value) : Attribute(Attribute::ATTR_NUMERIC), m_value(value) {
}

AsciiAttribute::AsciiAttribute(off_t offset, uint32_t size) : Attribute(Attribute::ATTR_ASCII),
                                                              m_offset(offset), m_size(size) {
}

BoolAttribute::BoolAttribute(bool value) : Attribute(Attribute::ATTR_BOOL),
                                           m_value(value) {
}

BinaryAttribute::BinaryAttribute(off_t offset, uint32_t size) : Attribute(Attribute::ATTR_BINARY),
                                                                m_offset(offset), m_size(size) {
}
