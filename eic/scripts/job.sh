#!/usr/bin/env bash

echo ""
echo "log from $0"

export JOB_NUMBER=$1
export INPUT_FILE=$2
export OUTPUT_FILE=$3
export OUTPUT_DIR=$(dirname $3)

echo "$$: $@"
source /opt/detector/setup.sh

# Environment variable setup

cd detector_benchmarks
rm -f .env
if [ ! -n "${LOCAL_PREFIX}" ] ; then
  source .local/bin/env.sh
else
  source ${LOCAL_PREFIX}/bin/env.sh
fi
${LOCAL_PREFIX}/bin/print_env.sh

rm -f sim_output
mkdir_local_data_link sim_output

# build and compile epic detector in ${DETECTOR_PREFIX}/${DETECTOR}
if [ "$JOB_NUMBER" == "1" ]; then
	rm .build_complete
	rm -rf ${DETECTOR_PREFIX}/${DETECTOR}_build
	mkdir -p ${DETECTOR_PREFIX}/${DETECTOR}_build
	pushd ${DETECTOR_PREFIX}/${DETECTOR}_build
	cmake ${DETECTOR_PREFIX}/${DETECTOR} -DCMAKE_INSTALL_PREFIX=${LOCAL_PREFIX} -DCMAKE_CXX_STANDARD=17 && make -j$(($(nproc)/4+1)) install || exit 1
	popd
	touch .build_complete
else
	sleep 5
	RET=1
	while [ $RET -eq 1 ]; do
		RET=$(ls .build_complete; echo $?)
		if [ $RET -ne 1 ]
			break
	done
fi


N_EVENTS=100
VALUE=0.00

## =============================================================================
## Step 1: Generate input and run GEANT4 simulation with resource usage logging
LOG_FILE="${OUTPUT_DIR}/timestamp_${JOB_NUMBER}.txt"
RUN_SCRIPT="./benchmarks/barrel_ecal/run_emcal_barrel_particles_ext.sh"
/usr/bin/time -f "%e seconds, %P cpu usage, %S system time, %U user time" -a -o $LOG_FILE bash $RUN_SCRIPT
echo "" >> $LOG_FILE

## =============================================================================
## Step 2: Run benchmark with output and store results in separate directory
BENCHMARK_SCRIPT="benchmarks/barrel_ecal/scripts/emcal_barrel_particles_analysis.cxx"
DIRECTORY="$(date +%s)_${VALUE}"

root -b -q $BENCHMARK_SCRIPT
CWD=$PWD
cd results
mkdir $DIRECTORY
mv emcal_barrel_uniform_pi0_* $DIRECTORY
cd $CWD
