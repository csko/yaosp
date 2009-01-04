# System call table generator
#
# Copyright (c) 2009 Zoltan Kovacs
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

import re
import sys

SYSCALL_MASK = re.compile( '.*{.*"(.+)",.*sys_.+,.*}.*' )

if len( sys.argv ) != 3 :
    print "Usage: " + sys.argv[ 0 ] + " input_file output_file"
    sys.exit()

input = open( sys.argv[ 1 ], "r" )
lines = input.readlines()
input.close()

start_index = -1
end_index = -1

for i in range( len( lines ) ) :
    if start_index == -1 and lines[ i ].find( "system_call_table" ) >= 0 :
        start_index = i
        continue

    if start_index != -1 and lines[ i ].find( "};" ) >= 0 :
        end_index = i
        break

if start_index == -1 or end_index == -1 :
    print "Invalid system call table file!"
    sys.exit()

lines = lines[ start_index + 1:end_index ]
syscalls = []

for line in lines :
    match = SYSCALL_MASK.match( line )

    if not match :
        print "Invalid syscall line: " + line
        sys.exit()

    syscalls.append( match.groups()[ 0 ] )
    
output = open( sys.argv[ 2 ], "w" )

output.write( "/*\n * This is an auto generated file, do NOT modify it!\n */\n" )
output.write( "\n#ifndef _SYSCALL_TABLE_H_\n#define _SYSCALL_TABLE_H_\n" )
output.write( "\nenum {\n" )

for syscall in syscalls :
    output.write( "    SYS_" + syscall )

    if syscall != syscalls[ -1 ] :
        output.write( "," )

    output.write( "\n" )

output.write( "};\n" )
output.write( "\n#endif // _SYSCALL_TABLE_H_\n" )

output.close()
