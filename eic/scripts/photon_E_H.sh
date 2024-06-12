#!/usr/bin/env bash

set -e

#============================================================
# you need to run docker container before running this script.
# ./eic-shell
#============================================================

if [ ! -n "$EIC_SHELL" ]; then
  JOB_EXE=$8
  echo "DOCKER_HOST=${DOCKER_HOST}"
  echo "JOB_EXE=${JOB_EXE}"
  NUM_CONTAINER=$(docker ps | grep eicweb | wc -l)
  if [ $NUM_CONTAINER -gt 1 ]; then
    # TODO: use docker info and find container which is created by $USER
    echo "WARNING: There're more than one attachable containers."
  fi
  CONTAINER_ID=$(docker ps | grep eicweb | cut -d ' ' -f 1 | head -n 1)
  echo "CONTAINER_ID=$CONTAINER_ID"
  if [ -n "$CONTAINER_ID" ]; then
    docker exec -e EIC_SHELL=1 ${CONTAINER_ID} /opt/local/bin/eic-shell "bash ${JOB_EXE} $1 $2 $3 $4 $5 $6 $7"
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
echo "JOB_DIR=$7"
export JOB_NUMBER=$1
export GEN_FILE=$2
export SIM_FILE=$3
export REC_FILE=$4
export BDIR=$5
export TIME=$6
export JOB_DIR=$7

# JOB_SIZE in run.sh should be equal to
# size of ENERGY_RANGE * size of ETA_LOW
#ENERGY_RANGE=(0.10 0.15 0.20 0.25 0.30 0.35 0.40 0.45 0.50 0.75 1.00 1.50 2.00 2.50 3.00 4.00 5.00 6.00 7.00 8.00 9.00 10.00 11.00 12.00 13.50 15.00 16.50 18.0) # 22
ENERGY_RANGE=(0.15 0.20 0.25 0.35 0.40 0.45) # 6
ETA_RANGE=(-1.7 -1.6 -1.5 -1.0 -0.5 0.0 0.5 1.0 1.2 1.3) # 10
ETA_RANGE_SIZE=${#ETA_RANGE[@]}
ETA_LOW=()
ETA_HIGH=()
for i in $(seq 0 $((ETA_RANGE_SIZE - 2)))
do
  ETA_LOW[$i]=${ETA_RANGE[i]}
  ETA_HIGH[$i]=${ETA_RANGE[i + 1]}
done

cd $BDIR

source /usr/local/share/eic/epic-local/bin/thisepic.sh epic

export BENCHMARK_N_EVENTS=5000
export JUGGLER_N_EVENTS=${BENCHMARK_N_EVENTS}

echo "DETECTOR_PATH=${DETECTOR_PATH}"
echo "DETECTOR_CONFIG=${DETECTOR_CONFIG}"

ETA_RANGE_COUNT=${#ETA_LOW[@]}
export PARTICLE="photon"
export E_START=${ENERGY_RANGE[$((JOB_NUMBER / ETA_RANGE_COUNT))]}
export E_END=$E_START
export ETA_START=${ETA_LOW[$((JOB_NUMBER % ETA_RANGE_COUNT))]}
export ETA_END=${ETA_HIGH[$((JOB_NUMBER % ETA_RANGE_COUNT))]}

echo "E=${E_START}"
echo "ETA_START=${ETA_START}"
echo "ETA_END=${ETA_END}"

export SIM_DIR=$(dirname $SIM_FILE)
export SIM_FILE="${SIM_DIR}/sim_E${E_START}_H${ETA_START}t${ETA_END}.root"

export REC_DIR=$(dirname $REC_FILE)
export REC_FILE="${REC_DIR}/rec_E${E_START}_H${ETA_START}t${ETA_END}.root"

echo "BENCHMARK_N_EVENTS=$BENCHMARK_N_EVENTS"
/usr/bin/time -f "%e seconds, %P cpu usage, %S system time, %U user time" -a -o  bash benchmarks/barrel_ecal/run_emcal_barrel_particles_parallel.sh

eicrecon -Ppodio:output_include_collections=MCParticles,GeneratedParticles,ReconstructedParticles,EcalBarrelScFiRawHits,EcalBarrelScFiRecHits,EcalBarrelScFiClusters,EcalBarrelScFiClusterAssociations -Pplugins=janadot -Ppodio:output_file=${REC_FILE} ${SIM_FILE}
