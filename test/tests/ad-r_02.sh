# /bin/sh

OUTPUT=$1

cp data/endian.bin $OUTPUT
ad -r data/patch-1.txt $OUTPUT
diff expected/ad-r_02.bin $OUTPUT

# vim:set et sw=2 ts=2:
