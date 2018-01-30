#!/bin/bash

mkdir -p tmp/kmer_bins

while IFS=' ' read -r exp
do
    ../build/bin/PreProcess \
    --k=21 \
    --cutoff=1 \
    --in=tmp/kmers/${exp}.kmer \
    --out=tmp/kmer_bins/${exp}.bin

done < experiments_list.10.txt
