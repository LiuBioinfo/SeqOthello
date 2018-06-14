#!/bin/bash

# Copyright 2013-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"). You may not use this file except in compliance with the
# License. A copy of the License is located at
#
# http://aws.amazon.com/apache2.0/
#
# or in the "LICENSE.txt" file accompanying this file. This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
# OR CONDITIONS OF ANY KIND, express or implied. See the License for the specific language governing permissions and
# limitations under the License.

# This script can help you download and run a script from S3 using aws-cli.
# It can also download a zip file from S3 and run a script from inside.
# See below for usage instructions.

PATH="/bin:/usr/bin:/sbin:/usr/sbin:/usr/local/bin:/usr/local/sbin"
BASENAME="${0##*/}"

usage () {
  if [ "${#@}" -ne 0 ]; then
    echo "* ${*}"
    echo
  fi
  cat <<ENDUSAGE
Usage:

export BATCH_FILE_S3_URL="s3://my-bucket/my-script"
export TOTAL_JOBS=N
export JOB_CPU_PERCENTILE=100
The folowing procedure will be executed in this job:
    download the file from BATCH_FILE_S3_URL,
    split the file into N files: script.10000, script.10001, script.10002, ...
    only the commands in the file with index 10000+ AWS_BATCH_JOB_ARRAY_INDEX will be executed;
    if JOB_CPU_PERCENTILE is set then these commands will be executed in parallel
        parallel --jobs JOB_CPU_PERCENTILE'%' {1} :::: ;
    otherwise will be executed sequential.

ENDUSAGE

  exit 2
}

# Standard function to print an error and exit with a failing return code
error_exit () {
  echo "${BASENAME} - ${1}" >&2
  exit 1
}


if [ -z "${BATCH_FILE_S3_URL}" ]; then
  usage "BATCH_FILE_S3_URL not set. No object to download."
fi

if [ -z "${JOB_CPU_PERCENTILE}" ]; then
  SEQUENTIAL=1
fi

if [ -z "${AWS_BATCH_JOB_ARRAY_INDEX}" ]; then
#    usage "need to be run in aws batch job array"
    AWS_BATCH_JOB_ARRAY_INDEX=0
    TOTAL_JOBS=1
fi    

scheme="$(echo "${BATCH_FILE_S3_URL}" | cut -d: -f1)"
if [ "${scheme}" != "s3" ]; then
  usage "BATCH_FILE_S3_URL must be for an S3 object; expecting URL starting with s3://"
fi

# Check that necessary programs are available
which aws >/dev/null 2>&1 || error_exit "Unable to find AWS CLI executable."


# Create a temporary directory to hold the downloaded contents, and make sure
# it's removed later, unless the user set KEEP_BATCH_FILE_CONTENTS.
cleanup () {
   if [ -z "${KEEP_BATCH_FILE_CONTENTS}" ] \
     && [ -n "${TMPDIR}" ] \
     && [ "${TMPDIR}" != "/" ]; then
      rm -r "${TMPDIR}"
   fi
}
trap 'cleanup' EXIT HUP INT QUIT TERM
# mktemp arguments are not very portable.  We make a temporary directory with
# portable arguments, then use a consistent filename within.
TMPDIR="$(mktemp -d -t tmp.XXXXXXXXX)" || error_exit "Failed to create temp directory."
TMPFILE="${TMPDIR}/batch-file-temp"
install -m 0600 /dev/null "${TMPFILE}" || error_exit "Failed to create temp file."

# Fetch and run a script
fetch_and_run_script () {
  # Create a temporary file and download the script
  aws s3 cp "${BATCH_FILE_S3_URL}" - > "${TMPFILE}" || error_exit "Failed to download S3 script."
  LINECOUNT=`wc -l ${TMPFILE} | cut -f1 -d' '`
  LOW_LINE=$(( $LINECOUNT * $AWS_BATCH_JOB_ARRAY_INDEX / $TOTAL_JOBS + 1 ))
  HIGH_LINE=$(( $LINECOUNT * (1 + $AWS_BATCH_JOB_ARRAY_INDEX) / $TOTAL_JOBS ))

  echo ${TMPFILE} has ${LINECOUNT} lines.
  sed -n ${LOW_LINE},${HIGH_LINE}p ${TMPFILE} > ${TMPFILE}.mypart
  echo This is job with index $AWS_BATCH_JOB_ARRAY_INDEX in array. Execut lines ${LOW_LINE} to ${HIGH_LINE}.
  if [ -z "${SEQUENTIAL}" ]; then 
      parallel --jobs ${JOB_CPU_PERCENTILE}'%' :::: ${TMPFILE}.mypart
  else
      echo "Run these commands"
      cat ${TMPFILE}.mypart
      sh ${TMPFILE}.mypart
  fi

# Make the temporary file executable and run it with any given arguments
#  local script="./${1}"; shift
#  chmod u+x "${TMPFILE}" || error_exit "Failed to chmod script."
# exec ${TMPFILE} "${@}" || error_exit "Failed to execute script."
}


echo Starting Job with `nproc --all` CPUS. `head -n 1 /proc/meminfo`. JOBS_PER_CPU=$JOB_CPU_PERCENTILE'%'

# Main - dispatch user request to appropriate function
    fetch_and_run_script "${@}"
