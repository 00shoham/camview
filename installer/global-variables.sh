# replace this to pull configuration files from your own folder:
CONFDIR=../config

CVUSER=camview
CVGROUP=$CVUSER
WEBDIR=/var/www/html/cam-view
CGIDIR=/var/www/cam-view-bin
WEBLOG=/mnt/ramdisk/security/cgi.log
ETCDIR=/usr/local/etc/cam-view
BINDIR=/usr/local/bin
RAMPATH=/mnt/ramdisk
SECPATH=$RAMPATH/security

# this list of subdirs with install stuff is ordered.
INSTDIRS="packages user bin config ramdisk web-server web-content scripts"
UNINSTDIRS="scripts web-content web-server ramdisk config bin user packages"
