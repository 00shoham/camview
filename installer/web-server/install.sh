#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$CGIDIR" ]; then
  echo "CGIDIR must be set"
  exit 1
fi
if [ -z "$VIRTUALDIR" ]; then
  echo "VIRTUALDIR must be set"
  exit 2
fi
if [ -z "$CVUSER" ]; then
  echo "CVUSER must be set"
  exit 3
fi
if [ -z "$CVGROUP" ]; then
  echo "CVGROUP must be set"
  exit 4
fi

if [ -z "$CONFDIR" ]; then
  echo "CONFDIR must be set"
  exit 5
fi

if [ -f "/etc/apache2/mods-enabled/cgi.conf" ]\
   && [ -f "/etc/apache2/mods-enabled/cgi.load" ]; then
  echo "CGI module is enabled"
else
  sudo /usr/sbin/a2enmod cgi
fi

MYNAME=`uname -n`
MYPASSWDFILE=$CONFDIR/htpasswd.$MYNAME
WEBCONFFILE=$CONFDIR/apache2-config.$MYNAME

if [ -f "$WEBCONFFILE" ]; then
  # there is a custom web configuration file - perhaps a multi-tenant setup.

  if [ -f "/etc/apache2/conf-enabled/cam-view.conf" ]; then
    echo "CGI to access cam-view is enabled"
  else
    sudo install -o root -g root -m 444 "$WEBCONFFILE"\
      /etc/apache2/conf-available/cam-view.conf
    sudo /usr/sbin/a2enconf cam-view
  fi

  # the a custom web config may reference its own .htpasswd file(s).
  # please locate them in /etc/apache2
  PASSWDFILES=`cat "$WEBCONFFILE" | grep AuthUserFile | sed 's|.*/||'`
  for pwfile in $PASSWDFILES; do
    if [ -f "$CONFDIR/$pwfile" ]; then
      if [ -f "/etc/apache2/$pwfile" ]; then
        echo "$pwfile already there" > /dev/null
      else
        sudo install -o root -g root -m 444 "$CONFDIR/$pwfile" "/etc/apache2"
      fi

      # See if there are config files to copy over for this
      # multi-tenant setup
      zone=`echo $pwfile | sed 's/\.htpasswd//'`
      if [ -f "$CONFDIR/config.$zone" ]; then
        cgidir=`cat "$WEBCONFFILE" | grep "ScriptAlias.*/$zone/" | sed 's/.* //'`
        if [ -d "$cgidir" ]; then
          echo "CGI dir already present" > /dev/null
        else
          sudo mkdir "$cgidir"
          sudo chown root:root "$cgidir"
          sudo chmod 755 "$cgidir"
        fi

        if [ -f "$cgidir/config.ini" ]; then
          echo "config already there" > /dev/null
        else
          sudo install -o root -g root -m 444 "$CONFDIR/config.$zone"\
            "$cgidir/config.ini" 
          includes=`../src/cam-config -listincludes -c "$CONFDIR/config.$zone"`
          for inc in $includes; do
            sudo install -o root -g root -m 444 "$CONFDIR/$inc" "$cgidir" 
          done
        fi
      fi
    else
      echo ".htpasswd file $CONFDIR/$pwfile is missing"
    fi
  done

else

  # the web config is standard - likely single tenant.
  if [ -f "/etc/apache2/conf-enabled/cam-view.conf" ]; then
    echo "CGI to access cam-view is already enabled" > /dev/null
  else
    MYCONF=$MYPATH/cam-view-nopasswd.conf
    if [ -f "$MYPASSWDFILE" ]; then
      MYCONF=$MYPATH/cam-view-passwd.conf
      if [ -f /etc/apache2/cam-view.htpasswd ]; then
        echo "Passwd file already present" > /dev/null
      else
        sudo install -o root -g root -m 444 "$MYPASSWDFILE" /etc/apache2/cam-view.htpasswd
      fi
    fi

    sudo sh -c "cat $MYCONF\
      | sed \"s|CGIDIR|/$CGIDIR/|g\"\
      | sed \"s|VIRTUALDIR|/$VIRTUALDIR/|g\"\
      | sed \"s|CVUSER|$CVUSER|g\"\
      | sed \"s|CVGROUP|$CVGROUP|g\"\
      | sed \"s|//|/|g\"\
      > /etc/apache2/conf-available/cam-view.conf"
    sudo /usr/sbin/a2enconf cam-view
  fi
fi

sudo systemctl reload apache2

