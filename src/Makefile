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

all: $(BIN)

CC=gcc
CFLAGS=-Wall -O2 -g
LIBS=-ljpeg -lpthread -lm
SHAREDOBJ=\
  cgi.o\
  config.o\
  daemon.o\
  error.o\
  util.o\
  ping.o\
  image.o\
  camera.o\
  nargv.o\
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

$(BIN): % : %.o $(SHAREDOBJ)
	$(CC) $(CFLAGS) -o $@ $< $(SHAREDOBJ) $(LIBS)

clean: NULL
	rm -f $(SHAREDOBJ) $(BIN)
	rm -f *.o

NULL:
