#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$CVUSER" ]; then
  echo "CVUSER must be set"
  exit 1
fi

MYNAME=`uname -n`

zz=`cat /etc/passwd | grep "$CVUSER"`
if [ -z "$zz" ]; then
  password=`tail -30 /var/log/syslog | md5sum | awk '{print $1}'`
  sudo /usr/sbin/useradd\
    --system\
    --create-home\
    --comment "Cam-View Service Account"\
    --password "$password"\
    --shell "/bin/sh"\
    --user-group\
    "$CVUSER"
fi

if [ -d "$CONFDIR/.ssh.$MYNAME" ]; then
  if [ -d "/home/$CVUSER/.ssh" ]; then
    echo ".ssh folder already there." > /dev/null
  else
    sudo mkdir /home/$CVUSER/.ssh
    sudo chown $CVUSER:$CVGROUP /home/$CVUSER/.ssh
    sudo chmod 700 /home/$CVUSER/.ssh
    sudo install -o $CVUSER -g $CVGROUP -m 600 $CONFDIR/.ssh.$MYNAME/*\
                 /home/$CVUSER/.ssh
    sudo chmod 644 /home/$CVUSER/.ssh/*pub
    sudo chmod 644 /home/$CVUSER/.ssh/known_hosts
  fi
fi
