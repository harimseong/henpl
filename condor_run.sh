#!/usr/bin/bash

########################################################
# array of files				       #
########################################################
export TEST_INPUT_FILES=("file1" "file2" "file3" "file4" "file5" "file6" "file7" "file8" "file9" "file10" "file11" "file12" "file13" "file14" "file15" "file16")
export JOB_SIZE=${#TEST_INPUT_FILES[@]}

echo "input file list:"
for i in $(seq 0 $((JOB_SIZE - 1)))
do
	TEST_INPUT_FILES[$i]="$HOME/${TEST_INPUT_FILES[$i]}"
	echo "${TEST_INPUT_FILES[$i]}"
done

export TEST_OUTPUT_FILES=("output1" "output2" "output3" "output4" "output5" "output6" "output7" "output8" "output9" "output10" "output11" "output12" "output13" "output14" "output15" "output16")

########################################################
# executable	                                       #
########################################################
export JOB_EXE=stress_test.sh

########################################################
# directory for each job                               #
########################################################
export TIME="$(date "+%y%m%d_%H%M%S")"
export JOB_DIR="/Users/skku_server/condor_test/${JOB_EXE}_${TIME}"


if [ -a $JOB_DIR ];
then
	echo "$0: $JOB_DIR directory already exists."
	exit 1
fi

mkdir $JOB_DIR
for i in $(seq ${JOB_SIZE})
do
	mkdir $JOB_DIR/job_$i
done

# generate condor job submit file from 'temp.condor_work'.

BATCH_NAME_FILE=.job_id_${TIME}
for i in $(seq 0 $((JOB_SIZE - 1)))
do
	########################################################
	# argument list for JOB_EXE. $1=$i, $2=${TEST_FILES[$i]} ...
	########################################################
	export JOB_NUMBER=$((i + 1))
	export JOB_ARG='"'"$i ${TEST_INPUT_FILES[$i]} ${TEST_OUTPUT_FILES[$i]}"'"'
	envsubst < .condor_work.template > condor_work.sub
	condor_submit condor_work.sub | grep cluster | sed 's/.*cluster \([0-9]*\)./\1/' >> ${BATCH_NAME_FILE}
done

########################################################
# postprocess                                          #
########################################################
DO_POSTPROCESS=1
if [ "$DO_POSTPROCESS" != "1" ]; then
	rm $BATCH_NAME_FILE
	exit
fi

BATCH_NAMES=$(cat $BATCH_NAME_FILE | sed 's/\([0-9]\) \([0-9]\)/\1|\2/g')
WAIT=1
while [ $WAIT -eq 1 ];
do
	condor_q | head -n $((JOB_SIZE + 1)) | tail -n $((JOB_SIZE)) | sed 's/.* ID: \([0-9]*\) .*/\1/' | tr '\n' ' ' | grep -E "'""$BATCH_NAMES""'" > /dev/null
	if [ $? -eq 1 ]; then
		WAIT=0
	fi
	sleep 5
	echo "waiting condor job to finish"
done

export FILENAME=output_merged
touch $FILENAME

echo "post-processing"
for i in $(seq 0 $((JOB_SIZE - 1)))
do
	cat $JOB_DIR/job_$((i + 1))/${TEST_OUTPUT_FILES[$i]} >> $FILENAME
done
		echo "finished"
