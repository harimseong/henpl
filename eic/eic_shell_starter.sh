#!/usr/bin/env bash
echo "log from $0"

CONTAINER_ID=$(docker ps | grep eicweb | cut -d ' ' -f 1)
echo "CONTAINER_ID=$CONTAINER_ID"
docker exec ${CONTAINER_ID} eic-shell "bash eic_job.sh $1 $2 $3"
