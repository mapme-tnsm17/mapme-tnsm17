#!/bin/bash
# Installer for the ndnSIM simulation environment
#
# A typical install on a Linux machine:
#
# 1) Install dependencies
# $ ./setup.sh deps
#
# 2) Download and build git repositories (run as a local user for a local
# install, and as root for a global install)
# $ ./setup.sh ndnsim [~/bin]

################################################################################
# PARAMETERS
################################################################################

# Installation mode
MODES="debug optimized" # debug, optimized, release
FLAVOURS="mapme" # default, mapme, vanilla
CLEAN=false

DEFAULT_DIR="$HOME/bin"
ARCH=64

################################################################################
# Command line parsing
################################################################################

# Repository subdirectory
if [[ $2 ]]; then
  DIR=$(realpath $2 || echo $2)
else
    DIR=$DEFAULT_DIR
fi

# Subdirectory for local install
BASE=$DIR/root


################################################################################
# Constants

SUDO=sudo
WAF=./waf
OS=$(grep "^ID=" /etc/os-release | cut -d "=" -f 2)
PYVER="2.7"

# XXX We might override these variables
if [[ "$USER" == "root" ]]; then
    if [[ "$OS" == "debian" ]]; then
        INSTALL_DEPENDENCIES=true
    else
        INSTALL_DEPENDENCIES=false
    fi

else
    INSTALL_DEPENDENCIES=false
fi

DEPS="git r-base-core"

NS3CONFIG=""
#--enable-examples --enable-tests"

GITHUB_CLONE_HEADER="https://github.com/"
#GITHUB_CLONE_HEADER="git@github.com:"


NDNCXX_REPO="${GITHUB_CLONE_HEADER}mapme-tnsm17/ndn-cxx.git"
NDNCXX_BRANCH="tnsm17"
NDNCXX_DIR_TMP="ndn-cxx"
NDNCXX_DIR=""
NDNCXX_DEPS="libsqlite3-dev libcrypto++-dev libboost-all-dev"

NFD_REPO="${GITHUB_CLONE_HEADER}mapme-tnsm17/NFD.git"
NFD_BRANCH="master"
NFD_DIR_TMP="NFD"
NFD_DIR=""
NFD_DEPS="libsqlite3-dev libcrypto++-dev libboost-all-dev"

PBG_REPO="${GITHUB_CLONE_HEADER}named-data-ndnSIM/pybindgen.git"
PBG_DIR="pybindgen"
PBG_DEPS=""
#PBG_DEPS="gccxml"

NS3_REPO="${GITHUB_CLONE_HEADER}mapme-tnsm17/ns-3.git"
NS3_BRANCH="master"
NS3_DIR="ns-3"
NS3_DEPS="python-pygraphviz python-pygccxml"
#NS3_DEPS="python-pygoocanvas python-pygraphviz python-pygccxml"

NDNSIM_REPO="${GITHUB_CLONE_HEADER}mapme-tnsm17/ndnsim.git"
NDNSIM_BRANCH="master"
NDNSIM_DIR="$NS3_DIR/src/ndnSIM"

SIM_REPO="${GITHUB_CLONE_HEADER}mapme-tnsm17/mapme-tnsm17.git"
SIM_BRANCH="master"
SIM_DIR="ndn-simulations"
SIM_DEPS="libxml2-dev pkg-config"

ALL_DEPS="$DEPS $NDNCXX_DEPS $NS3_DEPS $PBG_DEPS $SIM_DEPS"

################################################################################

function help
{
    echo "$0 : helper to install a full fledged ndnSIM environment"
    echo ""
    echo "Usage: $0 COMMAND [ARGUMENTS...]"
    echo ""
    echo "Available commands:"
    echo "  ndnsim"
    echo "  deps"
    echo "  simulations"
    echo "  r"
    echo "  python"
}

