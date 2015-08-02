#! /bin/sh
##
#       ad -- ASCII dump
#       test/run_test.sh
#
#       Copyright (C) 2015  Paul J. Lucas
#
#       This program is free software; you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation; either version 2 of the Licence, or
#       (at your option) any later version.
#
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#
#       You should have received a copy of the GNU General Public License
#       along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

# Uncomment the following line for shell tracing.
#set -x

########## Functions ##########################################################

local_basename() {
  ##
  # Autoconf, 11.15:
  #
  # basename
  #   Not all hosts have a working basename. You can use expr instead.
  ##
  expr "//$1" : '.*/\(.*\)'
}

pass() {
  print_result PASS $TEST_NAME
  {
    echo ":test-result: PASS"
    echo ":copy-in-global-log: no"
  } > $TRS_FILE
}

fail() {
  print_result FAIL $TEST_NAME
  {
    echo ":test-result: FAIL"
    echo ":copy-in-global-log: yes"
  } > $TRS_FILE
  exit $ACTUAL_EXIT
}

print_result() {
  RESULT=$1; shift
  MSG="$RESULT: $*"
  COLOR=`eval echo \\$COLOR_$RESULT`
  if [ "$COLOR" ]
  then echo $COLOR$MSG$COLOR_NONE
  else echo $MSG
  fi
}

usage() {
  [ "$1" ] && { echo "$ME: $*" >&2; usage; }
  cat >&2 <<END
usage: $ME --test-name=NAME --log-file=PATH --trs-file=PATH [options] TEST-COMMAND
options:
  --color-tests={yes|no}
  --enable-hard-errors={yes|no}
  --expect-failure={yes|no}
END
  exit 1
}

########## Begin ##############################################################

ME=`local_basename "$0"`

[ "$BUILD_SRC" ] || {
  echo "$ME: \$BUILD_SRC not set" >&2
  exit 2
}

########## Process command-line ###############################################

while [ $# -gt 0 ]
do
  case $1 in
  --color-tests)
    COLOR_TESTS=$2; shift
    ;;
  --color-tests=*)
    COLOR_TESTS=`expr "x$1" : 'x--color-tests=\(.*\)'`
    ;;
  --enable-hard-errors)
    ENABLE_HARD_ERRORS=$2; shift
    ;;
  --enable-hard-errors=*)
    ENABLE_HARD_ERRORS=`expr "x$1" : 'x--enable-hard-errors=\(.*\)'`
    ;;
  --expect-failure)
    EXPECT_FAILURE=$2; shift
    ;;
  --expect-failure=*)
    EXPECT_FAILURE=`expr "x$1" : 'x--expect-failure=\(.*\)'`
    ;;
  --help)
    usage
    ;;
  --log-file)
    LOG_FILE=$2; shift
    ;;
  --log-file=*)
    LOG_FILE=`expr "x$1" : 'x--log-file=\(.*\)'`
    ;;
  --test-name)
    TEST_NAME=$2; shift
    ;;
  --test-name=*)
    TEST_NAME=`expr "x$1" : 'x--test-name=\(.*\)'`
    ;;
  --trs-file)
    TRS_FILE=$2; shift
    ;;
  --trs-file=*)
    TRS_FILE=`expr "x$1" : 'x--trs-file=\(.*\)'`
    ;;
  --)
    shift
    break
    ;;
  -*)
    usage
    ;;
  *)
    break
    ;;
  esac
  shift
done

[ "$TEST_NAME" ] || usage "required --test-name not given"
[ "$LOG_FILE"  ] || usage "required --log-file not given"
[ "$TRS_FILE"  ] || usage "required --trs-file not given"
[ $# -ge 1     ] || usage "required test-command not given"
TEST=$1

########## Initialize #########################################################

if [ "$COLOR_TESTS" = yes ]
then
  COLOR_BLUE="[1;34m"
  COLOR_GREEN="[0;32m"
  COLOR_LIGHT_GREEN="[1;32m"
  COLOR_MAGENTA="[0;35m"
  COLOR_NONE="[m"
  COLOR_RED="[0;31m"

  COLOR_ERROR=$COLOR_MAGENTA
  COLOR_FAIL=$COLOR_RED
  COLOR_PASS=$COLOR_GREEN
  COLOR_SKIP=$COLOR_BLUE
  COLOR_XFAIL=$COLOR_LIGHT_GREEN
  COLOR_XPASS=$COLOR_RED
fi

case $EXPECT_FAILURE in
yes) EXPECT_FAILURE=1 ;;
  *) EXPECT_FAILURE=0 ;;
esac

##
# The automake framework sets $srcdir. If it's empty, it means this script was
# called by hand, so set it ourselves.
##
[ "$srcdir" ] || srcdir="."

DATA_DIR=$srcdir/data
EXPECTED_DIR=$srcdir/expected
TEST_NAME=`local_basename "$TEST_NAME"`
OUTPUT=/tmp/ad_test_output_$$_

########## Run test ###########################################################

run_sh_file() {
  if $TEST $OUTPUT $LOG_FILE
  then pass
  else fail
  fi
}

run_test_file() {
  IFS='|' read COMMAND OPTIONS INPUT USE_OUTFILE EXPECTED_EXIT < $TEST
  COMMAND=`echo $COMMAND`               # trims whitespace
  INPUT=$DATA_DIR/`echo $INPUT`         # trims whitespace
  USE_OUTFILE=`echo $USE_OUTFILE`       # trims whitespace
  EXPECTED_EXIT=`echo $EXPECTED_EXIT`   # trims whitespace

  if [ "$USE_OUTFILE" ]
  then
    #echo $COMMAND "$OPTIONS" $INPUT $OUTPUT
    $COMMAND $OPTIONS $INPUT $OUTPUT > $LOG_FILE 2>&1
  else
    #echo $COMMAND "$OPTIONS" $INPUT \> $OUTPUT
    $COMMAND $OPTIONS $INPUT > $OUTPUT 2> $LOG_FILE
  fi
  ACTUAL_EXIT=$?

  if [ $ACTUAL_EXIT -eq 0 ]
  then                                  # success: diff output file
    if [ 0 -eq $EXPECTED_EXIT ]
    then
      EXPECTED_TXT="$EXPECTED_DIR/`echo $TEST_NAME | sed 's/test$/txt/'`"
      EXPECTED_BIN="$EXPECTED_DIR/`echo $TEST_NAME | sed 's/test$/bin/'`"
      if [ -f $EXPECTED_TXT ]
      then EXPECTED_OUTPUT=$EXPECTED_TXT
      else EXPECTED_OUTPUT=$EXPECTED_BIN
      fi
      if diff $EXPECTED_OUTPUT $OUTPUT > $LOG_FILE
      then pass; mv $OUTPUT $LOG_FILE
      else fail
      fi
    else
      fail
    fi
  else                                  # failure: expected exit status?
    if [ $ACTUAL_EXIT -eq $EXPECTED_EXIT ]
    then pass
    else fail
    fi
  fi
}

##
# Must put BUILD_SRC first in PATH so we get the correct version of ad.
##
PATH=$BUILD_SRC:$PATH

##
# Must unset these so we get the default colors in test output.
##
unset AD_COLORS GREP_COLOR GREP_COLORS

trap "x=$?; rm -f /tmp/*_$$_* 2>/dev/null; exit $x" EXIT HUP INT TERM

case $TEST in
*.sh)   run_sh_file ;;
*.test) run_test_file ;;
esac

# vim:set et sw=2 ts=2:
