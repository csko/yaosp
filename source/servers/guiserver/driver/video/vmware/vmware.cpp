/* VMWare graphics driver
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

#include "vmware.hpp"

VMWareDriver::VMWareDriver(void) {
}

VMWareDriver::~VMWareDriver(void) {
}

bool VMWareDriver::detect(void) {
}

std::string VMWareDriver::getName(void) {
}

size_t VMWareDriver::getModeCount(void) {
}

ScreenMode* VMWareDriver::getModeInfo(size_t index) {
}

void* VMWareDriver::getFrameBuffer(void) {
}

bool VMWareDriver::setMode(ScreenMode* info) {
}

