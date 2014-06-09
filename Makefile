GTK_VERSION = 3
PRJ_NAME = gtk$(GTK_VERSION)Imager
CC = gcc
CPU = $(findstring arm, $(shell uname -p))
#$(info ************ $(CPU) ************)
ifeq ($(CPU), arm)
CFLAGS = -Wall `pkg-config --cflags glib-2.0 gtk+-$(GTK_VERSION).0`
else
ARCH = $(shell getconf LONG_BIT)
CFLAGS_32 = -Wall `pkg-config --cflags glib-2.0 gtk+-$(GTK_VERSION).0`
CFLAGS_64 = -Wall `pkg-config --cflags glib-2.0 gtk+-$(GTK_VERSION).0` -B/usr/lib/x86_64-linux-gnu
CFLAGS =  $(CFLAGS_$(ARCH))
endif
LDFLAGS =
#LDLIBS = `pkg-config --libs glib-2.0 gtk+-$(GTK_VERSION).0` -lcfitsio -lusb-1.0 -lrt -lm
LDLIBS = `pkg-config --libs glib-2.0 gtk+-$(GTK_VERSION).0` -I./libusb-custom/libusb ./libusb-custom/libusb/.libs/libusb-1.0.a -ludev -lcfitsio -lsbigudrv -lrt -lm -lpthread

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

$(PRJ_NAME):imgMain
	mv $< $@

imgMain:$(OBJ)

OSI_DIR = OpenSkyImager
INSTALL_DIR = /usr/local/bin/$(OSI_DIR)
.PHONY: install
install:$(PRJ_NAME)
	rm -rf $(INSTALL_DIR)
	mkdir -p $(INSTALL_DIR)
	cp -p $(PRJ_NAME) $(INSTALL_DIR)
	cp -p -r fr $(INSTALL_DIR)
	cp -p -r it $(INSTALL_DIR)
	cp -p -r zh $(INSTALL_DIR)
	cp -p -r po $(INSTALL_DIR)
	cp -p qhyReset.bash $(INSTALL_DIR)
	cp -p *.png $(INSTALL_DIR)
	chmod a+rx $(INSTALL_DIR)/$(PRJ_NAME)
	chmod a+rx $(INSTALL_DIR)/qhyReset.bash

.PHONY: update
update:$(PRJ_NAME)
	cp -p $(PRJ_NAME) $(INSTALL_DIR)
	cp -p -r fr $(INSTALL_DIR)
	cp -p -r it $(INSTALL_DIR)
	cp -p -r zh $(INSTALL_DIR)
	cp -p -r po $(INSTALL_DIR)
	cp -p qhyReset.bash $(INSTALL_DIR)
	cp -p *.png $(INSTALL_DIR)
	chmod a+rx $(INSTALL_DIR)/$(PRJ_NAME)
	chmod a+rx $(INSTALL_DIR)/qhyReset.bash

.PHONY: clean
clean:
	rm -f *.o
	rm -f $(PRJ_NAME)
