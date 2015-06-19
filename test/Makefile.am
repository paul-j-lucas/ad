##
#	ad -- ASCII dump
#	test/Makefile.am
#
#	Copyright (C) 2015  Paul J. Lucas
#
#	This program is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; either version 2 of the Licence, or
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

AUTOMAKE_OPTIONS = 1.12			# needed for TEST_LOG_DRIVER

TESTS =	tests/ad-no_options.test \
	tests/ad+16+X.test \
	tests/ad-B1-e257.test \
	tests/ad-B16-e1.test \
	tests/ad-B2-e65536.test \
	tests/ad-B2.test \
	tests/ad-B4-e4294967296.test \
	tests/ad-E0xD8FF-m.test \
	tests/ad-E0xFFD8-m.test \
	tests/ad-E1-sx-1.test \
	tests/ad-N00.test \
	tests/ad-N01.test \
	tests/ad-N02.test \
	tests/ad-N03.test \
	tests/ad-N04.test \
	tests/ad-N0x00.test \
	tests/ad-N0x01.test \
	tests/ad-N0x0f.test \
	tests/ad-N14+02.test \
	tests/ad-N15.test \
	tests/ad-N16.test \
	tests/ad-N17+02.test \
	tests/ad-N17.test \
	tests/ad-S02.test \
	tests/ad-b1-e1.test \
	tests/ad-b16-B2.test \
	tests/ad-b16-e65536.test \
	tests/ad-b16.test \
	tests/ad-b32-e4294967296.test \
	tests/ad-b8-e257.test \
	tests/ad-c01.test \
	tests/ad-d.test \
	tests/ad-e1-sx-2.test \
	tests/ad-h.test \
	tests/ad-i-s01.test \
	tests/ad-i.test \
	tests/ad-j02-N14.test \
	tests/ad-j02-N17.test \
	tests/ad-j1k-N16.test \
	tests/ad-j1x.test \
	tests/ad-m-s01.test \
	tests/ad-m.test \
	tests/ad-o.test \
	tests/ad-row_sep-01.test \
	tests/ad-s01.test \
	tests/ad-sxxx.test

AM_TESTS_ENVIRONMENT = BUILD_SRC=$(top_builddir)/src; export BUILD_SRC ;
TEST_EXTENSIONS = .test
TEST_LOG_DRIVER = $(srcdir)/run_test.sh

EXTRA_DIST = run_test.sh tests data expected
dist-hook:
	cd $(distdir)/tests && rm -f *.log *.trs

# vim:set noet sw=8 ts=8:
