##
#	ad -- ASCII dump
#	test/Makefile.am
#
#	Copyright (C) 2015-2026  Paul J. Lucas
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

AUTOMAKE_OPTIONS = 1.12			# needed for TEST_LOG_DRIVER

TESTS =	tests/ad-no_options.test \
	tests/ad-A.test \
	tests/ad-b16-B2.test \
	tests/ad-B16-e1.test \
	tests/ad-b16-e65536.test \
	tests/ad-b16.test \
	tests/ad-b1-e1.test \
	tests/ad-B1-e257.test \
	tests/ad-B2-e1-m_01.test \
	tests/ad-B2-E1-m_02.test \
	tests/ad-B2-e65536.test \
	tests/ad-B2.test \
	tests/ad-b32-e4294967296.test \
	tests/ad-B4-e1-m_01.test \
	tests/ad-B4-E1-m_02.test \
	tests/ad-B4-e4294967296.test \
	tests/ad-B4-E8-m-p.test \
	tests/ad-b8-e257.test \
	tests/ad-c_01.test \
	tests/ad--c-array.test \
	tests/ad-Cci.test \
	tests/ad-Cc.test \
	tests/ad-Cilu.test \
	tests/ad-Cis.test \
	tests/ad-Ci.test \
	tests/ad-Cit.test \
	tests/ad-Ciu.test \
	tests/ad-Cl.test \
	tests/ad-Clt.test \
	tests/ad-cnever-p_01.test \
	tests/ad-cnever-p_02.test \
	tests/ad-Ct.test \
	tests/ad-Ctu.test \
	tests/ad-Cu.test \
	tests/ad-Cx.test \
	tests/ad-d.test \
	tests/ad-e0x0102030405060708-m_01.test \
	tests/ad-E0x0102030405060708-m_02.test \
	tests/ad-e0x01020304050607-m_01.test \
	tests/ad-E0x01020304050607-m_02.test \
	tests/ad-e0x010203040506-m_01.test \
	tests/ad-E0x010203040506-m_02.test \
	tests/ad-e0x0102030405-m_01.test \
	tests/ad-E0x0102030405-m_02.test \
	tests/ad-e0x01020304-m_01.test \
	tests/ad-E0x01020304-m_02.test \
	tests/ad-e0x010203-m_01.test \
	tests/ad-E0x010203-m_02.test \
	tests/ad-e0x0102-m_01.test \
	tests/ad-E0x0102-m_02.test \
	tests/ad-e1-sx_01.test \
	tests/ad-E1-sx_02.test \
	tests/ad-g16.test \
	tests/ad-g1.test \
	tests/ad-g2.test \
	tests/ad-g32.test \
	tests/ad-g3.test \
	tests/ad-g4.test \
	tests/ad-g8.test \
	tests/ad-i-s_01.test \
	tests/ad-i.test \
	tests/ad-j1k-N16.test \
	tests/ad-j1x.test \
	tests/ad-j2-N14.test \
	tests/ad-j2-N17.test \
	tests/ad-last_row_01.test \
	tests/ad-last_row_02.test \
	tests/ad-m-s_01.test \
	tests/ad-m.test \
	tests/ad-m-V.test \
	tests/ad-N0.test \
	tests/ad-N0x0.test \
	tests/ad-N0x1.test \
	tests/ad-N0xf.test \
	tests/ad-N15.test \
	tests/ad-N16.test \
	tests/ad-N17.test \
	tests/ad-N1.test \
	tests/ad-N2.test \
	tests/ad-N3.test \
	tests/ad-N4.test \
	tests/ad-n6.test \
	tests/ad-no_options-out.test \
	tests/ad-n-Snst-01.test \
	tests/ad-n-Snst-02.test \
	tests/ad-nSw-u.test \
	tests/ad-n.test \
	tests/ad-o_01.test \
	tests/ad-O_02.test \
	tests/ad-P.test \
	tests/ad-p-V.test \
	tests/ad-r_01.test \
	tests/ad-r_02.sh \
	tests/ad-r_02.test \
	tests/ad-r_03.test \
	tests/ad-r_04.test \
	tests/ad-r_05.test \
	tests/ad-r_06.test \
	tests/ad-r_07.test \
	tests/ad-r_08.test \
	tests/ad-s_01.test \
	tests/ad-sxxx.test \
	tests/ad-t_01.test \
	tests/ad-T_02.test \
	tests/ad-U_01.test \
	tests/ad-u.test \
	tests/ad-u-U0x2192.test \
	tests/ad-u-U8594.test \
	tests/ad-u-UU+2192.test \
	tests/ad-V_01.test \
	tests/ad-v_02.test \
	tests/ad-x.test

if HAVE_RSRC_FORK
TESTS+=	tests/ad-R_09.sh
endif

AM_TESTS_ENVIRONMENT = BUILD_SRC=$(top_builddir)/src; export BUILD_SRC ;
TEST_EXTENSIONS = .sh .test

SH_LOG_DRIVER = $(srcdir)/run_test.sh
TEST_LOG_DRIVER = $(srcdir)/run_test.sh

EXTRA_DIST = run_test.sh tests data expected
dist-hook:
	cd $(distdir)/tests && rm -f *.log *.trs

# vim:set noet sw=8 ts=8:
