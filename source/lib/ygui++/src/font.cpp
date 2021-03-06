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

#include <string.h>

#include <ygui++/font.hpp>
#include <ygui++/application.hpp>
#include <ygui++/protocol.hpp>

namespace yguipp {

Font::Font(const std::string& name, const std::string& style, int pointSize) : m_handle(-1), m_name(name),
                                                                               m_style(style),
                                                                               m_pointSize(pointSize) {
}

Font::~Font(void) {
}

bool Font::init(void) {
    size_t size;
    uint8_t* tmp;
    uint8_t* data;
    uint32_t code;
    FontCreate* request;
    FontCreateReply reply;
    Application* app = Application::getInstance();

    size = sizeof(FontCreate) + m_name.size() + m_style.size() + 2;
    data = new uint8_t[size];
    request = reinterpret_cast<FontCreate*>(data);

    request->m_replyPort = app->getReplyPort()->getId();
    request->m_pointSize = m_pointSize;
    tmp = data + sizeof(FontCreate);
    memcpy(tmp, m_name.data(), m_name.size() + 1);
    tmp += m_name.size() + 1;
    memcpy(tmp, m_style.data(), m_style.size() + 1);

    app->getServerPort()->send(Y_FONT_CREATE, data, size);
    delete[] data;
    app->getReplyPort()->receive(code, reinterpret_cast<void*>(&reply), sizeof(FontCreateReply));

    if (reply.m_fontHandle < 0) {
        return false;
    }

    m_handle = reply.m_fontHandle;
    m_ascender = reply.m_ascender;
    m_descender = reply.m_descender;
    m_height = reply.m_height;

    return true;
}

int Font::getHandle(void) {
    return m_handle;
}

int Font::getAscender(void) {
    return m_ascender;
}

int Font::getDescender(void) {
    return m_descender;
}

int Font::getHeight(void) {
    return m_height;
}

int Font::getWidth(const std::string& text) {
    return getWidthFromServer(text);
}

int Font::getWidthFromServer(const std::string& text) {
    size_t size;
    uint8_t* data;
    uint32_t code;
    FontStringWidth* request;
    FontStringWidthReply reply;
    Application* app = Application::getInstance();

    size = sizeof(FontStringWidth) + text.size() + 1;
    data = new uint8_t[size];
    request = reinterpret_cast<FontStringWidth*>(data);

    request->m_replyPort = app->getReplyPort()->getId();
    request->m_fontHandle = m_handle;
    memcpy(reinterpret_cast<void*>(request + 1), text.data(), text.size());
    data[size - 1] = 0;

    app->getServerPort()->send(Y_FONT_STRING_WIDTH, data, size);
    app->getReplyPort()->receive(code, reinterpret_cast<void*>(&reply), sizeof(reply));

    return reply.m_width;
}

} /* namespace yguipp */
