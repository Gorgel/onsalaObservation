### Makefile for the XS package version
XS_VERSION= 1.2.18
###

BIN_DIR= /usr/local/bin
PGHOME = /usr/local/pgplot
LESSTIF= /usr/local/LessTif

### Use -DBYTESWAP for Intel (& Alpha) processors
###
OPTS = -O2 -Wall -DBYTESWAP
#OPTS = -O2 -Wall -DBYTESWAP -DSTATIC
XVER = X11R6
XLIB = -L/usr/$(XVER)/lib/
LESSLIB = -Wl,--rpath -Wl,$(LESSTIF)/lib $(LESSTIF)/lib/libXm.so
# We need libpng and libz for PGPLOT to handle .png files
# -lpng and -lz can be removed if the png PGPLOT driver isn't used
PGLIB = -L$(PGHOME) -lXmPgplot -lcpgplot -lpgplot -lpng -lz

LIBS = -lm $(PGLIB) $(XLIB) -lXm -lXp -lXpm -lXt -lXext -lSM -lICE -lX11
#LIBS = -lm $(PGLIB) $(LESSLIB) $(XLIB) -lXpm -lXt -lXext -lSM -lICE -lX11
LDOPTS = -O2
# Uncomment below to force use of static libs when linking
#LDOPTS = -O2 -static

XINC = -I/usr/$(XVER)/include
LESSINC = -I$(LESSTIF)/include
PIXDIR = /usr/$(XVER)/include/X11/pixmaps
PGINC = -I$(PGHOME)
#CFLAGS = $(OPTS) $(LESSINC) $(PGINC)
CFLAGS = $(OPTS) $(XINC) $(PGINC)

CC = gcc
LD = g77

### All C-file names in the XS package
FILES = average\
        baseline\
        boxes\
        channelmod\
	class\
        clip\
        color\
        convolve\
        coord\
        dfil\
        dialogs\
        drawing\
        fft\
        fftn\
        files\
        filter\
        fits\
        fitsBT\
        fold\
        freqfiles\
        gauss\
        gauss2D\
        getants\
        getdrp\
        getfits\
        help\
        interpolate\
        list\
        lmfit\
        lmfnc\
        macro\
        map\
        maphandler\
        mapinterpolate\
        mapvp\
        marks\
        memcalc\
        memfront\
        menus\
        misc\
        msgs\
        polyline\
        prefs\
        ps\
        redres\
        scale\
        scanhandler\
        scatter\
        scatterhandler\
        shift\
        simq\
        smooth\
        testmap\
        view\
        wprintf\
        xs

### All include files.
INCS = autoread.h\
       button_defs.h\
       class.h\
       defines.h\
       dfil.h\
       dialogs.h\
       drp.h\
       fftn.h\
       files.h\
       fits.h\
       global_structs.h\
       global_vars.h\
       list.h\
       lmfit.h\
       local.h\
       map.h\
       mem.h\
       menu_defs.h\
       menus.h\
       msgs.h\
       ps.h\
       scatter.h\
       xs.h

### All pixmap files.
PIXS  = gauss.xpm msgs.xpm xs.xpm

### Makefiles
MAKES = Makefile.sun Makefile.linux Makefile.hp COPYING README

### Directories
DIRS = data

### All *.o files
OBJ = $(FILES:%=%.o)

### All *.c files
SOU = $(FILES:%=%.c)

### ALL *.o files that depend on define.h
DEFOBJ = average.o baseline.o boxes.o channelmod.o drawing.o fft.o\
         files.o fold.o freqfiles.o gauss.o interpolate.o map.o\
         maphandler.o marks.o msgs.o redres.o scale.o scanhandler.o\
         smooth.o view.o xs.o getants.o getfits.o mapvp.o ps.o\
         convolve.o memfront.o memcalc.o misc.o prefs.o clip.o shift.o\
         filter.o color.o scatterhandler.o scatter.o getdrp.o help.o\
         mapinterpolate.o fitsBT.o coord.o testmap.o macro.o polyline.o\
         gauss2D.o

### All *.o files that depend on menus.h
MENUOBJ = menu.o scatter.o map.o xs.o ps.o shift.o marks.o prefs.o\
          polyline.o gauss.o gauss2D.o baseline.o smooth.o redres.o boxes.o

### ALL *.o files that depend on list.h
LISTOBJ = $(MENUOBJ) list.o maphandler.o scanhandler.o view.o convolve.o\
          memfront.o scatterhandler.o mapinterpolate.o fitsBT.o\
          testmap.o

### All *.o files that depend on local.h
LOCALOBJ = prefs.o

### All *.o files that depend on fits.h
FITSOBJ = fits.o getants.o getfits.o prefs.o fitsBT.o

### All *.o files that depend on drp.h
DRPOBJ = getants.o getfits.o filter.o getdrp.o fitsBT.o

### All *.o files that depend on mem.h
MEMOBJ = memfront.o memcalc.o

### All *.o files that depend on dialogs.h
DIAOBJ = baseline.o boxes.o clip.o convolve.o coord.o dialogs.o fold.o\
         gauss.o gauss2D.o macro.o map.o maphandler.o mapvp.o polyline.o\
         prefs.o ps.o redres.o scale.o scanhandler.o scatter.o\
         scatterhandler.o shift.o smooth.o testmap.o xs.o

xs:  $(OBJ) Makefile
	$(LD) $(OBJ) -o xs $(LDOPTS) $(LIBS)

xs.o: menu_defs.h button_defs.h global_vars.h autoread.h xs.h

prefs.o: prefs.c
	$(CC) $(CFLAGS) -o $@ -c prefs.c -DXS_DIR=\"`pwd`/\"

msgs.o: msgs.h

scatter.o: scatter.h

map.o: map.h

files.o: files.h

$(MENUOBJ): menus.h

$(DEFOBJ): defines.h global_structs.h

$(LISTOBJ): list.h

$(LOCALOBJ): local.h

$(FITSOBJ): fits.h

$(DRPOBJ): drp.h

$(MEMOBJ): mem.h

$(DIAOBJ): dialogs.h

lmfit.o lmfnc.o: lmfit.h

fft.o fftn.o: fftn.h

dfil.o getants.o: dfil.h

ps.o: ps.h

class.o: class.h

clean:
	rm -f xs *.o

tar:
	tar -cf xs-$(XS_VERSION).tar $(SOU) $(INCS) $(PIXS) $(MAKES) $(DIRS)
	gzip xs-$(XS_VERSION).tar
	ls -l xs*.tar.gz

install:
	mv xs $(BIN_DIR)/.
#	cp *.xpm $(PIXDIR)/.

ftpinstall:
	cp xs-$(XS_VERSION).tar.gz /local/ftp/pub/xs/.
	cp xs.gz /local/ftp/pub/xs/xs-$(XS_VERSION)-linux.gz
