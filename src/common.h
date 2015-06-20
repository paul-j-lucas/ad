/*
**      ad -- ASCII dump
**      common.h
**
**      Copyright (C) 1996-2015  Paul J. Lucas
**
**      This program is free software; you can redistribute it and/or modify
**      it under the terms of the GNU General Public License as published by
**      the Free Software Foundation; either version 2 of the Licence, or
**      (at your option) any later version.
** 
**      This program is distributed in the hope that it will be useful,
**      but WITHOUT ANY WARRANTY; without even the implied warranty of
**      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**      GNU General Public License for more details.
** 
**      You should have received a copy of the GNU General Public License
**      along with this program; if not, write to the Free Software
**      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef ad_common_H
#define ad_common_H

///////////////////////////////////////////////////////////////////////////////

// exit(3) status codes
#define EXIT_OK             0
#define EXIT_NO_MATCHES     1
#define EXIT_USAGE          2
#define EXIT_OUT_OF_MEMORY  3
#define EXIT_READ_OPEN      10
#define EXIT_READ_ERROR     11
#define EXIT_WRITE_ERROR    13
#define EXIT_SEEK_ERROR     20
#define EXIT_STAT_ERROR     21

enum endian {
  ENDIAN_UNSPECIFIED,
  ENDIAN_BIG,
  ENDIAN_LITTLE
};
typedef enum endian endian_t;

///////////////////////////////////////////////////////////////////////////////

#endif /* ad_common_H */
/* vim:set et sw=2 ts=2: */
