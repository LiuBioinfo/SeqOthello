#!/bin/sh

BASENAME="${0##*/}"
PATH="/bin:/usr/bin:/sbin:/usr/sbin:/usr/local/bin:/usr/local/sbin"


usage () {
  if [ "${#@}" -ne 0 ]; then
    echo "* ${*}"
    echo
  fi
  cat <<ENDUSAGE
Usage:

S3_SRC=\$1
S3_DEST=\$2
CUTOFF=\$3
K=\$4

convert a jellyfish-generated file located at S3_SRC to binary file and put it in S3_DEST
ENDUSAGE

  exit 2
}

TMPDIR="$(mktemp -d -t tmp.XXXXXXXXX)" || error_exit "Failed to create temp directory."
# Standard function to print an error and exit with a failing return code
error_exit () {
  echo "${BASENAME} - ${1}" >&2
  rm -rf ${TMPDIR}
}

if [ -z "$2" ]; then
    usage ""
fi

error_exit () {
  echo "${BASENAME} - ${1}" >&2
  exit 1
}

S3_SRC=$1
S3_DEST=$2
CUTOFF=$3
K=$4

FNAMEFULL=`basename ${S3_SRC}`
FNAME="${FNAMEFULL%.*}"
echo $FNAMEFULL
echo $FNAME
aws s3 cp ${S3_SRC} ${TMPDIR}/${FNAME}.kmer || error_exit "Failed to download jellyfish output "${FNAME}" from amazon S3"
PreProcess --in=${TMPDIR}/${FNAME}.kmer --out=${TMPDIR}/${FNAME}.bin --cutoff=${CUTOFF} --k=${K} || error_exit "fail while converting kmer"
aws s3 cp ${TMPDIR}/${FNAME}.bin.xml ${S3_DEST}${FNAME}.bin.xml || error_exit "fail uploading"${FNAME}".bin.xml"
PreProcess --in=${TMPDIR}/${FNAME}.kmer --out=${TMPDIR}/${FNAME}.histo --k=${K} --histogram || error_exit "fail while converting kmer"
aws s3 cp ${TMPDIR}/${FNAME}.bin ${S3_DEST}${FNAME}.bin || error_exit "fail uploading"
aws s3 cp ${TMPDIR}/${FNAME}.histo ${S3_DEST}${FNAME}.histo || error_exit "fail uploading"
rm -rf ${TMPDIR}

