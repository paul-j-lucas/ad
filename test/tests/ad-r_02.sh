# /bin/sh

[ -n "$TMPDIR" ] || TMPDIR=/tmp
trap "x=$?; rm -f $TMPDIR/*_$$_* 2>/dev/null; exit $x" EXIT HUP INT TERM

BINFILE="$TMPDIR/ad-r_02_$$_"

cp data/endian.bin "$BINFILE"
ad -r data/patch-1.txt "$BINFILE"
diff expected/ad-r_02.bin "$BINFILE"

# vim:set et sw=2 ts=2:
