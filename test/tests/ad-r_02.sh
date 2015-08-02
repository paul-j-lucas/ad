# /bin/sh

OUTPUT=$1
LOG_FILE=$2

cp data/endian.bin $OUTPUT
ad -r data/patch-1.txt $OUTPUT
diff expected/ad-r_02.bin $OUTPUT > $LOG_FILE

# vim:set et sw=2 ts=2:
