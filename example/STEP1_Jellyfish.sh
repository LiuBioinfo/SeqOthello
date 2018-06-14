#!/bin/bash

mkdir -p tmp/kmers
while IFS=' ' read -r exp;
do
    jellyfish count -s 10M \
    -m 21 -C -t 2 -o tmp/kmers/${exp}.jf \
    fq/${exp}.R1.fastq \
    fq/${exp}.R2.fastq;
    # -s [10M] Bloom filter size used in Jellyfish. You may need to use larger values for real experiments.
    # -m 21 Length of kmers

    jellyfish dump -t -L 3 \
    -c tmp/kmers/${exp}.jf \
    -o tmp/kmers/${exp}.kmer;

#    rm tmp/kmers/${exp}.jf;
done < experiments_list.10.txt
