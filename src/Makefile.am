##
#	ad -- ASCII dump
#	src/Makefile.am
#
#	Copyright (C) 2015-2018  Paul J. Lucas
#
#	This program is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

bin_PROGRAMS = ad

AM_CPPFLAGS = -I$(top_srcdir)/lib -I$(top_builddir)/lib
LDADD = $(top_builddir)/lib/libgnu.a

ad_SOURCES = \
	pjl_config.h \
	ad.c \
	color.c color.h \
	common.h \
	dump.c \
	match.c match.h \
	options.c options.h \
	reverse.c \
	slist.c slist.h \
	util.c util.h \
	utf8.c utf8.h

# vim:set noet sw=8 ts=8:
