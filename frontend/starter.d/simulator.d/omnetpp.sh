#!/bin/bash

# $1 path to omnetpp
# $2 path to bundle
# $3 runtime
# $4 path to result folder

set -e

cd $1;
cp $2 in/bundle.xml;
docker-compose exec omnetpp /bin/bash -c "cd /opt/seed && make";
docker-compose exec omnetpp /bin/bash -c "cd /opt/seed && ./seed --sim-time-limit=$3s";
python3 postprocess.py seed/results/ $4;
