#!/bin/bash

# Experiments configurations
SEQOTHELLO="../build/bin"
THREADS=8
KMER_LEN=21
FQ_DIR="fq"
EXP_LIST="experiments_list.10.txt"
KMER_DIR="kmers"
KMER_COUNT_TH=1
TMP='tmp'
BINS_DIR="${TMP}/kmer_bins"
GRP_DIR="${TMP}/grp"
MAP_DIR="map"
N_PER_GROUPS=10

if [ -d ${TMP} ]
	then
	rm -rf ${TMP}
fi

mkdir -p ${KMER_DIR} ${BINS_DIR} ${GRP_DIR} ${MAP_DIR}


echo "1.Generating k-mer file for each experiment, might takes a while..."
while IFS= read -r exp
do
	if [ -e ${KMER_DIR}/${exp}.${KMER_COUNT_TH}.kmer ]
	then
		echo "	$exp kmer file exists, will skip"
	else
		echo "	process $exp"
		jellyfish count -m 21 -s 1000M -C -t ${THREADS} \
		-o ${KMER_DIR}/${exp}.jf \
		${FQ_DIR}/${exp}.R1.fastq ${FQ_DIR}/${exp}.R2.fastq

		jellyfish dump -t -L ${KMER_COUNT_TH} \
		-c ${KMER_DIR}/${exp}.jf \
		-o ${KMER_DIR}/${exp}.${KMER_COUNT_TH}.kmer
		rm ${KMER_DIR}/${exp}.jf
	fi

done < ${EXP_LIST}

echo "2.Converting k-mer files to binary"
while IFS= read -r exp
do
	${SEQOTHELLO}/PreProcess --k=${KMER_LEN} \
	--cutoff=${KMER_COUNT_TH} \
	--in=${KMER_DIR}/${exp}.${KMER_COUNT_TH}.kmer \
	--out=${BINS_DIR}/${exp}.bin \



done < ${EXP_LIST}

echo "3. Create Group files"


sed "s/$/\.bin/g" ${EXP_LIST} | \
split -d -l ${N_PER_GROUPS} - ${TMP}/binary_list.part

for blist in $(ls -m1 ${TMP}/binary_list.part*); do
	${SEQOTHELLO}/Group --flist=${blist} \
	--folder=${BINS_DIR}/ \
	--output=${GRP_DIR}/Grp_"${blist##*.}"

	echo Grp_${blist##*.} >> ${TMP}/grp_list
done

echo "4. Build SeqOthello"
${SEQOTHELLO}/Build \
--flist=${TMP}/grp_list \
--folder=${GRP_DIR}/ \
--out-folder=${MAP_DIR}/

echo "5. Query transcripts"

${SEQOTHELLO}/Query \
--map-folder=${MAP_DIR}/ \
--transcript=transcripts.fa \
--output=query_results.txt

