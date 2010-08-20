/* GUI server
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
#include <yaosp/debug.h>
#include <yutil++/storage/directory.hpp>

#include <guiserver/graphicsdriverloader.hpp>
#include "../driver/video/vesa/vesa.hpp"

GraphicsDriver* GraphicsDriverLoader::detectDriver(void) {
    GraphicsDriver* driver = NULL;

    std::string entry;
    yutilpp::storage::Directory* dir;

    dir = new yutilpp::storage::Directory("/system/lib/graphicsdriver");
    dir->init();

    while (dir->nextEntry(entry)) {
        void* p;

        if ((entry == ".") ||
            (entry == "..")) {
            continue;
        }

        p = dlopen(("/system/lib/graphicsdriver/" + entry).c_str(), RTLD_NOW);

        if (p == NULL) {
            continue;
        }

        GetGfxDriverFunc* getDriver = reinterpret_cast<GetGfxDriverFunc*>(dlsym(p, "get_graphics_driver"));

        if (getDriver == NULL) {
            dlclose(p);
            continue;
        }

        driver = getDriver();

        if (driver->detect()) {
            break;
        }

        delete driver;
        driver = NULL;
        dlclose(p);
    }

    delete dir;

    if (driver == NULL) {
        driver = new VesaDriver();
    }

    if (!driver->detect()) {
        return NULL;
    }

    return driver;
}
