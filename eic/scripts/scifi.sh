#!/usr/bin/bash

# variables
# [MIN_T, MAX_T) with DIFF step
# number of events NEVETS per one simulation
MIN_T=0.03
MAX_T=1.00
DIFF=0.01
NEVENTS=100


# Environment variable setup
rm -f .env
if [ ! -n "${LOCAL_PREFIX}" ] ; then
  source .local/bin/env.sh
else
  source ${LOCAL_PREFIX}/bin/env.sh
fi
${LOCAL_PREFIX}/bin/print_env.sh


# build and compile epic detector in ${DETECTOR_PREFIX}/${DETECTOR}
rm -rf ${DETECTOR_PREFIX}/${DETECTOR}_build
mkdir -p ${DETECTOR_PREFIX}/${DETECTOR}_build
pushd ${DETECTOR_PREFIX}/${DETECTOR}_build
cmake ${DETECTOR_PREFIX}/${DETECTOR} -DCMAKE_INSTALL_PREFIX=${LOCAL_PREFIX} -DCMAKE_CXX_STANDARD=17 && make -j$(($(nproc)/4+1)) install || exit 1
popd


# run simulations
for i in $(seq $MIN_T $DIFF $MAX_T)
do
  ./sim_run.sh -v $i -n $NEVENTS
done


rm -rf ${DETECTOR_PREFIX}/${DETECTOR}_build
