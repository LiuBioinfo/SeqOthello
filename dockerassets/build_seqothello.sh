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

S3_GRP_PREFIX=\$1
S3_GRP_LIST=\$2
S3_DEST=\$3
S3_MEASURE_OUTPUT=\$4
ENDUSAGE

  exit 2
}

TMPDIR="$(mktemp -d -t tmp.XXXXXXXXX)" || error_exit "Failed to create temp directory."

# Standard function to print an error and exit with a failing return code
error_exit () {
  echo "${BASENAME} - ${1}" >&2
          exit 1
}

if [ -z "$3" ]; then
    usage ""
fi

S3_GRP_PREFIX=$1
S3_GRP_LIST=$2
S3_DEST=$3
S3_MEASURE_OUTPUT=$4

scheme="$(echo "${S3_GRP_LIST}" | cut -d: -f1)"
if [ "${scheme}" != "s3" ]; then
  usage "S3_GRP_LIST must be for an S3 object; expecting URL starting with s3://"
fi

scheme="$(echo "${S3_GRP_PREFIX}" | cut -d: -f1)"
if [ "${scheme}" != "s3" ]; then
  usage "S3_GRP_PREFIX must be a folder containing the grp files; expecting URL starting with s3://"
fi

scheme="$(echo "${S3_DEST}" | cut -d: -f1)"
if [ "${scheme}" != "s3" ]; then
  usage "S3_DEST must be a valid S3 prefix; expecting URL starting with s3://"
fi

if [ "$4" ]; then
scheme="$(echo "${S3_MEASURE_OUTPUT}" | cut -d: -f1)"
if [ "${scheme}" != "s3" ]; then
  usage "S3_MEASURE_OUTPUT must be a valid S3 prefix; expecting URL starting with s3://"
fi
fi


MAPNAMEFULL=`basename ${S3_GRP_LIST}`
MAPNAME="${MAPNAMEFULL%.*}"

#downloading all files
echo MAPNAME  $MAPNAME
echo S3_MEASURE_OUTPUT  $4
echo S3_MEASURE_OUTPUT  $S3_MEASURE_OUTPUT
aws s3 cp ${S3_GRP_LIST} ${TMPDIR}/${MAPNAME}.GrpList


if grep -q "grp" ${TMPDIR}/${MAPNAME}.GrpList
then
  parallel aws s3 cp ${S3_GRP_PREFIX}{1} ${TMPDIR}/{1} :::: ${TMPDIR}/${MAPNAME}.GrpList || error_exit "failed to download"
  parallel aws s3 cp ${S3_GRP_PREFIX}{1}.xml ${TMPDIR}/{1}.xml :::: ${TMPDIR}/${MAPNAME}.GrpList || error_exit "failed to download from s3"
  cp ${TMPDIR}/${MAPNAME}.GrpList ${TMPDIR}/${MAPNAME}.SeqOGroups
else
  parallel aws s3 cp ${S3_GRP_PREFIX}{1}.Grp ${TMPDIR}/{1}.Grp :::: ${TMPDIR}/${MAPNAME}.GrpList || error_exit "failed to download"
  parallel aws s3 cp ${S3_GRP_PREFIX}{1}.Grp.xml ${TMPDIR}/{1}.Grp.xml :::: ${TMPDIR}/${MAPNAME}.GrpList || error_exit "failed to download from s3"
  while read -r i; do
     echo ${i}.Grp >> ${TMPDIR}/${MAPNAME}.SeqOGroups
  done < ${TMPDIR}/${MAPNAME}.GrpList
fi
#head -n 1 /tmp/${MAPNAME}.SeqOGroups > /tmp/${MAPNAME}.SeqOGroups.only1
mkdir ${TMPDIR}/mapOut.${MAPNAME}


if [ -z "$4" ]; then
    Build --flist=${TMPDIR}/${MAPNAME}.SeqOGroups --folder=${TMPDIR}/ --out-folder=${TMPDIR}/mapOut.${MAPNAME}/ || error_exit "failed during building Seqothello";
else
    CMD="Build --flist="${TMPDIR}"/"${MAPNAME}".SeqOGroups --folder="${TMPDIR}"/ --out-folder="${TMPDIR}"/mapOut."${MAPNAME}"/"
    echo "Running command" ${CMD} 
    python /usr/local/bin/runmeasure.py --cmd  """${CMD}""" --log ${TMPDIR}/runlog 2>${TMPDIR}/timelog 
    aws s3 cp ${TMPDIR}/timelog ${S3_MEASURE_OUTPUT}${MAPNAME}.timelog
    aws s3 cp ${TMPDIR}/runlog ${S3_MEASURE_OUTPUT}${MAPNAME}.runlog
fi

#for i in `ls /tmp/mapOut.${MAPNAME}/`; do 
#    echo aws s3 cp /tmp/${MAPNAME}/$i ${S3_DEST}`basename $i`
#    aws s3 cp /tmp/${MAPNAME}/$i ${S3_DEST}`basename $i`
#done;
parallel aws s3 cp ${TMPDIR}/mapOut.${MAPNAME}/{1} ${S3_DEST}${MAPNAME}/`basename {1}` ::: `ls ${TMPDIR}/mapOut.${MAPNAME}/` || error_exit "failed during upload"

rm -rf ${TMPDIR}
