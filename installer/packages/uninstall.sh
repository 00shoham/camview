#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$PACKAGES" ]; then
  echo "PACKAGES must be set"
  exit 1
fi

if [ -f "$PACKAGES/apache2" ]; then
  dialog --title "Remove apache2?" \
         --yesno "Would you like to remove apache2?" 7 60\
         --defaultno
  response=$?
  case $response in
     0) sudo apt remove apache2
  esac
fi

if [ -f "$PACKAGES/ffmpeg" ]; then
  dialog --title "Remove ffmpeg?" \
         --yesno "Would you like to remove ffmpeg?" 7 60\
         --defaultno
  response=$?
  case $response in
     0) sudo apt remove ffmpeg
  esac
fi

if [ -f "$PACKAGES/dialog" ]; then
  dialog --title "Remove dialog?" \
         --yesno "Would you like to remove dialog?" 7 60\
         --defaultno
  response=$?
  case $response in
     0) sudo apt remove dialog
  esac
fi

if [ -d "$PACKAGES" ]; then
  sudo rm -rf $PACKAGES
fi

