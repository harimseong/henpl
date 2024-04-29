#!/usr/bin/bash

set -e

export TIME="$(date "+%y%m%d_%H%M%S")"
echo "TIME=$TIME"

########################################################
# job executable                                       #
########################################################
export JOB_EXE=$HOME/eic/simulation_parallel.sh
export POSTPROCESS_EXE=

export JOB_SIZE=16
export JOB_NUM_ARR=$(seq 0 $((JOB_SIZE - 1)))
BENCHMARK_DIR="$HOME/eic/detector_benchmarks"

########################################################
# job directory                                        #
########################################################
export JOB_DIR="${BENCHMARK_DIR}/benchmark_${TIME}"

if [ -a $JOB_DIR ];
then
	echo "$0: $JOB_DIR directory already exists."
	exit 1
fi

echo "job directory list:"
mkdir $JOB_DIR
for i in $(seq ${JOB_SIZE})
do
  # .condor_work.template
	mkdir $JOB_DIR/job_$i
  echo "$JOB_DIR/job_$i"
done

########################################################
# file arguments                                       #
########################################################

# ============================================= 
# file 1 for generated input
export PREFIX_FILES_0="./benchmark_${TIME}/gen"
mkdir -p $JOB_DIR/gen
export FILES_0=(\
  gen_0.hepmc \
  gen_1.hepmc \
  gen_2.hepmc \
  gen_3.hepmc \
  gen_4.hepmc \
  gen_5.hepmc \
  gen_6.hepmc \
  gen_7.hepmc \
  gen_8.hepmc \
  gen_9.hepmc \
  gen_10.hepmc \
  gen_11.hepmc \
  gen_12.hepmc \
  gen_13.hepmc \
  gen_14.hepmc \
  gen_15.hepmc \
)

echo ""
echo "FILES_0 list:"
for i in $(seq 0 $((JOB_SIZE - 1)))
do
	FILES_0[$i]="${PREFIX_FILES_0}/${FILES_0[$i]}"
	echo "${FILES_0[$i]}"
done

# ============================================= 
# file 2
export PREFIX_FILES_1="./benchmark_${TIME}/sim"
mkdir -p $JOB_DIR/sim
export FILES_1=(\
  sim_0.edm4hep.root \
  sim_1.edm4hep.root \
  sim_2.edm4hep.root \
  sim_3.edm4hep.root \
  sim_4.edm4hep.root \
  sim_5.edm4hep.root \
  sim_6.edm4hep.root \
  sim_7.edm4hep.root \
  sim_8.edm4hep.root \
  sim_9.edm4hep.root \
  sim_10.edm4hep.root \
  sim_11.edm4hep.root \
  sim_12.edm4hep.root \
  sim_13.edm4hep.root \
  sim_14.edm4hep.root \
  sim_15.edm4hep.root \
)

echo ""
echo "FILES_1 list:"
for i in $(seq 0 $((JOB_SIZE - 1)))
do
	FILES_1[$i]="${PREFIX_FILES_1}/${FILES_1[$i]}"
	echo "${FILES_1[$i]}"
done

# ============================================= 
# file 3
export PREFIX_FILES_2="./benchmark_${TIME}/rec"
mkdir -p $JOB_DIR/rec
export FILES_2=(\
  rec_0.root \
  rec_1.root \
  rec_2.root \
  rec_3.root \
  rec_4.root \
  rec_5.root \
  rec_6.root \
  rec_7.root \
  rec_8.root \
  rec_9.root \
  rec_10.root \
  rec_11.root \
  rec_12.root \
  rec_13.root \
  rec_14.root \
  rec_15.root \
)

echo ""
echo "FILES_2 list:"
for i in $(seq 0 $((JOB_SIZE - 1)))
do
	FILES_2[$i]="${PREFIX_FILES_2}/${FILES_2[$i]}"
	echo "${FILES_2[$i]}"
done

sleep 1

BATCH_NAME_FILE=.job_ids_${TIME}
for i in $(seq 0 $((JOB_SIZE - 1)))
do
	export JOB_NUMBER=$((i + 1))

	########################################################
	# argument list for JOB_EXE.                           #
  # $1=$JOB_NUMBER (by default)                          #
  # $2=${FILES_0[$JOB_NUMBER]}                           #
  # $3=${FILES_1[$JOB_NUMBER]}                           #
  # $4=${FILES_2[$JOB_NUMBER]}                           #
  # you can change this behavior by changing this file   #
  # and .condor_work.template                            #
	########################################################
	export JOB_ARG='"'"$i ${FILES_0[$i]} ${FILES_1[$i]} ${FILES_2[$i]}"'"'

	# generate condor job submit file from '.condor_work.template'.
	envsubst < .condor_work.template > condor_work.sub

	condor_submit condor_work.sub | grep cluster | sed 's/.*cluster \([0-9]*\)./\1/' >> ${BATCH_NAME_FILE}
  echo "job $i submitted"
done
rm condor_work.sub

########################################################
# postprocess                                          #
########################################################
if [ "$POSTPROCESS_EXE" == "" ]; then
	rm $BATCH_NAME_FILE
  echo "all jobs are submitted."
	exit
fi

# wait until jobs are finished
BATCH_NAMES=$(cat $BATCH_NAME_FILE | sed 's/\([0-9]\) \([0-9]\)/\1|\2/g')
WAIT=1
COUNT=0
echo "waiting for jobs to be finished"
while [ $WAIT -eq 1 ];
do
	RET=$(condor_q | head -n $((JOB_SIZE + 1)) | tail -n $((JOB_SIZE)) | sed 's/.* ID: \([0-9]*\) .*/\1/' | tr '\n' ' ' | grep -E "'""$BATCH_NAMES""'" > /dev/null; echo $?)
	if [ $RET -eq 1 ]; then
		WAIT=0
	fi
	sleep 1
	printf "."
	if [ $COUNT -eq 5 ]; then
		printf "\r            \r"
	fi
	COUNT=$((COUNT + 1))
done

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
