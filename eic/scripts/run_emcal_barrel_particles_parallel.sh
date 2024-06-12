#!/bin/bash

set -e

if [[ ! -n  "${DETECTOR}" ]] ; then 
  export DETECTOR="athena"
fi

if [[ ! -n  "${JUGGLER_N_EVENTS}" ]] ; then 
  export JUGGLER_N_EVENTS=100
fi

if [[ ! -n  "${ETA_START}" ]] ; then
  export ETA_START="-1.7"
fi

if [[ ! -n  "${ETA_END}" ]] ; then
  export E_END=1.3
fi

if [[ ! -n  "${E_START}" ]] ; then
  export E_START=5.0
fi

if [[ ! -n  "${E_END}" ]] ; then
  export E_END=5.0
fi

if [[ ! -n  "${PARTICLE}" ]] ; then
  export PARTICLE="electron"
fi

export JUGGLER_GEN_FILE="${GEN_FILE}"
export JUGGLER_SIM_FILE="${SIM_FILE}"
export JUGGLER_REC_FILE="${REC_FILE}"

echo "JUGGLER_N_EVENTS = ${JUGGLER_N_EVENTS}"
echo "DETECTOR = ${DETECTOR}"

SIGNATURE="${TIME}_${JOB_NUMBER}"
GEN=benchmarks/barrel_ecal/scripts/emcal_barrel_particles_gen_eta_${SIGNATURE}
READ=benchmarks/barrel_ecal/scripts/emcal_barrel_particles_reader_parallel_${SIGNATURE}

cp benchmarks/barrel_ecal/scripts/emcal_barrel_particles_gen_eta.cxx ${GEN}.cxx && sed -ibackup_${SIGNATURE} "s/emcal_barrel_particles_gen_eta/emcal_barrel_particles_gen_eta_${SIGNATURE}/" ${GEN}.cxx

cp benchmarks/barrel_ecal/scripts/emcal_barrel_particles_reader_parallel.cxx ${READ}.cxx && sed -ibackup_${SIGNATURE} "s/emcal_barrel_particles_reader_parallel/emcal_barrel_particles_reader_parallel_${SIGNATURE}/" ${READ}.cxx

# Generate the input events
root -b -q "${GEN}.cxx+(${JUGGLER_N_EVENTS}, ${E_START}, ${E_END}, ${ETA_START}, ${ETA_END}, \"${PARTICLE}\")"
if [[ "$?" -ne "0" ]] ; then
  echo "ERROR running script: generating input events"
  rm -f ${GEN}* ${READ}*
  exit 1
fi
echo "input is generated"

# Plot the input events
root -b -q "${READ}+(\"${PARTICLE}\")"
if [[ "$?" -ne "0" ]] ; then
  echo "ERROR running script: plotting input events"
  rm -f ${GEN} ${READ}
  exit 1
fi
echo "input is readable"

echo "simulation starts"
rm -f ${GEN}* ${READ}*

ddsim --runType batch \
      -v WARNING \
      --filter.tracker edep0 \
      --numberOfEvents ${JUGGLER_N_EVENTS} \
      --compactFile ${DETECTOR_PATH}/${DETECTOR_CONFIG}.xml \
      --inputFiles ${GEN_FILE} \
      --outputFile ${SIM_FILE}
#      --part.minimalKineticEnergy 0.5*GeV  \

if [[ "$?" -ne "0" ]] ; then
  echo "ERROR running npdet"
  exit 1
fi
