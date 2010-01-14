<!--

This file is part of the yaosp build system

Copyright (c) 2009 Kornel Csernai, Zoltan Kovacs

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
        <item>input.c</item>
        <item>control.c</item>
        <item>driver.c</item>
        <item>node.c</item>
        <item>ps2keyboard/ps2keyboard.c</item>
        <item>ps2keyboard/keymap.c</item>
        <item>ps2mouse/ps2mouse.c</item>
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
        <echo>Compiling input driver</echo>
        <echo/>

        <for var="i" array="${files}">
            <echo>[GCC    ] source/drivers/input/input/${i}</echo>
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
        <echo>Linking input driver</echo>
        <echo/>
        <echo>[LD     ] source/drivers/input/input/objs/input</echo>

        <ld>
            <input>objs/*.o</input>
            <output>objs/input</output>
            <flag>-shared</flag>
        </ld>
    </target>

    <target name="install">
        <copy from="objs/input" to="../../../../build/image/system/module/input/input"/>
    </target>

    <target name="all">
        <call target="clean"/>
        <call target="compile"/>
        <call target="install"/>
    </target>
</build>
