#!/bin/bash

# A POSIX variable
OPTIND=1         # Reset in case getopts has been used previously in the shell.

# Initialize our own variables:
output_file=""
verbose=0

while getopts "t:k:" opt; do
    case "$opt" in
    t)  q="$OPTARG"
        ;;
    k)  k="$OPTARG"
        ;;
    esac
done

shift $((OPTIND-1))

[ "${1:-}" = "--" ] && shift

parallel --jobs 100% oneBinaryWrapper.sh {1} $q $k ::: $@
# End of file

