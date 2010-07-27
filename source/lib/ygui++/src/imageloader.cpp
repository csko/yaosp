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

#include <dlfcn.h>

#include <ygui++/imageloader.hpp>
#include <yutil++/storage/directory.hpp>

namespace yguipp {

ImageLoaderManager* ImageLoaderManager::m_instance = NULL;

ImageLoaderManager::ImageLoaderManager( void ) {
}

ImageLoaderManager::~ImageLoaderManager( void ) {
}

void ImageLoaderManager::createInstance( void ) {
    m_instance = new ImageLoaderManager();
}

ImageLoaderManager* ImageLoaderManager::getInstance( void ) {
    return m_instance;
}

ImageLoader* ImageLoaderManager::getLoader( uint8_t* data, size_t size ) {
    for ( std::vector<ImageLoaderLibrary*>::const_iterator it = m_libraries.begin();
          it != m_libraries.end();
          ++it ) {
        ImageLoaderLibrary* library = *it;

        if ( library->identify(data, size) ) {
            return library->create();
        }
    }

    return NULL;
}

bool ImageLoaderManager::loadLibraries( void ) {
    yutilpp::storage::Directory* dir;
    std::string entry;

    dir = new yutilpp::storage::Directory("/system/lib/imageloader");
    dir->init();

    while ( dir->nextEntry(entry) ) {
        if ( ( entry == "." ) ||
             ( entry == ".." ) ) {
            continue;
        }

        std::string fullPath = "/system/lib/imageloader/" + entry;
        void* h = dlopen( fullPath.c_str(), RTLD_NOW );

        if ( h == NULL ) {
            continue;
        }

        loader_get_count_t* ldr_get_cnt = reinterpret_cast<loader_get_count_t*>(dlsym(h, "loader_get_count"));
        loader_get_at_t* ldr_get_at = reinterpret_cast<loader_get_at_t*>(dlsym(h, "loader_get_at"));

        if ( ( ldr_get_cnt == NULL ) ||
             ( ldr_get_at == NULL ) ) {
            dlclose(h);
            continue;
        }

        for ( int i = 0; i < ldr_get_cnt(); i++ ) {
            ImageLoaderLibrary* library = ldr_get_at(i);

            if ( library == NULL ) {
                continue;
            }

            m_libraries.push_back(library);
        }
    }

    delete dir;

    return true;
}

} /* namespace yguipp */
