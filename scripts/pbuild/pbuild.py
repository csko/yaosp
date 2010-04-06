#!/usr/bin/env python
# Python build system
#
# Copyright (c) 2008, 2010 Zoltan Kovacs
# Copyright (c) 2010 Kornel Csernai
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
import logging

if __name__ == "__main__" :
    # Set up logging.
    logging.basicConfig(level=logging.INFO,
#    logging.basicConfig(level=logging.DEBUG,
       format='%(asctime)s %(name)-6s %(levelname)-7s %(message)s',
       datefmt='%Y-%m-%d %H:%M')

    if not os.path.isfile( "pbuild.xml" ) :
        logging.critical( "pbuild.xml not found in the current directory!" )
        sys.exit( 1 )

    pcontext = ctx.ProjectContext()
    pcontext.init()
    context = ctx.BuildContext(pcontext)
    handler = hndlr.BuildHandler(context)

    handler.add_node_handlers( work_handlers.handlers )
    handler.add_node_handlers( definition_handlers.handlers )

    xml_parser = xml.sax.make_parser()
    xml_parser.setContentHandler( handler )
    xml_parser.parse( "pbuild.xml" )

    target_name = None

    if len( sys.argv ) == 1 :
        target_name = context.get_default_target()

        if target_name == None :
            logging.critical( "Default target not specified!" )
            sys.exit( 1 )
    else :
        target_name = sys.argv[ -1 ]

    target = context.get_target( target_name )

    if target == None :
        logging.critical( "Target " + target_name + " not found!" )
        sys.exit( 1 )

    target.execute( context )
