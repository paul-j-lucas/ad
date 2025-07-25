# /bin/sh

ME=`basename $0`

[ `uname` == Darwin ] ||
  { echo "$ME: this test is supported only on macOS" >&2; exit 1; }

INPUT=/tmp/rsrc_fork_$$_
trap "x=$?; rm -f /tmp/*_$$_*; exit $x" EXIT HUP INT TERM
OUTPUT=$1
LOG_FILE=$2

touch $INPUT
xattr -w com.apple.ResourceFork 'hello, world' $INPUT

ad -R $INPUT $OUTPUT
diff expected/ad-R_09.txt $OUTPUT > $LOG_FILE

# vim:set et sw=2 ts=2:
