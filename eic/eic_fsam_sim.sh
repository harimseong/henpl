#!/usr/bin/env bash

set -e

export TIME="$(date "+%y%m%d_%H%M%S")"
export BENCHMARK_N_EVENTS=5000
EIC_DIR="/eic"

# particles
PARTICLES=(
  #"ELECTRON"
  "PHOTON"
)

# energy 
ELECTRON_ENERGY_RANGE=(
  0.50
  0.60
  0.70
  0.80
  0.90
  1.00
  1.50
  2.00
  2.50
  3.00
  4.00
  5.00
  6.00
  7.00
  8.00
  9.00
  10.00
  11.00
  12.00
  13.50
  15.00
  16.50
  18.00
)
PHOTON_ENERGY_RANGE=(
  0.10
  0.15
  0.20
  0.25
  0.30
  0.35
  0.40
  0.45
  0.50
  0.75
  1.00
  1.50
  2.00
  2.50
  3.00
  4.00
  5.00
  6.00
  7.00
  8.00
  9.00
  10.00
  11.00
  12.00
  13.50
  15.00
  16.50
  18.00
)
ENERGY_RANGE=

# eta
ETA_RANGE=(
  -1.7
  -1.6
  -1.5
  -1.0
  -0.5
  0.0
  0.5
  1.0
  1.2
  1.3
)

PREFIX_GEN_FILES='${JOB_DIR}/gen'
PREFIX_SIM_FILES='${JOB_DIR}/sim'
PREFIX_REC_FILES='${JOB_DIR}/rec'

submit_jobs() {
  # path to store
  RESULT_DIR="${EIC_DIR}/results"

  for particle in ${PARTICLES[@]}
  do
    JOB_DIR="${RESULT_DIR}/${particle}_${TIME}"
    mkdir -p ${JOB_DIR}
    chmod 775 ${JOB_DIR}

    # ============================================= 
    # generated input directory
    PREFIX_GEN_FILES=$(eval echo ${PREFIX_GEN_FILES})
    mkdir -p ${PREFIX_GEN_FILES}
    chmod 775 ${PREFIX_GEN_FILES}

    # ============================================= 
    # simulation output directory
    PREFIX_SIM_FILES=$(eval echo ${PREFIX_SIM_FILES})
    mkdir -p ${PREFIX_SIM_FILES}
    chmod 775 ${PREFIX_SIM_FILES}

    # ============================================= 
    # reconstructed output directory
    PREFIX_REC_FILES=$(eval echo ${PREFIX_REC_FILES})
    mkdir -p ${PREFIX_REC_FILES}
    chmod 775 ${PREFIX_REC_FILES}

    TEMP="${particle}_ENERGY_RANGE[@]"
    ENERGY_RANGE=(${!TEMP})
    ENERGY_RANGE_SIZE=${#ENERGY_RANGE[@]}
    ETA_RANGE_COUNT=$((${#ETA_RANGE[@]} - 1))

    echo ${ENERGY_RANGE[@]} > ${JOB_DIR}/E_range
    echo ${ETA_RANGE[@]} > ${JOB_DIR}/ETA_range

    JOB_EXE=.$0.${TIME}
    cp $0 ${JOB_EXE}
    JOB_EXE=$(readlink -f ${JOB_EXE})

    echo "work directory created"
    bash condor_submit_script.sh ${JOB_EXE} ${JOB_DIR} $((ENERGY_RANGE_SIZE * ETA_RANGE_COUNT)) ${particle}
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
  source ${EIC_DIR}/epic-local-24.10.0/bin/thisepic.sh epic
  cd $BENCHMARK_DIR

  export JOB_DIR=$2
  export JOB_NUMBER=$3
  export PARTICLE=$4
  echo "JOB_DIR=${JOB_DIR}"
  echo "JOB_NUMBER=${JOB_NUMBER}"
  echo "PARTICLE=${PARTICLE}"

  echo "BENCHMARK_DIR=$BENCHMARK_DIR"
  echo "TIME=$TIME"

  ENERGY_RANGE=($(cat ${JOB_DIR}/E_range))
  ENERGY_RANGE_SIZE=${#ENERGY_RANGE[@]}

  ETA_RANGE=($(cat ${JOB_DIR}/ETA_range))
  ETA_RANGE_SIZE=${#ETA_RANGE[@]}
  ETA_LOW=()
  ETA_HIGH=()
  for i in $(seq 0 $((ETA_RANGE_SIZE - 2)))
  do
    ETA_LOW[$i]=${ETA_RANGE[i]}
    ETA_HIGH[$i]=${ETA_RANGE[i + 1]}
  done
  ETA_RANGE_COUNT=${#ETA_LOW[@]}

  export JUGGLER_N_EVENTS=${BENCHMARK_N_EVENTS}

  echo "DETECTOR_PATH=${DETECTOR_PATH}"
  echo "DETECTOR_CONFIG=${DETECTOR_CONFIG}"

  export UPPER_PARTICLE=${PARTICLE}
  export PARTICLE=$(echo ${PARTICLE} | perl -ne "print lc")

  export E_START=${ENERGY_RANGE[$((JOB_NUMBER / ETA_RANGE_COUNT))]}
  export E_END=$E_START
  export ETA_START=${ETA_LOW[$((JOB_NUMBER % ETA_RANGE_COUNT))]}
  export ETA_END=${ETA_HIGH[$((JOB_NUMBER % ETA_RANGE_COUNT))]}

  echo "E=${E_START}"
  echo "ETA_START=${ETA_START}"
  echo "ETA_END=${ETA_END}"

  export GEN_FILE="gen_${JOB_NUMBER}.hepmc"
  GEN_FILE="$(eval echo $PREFIX_GEN_FILES)/${GEN_FILE}"
  SIM_DIR="$(eval echo $PREFIX_SIM_FILES)"
  REC_DIR="$(eval echo $PREFIX_REC_FILES)"

  export SIM_FILE="${SIM_DIR}/sim_E${E_START}_H${ETA_START}t${ETA_END}.root"
  export REC_FILE="${REC_DIR}/rec_E${E_START}_H${ETA_START}t${ETA_END}.root"

  echo "GEN_FILE=$GEN_FILE"
  echo "SIM_FILE=$SIM_FILE"
  echo "REC_FILE=$REC_FILE"
  echo "BENCHMARK_N_EVENTS=$BENCHMARK_N_EVENTS"

  /usr/bin/time -f "%e seconds, %P cpu usage, %S system time, %U user time" -a -o  bash benchmarks/barrel_ecal/run_emcal_barrel_particles_parallel.sh

  echo "eicrecon starts"

eicrecon -Ppodio:output_collections=\
MCParticles,\
GeneratedParticles,\
ReconstructedParticles,\
EcalBarrelScFiRawHits,\
EcalBarrelScFiRecHits,\
EcalBarrelScFiHits,\
EcalBarrelImagingRawHits,\
EcalBarrelImagingRecHits,\
EcalBarrelImagingHits\
 -Pplugins=janadot -Ppodio:output_file=${REC_FILE} ${SIM_FILE}

  if [ $? -eq 0 ]; then
    echo "eicrecon ends successfully"
    rm ${SIM_FILE}
  else
    echo "eicrecon error"
  fi
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
