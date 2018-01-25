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

if [ -z "$3" ]; then
    usage ""
fi

S3_SRC_PREFIX=$1
S3_DEST=$2
S3_GRP_DESCRIPTION=$3

GRPNAMEFULL=`basename ${S3_GRP_DESCRIPTION}`
GRPNAME="${GRPNAMEFULL%.*}"

#downloading all files
echo GRPNAME $GRPNAME
echo GRPNAMEFULL $GRPNAMEFULL
aws s3 cp ${S3_GRP_DESCRIPTION} ${TMPDIR}/${GRPNAME}.GrpDescripton || error_exit "exit"

while read -r i; do
     aws s3 cp ${S3_SRC_PREFIX}${i}.10.bin ${TMPDIR}/${i}.10.bin || error_exit "exit"
     aws s3 cp ${S3_SRC_PREFIX}${i}.10.bin.xml ${TMPDIR}/${i}.10.bin.xml || error_exit "exit"
     echo ${i}.10.bin >> ${TMPDIR}/${GRPNAME}.SeqOGroup
done < ${TMPDIR}/${GRPNAME}.GrpDescripton

echo Running Group --flist=${TMPDIR}/${GRPNAME}.SeqOGroup --folder=${TMPDIR}/ --output=${TMPDIR}/${GRPNAME}.Grp
Group --flist=${TMPDIR}/${GRPNAME}.SeqOGroup --folder=${TMPDIR}/ --output=${TMPDIR}/${GRPNAME}.Grp || error_exit "Error while grouping"

aws s3 cp ${TMPDIR}/${GRPNAME}.Grp ${S3_DEST}${GRPNAME}.Grp || error_exit "exit"
aws s3 cp ${TMPDIR}/${GRPNAME}.Grp.xml ${S3_DEST}${GRPNAME}.Grp.xml || error_exit "exit"
TMPDIR="$(mktemp -d -t tmp.XXXXXXXXX)" || error_exit "Failed to create temp directory."

rm -rf ${TMPDIR}
