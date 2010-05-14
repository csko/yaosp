# Python build system
#
# Copyright (c) 2008, 2009, 2010 Zoltan Kovacs
# Copyright (c) 2009, 2010 Kornel Csernai
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
import logging

class Work :
    def __init__( self ) :
        self.sub_works = []

    def add_sub_work( self, work ) :
        self.sub_works.append( work )

    def execute_sub_works( self, context ) :
        for work in self.sub_works :
            logging.debug( "Executing work %s" % work )
            if work.execute( context ) == False:
                logging.critical( "Failed to execute work %s" % work )
                sys.exit( 1 )

    # If False returned, it means it's a critical error.
    def execute( self, context ) :
        pass

    def exec_shell( self, command ) :
        # Get the return code of the process
        try:
            retcode = subprocess.call( command )
            # If the return code is not 0, the build process must stop
            if retcode != 0 :
                logging.error( "Process returned with return code %d" % retcode )
                logging.error( "Build stopped." )
                return False
            return
        except OSError, e:
            logging.error( 'Failed to exec shell command ("%s"): %s' % ( command, e ) )
            return False

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
        return self.execute_sub_works( context )

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
                logging.error( "Definition " + def_name + " not found" )
                return False

            array = definition.to_array()
        else :
            array = self.array.split( "," )

        for i in array :
            string = definitions.String( self.loop_variable )
            string.set_value( i )

            context.add_definition( string )

            if self.execute_sub_works( context ) == False:
                context.remove_definition( self.loop_variable )
                return False

            context.remove_definition( self.loop_variable )

class EchoWork( Work ) :
    def __init__( self ) :
        self.text = ""

    def set_text( self, text ) :
        self.text = text

    def execute( self, context ) :
        # use logging.info()?
        print context.replace_definitions( self.text )

class GccWork( Work ) :
    GCC_COMMAND = "i686-pc-yaosp-gcc"
    GPP_COMMAND = "i686-pc-yaosp-g++"

    def __init__( self, need_gpp ) :
        self.inputs = []
        self.output = ""
        self.flags = []
        self.includes = []
        self.defines = {}
        self.need_gpp = need_gpp

    def add_input( self, input ) :
        self.inputs += [input]

    def set_output( self, output ) :
        self.output = output

    def add_flag( self, flag ) :
        self.flags += [flag]

    def add_include( self, include ) :
        self.includes += [include]

    def add_define( self, key, value ) :
        self.defines[ key ] = value

    def execute( self, context ) :
        real_inputs = []
        real_flags = None
        real_includes = []
        real_defines = []
        need_cpp = False

        # Parse the input files

        for input in self.inputs :
            input = context.replace_definitions( input )
            real_inputs += context.replace_wildcards( input )

        if not self.need_gpp :
            for input in real_inputs :
                if input.endswith(".cpp") :
                    self.need_gpp = True
                    break

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

        if self.need_gpp :
            command = [GccWork.GPP_COMMAND]
        else :
            command = [GccWork.GCC_COMMAND]

        command += real_flags + real_inputs + real_includes
        command += real_defines
        command += [ "-o", context.handle_everything( self.output ) ]

        return self.exec_shell( command )

class LdWork( Work ) :
    LD_COMMAND = "i686-pc-yaosp-ld"

    def __init__( self ) :
        self.linker_script = None
        self.inputs = []
        self.flags = []
        self.output = ""

    def add_input( self, input ) :
        self.inputs.append( input )

    def add_flag( self, flag ) :
        self.flags.append( flag )

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

        command = [ LdWork.LD_COMMAND ] + self.flags

        if self.linker_script != None :
            command += [ "-T" + self.linker_script ]

        command += real_inputs
        command += [ "-o", context.handle_everything( self.output ) ]

        # Execute a new LD process

        return self.exec_shell( command )

class ArWork( Work ) :

    AR_COMMAND = "i686-pc-yaosp-ar"

    def __init__( self ) :
        self.inputs = []
        self.flags = []
        self.output = ""

    def add_input( self, input ) :
        self.inputs.append( input )

    def add_flag( self, flag ) :
        self.flags.append( flag )

    def set_output( self, output ) :
        self.output = output

    def execute( self, context ) :
        real_inputs = []

        # Parse the input files

        for input in self.inputs :
            input = context.replace_definitions( input )
            real_inputs += context.replace_wildcards( input )

        # Build the command

        command = [ ArWork.AR_COMMAND ] + self.flags
        command += [ context.handle_everything( self.output ) ]
        command += real_inputs

        # Execute a new AR process

        return self.exec_shell( command )

class MakeDirectory( Work ) :
    def __init__( self ) :
        self.directory = ""

    def set_directory( self, directory ) :
        self.directory = directory

    def execute( self, context ) :
        try :
            dirname = context.replace_definitions(self.directory)
            os.mkdir( dirname )
        except OSError, e :
            logging.warning( 'Failed to make directory ("%s"): %s' % ( dirname, e ) )

class RemoveDirectory( Work ) :
    def __init__( self ) :
        self.directory = ""

    def set_directory( self, directory ) :
        self.directory = directory

    def execute( self, context ) :
        try :
            # TODO: no replace definitions?
            dirname = self.directory
            os.rmdir( dirname )
        except OSError, e :
            logging.warning( 'Failed to remove directory ("%s"): %s' % ( dirname, e ) )

