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

class Function :
    def handle( self, arguments ) :
        return ""

class FileName( Function ) :
    def handle( self, arguments ) :
        if len( arguments ) != 1 :
            return ""

        arg = arguments[ 0 ]
        dot = arg.rfind( "." )
        fn = arg.rfind( "/" )

        if dot == -1 :
            return arg[ fn + 1: ]
        else :
            return arg[ fn + 1:dot ]

class FunctionHandler :
    functions = {
        "filename" : FileName
    }

    def __init__( self ) :
        pass

    def handle_functions( self, text ) :
        arg_start = text.find( "(" )

        if arg_start == -1 :
            return text

        fn_start = arg_start - 1

        while fn_start > 0 and not text[ fn_start ] in [ "/", ">" ] :
            fn_start -= 1

        end = text.find( ")", arg_start + 1 )

        if end == -1 :
            return text

        function_name = text[ fn_start + 1:arg_start ]
        args = text[ arg_start + 1:end ]

        if not function_name in self.functions :
            return text

        function = self.functions[ function_name ]()

        return text[ 0:fn_start + 1 ] + function.handle( [ args ] ) + text[ end + 1: ]
