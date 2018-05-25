#!/bin/bash

FNAMEFULL=$1
CUTOFF=$2
K=$3
BASENAME=`basename $1`
FNAME="${BASENAME%.*}"
echo PreProcess --in=$1 --out=${FNAME}.bin --cutoff=${CUTOFF} --k=${K} || error_exit "fail while converting kmer"
 PreProcess --in=$1 --out=${FNAME}.bin --cutoff=${CUTOFF} --k=${K} || error_exit "fail while converting kmer"