function install-ndn-cxx
{
    # Dependencies
    if $INSTALL_DEPENDENCIES; then
        $SUDO apt-get install $NDNCXX_DEPS
    fi

    # Source code
    test -d $DIR/$NDNCXX_DIR_TMP || git clone $NDNCXX_REPO $DIR/$NDNCXX_DIR_TMP
    ln -s $DIR/$NDNCXX_DIR_TMP/ndn-cxx/ $DIR
    pushd $DIR/$NDNCXX_DIR && git checkout $NDNCXX_BRANCH && popd

    # Build and install all flavours
    for flavour in $FLAVOURS; do
        export PKG_CONFIG_PATH="$BASE/$flavour/lib$ARCH/pkgconfig"
        export PYTHONDIR="$BASE/$flavour/usr/lib/python$PYVER/dist-packages"
        export PYTHONARCHDIR="$BASE/$flavour/usr/lib/python$PYVER/dist-packages"
        mkdir -p $PKG_CONFIG_PATH $PYTHONDIR $PYTHONARCHDIR

        case $flavour in
            default)
                FLAVOUR_OPTS=""
                ;;
            mapme)
                FLAVOUR_OPTS="--without-mldr --without-wldr"
                ;;
            vanilla)
                FLAVOUR_OPTS="--without-mapme --without-anchor --without-path-labelling --without-raaqm --without-conf-file --without-lb-strategy --without-fix-random --without-unicast-ethernet --without-bugfixes --without-cache-extensions --without-fib-extensions"

                ;;
            *)
                FLAVOUR_OPTS=""
                ;;
        esac
        if $CLEAN; then
          pushd $DIR/$NDNCXX_DIR && $WAF clean && popd
        fi
        pushd $DIR/$NDNCXX_DIR && $WAF configure $FLAVOUR_OPTS --prefix=$BASE/$flavour/ -o build/$flavour && $WAF && $WAF install && popd
        sed -i "s|/usr/local|$BASE/$flavour|" $PKG_CONFIG_PATH/libndn-cxx.pc
    done
}

function install-nfd
{
    # Dependencies
    if $INSTALL_DEPENDENCIES; then
        $SUDO apt-get install $NFD_DEPS
    fi

    # Source code
    test -d $DIR/$NFD_DIR_TMP || git clone $NFD_REPO $DIR/$NFD_DIR_TMP
    ln -s $DIR/$NFD_DIR_TMP/NFD/ $DIR
    pushd $DIR/$NFD_DIR && git checkout $NFD_BRANCH && popd

    # Build and install all flavours
    for flavour in $FLAVOURS; do
        export PKG_CONFIG_PATH="$BASE/$flavour/lib$ARCH/pkgconfig"
        export PYTHONDIR="$BASE/$flavour/usr/lib/python$PYVER/dist-packages"
        export PYTHONARCHDIR="$BASE/$flavour/usr/lib/python$PYVER/dist-packages"
        mkdir -p $PKG_CONFIG_PATH $PYTHONDIR $PYTHONARCHDIR

        case $flavour in
            default)
                FLAVOUR_OPTS=""
                ;;
            mapme)
                FLAVOUR_OPTS="--without-mldr --without-wldr"
                ;;
            vanilla)
                FLAVOUR_OPTS="--without-mapme --without-anchor --without-path-labelling --without-raaqm --without-conf-file --without-lb-strategy --without-fix-random --without-unicast-ethernet --without-bugfixes --without-cache-extensions --without-fib-extensions"
                ;;
            *)
                FLAVOUR_OPTS=""
                ;;
        esac
        if $CLEAN; then
          pushd $DIR/$NFD_DIR && $WAF clean && popd
        fi
        pushd $DIR/$NFD_DIR && $WAF configure $FLAVOUR_OPTS --prefix=$BASE/$flavour/ -o build/$flavour && $WAF && $WAF install && popd
    done
}

function install-ns3-ndnsim
{
    # Dependencies
    if $INSTALL_DEPENDENCIES; then
        $SUDO apt-get install $NS3_DEPS
    fi

    # Code
    test -d $DIR/$NS3_DIR || git clone $NS3_REPO $DIR/$NS3_DIR
    pushd $DIR/$NS3_DIR && git checkout $NS3_BRANCH && popd

    test -d $DIR/$NDNSIM_DIR || git clone $NDNSIM_REPO $DIR/$NDNSIM_DIR

    pushd $DIR/$NDNSIM_DIR && git checkout $NDNSIM_BRANCH && popd

    # Build and install all flavours
    for flavour in $FLAVOURS; do
        case $flavour in
            default)
                FLAVOUR_OPTS=""
                ;;
            mapme)
                FLAVOUR_OPTS="--without-mldr --without-wldr"
                ;;
            vanilla)
                FLAVOUR_OPTS="--without-mapme --without-anchor --without-path-labelling --without-raaqm --without-conf-file --without-lb-strategy --without-fix-random --without-unicast-ethernet --without-bugfixes --without-cache-extensions --without-fib-extensions --without-network-dynamics --without-face-up-down" # --without-globalrouting-updates"
                ;;
        esac
        for mode in $MODES; do
            export PKG_CONFIG_PATH="$BASE/$flavour/lib$ARCH/pkgconfig"
            export LD_LIBRARY_PATH="$BASE/$flavour/lib$ARCH"
            export PYTHONDIR="$BASE/$flavour/usr/lib/python$PYVER/dist-packages"
            export PYTHONARCHDIR="$BASE/$flavour/usr/lib/python$PYVER/dist-packages"
            mkdir -p $PKG_CONFIG_PATH $PYTHONDIR $PYTHONARCHDIR

            NS3OPTS="--build-profile=$mode --out=build/$flavour/$mode --prefix=$BASE/$flavour/"
            if $CLEAN; then
              # || true is needed since clean fails if ns3 was not previously # configured
              pushd $DIR/$NS3_DIR && ($WAF clean || true) && popd
            fi
            pushd $DIR/$NS3_DIR && $WAF configure $NS3CONFIG $FLAVOUR_OPTS $NS3OPTS && $WAF && $WAF install && popd

            # XXX This patch should not be needed
            ESCAPED_DIR=$(echo $DIR | sed "s/\//\\\\\//g")
            sed -i "/Cflags/s/-pthread$/& -I\${includedir}\/ns3\/ndnSIM\/NFD -I\${includedir}\/ns3\/ndnSIM\/NFD\/daemon -I\${includedir}\/ns3\/ndnSIM\/NFD\/core -I\${includedir}\/ns3\/ndnSIM -I${ESCAPED_DIR}\/ns-3\/build\/$flavour\/$mode\/ns3\/ndnSIM\/ndn-cxx/" $PKG_CONFIG_PATH/libns3-dev-ndnSIM-$mode.pc
        done
    done
}

function install-pybindgen
{
    # Dependencies
    if $INSTALL_DEPENDENCIES; then
        $SUDO apt-get install $PBG_DEPS
    fi

    # Code
    test -d $DIR/$PBG_DIR || git clone $PBG_REPO $DIR/$PBG_DIR

    # With the move to GCC5, gccxml is now a wrapper, which breaks the pybindgen wscript
    #   GCC-XML compatibility CastXML wrapper
    #   Usage: gccxml [options] <input-file> -fxml=<output-file>
    #
    #   Note: not all the gccxml options are supported.
    #   The real gccxml (not compatible with GCC 5) is available as gccxml.real.
    test -f /usr/bin/gccxml.real && export GCCXML=/usr/bin/gccxml.real

    # Build and install all flavours
    for flavour in $FLAVOURS; do
        export PKG_CONFIG_PATH="$BASE/$flavour/lib$ARCH/pkgconfig"
        export PYTHONDIR="$BASE/$flavour/usr/lib/python$PYVER/dist-packages"
        export PYTHONARCHDIR="$BASE/$flavour/usr/lib/python$PYVER/dist-packages"
        mkdir -p $PKG_CONFIG_PATH $PYTHONDIR $PYTHONARCHDIR

        # Newer versions require to run python setup.py (?) to generate
        # version.py
        pushd $DIR/$PBG_DIR &&  python setup.py build && $WAF configure --prefix=$BASE/$flavour/ && $WAF && $WAF install && popd
        #pushd $DIR/$PBG_DIR && python setup.py 1>/dev/null 2>/dev/null && $WAF configure --prefix=$BASE/$flavour/ && $WAF && $WAF install && popd

    done
}

function install-ndn-simulations
{
    echo "Structure of the repo has changed... This script need to be updated"
    exit 0
    if [[ $MODE == "debug" ]]; then
        MODE_FLAG=--debug
    else
        MODE_FLAG=
    fi

    test -d $DIR/$SIM_DIR || git clone $SIM_REPO $DIR/$SIM_DIR
    pushd $DIR/$SIM_DIR/toy-cases && $WAF configure $CFGPOST $MODE_FLAG && $WAF && popd
    if [[ $USER != "root" ]]; then
        echo ""
        echo "#!/bin/bash"                       > $DIR/$SIM_DIR/toy-cases/setup.sh
        echo "export LD_LIBRARY_PATH=$BASE/lib" >> $DIR/$SIM_DIR/toy-cases/setup.sh
        chmod +x $DIR/$SIM_DIR/toy-cases/setup.sh 
        echo "Run the following commands inside '$DIR/$SIM_DIR/toy-cases' to setup the ndn-simulation environment:"
        echo "./setup.sh"
    fi
}

#function update
#{
# pushd $DIR/$NS3_DIR    && git pull && popd
# pushd $DIR/$NDNSIM_DIR && git pull && popd
# pushd $DIR/$NDNCXX_DIR && git pull && popd
# pushd $DIR/$SIM_DIR    && git pull && popd
#}

function install_R_packages
{
  mkdir -p ~/R
    # Max allowed version for R 3.0.2 (not found by R, so manual install)

    F="/tmp/plyr_1.8.1.tar.gz"
    wget http://cran.r-project.org/src/contrib/Archive/plyr/plyr_1.8.1.tar.gz -O $F
    Rscript -e "install.packages('/tmp/plyr_1.8.1.tar.gz', repos=NULL, type='source', lib='~/R/')"
    rm $F

    Rscript -e "install.packages('ggplot2', dependencies=TRUE, repos='http://cran.univ-paris1.fr', lib='~/R/')"
    Rscript -e "install.packages('RSvgDevice', repos='http://cran.univ-paris1.fr', lib='~/R/')"
}

function install_python_packages
{
    # numpy ok
    pip install --user scipy
    pip install --user networkx
    pip install --user fnss
}

function install_ndnSIM_deps
{
    apt-get install $ALL_DEPS
}

function ndnsim_export
{
  cat <<EOF

If you installed ndnSIM in a non default location (different from
$DEFAULT_DIR), please setup this environment variable:

export NDNSIM_DIR=$DIR
EOF
}

case $1 in
    gitinfo)
        LOCAL=$(git rev-parse @)
        REMOTE=$(git rev-parse @{u})
        BASE=$(git merge-base @ @{u})

        if [ $LOCAL = $REMOTE ]; then
          echo "Up-to-date"
        elif [ $LOCAL = $BASE ]; then
          echo "Need to pull"
        elif [ $REMOTE = $BASE ]; then
          echo "Need to push"
        else
          echo "Diverged"
        fi

        if ! git diff-index --quiet HEAD --; then
            echo "Has local changes"
        fi
        ;;
    ndnsim)
        if [[ "$USER" == "root" ]]; then
            echo "Not implemented"
            exit 1
        fi
        install-pybindgen
        install-ns3-ndnsim
        ndnsim_export
        ;;
    ndn)
        if [[ "$USER" == "root" ]]; then
            echo "Not implemented"
            exit 1
        fi
        install-nfd
        install-ndn-cxx
        ;;
    ns3)
        if [[ "$USER" == "root" ]]; then
            echo "Not implemented"
            exit 1
        fi
        install-ns3-ndnsim
        ndnsim_export
        ;;
    deps)
        install_ndnSIM_deps
        ;;
    simulations)
        install-ndn-simulations
        ;;
    r)
        install_R_packages
        ;;
    python)
        install_python_packages
        ;;
    *)
        help
        ;;
esac

