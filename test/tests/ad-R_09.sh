# /bin/sh

ME=`basename $0`

[ `uname` == Darwin ] ||
  { echo "$ME: this test is supported only on macOS" >&2; exit 1; }

[ -n "$TMPDIR" ] || TMPDIR=/tmp
trap "x=$?; rm -f $TMPDIR/*_$$_* 2>/dev/null; exit $x" EXIT HUP INT TERM

INPUT="$TMPDIR/rsrc_fork_$$_"
OUTPUT="$TMPDIR/ad_stdout_$$_"

touch "$INPUT"
xattr -w com.apple.ResourceFork 'hello, world' "$INPUT"

ad -R "$INPUT" "$OUTPUT"
diff expected/ad-R_09.txt "$OUTPUT"

# vim:set et sw=2 ts=2:
