#!/bin/bash
for i in 0 1 2; do 
    head -n 1024 tmp/kmers/experiment_0$i.kmer > tmp/kmers/experiment_a$i.kmer;
done

