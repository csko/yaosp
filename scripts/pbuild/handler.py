# Python build system
#
# Copyright (c) 2008 Zoltan Kovacs
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of version 2 of the GNU General Public License
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

class NodeHandler :
    def __init__( self, parent, context ) :
        self.parent = parent
        self.context = context

    def get_parent( self ) :
        return self.parent

    def get_context( self ) :
        return self.context

    def add_work( self, work ) :
        pass

    def node_started( self, attrs ) :
        pass

    def node_finished( self ) :
        pass

    def start_element( self, name, attrs ) :
        pass

    def end_element( self, name ) :
        pass

    def element_data( self, data ) :
        pass
