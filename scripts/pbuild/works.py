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

import definitions

class Work :
    def __init__( self ) :
        self.sub_works = []

    def add_sub_work( self, work ) :
        self.sub_works.append( work )

    def execute_sub_works( self, context ) :
        for work in self.sub_works :
            work.execute( context )

    def execute( self, context ) :
        pass

class Target( Work ) :
    def __init__( self, name ) :
        Work.__init__( self )

        self.name = name
        self.works = []

    def get_name( self ) :
        return self.name

    def execute( self, context ) :
        self.execute_sub_works( context )

class ForWork( Work ) :
    def __init__( self, loop_variable, array_name ) :
        Work.__init__( self )

        self.loop_variable = loop_variable
        self.array = array_name

    def execute( self, context ) :
        array = None

        if definitions.is_definition( self.array ) :
            def_name = definitions.extract_definition_name( self.array )
            definition = context.get_definition( def_name )

            if definition == None :
                print "Definition " + def_name + " not found"
                return

            array = definition.to_array()
        else :
            array = self.array.split( "," )

        for i in array :
            string = definitions.String( self.loop_variable )
            string.set_value( i )

            context.add_definition( string )

            self.execute_sub_works( context )

            context.remove_definition( self.loop_variable )

class EchoWork( Work ) :
    def __init__( self ) :
        self.text = ""

    def set_text( self, text ) :
        self.text = text

    def execute( self, context ) :
        print context.replace_definitions( self.text )

class GccWork( Work ) :
    def __init__( self ) :
        self.inputs = []
        self.output = ""
        self.flags = []

    def add_input( self, input ) :
        self.inputs.append( input )

    def set_output( self, output ) :
        self.output = output

    def add_flag( self, flag ) :
        self.flags.append( flag )

    def execute( self, context ) :
        real_inputs = []
        real_flags = None

        # Parse the input files

        for input in self.inputs :
            input = context.replace_definitions( input )

            real_inputs.append( input )

        # Parse flags

        real_flags = context.replace_definitions_in_array( self.flags )

        command = [ "gcc" ] + real_flags + real_inputs
        command += [ "-o", context.replace_definitions( self.output ) ]
