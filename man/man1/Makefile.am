##
#	ad -- ASCII dump
#	man/man1/Makefile.am
#
#	Copyright (C) 2015-2025  Paul J. Lucas
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

dist_man1_MANS = ad.1

GROFF_FLAGS =	-man -Tps

PS2PDF_FLAGS =	-dEmbedAllFonts=true \
		-dHaveTrueTypes=true \
		-dPDFSETTINGS=/prepress \
		-dSubsetFonts=false

pdf-local: ad.pdf

ad.ps: ad.1
	groff $(GROFF_FLAGS) $< > $@

ad.pdf: ad.ps
	ps2pdf $(PS2PDF_FLAGS) $<

clean-local:
	rm -f ad.ps

distclean-local:
	rm -f ad.pdf

# vim:set noet sw=8 ts=8:
