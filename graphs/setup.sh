#!/bin/bash

ROCKETFUEL_ISPMAPS="http://research.cs.washington.edu/networking/rocketfuel/maps/rocketfuel_maps_cch.tar.gz"
ROCKETFUEL_WEIGTHS="http://research.cs.washington.edu/networking/rocketfuel/maps/weights-dist.tar.gz"
DATA_DIR="data/rocketfuel"

function help()
{
cat <<EOF
 deps : install required dependencies
 data : download rocketfuel data
EOF
}

function install_dependencies()
{
    apt-get install python-scipy python-numpy python-networkx
    pip install fnss
}
            
function install_data()
{
    mkdir -p $DATA_DIR/maps
    FILE=$(basename $ROCKETFUEL_ISPMAPS)
    test -f $DATA_DIR/$FILE || wget $ROCKETFUEL_ISPMAPS -P $DATA_DIR
    tar xf $DATA_DIR/$FILE -C $DATA_DIR/maps

    mkdir -p $DATA_DIR/weights
    FILE=$(basename $ROCKETFUEL_WEIGTHS)
    test -f $DATA_DIR/$FILE || wget $ROCKETFUEL_WEIGTHS -P $DATA_DIR
    tar xf $DATA_DIR/$FILE -C $DATA_DIR/weights
}

function main()
{
    command=$1

    case $command in 
        help)
            help
            ;;

        data)
            install_data
            ;;
        deps)
            install_dependencies
            ;;
        *)
            echo "Configuring paths"
            export R_INCLUDE_DIR="$(pwd)/../common/"
            echo
            echo "Additional setup commands:"
            help
            ;;
    esac
}

main $1
