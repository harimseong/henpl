#!/usr/bin/env bash

set -e

export TIME="$(date "+%y%m%d_%H%M%S")"
export BENCHMARK_N_EVENTS=2000
EIC_DIR="/usr/local/share/eic"

# particles
PARTICLES=(
"ELECTRON"
"PHOTON"
)


# energy 
ELECTRON_ENERGY_RANGE=(
0.50  0.75
1.00  1.50
2.00  2.50
3.00  4.00
5.00  6.00
7.00  8.00
9.00  10.00
11.00 12.00
13.50 15.00
16.50 18.00
)
PHOTON_ENERGY_RANGE=(
0.10  0.15
0.20  0.25
0.30  0.35
0.40  0.45
0.50  0.75
1.00  1.50
2.00  2.50
3.00  4.00
5.00  6.00
7.00  8.00
9.00  10.00
11.00 12.00
13.50 15.00
16.50 18.0
)
ENERGY_RANGE=


# eta
ETA_RANGE=(
-1.7  -1.6
-1.5  -1.0
-0.5  0.0
0.5   1.0
1.2   1.3
)
ETA_RANGE_SIZE=${#ETA_RANGE[@]}
ETA_LOW=()
ETA_HIGH=()
for i in $(seq 0 $((ETA_RANGE_SIZE - 2)))
do
  ETA_LOW[$i]=${ETA_RANGE[i]}
  ETA_HIGH[$i]=${ETA_RANGE[i + 1]}
done
ETA_RANGE_COUNT=${#ETA_LOW[@]}

PREFIX_GEN_FILES='${JOB_DIR}/gen'
PREFIX_SIM_FILES='${JOB_DIR}/sim'
PREFIX_REC_FILES='${JOB_DIR}/rec'

submit_jobs() {
  RESULT_DIR="${EIC_DIR}/results"

  for particle in ${PARTICLES[@]}
  do
    JOB_DIR="${RESULT_DIR}/${particle}_${TIME}"
    mkdir -p ${JOB_DIR}

    # ============================================= 
    # generated input directory
    PREFIX_GEN_FILES=$(eval echo ${PREFIX_GEN_FILES})
    mkdir -p ${PREFIX_GEN_FILES}
    chmod -R 775 ${PREFIX_GEN_FILES}

    # ============================================= 
    # simulation output directory
    PREFIX_SIM_FILES=$(eval echo ${PREFIX_SIM_FILES})
    mkdir -p ${PREFIX_SIM_FILES}
    chmod -R 775 ${PREFIX_SIM_FILES}

    # ============================================= 
    # reconstructed output directory
    PREFIX_REC_FILES=$(eval echo ${PREFIX_REC_FILES})
    mkdir -p ${PREFIX_REC_FILES}
    chmod -R 775 ${PREFIX_REC_FILES}

    TEMP="${particle}_ENERGY_RANGE[@]"
    ENERGY_RANGE=(${!TEMP})
    ENERGY_RANGE_SIZE=${#ENERGY_RANGE[@]}

    JOB_EXE=$(readlink -f $0)

    bash run.sh ${JOB_EXE} ${JOB_DIR} $((ENERGY_RANGE_SIZE * ETA_RANGE_SIZE)) ${particle}
  done
}

#============================================================
# you need to run docker container before running this script.
# ./eic-shell
#============================================================
attach_container() {
  NUM_CONTAINER=$(docker ps | grep eicweb | wc -l)
  if [ $NUM_CONTAINER -gt 1 ]; then
    # TODO: use docker info and find container which is created by $USER
    echo "WARNING: There're more than one attachable eic-shell containers. recommend to stop the containers except one."
  fi
  CONTAINER_ID=$(docker ps | grep eicweb | cut -d ' ' -f 1 | head -n 1)
  echo "CONTAINER_ID=$CONTAINER_ID"
  echo "command=bash $0 run_simulation $2 $3 ${@:4:256}"
  if [ -n "$CONTAINER_ID" ]; then
    docker exec ${CONTAINER_ID} /opt/local/bin/eic-shell "bash $0 run_simulation $2 $3 ${@:4:256}"
  else
    echo "ERROR: No attachable container found. Run eic-shell in other terminal."
    echo "If it's not fixed while eic-shell is running, check \`docker info\`. If outputs from inside and outside of condor job are different, try removing ${HOME}/.docker"
    #docker info
    exit 1
  fi
}

run_simulation() {
  export BENCHMARK_DIR="${EIC_DIR}/detector_benchmarks"

  export JOB_DIR=$2
  export JOB_NUMBER=$3
  export PARTICLE=$4
  echo "JOB_DIR=${JOB_DIR}"
  echo "JOB_NUMBER=${JOB_NUMBER}"
  echo "PARTICLE=${PARTICLE}"

  export GEN_FILE="gen_${JOB_NUMBER}.hepmc"
  export SIM_FILE="sim_${JOB_NUMBER}.edm4hep.root"
  export REC_FILE="rec_${JOB_NUMBER}.root"
  GEN_FILE="$(eval echo $PREFIX_GEN_FILES)/${GEN_FILE}"
  SIM_FILE="$(eval echo $PREFIX_SIM_FILES)/${SIM_FILE}"
  REC_FILE="$(eval echo $PREFIX_REC_FILES)/${REC_FILE}"
  echo "GEN_FILE=$GEN_FILE"
  echo "SIM_FILE=$SIM_FILE"
  echo "REC_FILE=$REC_FILE"

  echo "BENCHMARK_DIR=$BENCHMARK_DIR"
  echo "TIME=$TIME"

  TEMP="$4_ENERGY_RANGE[@]"
  ENERGY_RANGE=(${!TEMP})
  ENERGY_RANGE_SIZE=${#ENERGY_RANGE[@]}

  cd $BENCHMARK_DIR

  source /usr/local/share/eic/epic-local/bin/thisepic.sh epic

  export JUGGLER_N_EVENTS=${BENCHMARK_N_EVENTS}

  echo "DETECTOR_PATH=${DETECTOR_PATH}"
  echo "DETECTOR_CONFIG=${DETECTOR_CONFIG}"

  export PARTICLE="electron"
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
}

case "$1" in
  "")
    submit_jobs $@
    exit 0
  ;;
  "$0")
    attach_container $@
    exit 0
  ;;
  "run_simulation")
    run_simulation $@
    exit 0
  ;;
  *)
    echo "$0: '$1': invalid arguments"
    exit 1
  ;;
esac
