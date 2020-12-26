#!/bin/sh

# run this from its parent directory!

MYPATH=`echo $0 | sed 's/[a-zA-Z0-9\._-]*$//'`

. $MYPATH/local-variables.sh

if [ -z "$CVUSER" ]; then
  echo "CVUSER must be set"
  exit 1
fi
if [ -z "$CVGROUP" ]; then
  echo "CVGROUP must be set"
  exit 2
fi
if [ -z "$WEBETC" ]; then
  echo "WEBETC must be set"
  exit 3
fi
if [ -z "$WEBBIN" ]; then
  echo "WEBBIN must be set"
  exit 4
fi
if [ -z "$WEBDOCS" ]; then
  echo "WEBDOCS must be set"
  exit 5
fi
if [ -z "$WEBDIR" ]; then
  echo "WEBDIR must be set"
  exit 6
fi
if [ -z "$CGIDIR" ]; then
  echo "CGIDIR must be set"
  exit 7
fi
if [ -z "$WEBLOG" ]; then
  echo "WEBLOG must be set"
  exit 8
fi

VALIDUSER=`/bin/cat /etc/passwd | /bin/grep "^$CVUSER:"`
if [ -z "$VALIDUSER" ]; then
  echo "User $CVUSER is not valid"
  exit 9
fi

VALIDGROUP=`/bin/cat /etc/group | /bin/grep "^$CVGROUP:"`
if [ -z "$VALIDGROUP" ]; then
  echo "Group $CVGROUP is not valid"
  exit 10
fi

MYNAME=`uname -n`
WEBCONFFILE=$CONFDIR/apache2-config.$MYNAME
if [ -f "$WEBCONFFILE" ]; then
  CGIDIR=`cat "$WEBCONFFILE" | grep ScriptAlias | awk '{print $3}'`
fi

for oneDir in $CGIDIR; do
  if [ -d "$oneDir" ]; then
    echo -e "$oneDir ok" > /dev/null
  else
    sudo mkdir -p "$oneDir"
    sudo chown root:root "$oneDir"
    if [ -d "$oneDir" ]; then
      echo "Created $oneDir"
    else
      echo "Path $oneDir does not exist"
      exit 11
    fi
  fi
done

if [ -d "$WEBDIR" ]; then
  echo -e "$WEBDIR ok" > /dev/null
else
  sudo mkdir -p "$WEBDIR"
  sudo chown "root:root" "$WEBDIR"
  if [ -d "$WEBDIR" ]; then
    echo "Created $WEBDIR"
  else
    echo "Path $WEBDIR does not exist"
    exit 12
  fi
fi

for file in $WEBETC; do
  srcFile=../src/$file
  if [ -f "$srcFile" ]; then
    for oneDir in $CGIDIR; do
      echo "Install $file in $oneDir"
      /usr/bin/sudo /usr/bin/install -o root -g root -m 444 "$srcFile" "$oneDir/"
    done
  else
    echo "Cannot locate $srcFile"
  fi
done

for file in $WEBBIN; do
  srcFile=../src/$file
  if [ -f "$srcFile" ]; then
    for oneDir in $CGIDIR; do
      echo "Install $file in $oneDir"
      /usr/bin/sudo /usr/bin/install -o $CVUSER -g $CVGROUP -m 6555 "$srcFile" "$oneDir/"
    done
  else
    echo "Cannot locate $file"
  fi
done

for file in $WEBDOCS; do
  srcFile=../src/$file
  if [ -f "$srcFile" ]; then
    echo "Install $file in $WEBDIR"
    /usr/bin/sudo /usr/bin/install -o root -g root -m 444 "$srcFile" "$WEBDIR/"
  else
    echo "Cannot locate $file"
  fi
done

sudo touch "$WEBLOG"
sudo chown "$CVUSER:$CVGROUP" "$WEBLOG"
sudo chmod 666 "$WEBLOG"
