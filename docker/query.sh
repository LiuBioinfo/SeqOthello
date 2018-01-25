#!/bin/sh

BASENAME="${0##*/}"
PATH="/bin:/usr/bin:/sbin:/usr/sbin:/usr/local/bin:/usr/local/sbin"

#build_seqothello.sh s3://seqothellotest/grp_c_10/ s3://seqothellotest/grp/0test.Grplist s3://seqothellotest/map_10/
usage () {
  if [ "${#@}" -ne 0 ]; then
    echo "* ${*}"
    echo
  fi
  cat <<ENDUSAGE
Usage:

S3_SEQOTHELLO_PATH=\$1
S3_TRANSCRIPT=\$2
S3_RESULT=\$2


ENDUSAGE

  exit 2
}

# Standard function to print an error and exit with a failing return code
error_exit () {
  echo "${BASENAME} - ${1}" >&2
}

if [ -z "$3" ]; then
    usage ""
fi

S3_SEQOTHELLO_PATH=$1
S3_TRANSCRIPT=$2
S3_RESULT=$3
QTHREAD=$4
S3_MEASURE_OUTPUT=$5

if [ -z "${QTHREAD}" ]; then
QTHREAD=4
fi

TMPDIR="$(mktemp -d -t tmp.XXXXXXXXX)" || error_exit "Failed to create temp directory."

MAPNAMEFULL=`basename ${S3_SEQOTHELLO_PATH}`
MAPNAME="${MAPNAMEFULL%.*}"

#downloading all files
echo MAPNAME  $MAPNAME
#aws s3 sync ${S3_SEQOTHELLO_PATH} ${TMPDIR}
aws s3 ls ${S3_SEQOTHELLO_PATH} | cut -c32- > ${TMPDIR}/flist
parallel aws s3 cp ${S3_SEQOTHELLO_PATH}{1} ${TMPDIR}/ :::: ${TMPDIR}/flist
aws s3 cp ${S3_TRANSCRIPT} ${TMPDIR}/transcript
CMD="Query --map-folder="${TMPDIR}"/ --transcript="${TMPDIR}"/transcript --qthread="${QTHREAD}" --output "${TMPDIR}"/result"
python /usr/local/bin/runmeasure.py --cmd  """${CMD}""" --log ${TMPDIR}/runlog 2>${TMPDIR}/timelog 
tar czf ${TMPDIR}/result.tar.gz ${TMPDIR}/result
if [ "${S3_MEASURE_OUTPUT}" ]; then
    aws s3 cp ${TMPDIR}/timelog ${S3_MEASURE_OUTPUT}${MAPNAME}.${QTHREAD}.timelog
    aws s3 cp ${TMPDIR}/runlog ${S3_MEASURE_OUTPUT}${MAPNAME}.${QTHREAD}.runlog
fi

aws s3 cp ${TMPDIR}/result.tar.gz ${S3_RESULT}${MAPNAME}.${QTHREAD}.tar.gz
rm -rf ${TMPDIR}
