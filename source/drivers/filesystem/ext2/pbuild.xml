<!--

This file is part of the yaosp build system

Copyright (c) 2009, 2010 Zoltan Kovacs
Copyright (c) 2009 Kornel Csernai

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

-->

<build default="all">
    <array name="files">
        <item>ext2.c</item>
        <item>inode.c</item>
        <item>directory.c</item>
        <item>block.c</item>
    </array>

    <target name="clean">
        <delete>objs/*</delete>
        <rmdir>objs</rmdir>
    </target>

    <target name="prepare" type="private">
        <mkdir>objs</mkdir>
    </target>

    <target name="compile">
        <call target="prepare"/>

        <echo/>
        <echo>Compiling ext2 filesystem driver</echo>
        <echo/>

        <for var="i" array="${files}">
            <echo>[GCC    ] source/drivers/filesystem/ext2/${i}</echo>
            <gcc>
                <input>${i}</input>
                <output>objs/filename(${i}).o</output>
                <include>../../../kernel/include</include>
                <flag>-c</flag>
                <flag>-O2</flag>
                <flag>-Wall</flag>
                <flag>-Wshadow</flag>
                <flag>-Werror</flag>
                <flag>-nostdinc</flag>
                <flag>-nostdlib</flag>
                <flag>-fno-builtin</flag>
                <flag>-fPIC</flag>
            </gcc>
        </for>

        <echo/>
        <echo>Linking ext2 filesystem driver</echo>
        <echo/>
        <echo>[LD     ] source/drivers/filesystem/ext2/objs/ext2</echo>

        <ld>
            <input>objs/*.o</input>
            <output>objs/ext2</output>
            <flag>-shared</flag>
        </ld>
    </target>

    <target name="install">
        <copy from="objs/ext2" to="../../../../build/image/system/module/filesystem/ext2"/>
    </target>

    <target name="all">
        <call target="clean"/>
        <call target="compile"/>
        <call target="install"/>
    </target>
</build>
