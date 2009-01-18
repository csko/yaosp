/* yaosp C library, based on GNU libc's getopt_int.h
 *
 * Copyright (C) 1989-1994,1996-1999,2001,2003,2004
 * Free Software Foundation, Inc.
 * Copyright (c) 2009 Kornel Csernai
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
#ifndef _GETOPT_INT_H_
#define _GETOPT_INT_H_

struct _getopt_data
{
  int optind;
  int opterr;
  int optopt;
  char *optarg;

  int __initialized;

  char *__nextchar;
  enum
    {
      REQUIRE_ORDER, PERMUTE, RETURN_IN_ORDER
    } __ordering;

  int __posixly_correct;

  int __first_nonopt;
  int __last_nonopt;

  int __nonoption_flags_max_len;
  int __nonoption_flags_len;
};

#define _GETOPT_DATA_INITIALIZER        { 1, 1 }

extern int _getopt_internal (int ___argc, char *const *___argv,
                 const char *__shortopts,
                     const struct option *__longopts, int *__longind,
                 int __long_only);

extern int _getopt_internal_r (int ___argc, char *const *___argv,
                               const char *__shortopts,
                               const struct option *__longopts, int *__longind,
                               int __long_only, struct _getopt_data *__data);

extern int _getopt_long_r (int ___argc, char *const *___argv,
                           const char *__shortopts,
                           const struct option *__longopts, int *__longind,
                           struct _getopt_data *__data);

extern int _getopt_long_only_r (int ___argc, char *const *___argv,
                                const char *__shortopts,
                                const struct option *__longopts,
                                int *__longind,
                                struct _getopt_data *__data);
#endif
