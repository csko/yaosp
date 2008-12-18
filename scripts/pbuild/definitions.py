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

class Definition :
    def __init__( self, name ) :
        self.name = name

    def get_name( self ) :
        return self.name

    def to_string( self ) :
        return ""

    def to_array( self ) :
        return []

class String( Definition ) :
    def __init__( self, name ) :
        Definition.__init__( self, name )

        self.value = ""

    def set_value( self, value ) :
        self.value = value

    def to_string( self ) :
        return self.value

    def to_array( self ) :
        return [ self.value ]

class Array( Definition ) :
    def __init__( self, name ) :
        Definition.__init__( self, name )

        self.items = []

    def add_item( self, item ) :
        self.items.append( item )

    def to_string( self ) :
        return "[" + ",".join( self.items ) + "]"

    def to_array( self ) :
        return self.items

class DefinitionManager :
    def __init__( self ) :
        self.definitions = []

    def add_definition( self, definition ) :
        self.definitions.append( definition )

    def remove_definition( self, name ) :
        for definition in self.definitions :
            if definition.get_name() == name :
                self.definitions.remove( definition )

    def get_definition( self, name ) :
        for definition in self.definitions :
            if definition.get_name() == name : 
                return definition

        return None

def is_definition( name ) :
    return len( name ) > 3 and \
           name[ 0 ] == "$" and \
           name[ 1 ] == "{" and \
           name[ -1 ] == "}"

def extract_definition_name( definition ) :
    if not is_definition( definition ) :
        return definition

    return definition[ 2:-1 ]
