/* yaosp C library
 *
 * Copyright (c) 2009 Zoltan Kovacs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _REGEX_H_
#define _REGEX_H_

typedef off_t regoff_t;

typedef struct regex {
    int re_magic;
    size_t re_nsub;
    __const char* re_endp;
    struct re_guts* re_g;
} regex_t;

typedef struct regmatch {
    regoff_t rm_so;
    regoff_t rm_eo;
} regmatch_t;

/* regcomp() flags */
#define REG_BASIC       0000
#define REG_EXTENDED        0001
#define REG_ICASE       0002
#define REG_NOSUB       0004
#define REG_NEWLINE     0010
#define REG_NOSPEC      0020
#define REG_PEND        0040
#define REG_DUMP        0200

/* regerror() flags */
#define REG_NOMATCH     1
#define REG_BADPAT      2
#define REG_ECOLLATE        3
#define REG_ECTYPE      4
#define REG_EESCAPE     5
#define REG_ESUBREG     6
#define REG_EBRACK      7
#define REG_EPAREN      8
#define REG_EBRACE      9
#define REG_BADBR       10
#define REG_ERANGE      11
#define REG_ESPACE      12
#define REG_BADRPT      13
#define REG_EMPTY       14
#define REG_ASSERT      15
#define REG_INVARG      16
#define REG_ATOI        255 /* convert name to number (!) */
#define REG_ITOA        0400    /* convert number to name (!) */

/* regexec() flags */
#define REG_NOTBOL      00001
#define REG_NOTEOL      00002
#define REG_STARTEND        00004
#define REG_TRACE       00400   /* tracing of execution */
#define REG_LARGE       01000   /* force large representation */
#define REG_BACKR       02000   /* force use of backref code */

int regcomp( regex_t* preg, const char* pattern, int cflags );
size_t regerror( int errcode, const regex_t* preg, char* errbuf, size_t size );
int regexec( const regex_t* preg, const char* string, size_t nmatch, regmatch_t pmatch[], int eflags );
void regfree( regex_t* preg );

#endif /* _REGEX_H_ */
