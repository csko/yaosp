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

#include <guiserver/decoratorloader.hpp>

Decorator* DecoratorLoader::loadDecorator(const std::string& name) {
    void* p;
    CreateFunction* create;

    p = dlopen(("/system/lib/decorator/" + name + ".so").c_str(), RTLD_NOW);

    if (p == NULL) {
        return NULL;
    }

    create = reinterpret_cast<CreateFunction*>(dlsym(p, "createDecorator"));

    if (create == NULL) {
        dlclose(p);
        return NULL;
    }

    return create();
}
