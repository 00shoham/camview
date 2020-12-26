#!/bin/sh

make clean && make && ./stop-cam-view.sh
./cam-view-health-check.sh
