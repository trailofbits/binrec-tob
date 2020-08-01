set -e  # fail on error

# these can be overwritten in files sourcing this file
additional_nargs=${additional_nargs-0}
additional_argsdesc=${additional_argsdesc-}

usage() {
    echo -n "usage: bash $0 [-h] [-d DEBUG_LEVEL] [-l LOG_LEVEL] [-a] [-t] "
    echo "[-f FALLBACK_MODE] [-n NO_LINK_LIFT] [--] IN.BC OUT.BC $additional_argsdesc"
    exit $1
}

debug_level=1
always_flush=0
time_passes=0
log_level=debug
fallback_mode=none
no_link_lift=0

while getopts "hd:l:atf:n:" opt; do
    case $opt in
    h)  usage 0;;
    \?) usage 1;;
    d)  debug_level=$OPTARG;;
    l)  log_level=$OPTARG;;
    a)  always_flush=1;;
    t)  time_passes=1;;
    f)  fallback_mode=$OPTARG;;
    n)  no_link_lift=$OPTARG;;
    esac
done

shift $((OPTIND - 1))
[ $# -lt $((2 + $additional_nargs)) ] && usage 1

get_options() {
    echo -n "-d $debug_level "
    echo -n "-l $log_level "
    [ $always_flush -eq 1 ] && echo -n "-a "
    [ $time_passes -eq 1 ] && echo -n "-t "
    echo -n "-f $fallback_mode "
    echo -n "-n $no_link_lift"
}

infile=$1
outfile=$2
shift 2
s2e_outdir=$(dirname $(realpath -- $infile))
