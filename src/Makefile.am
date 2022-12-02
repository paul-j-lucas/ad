##
#	ad -- ASCII dump
#	src/Makefile.am
#
#	Copyright (C) 2015-2021  Paul J. Lucas
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

bin_PROGRAMS =	ad
check_PROGRAMS=	red_black_test slist_test

AM_CPPFLAGS =	-I$(top_srcdir)/lib -I$(top_builddir)/lib
if ENABLE_FLEX_DEBUG
AM_LFLAGS =     -d
endif
AM_YFLAGS =     -d
LDADD =		$(top_builddir)/lib/libgnu.a

BUILT_SOURCES =	parser.c parser.h lexer.c

ad_SOURCES =	parser.y\
		lexer.l lexer.h \
		pjl_config.h \
		ad.c ad.h \
		color.c color.h \
		dam_lev.c dam_lev.h \
		did_you_mean.c did_you_mean.h \
		debug.c debug.h \
		dump.c \
		expr.c expr.h \
		keyword.c keyword.h \
		literals.c literals.h \
		match.c match.h \
		options.c options.h \
		print.c print.h \
		red_black.c red_black.h \
		reverse.c \
		slist.c slist.h \
		strbuf.c strbuf.h \
		typedef.c typedef.h \
		types.c types.h \
		unicode.c unicode.h \
		util.c util.h

red_black_test_SOURCES = \
		ad.h \
		pjl_config.h \
		red_black.c red_black.h \
		red_black_test.c \
		slist.c slist.h \
		types.h \
		util.c util.h \
		unit_test.h

slist_test_SOURCES = \
		ad.h \
		pjl_config.h \
		slist.c slist.h \
		slist_test.c \
		types.h \
		util.c util.h \
		unit_test.h

TESTS =		$(check_PROGRAMS)

clean-local:
	rm -f $(BUILT_SOURCES) parser.output

parser.output: parser.y
	bison -v --report=all $<
	@rm -f parser.tab.c

# vim:set noet sw=8 ts=8:
