#!/usr/bin/bash

set -e

DEFAULT_N_EVENTS=100
VALUE=0.00

if [ -n "${CONDOR_JOB}" && ! -n "${OUTPUT_DIRNAME}" ]; then
  echo "There's no OUTPUT_FILENAME environment variable."
  exit
fi

if [ -n "${BENCHMARK_N_EVENTS}" ] ; then
  echo "current number of events = ${BENCHMARK_N_EVENTS}"
else
  BENCHMARK_N_EVENTS=${DEFAULT_N_EVENTS}
  echo "current number of events = ${BENCHMARK_N_EVENTS}"
fi


## =============================================================================
## Step 2: Generate input and run GEANT4 simulation with resource usage logging
export PARTICLE="electron"
RUN_SCRIPT="./benchmarks/barrel_ecal/run_emcal_barrel_particles.sh"


LOG_LABEL="$(date) - events=${BENCHMARK_N_EVENTS}"
echo $LOG_LABEL
/usr/bin/time -f "%e seconds, %P cpu usage, %S system time, %U user time" -a -o bash $RUN_SCRIPT
echo ""

## =============================================================================
## Step 4: Run benchmark with output and store results in separate directory
BENCHMARK_SCRIPT="benchmarks/barrel_ecal/scripts/emcal_barrel_particles_analysis.cxx"

if [ -n "$CONDOR_JOB" ]; then
  DIRECTORY=${OUTPUT_DIRNAME}
else
  mkdir -p $PWD/results/$DIRECTORY
  DIRECTORY="E${E_START}_Eta${ETA_MIN}_${ETA_MAX}"
fi

root -b -q $BENCHMARK_SCRIPT
mv emcal_barrel_electron_* $DIRECTORY
