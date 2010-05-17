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

#include <ygui++/bitmap.hpp>
#include <ygui++/application.hpp>
#include <ygui++/imageloader.hpp>
#include <yutil++/storage/file.hpp>

namespace yguipp {

Bitmap::Bitmap( const Point& size, color_space_t colorSpace ) : m_id(-1), m_data(NULL), m_region(-1),
                                                                m_size(size), m_colorSpace(colorSpace) {
}

Bitmap::~Bitmap( void ) {
}

bool Bitmap::init( void ) {
    uint32_t code;
    Application* app;
    msg_create_bitmap_t request;
    msg_create_bmp_reply_t reply;

    app = Application::getInstance();

    request.reply_port = app->getReplyPort()->getId();
    request.width = m_size.m_x;
    request.height = m_size.m_y;
    request.color_space = m_colorSpace;

    app->lock();

    app->getServerPort()->send( MSG_BITMAP_CREATE, reinterpret_cast<void*>(&request), sizeof(msg_create_bitmap_t) );
    app->getReplyPort()->receive( code, reinterpret_cast<void*>(&reply), sizeof(msg_create_bmp_reply_t) );

    app->unLock();

    if ( reply.id < 0 ) {
        return false;
    }

    m_id = reply.id;
    m_region = memory_region_clone_pages( reply.bitmap_region, reinterpret_cast<void**>(&m_data) );

    if ( m_region < 0 ) {
        /* todo: delete bitmap from guiserver */
        return false;
    }

    return true;
}

int Bitmap::getId( void ) {
    return m_id;
}

uint8_t* Bitmap::getData( void ) {
    return m_data;
}

const Point& Bitmap::getSize( void ) {
    return m_size;
}

Bitmap* Bitmap::loadFromFile( const std::string& path ) {
    int size;
    bool final;
    int available;
    Bitmap* bitmap = NULL;
    uint8_t* bitmapData = NULL;
    ImageLoader* loader;
    yutilpp::storage::File* file;
    uint8_t buffer[LOAD_BUFFER_SIZE];

    file = new yutilpp::storage::File(path);

    if ( !file->init() ) {
        delete file;
        return NULL;
    }

    size = file->read(buffer,LOAD_BUFFER_SIZE);
    final = ( size != LOAD_BUFFER_SIZE );

    if ( size <= 0 ) {
        delete file;
        return NULL;
    }

    loader = ImageLoaderManager::getInstance()->getLoader(buffer,size);

    if ( loader == NULL ) {
        delete file;
        return NULL;
    }

    loader->addData(buffer, size, final);

    if ( loader->availableData() >= sizeof(ImageInfo) ) {
        ImageInfo info;

        loader->readData(reinterpret_cast<uint8_t*>(&info), sizeof(ImageInfo));
        bitmap = new Bitmap( Point(info.m_width, info.m_height) );
        bitmap->init(); /* todo */
        bitmapData = bitmap->getData();
    }

    while ( !final ) {
        size = file->read(buffer,LOAD_BUFFER_SIZE);
        final = ( size != LOAD_BUFFER_SIZE );

        loader->addData(buffer, size, final);

        if ( bitmap == NULL ) {
            if ( loader->availableData() >= sizeof(ImageInfo) ) {
                ImageInfo info;

                loader->readData(reinterpret_cast<uint8_t*>(&info), sizeof(ImageInfo));
                bitmap = new Bitmap( Point(info.m_width, info.m_height) );
                bitmap->init(); /* todo */
                bitmapData = bitmap->getData();
            }
        } else {
            available = loader->availableData();

            if ( available > 0 ) {
                loader->readData(bitmapData, available);
                bitmapData += available;
            }
        }
    }

    delete file;

    available = loader->availableData();

    if ( available > 0 ) {
        loader->readData(bitmapData, available);
    }

    delete loader;

    return bitmap;
}

} /* namespace yguipp */
