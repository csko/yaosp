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
import sys
import xml.sax
import context as ctx
import handler as hndlr
import work_handlers
import definition_handlers

if __name__ == "__main__" :
    if not os.path.isfile( "pbuild.xml" ) :
        print "pbuild.xml not found in the current directory!"
        sys.exit( 0 )

    context = ctx.BuildContext()
    handler = hndlr.BuildHandler( context )

    handler.add_node_handlers( work_handlers.handlers )
    handler.add_node_handlers( definition_handlers.handlers )

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
        target_name = sys.argv[ -1 ]

    target = context.get_target( target_name )

    if target == None :
        print "Target " + target_name + " not found!"
        sys.exit( 0 )

    target.execute( context )
