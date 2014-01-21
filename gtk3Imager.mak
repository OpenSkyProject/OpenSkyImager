CC = gcc `pkg-config --cflags glib-2.0 gtk+-3.0 cfitsio libusb-1.0`
CFLAGS = -Wall

LIBS = `pkg-config --libs glib-2.0 gtk+-3.0 cfitsio libusb-1.0` -lrt -lm

OBJS = \
	tools.o \
	gtkTools.o \
	imgFitsio.o \
	imgPixbuf.o \
	imgWindow.o \
	imgWFuncs.o \
	imgWCallbacks.o \
	imgCamio.o \
	qhy2old.o \
	qhy5.o \
	qhy5ii.o \
	qhy6.o \
	qhy6old.o \
	qhy7.o \
	qhy8old.o \
	qhy8l.o \
	qhy9.o \
	qhy11.o \
	qhycore.o \
	libusbio.o \
	imgCFWio.o \
	avilib.o \
	imgAvi.o \
	ttylist.o \
	ttycom.o \
	imgMain.o 

all: gtk3Imager

gtk3Imager: $(INCS) $(OBJS) 
	$(CC) -o $@ $(OBJS) $(LIBS) $(CFLAGS)

clean:
	rm -fr *.o gtk3Imager
