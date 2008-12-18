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

import sys
import xml.sax
import definitions
import functions
import work_handlers
import definition_handlers

class BuildContext :
    def __init__( self ) :
        self.targets = []
        self.default_target = None
        self.definition_manager = definitions.DefinitionManager()
        self.function_handler = functions.FunctionHandler()

    def add_definition( self, definition ) :
        self.definition_manager.add_definition( definition )

    def remove_definition( self, name ) :
        self.definition_manager.remove_definition( name )

    def add_target( self, target ) :
        self.targets.append( target )

    def get_target( self, name ) :
        for target in self.targets :
            if target.get_name() == name :
                return target

        return None

    def get_definition( self, name ) :
        return self.definition_manager.get_definition( name )

    def get_default_target( self ) :
        return self.default_target

    def set_default_target( self, target ) :
        self.default_target = target

    def replace_definitions( self, text ) :
        while True :
            start = text.find( "${" )

            if start == -1 :
                break

            end = text.find( "}", start + 2 )

            if end == -1 :
                break;

            def_text = text[ start:end + 1 ]
            def_name = definitions.extract_definition_name( def_text )
            definition = self.get_definition( def_name )

            if definition == None :
                text = text[ :start ] + text[ end + 1: ]
            else :
                text = text[ :start ] + definition.to_string() + text[ end + 1: ]

        return text

    def replace_definitions_in_array( self, array ) :
        new_array = []

        for item in array :
            new_array.append( self.replace_definitions( item ) )

        return new_array

class BuildHandler( xml.sax.handler.ContentHandler ) :
    node_handlers = [
        work_handlers.TargetHandler,
        work_handlers.GccHandler,
        work_handlers.EchoHandler,
        work_handlers.ForHandler,
        definition_handlers.ArrayHandler
    ]

    def __init__( self, context ) :
        self.context = context
        self.handler_stack = []

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

if __name__ == "__main__" :
    context = BuildContext()
    handler = BuildHandler( context )

    xml_parser = xml.sax.make_parser()
    xml_parser.setContentHandler( handler )
    xml_parser.parse( "pbuild.xml" )

    target_name = None

    if len( sys.argv ) == 1 :
        target_name = context.get_default_target()

        if target_name == None :
            print "Default target not specified!"
            sys.exit( 0 )
    else :
        target_name = sys.arv[ -1 ]

    target = context.get_target( target_name )

    if target == None :
        print "Target " + target_name + " not found!"
        sys.exit( 0 )

    target.execute( context )
