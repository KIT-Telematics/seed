#!/usr/bin/env bash

cd $1;
CXXFLAGS="-Werrror" ./waf --run "scratch/seed --bundle=$2 --runtime=$3 --bindings=$5"
