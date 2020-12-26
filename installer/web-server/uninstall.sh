#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$CGIDIR" ]; then
  echo "CGIDIR must be set"
  exit 1
fi
if [ -z "$CONFDIR" ]; then
  echo "CONFDIR must be set"
  exit 2
fi

MYNAME=`uname -n`
MYPASSWDFILE=$CONFDIR/htpasswd.$MYNAME
WEBCONFFILE=$CONFDIR/apache2-config.$MYNAME

if [ -f "/etc/apache2/conf-enabled/cam-view.conf" ]; then
  PASSWDFILES=`cat "/etc/apache2/conf-enabled/cam-view.conf" | grep AuthUserFile | sed 's|.*/||'`
  for pwfile in $PASSWDFILES; do
    if [ -f "/etc/apache2/$pwfile" ]; then
      sudo rm "/etc/apache2/$pwfile"

      zone=`echo $pwfile | sed 's/\.htpasswd//'`
      cgidir=`cat "/etc/apache2/conf-enabled/cam-view.conf"\
        | grep "ScriptAlias.*/$zone/" | sed 's/.* //'`
      if [ -d "$cgidir" ]; then
        sudo rm -rf "$cgidir"
      fi
    fi
  done

  sudo rm "/etc/apache2/conf-enabled/cam-view.conf"
  sudo rm "/etc/apache2/conf-available/cam-view.conf"
fi


# NOTE: we may have enabled the CGI module.
#       since those may have pre-existed our install,
#       we will just leave them in place.  Sorry.

if [ -f "/etc/apache2/conf-enabled/cam-view.conf" ]; then
  sudo rm "/etc/apache2/conf-enabled/cam-view.conf" 
  sudo rm "/etc/apache2/conf-available/cam-view.conf" 
fi

sudo /etc/init.d/apache2 reload
