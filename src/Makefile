WEBBIN=\
  single-image\
  download-recordings\
  cam-view\
  cam-status\

WEBETC=\
  cam-view.template\
  download.template\
  cam-status-foot.template\
  cam-status-head.template\

WEBDOCS=\
  favicon.ico\
  left-small.png\
  right-small.png

BIN=\
  cam-monitor\
  cam-archive\
  cam-config\
  jpegtest\
  imagediff\
  brightenimage\
  testimagelist\
  $(WEBBIN)

USER=nobody
GROUP=nogroup
CGIDIR=/var/www/cgi-bin
HTMLDIR=/var/www/html
#STAGEDIR=/var/www/stage
WEBLOG=/mnt/ramdisk/security/cgi.log

LUADIR := $(shell /usr/bin/find /usr/include -type d -name 'lua*' | sort | tail -1)
LUAPARTS := $(subst /, ,$(LUADIR))
LUAVER := $(lastword $(LUAPARTS))
INCLUDES=-I$(LUADIR)

all: $(BIN)

UTILDIR=../../idanlib/src
UTILSLIB=$(UTILDIR)/utils.a

CC=gcc
# CFLAGS=-Wall -g -rdynamic -fsanitize=address -I$(UTILDIR) $(INCLUDES)
CFLAGS=-Wall -O2 -I$(UTILDIR) $(INCLUDES)
LIBS=-ljpeg -lpthread -lm -luuid
SHAREDOBJ=\
  access.o\
  cgi.o\
  config.o\
  image.o\
  camera.o\
  filenames.o\
  jpeg.o\

%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) -c $<

install: $(BIN) $(WEBBIN) $(WEBDOCS) $(WEBETC)
	chmod 755 ./replace-binaries.sh
	./replace-binaries.sh

stop: NULL
	chmod 755 ./stop.sh
	./stop.sh

$(BIN): % : %.o $(SHAREDOBJ) $(UTILSLIB)
	$(CC) $(CFLAGS) -o $@ $< $(SHAREDOBJ) $(UTILSLIB) $(LIBS)

clean: NULL
	rm -f $(SHAREDOBJ) $(BIN)
	rm -f *.o

NULL:
