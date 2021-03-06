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

#include <ygui++/event/adjustmentlistener.hpp>

namespace yguipp {
namespace event {

void AdjustmentSpeaker::addAdjustmentListener(AdjustmentListener* listener) {
    for (std::vector<AdjustmentListener*>::const_iterator it = m_listeners.begin();
         it != m_listeners.end();
         ++it) {
        if (*it == listener) {
            return;
        }
    }

    m_listeners.push_back(listener);
}

void AdjustmentSpeaker::removeAdjustmentListener(AdjustmentListener* listener) {
    for (std::vector<AdjustmentListener*>::iterator it = m_listeners.begin();
         it != m_listeners.end();
         ++it) {
        if (*it == listener) {
            m_listeners.erase(it);
            return;
        }
    }
}

void AdjustmentSpeaker::fireAdjustmentListeners(Widget* widget) {
    std::vector<AdjustmentListener*> tmpList = m_listeners;

    for (std::vector<AdjustmentListener*>::const_iterator it = tmpList.begin();
         it != tmpList.end();
         ++it) {
        (*it)->onAdjustmentValueChanged(widget);
    }
}

} /* namespace event */
} /* namespace yguipp */
