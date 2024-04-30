#!/usr/bin/bash

watch() {
  while [ 1 ]; do
    JOB_STATUS=$(condor_q);
    LINES=$(printf "${JOB_STATUS}" | wc -l | tr -d ' ');
    LINES=$((LINES + 1))
    printf "\\033[${LINES}F\\033[0J";
    printf "${JOB_STATUS}"
    printf "\nEXIT: Ctrl + C"
    sleep 1;
  done
}

if [ "$1" == "-w" ]; then
  watch;
  exit
fi

set -e

WAIT_JOB_COMPLETION=1
export TIME="$(date "+%y%m%d_%H%M%S")"
echo "TIME=$TIME"

########################################################
# job executable & environment variables               #
########################################################
EIC_DIR="/Users/skku_server/eic"

export JOB_EXE=${HOME}/eic/simulation_parallel.sh
export POSTPROCESS_EXE=

export JOB_SIZE=16
export JOB_NUM_ARR=$(seq 0 $((JOB_SIZE - 1)))
export BENCHMARK_DIR="${EIC_DIR}/detector_benchmarks"

########################################################
# job directory                                        #
########################################################
export JOB_DIR="${HOME}/eic/benchmark_${TIME}"

if [ -a $JOB_DIR ];
then
  echo "$0: $JOB_DIR directory already exists."
  exit 1
fi

echo "job directory list:"
mkdir $JOB_DIR
for i in ${JOB_NUM_ARR}
do
  # .condor_work.template
  # .condor_work_queue.template
  mkdir $JOB_DIR/job_$i
  echo "$JOB_DIR/job_$i"
done

########################################################
# file arguments                                       #
########################################################

# ============================================= 
# file 0 for generated input
export PREFIX_FILES_0="${JOB_DIR}/gen"
mkdir -p ${PREFIX_FILES_0}
chmod 775 ${PREFIX_FILES_0}
export FILE_0='gen_$(Process).hepmc'
FILE_0="${PREFIX_FILES_0}/${FILE_0}"

# ============================================= 
# file 1
export PREFIX_FILES_1="${JOB_DIR}/sim"
mkdir -p $JOB_DIR/sim
chmod 775 ${PREFIX_FILES_1}
export FILE_1='sim_$(Process).edm4hep.root'
FILE_1="${PREFIX_FILES_1}/${FILE_1}"

# ============================================= 
# file 2
export PREFIX_FILES_2="${JOB_DIR}/rec"
mkdir -p $JOB_DIR/rec
chmod 775 ${PREFIX_FILES_2}
export FILE_2='rec_$(Process).root'
FILE_2="${PREFIX_FILES_2}/${FILE_2}"

sleep 1

########################################################
# argument list for JOB_EXE.                           #
# $1=$JOB_NUMBER                                       #
# $2=${FILES_0[$JOB_NUMBER]}                           #
# $3=${FILES_1[$JOB_NUMBER]}                           #
# $4=${FILES_2[$JOB_NUMBER]}                           #
# $5=${BENCHMARK_DIR}                                  #
# $6=${TIME}                                           #
# you can change this behavior by changing this file   #
# and .condor_work.template                            #
########################################################
export JOB_ARG='"''$(Process)'" ${FILE_0} ${FILE_1} ${FILE_2} ${BENCHMARK_DIR} ${TIME}"'"'

envsubst < .condor_work_queue.template > condor_work.sub
BATCH_NAMES=$(condor_submit condor_work.sub | grep cluster | sed 's/.*cluster \([0-9]*\)./\1/')
echo "batch '${BATCH_NAMES}' submitted"

########################################################
# postprocess                                          #
########################################################
if [[ "$POSTPROCESS_EXE" == "" && "$WAIT_JOB_COMPLETION" == "" ]]; then
  #rm $BATCH_NAME_FILE
  echo "all jobs are submitted."
  exit
fi

# wait until jobs are finished
WAIT=1
COUNT=0
echo "waiting for jobs to be finished"
condor_q
while [ $WAIT -eq 1 ];
do
  RET=$(condor_q | grep "ID: " | sed 's/.* ID: \([0-9]*\) .*/\1/' | tr '\n' ' ' | grep -E $BATCH_NAMES > /dev/null; echo $?)
  if [ $RET -eq 1 ]; then
    WAIT=0
  fi
  JOB_STATUS=$(condor_q);
  LINES=$(printf "${JOB_STATUS}" | wc -l | tr -d ' ');
  LINES=$((LINES + 1))
  printf "\\033[${LINES}F\\033[0J";
  printf "${JOB_STATUS}"
  printf "\nexit waiting: Ctrl + C"
  sleep 1;
done

if [ "$POSTPROCESS_EXE" == "" ]; then
  exit
fi

echo "post-processing"
bash -c "$POSTPROCESS_EXE"

# echo "post-processing example"
#
# export FILENAME=output_merged
# touch $FILENAME
# for i in $(seq 0 $((JOB_SIZE - 1)))
# do
#   cat ${FILES_1[$i]} >> $FILENAME
# done
# echo "finished"
