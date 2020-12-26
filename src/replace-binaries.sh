#!/bin/sh

WEBBIN="single-image download-recordings cam-view cam-status"
CLIBIN="cam-monitor cam-archive cam-config"

for wb in $WEBBIN; do
  for t in `find /var/www -name "$wb"`; do
    sudo install -o camview -g camview -m 6555 "$wb" "$t"
  done
done

for js in "cam-view.js"; do
  for t in `find /var/www -name "$js"`; do
    sudo install -o root -g root -m 644 "$js" "$t"
  done
done

for lb in $CLIBIN; do
  sudo install -o root -g root -m 755 "$lb" /usr/local/bin
done

sudo /bin/su -c /usr/local/bin/cam-view-stop.sh - camview
sudo /bin/su -c /usr/local/bin/cam-view-health-check.sh - camview
