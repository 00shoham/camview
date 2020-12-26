#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$BINDIR" ]; then
  echo "BINDIR must be set"
  exit 1
fi

if [ -z "$SCRIPTS" ]; then
  echo "SCRIPTS must be set"
  exit 2
fi

if [ -z "$RAMPATH" ]; then
  echo "RAMPATH must be set"
  exit 4
fi

WORKFILE=`mktemp`
sudo crontab -l -u "$CVUSER" > $WORKFILE

for scr in $SCRIPTS; do
  txt=$scr.txt
  sh=$BINDIR/$scr.sh
  sched=`cat "$MYPATH/$txt" | grep '^#SCHEDULE:' | sed 's/^#SCHEDULE: //'`

  if [ -f "$sh" ]; then
    echo "already there" > /dev/null
  else
    sudo sh -c "cat \"$MYPATH/$txt\"\
      | sed \"s|BINDIR|$BINDIR|g\"\
      | sed \"s|ETCDIR|$ETCDIR|g\"\
      | sed \"s|//|/|g\"\
      > \"$sh\""
    sudo chown root:root "$sh"
    sudo chmod 755 "$sh"

    if [ -z "$sched" ]; then
      echo "no schedule" > /dev/null
    else
      zz=`cat $WORKFILE | grep $scr`
      if [ -z "$zz" ]; then
        echo "$sched $sh 2>> $RAMPATH/$scr.log" >> $WORKFILE
      fi
    fi
  fi
done

sudo crontab -u "$CVUSER" $WORKFILE
