#!/bin/bash
aws s3 cp s3://seqothellotest/bin_c_10/$1.histo $1.histo
CNT10=`awk -F',' '{ SUM += $2 } END { print SUM }' $1.histo`
CNT15=`awk -F',' '{ if ($1>=15) SUM += $2; } END { print SUM }' $1.histo`
CNT20=`awk -F',' '{ if ($1>=20) SUM += $2; } END { print SUM }' $1.histo`
CNT25=`awk -F',' '{ if ($1>=25) SUM += $2; } END { print SUM }' $1.histo`
CNT30=`awk -F',' '{ if ($1>=30) SUM += $2; } END { print SUM }' $1.histo`
CNT40=`awk -F',' '{ if ($1>=40) SUM += $2; } END { print SUM }' $1.histo`
CNT50=`awk -F',' '{ if ($1>=50) SUM += $2; } END { print SUM }' $1.histo`
echo $CNT10 $CNT15 $CNT20 $CNT25 $CNT30 $CNT40 $CNT50
TMPFILE=`mktemp`
for i in 10 15 20 25 30 40 50; do 
    echo '<Root>' > ${TMPFILE}.$i
done
echo '    <SampleInfo KmerFile="'$1'-kmer-s3" KmerLength="21" BinaryFile="'$1'.bin" KmerCount="'${CNT10}'" Cutoff="10" MinExpressionInKmerFile="10" UniqueKmerCount="'${CNT10}'"/>' >> ${TMPFILE}.10
echo '    <SampleInfo KmerFile="'$1'-kmer-s3" KmerLength="21" BinaryFile="'$1'.bin" KmerCount="'${CNT15}'" Cutoff="15" MinExpressionInKmerFile="15" UniqueKmerCount="'${CNT15}'"/>' >> ${TMPFILE}.15
echo '    <SampleInfo KmerFile="'$1'-kmer-s3" KmerLength="21" BinaryFile="'$1'.bin" KmerCount="'${CNT20}'" Cutoff="20" MinExpressionInKmerFile="20" UniqueKmerCount="'${CNT20}'"/>' >> ${TMPFILE}.20
echo '    <SampleInfo KmerFile="'$1'-kmer-s3" KmerLength="21" BinaryFile="'$1'.bin" KmerCount="'${CNT25}'" Cutoff="25" MinExpressionInKmerFile="25" UniqueKmerCount="'${CNT25}'"/>' >> ${TMPFILE}.25
echo '    <SampleInfo KmerFile="'$1'-kmer-s3" KmerLength="21" BinaryFile="'$1'.bin" KmerCount="'${CNT30}'" Cutoff="30" MinExpressionInKmerFile="30" UniqueKmerCount="'${CNT30}'"/>' >> ${TMPFILE}.30
echo '    <SampleInfo KmerFile="'$1'-kmer-s3" KmerLength="21" BinaryFile="'$1'.bin" KmerCount="'${CNT40}'" Cutoff="40" MinExpressionInKmerFile="40" UniqueKmerCount="'${CNT40}'"/>' >> ${TMPFILE}.40
echo '    <SampleInfo KmerFile="'$1'-kmer-s3" KmerLength="21" BinaryFile="'$1'.bin" KmerCount="'${CNT50}'" Cutoff="50" MinExpressionInKmerFile="50" UniqueKmerCount="'${CNT50}'"/>' >> ${TMPFILE}.50
for i in 10 15 20 25 30 40 50; do 
    echo '</Root>' >> ${TMPFILE}.$i
    aws s3 cp ${TMPFILE}.$i s3://seqothellotest/bin_c_$i/$1.bin.xml
done

