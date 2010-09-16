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

#include <memory>

#include <ygui++/bitmap.hpp>
#include <ygui++/application.hpp>
#include <ygui++/imageloader.hpp>
#include <ygui++/protocol.hpp>
#include <yutil++/storage/file.hpp>

namespace yguipp {

Bitmap::Bitmap( const Point& size, ColorSpace colorSpace ) : m_handle(-1), m_data(NULL), m_region(-1),
                                                             m_size(size), m_colorSpace(colorSpace) {
}

Bitmap::~Bitmap( void ) {
}

bool Bitmap::init( void ) {
    uint32_t code;
    BitmapCreate request;
    BitmapCreateReply reply;
    Application* app = Application::getInstance();

    request.m_replyPort = app->getReplyPort()->getId();
    request.m_size = m_size;
    request.m_colorSpace = m_colorSpace;

    app->getServerPort()->send(Y_BITMAP_CREATE, reinterpret_cast<void*>(&request), sizeof(request));
    app->getReplyPort()->receive(code, reinterpret_cast<void*>(&reply), sizeof(reply));

    if (reply.m_bitmapHandle < 0) {
        return false;
    }

    m_handle = reply.m_bitmapHandle;
    m_region = memory_region_clone_pages(reply.m_bitmapRegion, reinterpret_cast<void**>(&m_data));

    if (m_region < 0) {
        /* todo: delete from guiserver */
        return false;
    }

    return true;
}

Rect Bitmap::bounds( void ) {
    return Rect(m_size);
}

int Bitmap::width(void) {
    return m_size.m_x;
}

int Bitmap::height(void) {
    return m_size.m_y;
}

int Bitmap::getHandle( void ) {
    return m_handle;
}

uint8_t* Bitmap::getData( void ) {
    return m_data;
}

const Point& Bitmap::getSize( void ) {
    return m_size;
}

Bitmap* Bitmap::loadFromFile(const std::string& path) {
    int size;
    bool final;
    int available;
    Bitmap* bitmap = NULL;
    uint8_t* bitmapData = NULL;
    uint8_t buffer[LOAD_BUFFER_SIZE];

    std::auto_ptr<ImageLoader> loader;
    std::auto_ptr<yutilpp::storage::File> file(new yutilpp::storage::File(path));

    if (!file->init()) {
        return NULL;
    }

    size = file->read(buffer, sizeof(buffer));
    final = (size != sizeof(buffer));

    if (size <= 0) {
        return NULL;
    }

    loader.reset(ImageLoaderManager::getInstance()->getLoader(buffer, size));

    if (loader.get() == NULL) {
        return NULL;
    }

    loader->addData(buffer, size, final);

    if (loader->availableData() >= sizeof(ImageInfo)) {
        ImageInfo info;

        loader->readData(reinterpret_cast<uint8_t*>(&info), sizeof(ImageInfo));
        bitmap = new Bitmap( Point(info.m_width, info.m_height) );
        bitmap->init(); /* todo */
        bitmapData = bitmap->getData();
    }

    while (!final) {
        size = file->read(buffer, sizeof(buffer));
        final = (size != sizeof(buffer));

        loader->addData(buffer, size, final);

        if (bitmap == NULL) {
            if ( loader->availableData() >= sizeof(ImageInfo) ) {
                ImageInfo info;

                loader->readData(reinterpret_cast<uint8_t*>(&info), sizeof(ImageInfo));
                bitmap = new Bitmap( Point(info.m_width, info.m_height) );
                bitmap->init(); /* todo */
                bitmapData = bitmap->getData();
            }
        } else {
            available = loader->availableData();

            if (available > 0) {
                loader->readData(bitmapData, available);
                bitmapData += available;
            }
        }
    }

    available = loader->availableData();

    if (available > 0) {
        loader->readData(bitmapData, available);
    }

    return bitmap;
}

Bitmap* Bitmap::loadFromBuffer(uint8_t* data, size_t length) {
    ImageInfo info;
    Bitmap* bitmap;
    uint8_t* bitmapData;
    std::auto_ptr<ImageLoader> loader(ImageLoaderManager::getInstance()->getLoader(data, length));

    if (loader.get() == NULL) {
        return NULL;
    }

    loader->addData(data, length, true);

    if (loader->availableData() < sizeof(ImageInfo)) {
        return NULL;
    }

    loader->readData(reinterpret_cast<uint8_t*>(&info), sizeof(ImageInfo));

    bitmap = new Bitmap(Point(info.m_width, info.m_height));
    bitmap->init();
    bitmapData = bitmap->getData();

    loader->readData(bitmapData, loader->availableData());

    return bitmap;
}

} /* namespace yguipp */
