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

import handler
import definitions

class ArrayHandler( handler.NodeHandler ) :
    handled_node = "array"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.data = ""
        self.array = None

    def node_started( self, attrs ) :
        if not "name" in attrs :
            return

        self.array = definitions.Array( attrs[ "name" ] )

    def node_finished( self ) :
        if self.array != None :
            self.get_context().add_definition( self.array )

    def start_element( self, name, attrs ) :
        self.data = ""

    def end_element( self, name ) :
        if name == "item" :
            self.array.add_item( self.data )

    def element_data( self, data ) :
        self.data = data

handlers = [
    ArrayHandler
]
