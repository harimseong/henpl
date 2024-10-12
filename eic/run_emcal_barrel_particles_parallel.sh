#!/bin/bash

set -e

if [ -z "${GEN_FILE}" -o -z "${SIM_FILE}" -o -z "${REC_FILE}" ]; then
  echo "necessary environment variables are not set"
  exit
fi

if [ -z "${DETECTOR}" ] ; then 
  export DETECTOR="athena"
fi

if [ -z "${JUGGLER_N_EVENTS}" ] ; then 
  export JUGGLER_N_EVENTS=100
fi

if [ -z "${ETA_START}" ] ; then
  export ETA_START="-1.7"
fi

if [ -z "${ETA_END}" ] ; then
  export E_END=1.3
fi

if [ -z "${E_START}" ] ; then
  export E_START=5.0
fi

if [ -z "${E_END}" ] ; then
  export E_END=5.0
fi

if [ -z "${PARTICLE}" ] ; then
  export PARTICLE="electron"
fi

export JUGGLER_GEN_FILE="${GEN_FILE}"
export JUGGLER_SIM_FILE="${SIM_FILE}"
export JUGGLER_REC_FILE="${REC_FILE}"

echo "JUGGLER_N_EVENTS = ${JUGGLER_N_EVENTS}"
echo "DETECTOR = ${DETECTOR}"

SIGNATURE="${TIME}_${JOB_NUMBER}"
INPUT=benchmarks/barrel_ecal/scripts/emcal_barrel_particles_gen_eta_${SIGNATURE}
INPUT_FILE="${INPUT}.cxx"
READ=benchmarks/barrel_ecal/scripts/emcal_barrel_particles_reader_parallel_${SIGNATURE}
READ_FILE="${READ}.cxx"

cp benchmarks/barrel_ecal/scripts/emcal_barrel_particles_gen_eta.cxx ${INPUT_FILE} && sed -ibackup_${SIGNATURE} "s/emcal_barrel_particles_gen_eta/emcal_barrel_particles_gen_eta_${SIGNATURE}/" ${INPUT_FILE}

cp benchmarks/barrel_ecal/scripts/emcal_barrel_particles_reader_parallel.cxx ${READ_FILE} && sed -ibackup_${SIGNATURE} "s/emcal_barrel_particles_reader_parallel/emcal_barrel_particles_reader_parallel_${SIGNATURE}/" ${READ_FILE}

# Generate the input events
root -b -q "${INPUT_FILE}+(${JUGGLER_N_EVENTS}, ${E_START}, ${E_END}, ${ETA_START}, ${ETA_END}, \"${PARTICLE}\")"
if [[ "$?" -ne "0" ]] ; then
  echo "ERROR running script: generating input events"
  rm -f ${INPUT}* ${READ}*
  exit 1
fi
echo "input is generated"

# Plot the input events
root -b -q "${READ_FILE}+(\"${PARTICLE}\")"
if [[ "$?" -ne "0" ]] ; then
  echo "ERROR running script: plotting input events"
  rm -f ${INPUT}* ${READ}*
  exit 1
fi
echo "input is readable"

echo "simulation starts"
rm -f ${INPUT_FILE} ${READ_FILE}

ddsim --runType batch \
      -v WARNING \
      --filter.tracker edep0 \
      --numberOfEvents ${JUGGLER_N_EVENTS} \
      --compactFile ${DETECTOR_PATH}/${DETECTOR_CONFIG}.xml \
      --inputFiles ${GEN_FILE} \
      --outputFile ${SIM_FILE} \
      --part.minimalKineticEnergy 0.5*GeV

if [[ "$?" -ne "0" ]] ; then
  echo "ERROR running npdet"
  exit 1
fi
