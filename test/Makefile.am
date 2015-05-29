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
	tests/ad-d.test \
	tests/ad-h.test \
	tests/ad-N00.test \
	tests/ad-N01.test \
	tests/ad-N02.test \
	tests/ad-N03.test \
	tests/ad-N04.test \
	tests/ad-N14+02.test \
	tests/ad-N15.test \
	tests/ad-N16.test \
	tests/ad-N17.test \
	tests/ad-N17+02.test \
	tests/ad-o.test

AM_TESTS_ENVIRONMENT = BUILD_SRC=$(top_builddir)/src; export BUILD_SRC ;
TEST_EXTENSIONS = .test
TEST_LOG_DRIVER = $(srcdir)/run_test.sh

EXTRA_DIST = run_test.sh tests data expected
dist-hook:
	cd $(distdir)/tests && rm -f *.log *.trs

# vim:set noet sw=8 ts=8:
