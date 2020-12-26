#!/bin/sh

find . -type f -name '*.sh' | xargs chmod 755

.  ./global-variables.sh

while getopts c:h flag; do
  case "${flag}" in
    c) CONFDIR=${OPTARG};;
    h) echo "USAGE: $0 [-c confdir]"; exit 0;;
  esac
done

if [ -z "$CONFDIR" ]; then
  echo "You must specify a CONFDIR in global-variables.sh or via -c argument"
  exit 1
fi

if [ -d "$CONFDIR" ]; then
  echo "ok" > /dev/null
else
  echo "$CONFDIR must actually exist..." > /dev/null
  exit 2
fi

for dir in $INSTDIRS; do
  inst=$dir/install.sh
  if [ -f "$inst" ]; then
    $inst
    if [ $? -eq 0 ] ; then
      echo 'that worked' > /dev/null
    else
      echo "$inst returned an error code - $?"
      exit $?
    fi
  fi
done

# start the daemon:
sudo su -c '/usr/local/bin/cam-view-health-check.sh' - "$CVUSER" 
