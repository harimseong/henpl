#!/usr/bin/bash

set -e

export TIME="$(date "+%y%m%d_%H%M%S")"
echo "TIME=$TIME"

########################################################
# I/O files                                            #
########################################################
export PREFIX_INPUT=$HOME
export INPUT_FILES=("file1" "file2" "file3" "file4" "file5" "file6" "file7" "file8" "file9" "file10" "file11" "file12" "file13" "file14" "file15" "file16")
export JOB_SIZE=${#INPUT_FILES[@]}

echo "input file list:"
for i in $(seq 0 $((JOB_SIZE - 1)))
do
	INPUT_FILES[$i]="${PREFIX_INPUT}/${INPUT_FILES[$i]}"
	echo "${INPUT_FILES[$i]}"
done

export PREFIX_OUTPUT=${HOME}/eic/output_${TIME}
export OUTPUT_FILES=("output1" "output2" "output3" "output4" "output5" "output6" "output7" "output8" "output9" "output10" "output11" "output12" "output13" "output14" "output15" "output16")

echo ""
echo "output file list:"
for i in $(seq 0 $((JOB_SIZE - 1)))
do
	OUTPUT_FILES[$i]="${PREFIX_OUTPUT}/${OUTPUT_FILES[$i]}"
	echo "${OUTPUT_FILES[$i]}"
done

########################################################
# job executable                                       #
########################################################
export JOB_EXE=eic/eic_shell_starter.sh
export POSTPROCESS_EXE=

########################################################
# job directory                                        #
########################################################
export JOB_DIR="${PREFIX_OUTPUT}"

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


BATCH_NAME_FILE=.job_id_${TIME}
for i in $(seq 0 $((JOB_SIZE - 1)))
do
	export JOB_NUMBER=$((i + 1))

	########################################################
	# argument list for JOB_EXE. $1=$i, $2=${TEST_FILES[$i]} ...
	########################################################
	export JOB_ARG='"'"$i ${INPUT_FILES[$i]} ${OUTPUT_FILES[$i]}"'"'

	# generate condor job submit file from '.condor_work.template'.
	envsubst < .condor_work.template > condor_work.sub

	condor_submit condor_work.sub | grep cluster | sed 's/.*cluster \([0-9]*\)./\1/' >> ${BATCH_NAME_FILE}
done

########################################################
# postprocess                                          #
########################################################
if [ "$POSTPROCESS_EXE" == "" ]; then
	rm $BATCH_NAME_FILE
	exit
fi

# wait until jobs are finished
BATCH_NAMES=$(cat $BATCH_NAME_FILE | sed 's/\([0-9]\) \([0-9]\)/\1|\2/g')
WAIT=1
COUNT=0
echo "waiting condor jobs to be finished"
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
#   cat ${OUTPUT_FILES[$i]} >> $FILENAME
# done
# echo "finished"
