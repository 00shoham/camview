#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$ETCDIR" ]; then
  echo "ETCDIR must be set"
  exit 1
fi

if [ -f "$ETCDIR/config.ini" ]; then
  DOWNLOADDIR=`../src/cam-config -c $ETCDIR/config.ini -printvar DOWNLOAD_DIR`
  if [ -d "$DOWNLOADDIR" ]; then
    sudo rm -rf "$DOWNLOADDIR"
  else
    echo 'download dir already gone.' > /dev/null
  fi

  BACKUPDIR=`../src/cam-config -c $ETCDIR/config.ini -printvar BACKUP_DIR`
  if [ -d "$BACKUPDIR" ]; then
    dialog --title "Remove $BACKUPDIR?" \
           --yesno "Would you like to remove $BACKUPDIR?" 7 60\
           --defaultno
    response=$?
    case $response in
       0) sudo rm -rf "$BACKUPDIR"
    esac
  else
    echo 'backup dir already gone.' > /dev/null
  fi
fi

if [ -d "$ETCDIR" ]; then
  sudo rm -rf "$ETCDIR"
else
  echo 'nothing there' > /dev/null
fi

