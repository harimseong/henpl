#!/usr/bin/env bash

set -e

# you need to run docker container before running this script.
# ./eic-shell
# then setup enviromnet and build detector
# source .local/bin/env.sh && .local/bin/build_detector.sh

if [ "$(uname)" == "Darwin" ]; then
  source /etc/zprofile # using colima for Linux kernel VM
fi

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
    docker exec -e EIC_SHELL=1 ${CONTAINER_ID} eic-shell "bash ${JOB_EXE} $1 $2 $3 $4 $5 $6 $7"
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

ENERGY_RANGE=(0.5 1) # 2 3 5 8 10 12 15 18) # 10
ETA_LOW=( -1.7 -1.6 -1.5 -1   -0.5 0   0.5 1   1.2) # 10
ETA_HIGH=(-1.6 -1.5 -1   -0.5  0   0.5 1   1.2 1.3)

cd $BDIR

export BENCHMARK_N_EVENTS=1000

source /opt/detector/epic-*_main/bin/thisepic.sh
source .local/bin/env.sh

export PARTICLE="electron"
export E_START=${ENERGY_RANGE[$((JOB_NUMBER / 9))]}
export E_END=$E_START
export ETA_START=${ETA_LOW[$((JOB_NUMBER % 9))]}
export ETA_END=${ETA_HIGH[$((JOB_NUMBER % 9))]}

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

chmod -R 775 ${BDIR}
