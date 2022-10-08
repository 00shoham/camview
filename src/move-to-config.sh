#!/bin/sh

BINS="\
  brightenimage\
  cam-archive\
  cam-config\
  cam-monitor\
  cam-status
  cam-view\
  download-recordings\
  imagediff\
  jpegtest\
  single-image\
  testimagelist"

for f in $BINS; do
  cp $f ../../v/src
done
