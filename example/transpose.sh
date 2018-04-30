#!/bin/bash
sed $1 -e 's/$/\^/g' | fold -w1 | paste -sd' ' | sed 's/\^/\n/g' | sed "1 s|^| |"  | sed '$ d'  | datamash transpose -t' ' 
