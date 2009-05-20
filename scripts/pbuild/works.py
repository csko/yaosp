# Python build system
#
# Copyright (c) 2008, 2009 Zoltan Kovacs
# Copyright (c) 2009 Kornel Csernai
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
import sys
import urllib
import xml.sax
import subprocess
import hashlib
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
        self.defines = {}

    def add_input( self, input ) :
        self.inputs.append( input )

    def set_output( self, output ) :
        self.output = output

    def add_flag( self, flag ) :
        self.flags.append( flag )

    def add_include( self, include ) :
        self.includes.append( include )

    def add_define( self, key, value ) :
        self.defines[ key ] = value

    def execute( self, context ) :
        real_inputs = []
        real_flags = None
        real_includes = []
        real_defines = []

        # Parse the input files

        for input in self.inputs :
            input = context.replace_definitions( input )
            real_inputs += context.replace_wildcards( input )

        # Parse flags

        real_flags = context.replace_definitions_in_array( self.flags )

        # Parse includes

        for include in self.includes :
            real_includes.append( "-I" + include )

        # Add defines

        for key in self.defines :
            value = self.defines[ key ]

            if len( value ) > 0 :
                real_defines.append( "-D" + key + "=" + value )
            else :
                real_defines.append( "-D" + key )

        # Build the command

        command = [ "gcc" ] + real_flags + real_inputs + real_includes
        command += real_defines
        command += [ "-o", context.handle_everything( self.output ) ]

        # Get the return code of the process
        retcode = subprocess.call( command )
        # If the return code is not 0, the build process must stop
        if retcode != 0 :
            print "Process returned with return code %d" % retcode
            print "Build stopped."
            sys.exit( retcode )


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

        # Get the return code of the process
        retcode = subprocess.call( command )
        # If the return code is not 0, the build process must stop
        if retcode != 0 :
            print "Process returned with return code %d" % retcode
            print "Build stopped."
            sys.exit( retcode )

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

class CleanDirectory( Work ) :
    def __init__( self ) :
        self.directory = ""

    def set_directory( self, directory ) :
        self.directory = directory

    def execute( self, context ) :
        if not os.path.isdir( self.directory ) :
            return

        self._clean_directory( self.directory )

    def _clean_directory( self, dir ) :
        entries = os.listdir( dir )

        for entry in entries :
            path = dir + os.path.sep + entry

            if os.path.isdir( path ) :
                self._clean_directory( path )

                try :
                    os.rmdir( path )
                except OSError, e :
                    pass
            else :
                try :
                    os.remove( path )
                except OSError, e :
                    pass

class CallTarget( Work ) :
    def __init__( self, target, directory ) :
        self.target = target
        self.directory = directory

    def execute( self, context ) :
        if self.directory != None :
            directory = context.handle_everything( self.directory )

            if not os.path.isdir( directory ) :
                print directory + " is not a directory!"
                return

            cached_cwd = os.getcwd()
            os.chdir( cached_cwd + os.sep + directory )

            context = ctx.BuildContext()
            handler = hndlr.BuildHandler( context )

            if not os.path.isfile( "pbuild.xml" ) :
                print "pbuild.xml not found in " + os.getcwd()
                print "Build stopped."
                sys.exit( 1 );

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
        src = context.replace_definitions( self.src )
        src = context.replace_wildcards( src )
        dest = context.replace_definitions( self.dest )

        dest_is_dir = os.path.isdir( dest )

        # In the case of more than one input is specified dest
        # must be a directory

        if len( src ) > 1 and not dest_is_dir :
            return

        # Put the path separator character to the end of dest

        if dest_is_dir and not dest.endswith( os.path.sep ) :
            dest += os.path.sep

        # Copy the file(s)

        for sf in src :
            if dest_is_dir :
                sep = sf.rfind( os.path.sep )

                if sep == -1 :
                    df = dest + sf
                else :
                    df = dest + sf[ sep + 1: ]
            else :
                df = dest

            self._do_copy_file( sf, df )

    def _do_copy_file( self, src, dest ) :
        try :
            src_file = open( src, "r" )
        except IOError, e :
            print "CopyWork: Failed to open source file: " + src
            print "Build stopped."
            sys.exit( 1 );

        try :
            dest_file = open( dest, "w" )
        except IOError, e :
            print "CopyWork: Failed to open destination file: " + dest
            print "Build stopped."
            sys.exit( 1 )

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
        real_args = []

        for arg in self.args :
            arg = context.handle_everything( arg )
            real_args += context.replace_wildcards( arg )

        command = [ self.executable ] + real_args

        # Get the return code of the process

        try :
            retcode = subprocess.call( command )
        except OSError, e :
            retcode = -1

        # If the return code is not 0, the build process must stop

        if retcode != 0 :
            print "Process returned with return code %d" % retcode
            print "Build stopped."
            sys.exit( retcode )

class SymlinkWork( Work ) :
    def __init__( self, src, dest ) :
        self.src = src
        self.dest = dest

    def execute( self, context ) :
        try :
            os.symlink( self.src, self.dest )
        except OSError, e :
            pass

class ChdirWork( Work ) :
    def __init__( self, directory ) :
        self.directory = directory

    def execute( self, context ) :
        os.chdir( self.directory )

class HTTPGetWork( Work ) :
    def __init__( self, address, dest, md5sum ) :
        self.address = address
        self.dest = dest
        self.md5sum = md5sum

    def execute( self, context ) :
        if os.path.isfile( self.dest ) :
            m = hashlib.md5()

            localfile = open( self.dest, "r" )
            data = localfile.read( 4096 )

            while data :
                m.update( data )
                data = localfile.read( 4096 )

            if m.hexdigest() == self.md5sum :
                print "File %s already exists." % self.dest
                return

        print "Downloading " + self.address

        urlfile = urllib.urlopen( self.address )
        urlfile_size = int( urlfile.headers.getheader( "Content-Length" ) )
        localfile = open( self.dest, "w" )

        size = 0
        data = urlfile.read( 4096 )

        while data :
            localfile.write( data )

            size += len( data )
            percent = "%.1f" % ( float(size) / urlfile_size * 100 )
            sys.stdout.write( "\r" + percent + "% done [" + self.format_size( size ) + "/" + self.format_size( urlfile_size ) + "]      \b\b\b\b\b\b" )
            sys.stdout.flush()

            data = urlfile.read( 4096 )

        urlfile.close()
        localfile.close()

        sys.stdout.write( "\n" )

    def format_size( self, size ) :
        if size < 1024 : return "%d b" % size
        elif size < 1024 ** 2 : return "%.2f Kb" % ( size / 1024.0 )
        elif size < 1024 ** 3 : return "%.2f Mb" % ( size / ( 1024.0 ** 2 ) )
        else : return "%.2f Gb" % ( size / ( 1024.0 ** 3 ) )
