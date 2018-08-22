#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

DOCKER_IMAGE=${DOCKER_IMAGE:-dashpay/dashd-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/dmsd docker/bin/
cp $BUILD_DIR/src/dms-cli docker/bin/
cp $BUILD_DIR/src/dms-tx docker/bin/
strip docker/bin/dmsd
strip docker/bin/dms-cli
strip docker/bin/dms-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