class CleanDirectory( Work ) :
    def __init__( self ) :
        self.directory = ""

    def set_directory( self, directory ) :
        self.directory = directory

    def execute( self, context ) :
        if os.path.islink( self.directory ) :
            try :
                # TODO: no replace definitions?
                dirname = self.directory
                os.remove( dirname )
            except OSError, e :
                logging.warning( 'Failed to remove directory ("%s") while cleaning: %s' % ( dirname, e ) )
        elif os.path.isdir( self.directory ) :
            self._clean_directory( self.directory )

    def _clean_directory( self, dir ) :
        entries = os.listdir( dir )

        for entry in entries :
            path = dir + os.path.sep + entry

            if os.path.islink( path ) :
                try :
                    os.remove( path )
                except OSError, e :
                    logging.warning( 'Failed to remove directory ("%s") while cleaning: %s' % ( path, e ) )
            elif os.path.isdir( path ) :
                self._clean_directory( path )

                try :
                    os.rmdir( path )
                except OSError, e :
                    logging.warning( 'Failed to remove directory ("%s") while cleaning: %s' % ( path, e ) )
            else :
                try :
                    os.remove( path )
                except OSError, e :
                    logging.warning( 'Failed to remove directory ("%s") while cleaning: %s' % ( path, e ) )

class CallTarget( Work ) :
    def __init__( self, target, directory ) :
        self.target = target
        self.directory = directory

    def execute( self, context ) :
        if self.directory != None :
            directory = context.handle_everything( self.directory )

            if not os.path.isdir( directory ) :
                logging.error( directory + " is not a directory!" )
                return

            cached_cwd = os.getcwd()
            os.chdir( cached_cwd + os.sep + directory )

            context = ctx.BuildContext( context.get_project_context() )
            handler = hndlr.BuildHandler( context )

            if not os.path.isfile( "pbuild.xml" ) :
                logging.critical( "pbuild.xml not found in " + os.getcwd() )
                logging.critical( "Build stopped." )
                sys.exit( 1 )

            xml_parser = xml.sax.make_parser()
            xml_parser.setContentHandler( handler )

            try :
                xml_parser.parse( "pbuild.xml" )
            except :
                logging.critical( "pbuild.xml is not valid at %s" % ( os.getcwd() ) )
                logging.critical( "Build stopped." )
                sys.exit( 1 )

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

        # In the case of more than one input is specified, dest
        # must be a directory.

        if len( src ) > 1 and not dest_is_dir :
            return

        # Put the path separator character to the end of dest.

        if dest_is_dir and not dest.endswith( os.path.sep ) :
            dest += os.path.sep

        # Copy the file(s).

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
            logging.critical( "CopyWork: Failed to open source file: " + src )
            logging.critical( "Build stopped." )
            sys.exit( 1 );

        try :
            dest_file = open( dest, "w" )
        except IOError, e :
            logging.critical( "CopyWork: Failed to open destination file: " + dest )
            logging.critical( "Build stopped." )
            sys.exit( 1 )

        dest_file.write( src_file.read() )

        src_file.close()
        dest_file.close()

class MoveWork( Work ) :
    def __init__( self, src, dest ) :
        self.src = src
        self.dest = dest

    def execute( self, context ) :
        src = context.replace_definitions( self.src )
        dest = context.replace_definitions( self.dest )

        if os.path.isfile( src ) or not os.path.isfile( dest ) :
            return

        try :
            src_file = open( src, "r" )
        except IOError, e :
            logging.critical( "MoveWork: Failed to open source file: " + src )
            logging.critical( "Build stopped." )
            sys.exit( 1 );

        try :
            dest_file = open( dest, "w" )
        except IOError, e :
            logging.critical( "MoveWork: Failed to open destination file: " + dest )
            logging.critical( "Build stopped." )
            sys.exit( 1 )

        dest_file.write( src_file.read() )

        src_file.close()
        dest_file.close()

        os.unlink( src )

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
                logging.warning( 'Failed to delete "%s": %s' % ( file, e ) )

class ExecWork( Work ) :
    def __init__( self, executable ) :
        self.args = []
        self.executable = executable

    def add_argument( self, arg ) :
        self.args += [arg]

    def add_arguments( self, args ) :
        self.args += args

    def execute( self, context ) :
        real_args = []

        for arg in self.args :
            arg = context.handle_everything( arg )
            real_args += context.replace_wildcards( arg )

        command = [ self.executable ] + real_args

        # Get the return code of the process

        self.exec_shell( command )

class SymlinkWork( Work ) :
    def __init__( self, src, dest ) :
        self.src = src
        self.dest = dest

    def execute( self, context ) :
        try :
            os.symlink( self.src, self.dest )
        except OSError, e :
            logging.warning( 'Failed to symlink "%s" to "%s": %s' % ( self.src, self.dest, e ) )

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
                logging.info( "File %s already exists." % self.dest )
                return

        logging.info( "Downloading " + self.address )

        try:
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
        except IOError, e:
            logging.error( 'Failed to httpget "%s": %s' % ( self.address, e ) )
            return False

    def format_size( self, size ) :
        if size < 1024 : return "%d b" % size
        elif size < 1024 ** 2 : return "%.2f Kb" % ( size / 1024.0 )
        elif size < 1024 ** 3 : return "%.2f Mb" % ( size / ( 1024.0 ** 2 ) )
        else : return "%.2f Gb" % ( size / ( 1024.0 ** 3 ) )
