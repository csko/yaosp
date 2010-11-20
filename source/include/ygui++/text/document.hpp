/* yaosp GUI library
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

#ifndef _YGUIPP_TEXT_DOCUMENT_HPP_
#define _YGUIPP_TEXT_DOCUMENT_HPP_

#include <string>

#include <ygui++/text/element.hpp>
#include <ygui++/event/documentlistener.hpp>

namespace yguipp {
namespace text {

class Document : public yguipp::event::DocumentSpeaker {
  public:
    virtual ~Document(void) {}

    virtual int getLength(void) = 0;
    virtual std::string getText(int offset, int length) = 0;

    virtual Element* getRootElement(void) = 0;

    virtual bool insert(int offset, const std::string& text) = 0;
    virtual bool remove(int offset, int length) = 0;
}; /* class Document */

} /* namespace text */
} /* namespace yguipp */

#endif /* _YGUIPP_TEXT_DOCUMENT_HPP_ */
