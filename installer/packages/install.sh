#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$PACKAGES" ]; then
  echo "PACKAGES must be set"
  exit 1
fi

if [ -d "$PACKAGES" ]; then
  echo "$PACKAGES already exists"
else
  sudo mkdir "$PACKAGES"
fi

zz=`ps -ef|grep apache2`
if [ -z "$zz" ] ; then
  echo 'Apache2 not running' > /dev/null
  if [ -f /etc/init.d/apache2 ] ; then
    sudo /etc/init.d/apache2 start
  else
    sudo apt install apache2
    sudo touch "$PACKAGES/apache2"
  fi
fi

if [ -f /usr/bin/ffmpeg ]; then
  echo 'ffmpeg present' > /dev/null
else
  sudo apt install ffmpeg
  sudo touch "$PACKAGES/ffmpeg"
fi

if [ -f /usr/bin/dialog ]; then
  echo 'dialog present' > /dev/null
else
  sudo apt install dialog
  sudo touch "$PACKAGES/dialog"
fi

