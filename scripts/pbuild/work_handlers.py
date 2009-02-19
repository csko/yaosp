# Python build system
#
# Copyright (c) 2008, 2009 Zoltan Kovacs
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

import works
import handler

class TargetHandler( handler.NodeHandler ) :
    handled_node = "target"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.target = None

    def add_work( self, work ) :
        if self.target != None :
            self.target.add_sub_work( work )

    def node_started( self, attrs ) :
        private = False

        if not "name" in attrs :
            return

        if "type" in attrs and attrs[ "type" ] == "private" :
            private = True

        self.target = works.Target( attrs[ "name" ], private )

    def node_finished( self ) :
        self.get_context().add_target( self.target )

class GccHandler( handler.NodeHandler ) :
    handled_node = "gcc"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.work = None
        self.data = ""
        self.def_key = ""

    def node_started( self, attrs ) :
        self.work = works.GccWork()

    def node_finished( self ) :
        if self.work != None :
            self.get_parent().add_work( self.work )

    def start_element( self, name, attrs ) :
        self.data = ""

        if name == "define" :
            if "key" in attrs :
                self.def_key = attrs[ "key" ]

    def end_element( self, name ) :
        if name == "input" :
            self.work.add_input( self.data )
        elif name == "output" :
            self.work.set_output( self.data )
        elif name == "flag" :
            self.work.add_flag( self.data )
        elif name == "include" :
            self.work.add_include( self.data )
        elif name == "define" :
            if self.def_key != None and len( self.def_key ) > 0 :
              self.work.add_define( self.def_key, self.data )

            self.def_key = None

    def element_data( self, data ) :
        self.data = data

class LdHandler( handler.NodeHandler ) :
    handled_node = "ld"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.work = None
        self.data = ""

    def node_started( self, attrs ) :
        self.work = works.LdWork()

    def node_finished( self ) :
        if self.work != None :
            self.get_parent().add_work( self.work )

    def start_element( self, name, attrs ) :
        self.data = ""

    def end_element( self, name ) :
        if name == "input" :
            self.work.add_input( self.data )
        elif name == "output" :
            self.work.set_output( self.data )
        elif name == "linkerscript" :
            self.work.set_linker_script( self.data )

    def element_data( self, data ) :
        self.data = data

class EchoHandler( handler.NodeHandler ) :
    handled_node = "echo"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.work = None

    def node_started( self, attrs ) :
        self.work = works.EchoWork()

    def node_finished( self ) :
        if self.work != None :
            self.get_parent().add_work( self.work )

    def element_data( self, data ) :
        self.work.set_text( data )

class ForHandler( handler.NodeHandler ) :
    handled_node = "for"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.work = None

    def add_work( self, work ) :
        if self.work != None :
            self.work.add_sub_work( work )

    def node_started( self, attrs ) :
        if not "array" in attrs or not "var" in attrs :
            return

        self.work = works.ForWork( attrs[ "var" ], attrs[ "array" ] )

    def node_finished( self ) :
        if self.work != None :
            self.get_parent().add_work( self.work )

class MkDirHandler( handler.NodeHandler ) :
    handled_node = "mkdir"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.data = ""
        self.work = None

    def node_started( self, attrs ) :
        self.work = works.MakeDirectory()

    def node_finished( self ) :
        if self.work != None :
            self.work.set_directory( self.data )
            self.get_parent().add_work( self.work )

    def element_data( self, data ) :
        self.data = data

class RmDirHandler( handler.NodeHandler ) :
    handled_node = "rmdir"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.data = ""
        self.work = None

    def node_started( self, attrs ) :
        self.work = works.RemoveDirectory()

    def node_finished( self ) :
        if self.work != None :
            self.work.set_directory( self.data )
            self.get_parent().add_work( self.work )

    def element_data( self, data ) :
        self.data = data

class CleanDirHandler( handler.NodeHandler ) :
    handled_node = "cleandir"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.data = ""
        self.work = None

    def node_started( self, attrs ) :
        self.work = works.CleanDirectory()

    def node_finished( self ) :
        if self.work != None :
            self.work.set_directory( self.data )
            self.get_parent().add_work( self.work )

    def element_data( self, data ) :
        self.data = data

class CallHandler( handler.NodeHandler ) :
    handled_node = "call"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.work = None

    def node_started( self, attrs ) :
        if not "target" in attrs :
            return

        if "directory" in attrs :
            directory = attrs[ "directory" ]
        else :
            directory = None

        self.work = works.CallTarget( attrs[ "target" ], directory )

    def node_finished( self ) :
        if self.work != None :
            self.get_parent().add_work( self.work )

class CopyHandler( handler.NodeHandler ) :
    handled_node = "copy"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.work = None

    def node_started( self, attrs ) :
        if not "from" in attrs or not "to" in attrs :
            return

        self.work = works.CopyWork( attrs[ "from" ], attrs[ "to" ] )

    def node_finished( self ) :
        if self.work != None :
            self.get_parent().add_work( self.work )

class DeleteHandler( handler.NodeHandler ) :
    handled_node = "delete"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.data = ""
        self.work = None

    def node_started( self, attrs ) :
        self.work = works.DeleteWork()

    def node_finished( self ) :
        if self.work != None :
            self.work.set_file( self.data )
            self.get_parent().add_work( self.work )

    def element_data( self, data ) :
        self.data = data

class ExecHandler( handler.NodeHandler ) :
    handled_node = "exec"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.data = ""
        self.work = None

    def node_started( self, attrs ) :
        if not "executable" in attrs :
            return

        self.work = works.ExecWork( attrs[ "executable" ] )

    def node_finished( self ) :
        if self.work != None :
            self.get_parent().add_work( self.work )

    def end_element( self, name ) :
        if name == "arg" :
            self.work.add_argument( self.data )

    def element_data( self, data ) :
        self.data = data

class SymlinkHandler( handler.NodeHandler ) :
    handled_node = "symlink"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.work = None

    def node_started( self, attrs ) :
        if not "from" in attrs or not "to" in attrs :
            return

        self.work = works.SymlinkWork( attrs[ "from" ], attrs[ "to" ] )

    def node_finished( self ) :
        if self.work != None :
            self.get_parent().add_work( self.work )

class ChdirHandler( handler.NodeHandler ) :
    handled_node = "chdir"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.work = None

    def node_started( self, attrs ) :
        if not "directory" in attrs :
            return

        self.work = works.ChdirWork( attrs[ "directory" ] )

    def node_finished( self ) :
        if self.work != None :
            self.get_parent().add_work( self.work )

class HTTPGetHandler( handler.NodeHandler ) :
    handled_node = "httpget"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.work = None

    def node_started( self, attrs ) :
        if not "address" in attrs or not "to" in attrs :
            return

        md5sum = None

        if "md5" in attrs :
            md5sum = attrs[ "md5" ]

        self.work = works.HTTPGetWork( attrs[ "address" ], attrs[ "to" ], md5sum )

    def node_finished( self ) :
        if self.work != None :
            self.get_parent().add_work( self.work )

handlers = [
    TargetHandler,
    GccHandler,
    LdHandler,
    EchoHandler,
    ForHandler,
    MkDirHandler,
    RmDirHandler,
    CleanDirHandler,
    CallHandler,
    CopyHandler,
    DeleteHandler,
    ExecHandler,
    SymlinkHandler,
    ChdirHandler,
    HTTPGetHandler
]
