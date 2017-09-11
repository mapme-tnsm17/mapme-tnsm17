#!/bin/bash
#
# Defines required environment variables for running simulations against a
# ndnSIM installation, and accessing common files. It basically configures and
# builds the scenarios, and list a couples of environment variables to export to
# link against the right ndnSIM library.
#
# Input:
#    ENV: NDNSIM_DIR (optional), defaults to $HOME/bin
#
# Usage:
# $(./setup.sh [MODE [FLAVOUR]])
#
# MODE : optimized / debug
# FLAVOUR : vanilla / mapme
#

DEFAULT_NDNSIM_DIR="$HOME/bin"
DEFAULT_SCENARIO=synthetic

# See .waf-1.8.12-*/waflib/Utils.py, function lib64()
ARCH=$(test -d /usr/lib64 -a ! -d /usr/lib32 && echo "64" || echo "")

NDNSIM_DIR=${NDNSIM_DIR:-$DEFAULT_NDNSIM_DIR}

# Previous versions used to be incompatible with GCC5, which should be solved now
# export CXX="g++-4.9"

################################################################################
# Internals
################################################################################

function help()
{
  echo
}

################################################################################
# Command line parsing
################################################################################

cmd="${1:-optimized}"
flavour="${2:-mapme}"

>&2 echo "I: cmd=$cmd flavour=$flavour"

if [[ "$NDNSIM_DIR" == "" ]]; then
  >&2 echo "I: No NDNSIM_DIR environment variable. Defaulting to $DEFAULT_NDNSIM_DIR"
  NDNSIM_DIR=$DEFAULT_NDNSIM_DIR
fi

if [[ ! -d $NDNSIM_DIR ]]; then
  >&2 cat <<EOF
E: ndnSIM does not seem installed in $NDNSIM_DIR. Please set up the
   NDNSIM_DIR environment variable if you have installed it in another
   location
EOF
  exit
fi

case $cmd in
    debug)
        mode_flag=" --debug"
        ;;
    optimized | release)
        mode_flag=""
        ;;

    *)
        >&2 echo "E: Invalid mode"
        exit
esac

case $flavour in
    default)
        flavour_flag=""
        ;;
    mapme)
        flavour_flag="--without-mldr --without-wldr"
        ;;
    vanilla)
        flavour_flag="--without-mapme --without-anchor --without-path-labelling --without-raaqm --without-conf-file --without-lb-strategy --without-fix-random --without-unicast-ethernet --without-bugfixes --without-cache-extensions --without-fib-extensions"
        ;;
#TODO|    full)
#TODO|        flavour_flag=""
#TODO|        ;;
#TODO|    vanilla)
#TODO|        flavour_flag=""
#TODO|        ;;
    *)
        >&2 echo "E: Invalid flavour"
        exit
esac

BASE="$NDNSIM_DIR/root/$flavour"
if [[ ! -d $BASE ]]; then
    >&2 echo "Base directory for ndnSIM not found [$BASE]. Please check your installation."
    exit
fi

>&2 echo "I: Configuring and building in $cmd mode..."
#./waf clean 1>&2
export PKG_CONFIG_PATH=$BASE/lib$ARCH/pkgconfig
./waf configure $mode_flag $flavour_flag --prefix=$BASE 1>/dev/null
if [ $? -ne 0 ]; then
  echo FAIL
  exit 1
fi

# Ensure build is ok before launching several simulations in parallel
./waf --run "$DEFAULT_SCENARIO --output-only" 1>/dev/null 2>/dev/null

PWD=$(pwd)

cat <<EOF
export LD_LIBRARY_PATH=$BASE/lib$ARCH
export PKG_CONFIG_PATH=$BASE/lib$ARCH/pkgconfig
export PYTHONPATH=$BASE/usr/lib/python2.7/dist-packages
export R_INCLUDE_DIR=$PWD/scripts/helpers/
EOF
