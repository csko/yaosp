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

import xml.sax

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

class BuildHandler( xml.sax.handler.ContentHandler ) :
    node_handlers = []

    def __init__( self, context ) :
        self.context = context
        self.handler_stack = []

    def add_node_handlers( self, handlers ) :
        self.node_handlers += handlers

    def startElement( self, name, attrs ) :
        handler = None
        real_attrs = self.__convert_attributes( attrs )

        if name == "build" :
            if "default" in real_attrs :
                self.context.set_default_target( real_attrs[ "default" ] )

            return

        for c in self.node_handlers :
            if "handled_node" in dir(c) and c.handled_node == name :
                if len( self.handler_stack ) > 0 :
                    parent = self.handler_stack[ -1 ]
                else :
                    parent = None

                handler = c( parent, self.context )

                break

        if handler == None :
            if len( self.handler_stack ) > 0 :
                self.handler_stack[ -1 ].start_element( name, real_attrs )

            return

        handler.node_started( real_attrs )
        self.handler_stack.append( handler )

    def endElement( self, name ) :
        if len( self.handler_stack ) > 0 :
            last_handler = self.handler_stack[ -1 ]

            if last_handler.handled_node == name :
                last_handler.node_finished()
                self.handler_stack = self.handler_stack[ :-1 ]
            else :
                last_handler.end_element( name )

    def characters( self, content ) :
        if len( self.handler_stack ) > 0 :
            last_handler = self.handler_stack[ -1 ]
            last_handler.element_data( content )

    def __convert_attributes( self, attrs ) :
        attr_dict = {}
        keys = attrs.keys()

        for key in keys :
            attr_dict[ key ] = attrs.getValue( key )

        return attr_dict
