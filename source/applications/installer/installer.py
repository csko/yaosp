# yaOSp installer
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

import os
import sys
import tarfile

def get_partition_list() :
    i = 1
    partitions = {}

    for entry in os.listdir( "/device/storage/partition" ) :
        partitions[ i ] = {
            "name" : "/device/storage/partition/" + entry
        }

    return partitions

def get_number( msg, valid_nums = None ) :
    num = None

    while True :
        data = raw_input( msg )

        try :
            num = int( data )
        except :
            continue

        if valid_nums != None and num not in valid_nums :
            continue

        break

    return num

def execute_and_wait( app, args ) :
    pid = os.fork()

    if pid == 0 :
        try :
            os.execve( app, args, os.environ )
        except OSError, e :
            sys.exit( -1 )
    elif pid < 0 :
        return False
    else :
        pid, status = os.waitpid( pid, 0 )

        if status != 0 :
            return False

    return True

# Select partition to install

print "Welcome to the yaOSp installer."
print

partitions = get_partition_list()

if len( partitions ) == 0 :
    print "There are no partitions to install!"
    sys.exit(1)

print "Partitions: "

for p in partitions :
    print "  %d) %s" % ( p, partitions[ p ][ "name" ] )

print "  0) Exit"
print

selected = get_number( "Select the partition you want to install yaOSp on: ", partitions.keys() + [ 0 ] )

if selected == 0 :
    sys.exit(0)

# Create ext2 filesystem on the selected partition

partition = partitions[ selected ]

print
print "Creating ext2 filesystem on %s ... " % ( partition[ "name" ] )

result = execute_and_wait(
    "/application/fstools",
    [ "fstools", "--action=create", "--filesystem=ext2", "--device=" + partition[ "name" ] ]
)

if not result :
    print "Failed to execute fstools, make sure it's installed on your system!"
    sys.exit(0)

print

# Unpack the base yaOSp package

print
print "Installing yaOSp base package ..."

try :
    os.mkdir( "/media/install" )
except OSError, e :
    print "Failed to create /media/install directory!"
    sys.exit(0)

try :
    os.mount( partition[ "name" ], "/media/install", "ext2" )
except OSError, e :
    print "Failed to mount %s to /media/install" % ( partition[ "name" ] )
    sys.exit(0)

try :
    base = tarfile.open( "/yaosp/yaosp.tar.bz2" )
except IOError, e :
    print "The yaosp base package is not found!"
    sys.exit(0)

print

for entry in base :
    print "Extracting /media/install/%s" % ( entry.name )

    base.extract( entry, "/media/install" )

# Build a new menu.lst file for GRUB

grub_partition = "(hd%c,%c)" % ( partition[ "name" ][ -2 ], partition[ "name" ][ -1 ] )

f = open( "/media/install/boot/grub/menu.lst", "w" )

f.write( "title yaOSp\n" )
f.write( "root %s\n" % grub_partition )
f.write( "kernel /system/kernel root=%s\n" % ( partition[ "name" ] ) )
f.write( "module /system/module/bus/pci\n" )
f.write( "module /system/module/disk/pata\n" )
f.write( "module /system/module/disk/partition\n" )
f.write( "module /system/module/filesystem/ext2\n" )
f.write( "module /system/module/input/ps2kbd\n" )
f.write( "module /system/module/char/terminal\n" )

f.close()

os.umount( "/media/install" )

# We're done :)

print
print "The installation of yaOSp is done."
print "You have to restart and install GRUB manually by following these steps:"
print "  * Press 'c' when GRUB is loaded"
print "  * Type 'root %s'" % ( grub_partition )
print "  * Type 'setup (hd%c)'" % ( partition[ "name" ][ -2 ] )
print "  * You're done, remove the CD and reboot."
print
print "Thanks for choosing yaOSp :)"
