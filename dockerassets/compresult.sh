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

S3_SRC_PREFIX=\$1
S3_SRC_SUFFIX=\$2
S3_DEST=\$3
S3_GRP_DESCRIPTION=\$4

The files at location ${S3_SRC_PREFIX}${FILENAME}${S3_SRC_SUFFIX} will be downloaded
the FILENAMES are defined in the file S3_GRP_DESCRIPTION

ENDUSAGE

  exit 2
}

TMPDIR="$(mktemp -d -t tmp.XXXXXXXXX)" || error_exit "Failed to create temp directory."
# Standard function to print an error and exit with a failing return code
error_exit () {
  echo "${BASENAME} - ${1}" >&2
  rm -rf ${TMPDIR}
  exit 1
}


N=198092
parallel 'aws s3 cp s3://seqothellotest/2652comp/trans.tara{1} '${TMPDIR} ::: ` for i in {a..m}; do echo $i; done `
cat ${TMPDIR}/trans.tara* | tar xf - -C ${TMPDIR}
aws s3 cp s3://seqothellotest/2652comp/$1.bin ${TMPDIR}
ls ${TMPDIR}
for i in `seq $2 $3`; do 
  echo -n $i' ';
  comp ${TMPDIR}/$1.bin ${TMPDIR}/trans/$i 2>/dev/null | tail -n 1
done > ${TMPDIR}/$1.CompResult.$2.$3

aws s3 cp ${TMPDIR}/$1.CompResult.$2.$3 s3://seqothellotest/2652compResult/

rm -rf ${TMPDIR}
