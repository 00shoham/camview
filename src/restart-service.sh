#!/bin/sh

sudo /bin/su -c /usr/local/bin/cam-view-stop.sh - camview
sudo /bin/su -c /usr/local/bin/cam-view-health-check.sh - camview
