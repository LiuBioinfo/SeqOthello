#!/bin/bash

mkdir -p tmp/kmer_bins
ls -p tmp/kmers/experiment_a*.kmer | xargs -n 1 basename | cut -d. -f1 |
while IFS=' ' read -r exp
do
    ../build/bin/PreProcess \
    --k=21 \
    --cutoff=1 \
    --in=tmp/kmers/${exp}.kmer \
    --out=tmp/kmer_bins/${exp}.bin

done
