#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$ETCDIR" ]; then
  echo "ETCDIR must be set"
  exit 1
fi

if [ -z "$CVUSER" ]; then
  echo "CVUSER must be set"
  exit 2
fi

if [ -z "$CVGROUP" ]; then
  echo "CVGROUP must be set"
  exit 3
fi

if [ -z "$CONFDIR" ]; then
  echo "CONFDIR must be set"
  exit 4
fi

if [ -d "$ETCDIR" ]; then
  echo 'already exists' > /dev/null
else
  sudo mkdir -p "$ETCDIR"
  sudo chown root:root "$ETCDIR"
  sudo chown 755 "$ETCDIR"
fi

MYNAME=`uname -n`
MYCONF=$CONFDIR/config.$MYNAME

if [ -f "$MYCONF" ]; then
  echo "Config for this machine exists" > /dev/null
else
  MYCONF=$CONFDIR/config.ini
  if [ -f "$MYCONF" ]; then
    echo "Generic config exists" > /dev/null
  else
    echo "No configuration file ($MYCONF) present. Aborting."
    exit 5
  fi
fi

sudo install -o root -g root -m 444 "$MYCONF" "$ETCDIR/config.ini"

INCLUDES=`../src/cam-config -listincludes -c "$MYCONF"`
for inc in $INCLUDES; do
  target="$ETCDIR/$inc"
  if [ -f "$target" ]; then
    echo 'there is already a config file there' > /dev/null
  else
    sudo install -o root -g root -m 444 "$CONFDIR/$inc" "$target"
  fi
done

if [ -f "$ETCDIR/config.ini" ]; then
  BACKUPDIR=`../src/cam-config -c $ETCDIR/config.ini -printvar BACKUP_DIR`
  if [ -z "$BACKUPDIR" ]; then
    echo "There is no BACKUP_DIR in $ETCDIR/config.ini!"
    exit 6
  else
    if [ -d "$BACKUPDIR" ]; then
      echo 'backup dir already there.  good.' > /dev/null
    else
      sudo mkdir -p "$BACKUPDIR"
    fi
    sudo chown -R "$CVUSER:$CVGROUP" "$BACKUPDIR/"
    sudo chmod 755 "$BACKUPDIR"
  fi

  DOWNLOADDIR=`../src/cam-config -c $ETCDIR/config.ini -printvar DOWNLOAD_DIR`
  if [ -z "$DOWNLOADDIR" ]; then
    echo "There is no DOWNLOAD_DIR in $ETCDIR/config.ini!"
    exit 7
  else
    if [ -d "$DOWNLOADDIR" ]; then
      echo 'DOWNLOAD dir already there.  good.' > /dev/null
    else
      sudo mkdir -p "$DOWNLOADDIR"
    fi
    sudo chown -R "$CVUSER:$CVGROUP" "$DOWNLOADDIR/"
    sudo chmod 755 "$DOWNLOADDIR"
  fi
fi
