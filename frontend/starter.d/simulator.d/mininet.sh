#!/bin/bash

# $1 path to seed
# $2 path to bundle
# $3 runtime

cd $1;
#if sudo docker inspect -f {{.State.Running}} mininet
#then
#	echo "Container not running - running compose"
#	sudo docker-compose up -d --build
#fi

echo Copying bundle from $2
cp $2 bundle.xml;
sudo docker exec -it mininet python2 /seed/seed.py  #--sim-time-limit $3"


