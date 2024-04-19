#!/usr/bin/env bash

CONTAINER_ID=$(docker ps | grep eicweb | cut -d ' ' -f 1)
docker exec ${CONTAINER_ID} eic-shell "bash job.sh $1 $2 $3"
