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
        if not "name" in attrs :
            return

        self.target = works.Target( attrs[ "name" ] )

    def node_finished( self ) :
        self.get_context().add_target( self.target )

class GccHandler( handler.NodeHandler ) :
    handled_node = "gcc"

    def __init__( self, parent, context ) :
        handler.NodeHandler.__init__( self, parent, context )

        self.work = None
        self.data = ""

    def node_started( self, attrs ) :
        self.work = works.GccWork()

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
        elif name == "flag" :
            self.work.add_flag( self.data )

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
