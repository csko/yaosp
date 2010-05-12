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

#include <ygui/protocol.h>

#include <ygui++/font.hpp>
#include <ygui++/application.hpp>

#include <yaosp/debug.h>

namespace yguipp {

Font::Font( const std::string& name, const std::string& style, int size ) : m_handle(-1), m_name(name),
                                                                            m_style(style), m_size(size) {
}

Font::~Font( void ) {
}

bool Font::init( void ) {
    size_t size;
    uint8_t* tmp;
    uint8_t* data;
    uint32_t code;
    Application* app;
    msg_create_font_t* request;
    msg_create_font_reply_t reply;

    app = Application::getInstance();

    size = sizeof(msg_create_font_t) + m_name.size() + m_style.size() + 2;
    data = new uint8_t[size];
    request = reinterpret_cast<msg_create_font_t*>(data);

    request->reply_port = app->getReplyPort()->getId();
    request->properties.point_size = m_size;
    request->properties.flags = FONT_SMOOTHED;
    tmp = data + sizeof(msg_create_font_t);
    memcpy( tmp, m_name.data(), m_name.size() + 1 );
    tmp += m_name.size() + 1;
    memcpy( tmp, m_style.data(), m_style.size() + 1 );

    app->lock();

    app->getServerPort()->send( MSG_FONT_CREATE, data, size );
    delete[] data;
    app->getReplyPort()->receive( code, reinterpret_cast<void*>(&reply), sizeof(msg_create_font_reply_t) );

    app->unLock();

    if ( reply.handle < 0 ) {
        return false;
    }

    m_handle = reply.handle;
    m_ascender = reply.ascender;
    m_descender = reply.descender;
    m_lineGap = reply.line_gap;

    return true;
}

int Font::getHandle( void ) {
    return m_handle;
}

int Font::getHeight( void ) {
    return ( m_ascender - m_descender + m_lineGap );
}

int Font::getAscender( void ) {
    return m_ascender;
}

int Font::getDescender( void ) {
    return m_descender;
}

int Font::getLineGap( void ) {
    return m_lineGap;
}

} /* namespace yguipp */
