#!/usr/bin/env bash

# you need to run docker container before running this script.
# ./eic-shell
# then setup enviromnet and build detector
# source .local/bin/env.sh && .local/bin/build_detector.sh

if [ ! -n "$EIC_SHELL" ]; then
  NUM_CONTAINER=$(docker ps | grep eicweb | wc -l)
  if [ $NUM_CONTAINER -gt 1 ]; then
    echo "ERROR: There're more than one attachable containers. Containers should be stopped except one."
    exit 1
  fi
  CONTAINER_ID=$(docker ps | grep eicweb | cut -d ' ' -f 1)
  echo "CONTAINER_ID=$CONTAINER_ID"
  if [ -n "$CONTAINER_ID" ]; then
    docker exec -e EIC_SHELL=1 ${CONTAINER_ID} eic-shell "bash simulation_parallel.sh $1 $2 $3 $4 $5 $6"
  else
    echo "ERROR: No attachable container found. Run eic-shell in other terminal."
    echo "If it's not fixed while eic-shell is running, check \`docker info\`. If outputs from inside and outside of condor job are different, try removing ${HOME}/.docker"
    #docker info
    exit 1
  fi

  exit
fi

echo "JOB_NUMBER=$1"
echo "GEN_FILE=$2"
echo "SIM_FILE=$3"
echo "REC_FILE=$4"
echo "BENCHMARK_DIR=$5"
echo "TIME=$6"
export JOB_NUMBER=$1
export GEN_FILE=$2
export SIM_FILE=$3
export REC_FILE=$4
export BDIR=$5
export TIME=$6
# if GEN_FILE="/foo/bar/file"
# GEN_DIR="/foo/bar"
export GEN_DIR=$(dirname $GEN_FILE)
# GEN_BASE="file"
export GEN_BASE=$(basename $GEN_FILE)

ENERGY_RANGE=($(seq 0.5 0.3 5.0))

cd $BDIR

source /opt/detector/setup.sh
source .local/bin/env.sh

export PARTICLE="electron"
export E_START=${ENERGY_RANGE[$JOB_NUMBER]}
export E_END=$E_START

echo "BENCHMARK_N_EVENTS=$BENCHMARK_N_EVENTS"
/usr/bin/time -f "%e seconds, %P cpu usage, %S system time, %U user time" -a -o  bash benchmarks/barrel_ecal/run_emcal_barrel_particles_parallel.sh
