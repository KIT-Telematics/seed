#!/usr/bin/env bash

# $1 path to ns3
# $2 path to bundle
# $3 runtime
# $4 path to result folder
# $5 path to bindings

set -e

cd $1;
cp $2 io/bundle.xml;
cp $5 io/bindings.yml;
docker-compose exec ns-3 ./waf --run "scratch/seed --bundle=/home/ns-3/io/bundle.xml --runtime=$3"
