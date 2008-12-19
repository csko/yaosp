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

import os
import xml.sax
import subprocess
import definitions
import handler as hndlr
import context as ctx

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
    def __init__( self, name, private ) :
        Work.__init__( self )

        self.name = name
        self.private = private
        self.works = []

    def get_name( self ) :
        return self.name

    def is_private( self ) :
        return self.private

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
        self.includes = []

    def add_input( self, input ) :
        self.inputs.append( input )

    def set_output( self, output ) :
        self.output = output

    def add_flag( self, flag ) :
        self.flags.append( flag )

    def add_include( self, include ) :
        self.includes.append( include )

    def execute( self, context ) :
        real_inputs = []
        real_flags = None
        real_includes = []

        # Parse the input files

        for input in self.inputs :
            input = context.replace_definitions( input )

            real_inputs.append( input )

        # Parse flags

        real_flags = context.replace_definitions_in_array( self.flags )

        # Parse includes

        for include in self.includes :
            real_includes.append( "-I" + include )

        # Build the command

        command = [ "gcc" ] + real_flags + real_inputs + real_includes
        command += [ "-o", context.handle_everything( self.output ) ]

        # Execute a new GCC process

        process = subprocess.Popen( command )
        process.wait()

class LdWork( Work ) :
    def __init__( self ) :
        self.linker_script = None
        self.inputs = []
        self.output = ""

    def add_input( self, input ) :
        self.inputs.append( input )

    def set_output( self, output ) :
        self.output = output

    def set_linker_script( self, linker_script ) :
        self.linker_script = linker_script

    def execute( self, context ) :
        real_inputs = []

        # Parse the input files

        for input in self.inputs :
            input = context.replace_definitions( input )
            real_inputs += context.replace_wildcards( input )

        # Build the command

        command = [ "ld" ]

        if self.linker_script != None :
            command += [ "-T" + self.linker_script ]

        command += real_inputs
        command += [ "-o", context.handle_everything( self.output ) ]

        # Execute a new LD process

        process = subprocess.Popen( command )
        process.wait()

class MakeDirectory( Work ) :
    def __init__( self ) :
        self.directory = ""

    def set_directory( self, directory ) :
        self.directory = directory

    def execute( self, context ) :
        try :
            os.mkdir( self.directory )
        except OSError, e :
            pass

class RemoveDirectory( Work ) :
    def __init__( self ) :
        self.directory = ""

    def set_directory( self, directory ) :
        self.directory = directory

    def execute( self, context ) :
        try :
            os.rmdir( self.directory )
        except OSError, e :
            pass

class CallTarget( Work ) :
    def __init__( self, target, directory ) :
        self.target = target
        self.directory = directory

    def execute( self, context ) :
        if self.directory != None :
            self.directory = context.handle_everything( self.directory )

            if not os.path.isdir( self.directory ) :
                print self.directory + " is not a directory!"
                return

            cached_cwd = os.getcwd()
            os.chdir( cached_cwd + os.sep + self.directory )

            context = ctx.BuildContext()
            handler = hndlr.BuildHandler( context )

            xml_parser = xml.sax.make_parser()
            xml_parser.setContentHandler( handler )
            xml_parser.parse( "pbuild.xml" )

        target = context.get_target( self.target, True )

        if target != None :
            target.execute( context )

        if self.directory != None :
            os.chdir( cached_cwd )

class CopyWork( Work ) :
    def __init__( self, src, dest ) :
        self.src = src
        self.dest = dest

    def execute( self, context ) :
        if not os.path.isfile( self.src ) :
            return

        src_file = open( self.src, "r" )
        dest_file = open( self.dest, "w" )

        dest_file.write( src_file.read() )

        src_file.close()
        dest_file.close()

class DeleteWork( Work ) :
    def __init__( self ) :
        self.file = ""

    def set_file( self, file ) :
        self.file = file

    def execute( self, context ) :
        files = context.replace_wildcards( self.file )

        for file in files :
            try :
                os.remove( file )
            except OSError, e :
                pass

class ExecWork( Work ) :
    def __init__( self, executable ) :
        self.args = []
        self.executable = executable

    def add_argument( self, arg ) :
        self.args.append( arg )

    def execute( self, context ) :
        command = [ self.executable ] + self.args

        process = subprocess.Popen( command )
        process.wait()

class SymlinkWork( Work ) :
    def __init__( self, src, dest ) :
        self.src = src
        self.dest = dest

    def execute( self, context ) :
        try :
            os.symlink( self.src, self.dest )
        except OSError, e :
            pass
