#!/bin/bash
#
# scenario > simulation > round

if [[ $RUNS == "" ]]; then
  RUNS=1
  echo "RUNS undefined, default to $RUNS"
fi

#QUIET="--quiet=true "
QUIET=""

OVERWRITE=false
MAX_PROCESS=$(grep -c ^processor /proc/cpuinfo)
DEFAULT_NDNSIM_DIR="$HOME/bin"

R_PATH="data" # This has currently to be in sync with the simulation file
T_PATH="tmp"
S_PATH="scripts"
P_PATH="plots"

if [[ $process_round == "" ]]; then
	function process_round() { echo; }
fi
if [[ $graph_round == "" ]]; then
	function graph_round() { echo; }
fi
if [[ $graph == "" ]]; then
	function graph() { echo; }
fi
if [[ $process_scenario == "" ]]; then
	function process_scenario() { echo; }
fi
export -f process_round graph_round graph process_scenario 

export DEFAULT_NDNSIM_DIR RUNS R_PATH T_PATH S_PATH P_PATH

################################################################################
# SCENARIOS
#
# Scenarios consist in 4 steps... TODO
# We need dependency resolution for the graphs
################################################################################

# parameter from filename
function param()
{
    echo $1 | sed "s/.*-$2\([^-]*\).*/\1/"
}

function split_cmdline() {
    cmdline=$1

    while read key value; do
        arr+=(["$key"]="$value")
    done < <(awk -F' ' '{for(i=1;i<=NF;i++) {print $i}}' <<< $cmdline | awk -F'=' '{print $1" "$2}')
}
export -f split_cmdline


################################################################################
# HELPERS
################################################################################

# \brief Run enough rounds to measure statistics (currently 10)
# \param scenario (string) : scenario filename
# \params params (string) : commandline parameters for the simulation scenario
#
# A round consists in
#   1) producing simulation results
#   2) postprocessing them
#   3) plotting graphs

function simulation()
{
    local scenario=$1
    local params=$2

    echo "[$scenario] with params [$params]..."

    declare -A arr
    split_cmdline "$params"
    scheme=${arr['--mobility-scheme']}
    flavour=default
    # if [[ $scheme == 'mapme' ]]; then
    #     # flavour=mapme
    # elif [[ $scheme == 'mapmein' ]]; then
    #     # flavour=mapme
    # elif [[ $scheme == 'oldmapme' ]]; then
    #     # flavour=oldmapme
    # elif [[ $scheme == 'oldmapmein' ]]; then
    #     # flavour=oldmapme
    # else
    #     # flavour=vanilla
    # fi 

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

    BASE="$NDNSIM_DIR/root/$flavour"
    if [[ ! -d $BASE ]]; then
        >&2 echo "Base directory for ndnSIM not found [$BASE]. Please check your installation."
        exit
    fi
    export LD_LIBRARY_PATH=$BASE/lib
    export PKG_CONFIG_PATH=$BASE/lib/pkgconfig
    export PYTHONDIR=$BASE/usr/lib/python$PYVER/dist-packages
    export PYTHONARCHDIR=$BASE/usr/lib/python$PYVER/dist-packages
    echo $LD_LIBRARY_PATH

    # Detect mobility scheme to set environment variables accordingly
    # XXX

    if [[ ! -d "$R" || ! -f "$R/done" || "$proceed" == "true" ]]; then
        # Create all output directories for the current run
        # NOTE: We retrieve the output directory name from the simulation. This
        # allows up to have a simulation script that can be run indepdently from
        # this script.
        export RX=$(./waf --run "$scenario $params $QUIET --output-only" | awk '/OUTPUT/{print $2}')
        export TX=$RX # ${RX/$R_PATH/$T_PATH}
        export PX=${RX/$R_PATH/$P_PATH}
        export S="$S_PATH"
        mkdir -p $RX $TX $PX

        for run in $(seq 1 $RUNS); do
            local args="$params --RngRun=$run"

            echo " - Round #$run/$RUNS"

            # force overwrite in further steps
            local proceed="$OVERWRITE "

            export R="$RX/$run"
            export T="$TX/$run"
            export P="$PX/$run"
            mkdir -p $R $T $P

            # Unless we want to overwrite, we skip already done steps
            # 1)
            if [[ ! -f $R/done || "$proceed" == "true" ]]; then
                echo "    . Simulating... $R"
                rm -Rf $R && mkdir -p $R
                ./waf --run "$scenario $args $QUIET"
                touch "$R/done"
                proceed=true
            fi

            # Process round
            if [[ ! -f $T/donetmp || "$proceed" == "true" ]]; then
                echo "    . Processing $T"
                #rm -Rf $T && mkdir -p $T # XXX $T same as $R !!!
                process_round
                touch "$T/donetmp"
                proceed=true
            fi

            # Graph
            if [[ ! -f $P/done || "$proceed" == "true" ]]; then
                echo "    . Plotting"
                rm -Rf $P && mkdir -p $P
                graph_round
                touch "$P/done"
                proceed=true
            fi
        done
    fi

    if [[ "$proceed" == true ]]; then
        process_scenario
        proceed=true
    fi

    # 4) plotting graphs
    if [[ "$proceed" == true ]]; then
        graph
    fi
    
}

function display_help()
{
    prog=$1; msg=$2
    if [ -n "$msg" ]; then echo "Error: $msg"; echo; exit -1; fi
    cat <<EOF
$prog [SCENARIO]

TODO
EOF
}
 
function parse_cmdline()
{
    while getopts ":ah" opt; do
      case $opt in
        a)
          echo "-a was triggered!" >&2
          OVERWRITE=true
          ;;

        \?)
          echo "Invalid option: -$OPTARG" >&2
          ;;

        h)
        #-h | --help)
          display_help
          exit 0
          ;;

        :)
          echo "Option -$OPTARG requires an argument." >&2
          exit 1
                                ;;

        *) echo "Internal error!" ; exit 1 ;;
      esac
    done
    shift $(expr $OPTIND - 1 )

    #[[ ($# -eq 1 || ($# -eq 2 && $2 == <glob pattern>)) && $1 =~ <regex pattern> ]]
    #if [[ $# -ne 1 ]]; then
    #    display_help $0 "Too many positional arguments"
    #    exit 1
    #fi

    export -f simulation param
}

function run()
{
    # XXX ensure necessary variables are defined
    # SCENARIO
    # DEFAULT_ARGS
    # ARGS

    #setup
    parse_cmdline

    if [[ $1 == "" ]]; then
        DATA="./data"
    else
        DATA=$1
    fi

    S="scripts/"

    # Run simulations in parallel (Note that the simulation function has been exported)
    # http://coldattic.info/shvedsky/pro/blogs/a-foo-walks-into-a-bar/posts/7
    # http://stackoverflow.com/questions/17307800/how-to-run-given-function-in-bash-in-parallel
    export process_round graph_round process_scenario graph
    printf "%s\n" "${SIMULATION_ARGS[@]}" | xargs -P $MAX_PROCESS -I PLACEHOLDER bash -c "simulation $SCENARIO \"--data_path=$DATA $SCENARIO_ARGS PLACEHOLDER\""

    P="plots/"
    graph
}

export -f run
