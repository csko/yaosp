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

#ifndef _CONFIGSERVER_ATTRIBUTE_HPP_
#define _CONFIGSERVER_ATTRIBUTE_HPP_

#include <map>
#include <string>

#include <yutil++/storage/file.hpp>

class Attribute {
  public:
    Attribute(uint8_t type);
    virtual ~Attribute(void) {}

    uint8_t getType(void);

    virtual uint32_t getSize(void) = 0;
    virtual bool getData(yutilpp::storage::File* storageFile, uint8_t* data, size_t size) = 0;

    typedef std::map<std::string, Attribute*> Map;
    typedef Map::const_iterator MapCIter;

  private:
    uint8_t m_type;
}; /* class Attribute */

class NumericAttribute : public Attribute {
  public:
    NumericAttribute(uint64_t value);

    uint32_t getSize(void);
    bool getData(yutilpp::storage::File* storageFile, uint8_t* data, size_t size);

  private:
    uint64_t m_value;
}; /* class NumericAttribute */

class AsciiAttribute : public Attribute {
  public:
    AsciiAttribute(off_t offset, uint32_t size);

    uint32_t getSize(void);
    bool getData(yutilpp::storage::File* storageFile, uint8_t* data, size_t size);

  private:
    off_t m_offset;
    uint32_t m_size;
}; /* class AsciiAttribute */

class BoolAttribute : public Attribute {
  public:
    BoolAttribute(bool value);

    uint32_t getSize(void);
    bool getData(yutilpp::storage::File* storageFile, uint8_t* data, size_t size);

  private:
    bool m_value;
}; /* class BoolAttribute */

class BinaryAttribute : public Attribute {
  public:
    BinaryAttribute(off_t offset, uint32_t size);

    uint32_t getSize(void);
    bool getData(yutilpp::storage::File* storageFile, uint8_t* data, size_t size);

  private:
    off_t m_offset;
    uint32_t m_size;
}; /* class BinaryAttribute */

#endif /* _CONFIGSERVER_ATTRIBUTE_HPP_ */
