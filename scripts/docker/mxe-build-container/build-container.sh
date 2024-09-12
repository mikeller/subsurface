#!/bin/bash
set -x
set -e

# known good MXE sha
MXE_SHA="974808c2ecb02866764d236fe533ae57ba342e7a"
SCRIPTPATH=$(dirname $0)

# version of the docker image
VERSION=3.2.0

pushd $SCRIPTPATH

# we use the 'experimental' --squash argument to significantly reduce the size of the massively huge
# Docker container this produces
docker build -t subsurface/mxe-build:$VERSION --build-arg=mxe_sha=$MXE_SHA -f Dockerfile .
docker images
popd
