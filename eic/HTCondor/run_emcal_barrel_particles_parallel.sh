#!/bin/bash

if [[ ! -n  "${DETECTOR}" ]] ; then 
  export DETECTOR="athena"
fi

if [[ ! -n  "${JUGGLER_N_EVENTS}" ]] ; then 
  export JUGGLER_N_EVENTS=100
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

GEN=benchmarks/barrel_ecal/scripts/emcal_barrel_particles_gen_${TIME}_${JOB_NUMBER}.cxx

cp benchmarks/barrel_ecal/scripts/emcal_barrel_particles_gen.cxx ${GEN} && sed -ibackup_${JOB_NUMBER} "s/emcal_barrel_particles_gen/emcal_barrel_particles_gen_${TIME}_${JOB_NUMBER}/" ${GEN}

# Generate the input events
root -b -q "${GEN}+(${JUGGLER_N_EVENTS}, ${E_START}, ${E_END}, \"${PARTICLE}\")"
if [[ "$?" -ne "0" ]] ; then
  echo "ERROR running script: generating input events"
  exit 1
fi

rm ${GEN}

ddsim --runType batch \
      -v WARNING \
      --part.minimalKineticEnergy 0.5*GeV  \
      --filter.tracker edep0 \
      --numberOfEvents ${JUGGLER_N_EVENTS} \
      --compactFile ${DETECTOR_PATH}/${DETECTOR_CONFIG}.xml \
      --inputFiles ${GEN_FILE} \
      --outputFile ${SIM_FILE}

if [[ "$?" -ne "0" ]] ; then
  echo "ERROR running npdet"
  exit 1
fi
