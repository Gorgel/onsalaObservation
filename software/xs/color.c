/**************************** XS ********************************************
Copyright (C) 2000-2012  P. Bergman

This program is free software; you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
software; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA  02111-1307 USA                             
*****************************************************************************/
#include <stdio.h>
#include <Xm/Xm.h>

#include "defines.h"
#include "global_structs.h"

/* The MAXSHADES and FALSE_MAGN values are suitable for PseudoColor visual
   types. For TrueColor visual types we are using twice as large values. */
#define MAXSHADES      48
#define FALSE_MAGN      6

/*** External variables ***/
extern GLOBAL *gp;
void PostWarningDialog(Widget, char *);

/*** Local variables ***/
static int false_magn;
static COLOR gColor;

#define MAXCOLORS       7   /* Predefined colors */
static char *col_name[] = {
  "red", "green", "blue", "yellow", "cyan", "magenta2", "coral"
};

typedef enum {Red, Green, Blue, Yellow, Cyan, Magenta, Coral} BCOLS;

static int colorWarningMsg = 0;
static char colorMsgString[512];
static char *colorMsgFormat =
"         Could only allocate\n\
       %d base colours out of %d,\n\
      %d grey shades out of %d, and\n\
      %d \"false\" colours out of %d.\n\n\
Start '" PRGNAME " -p' to use a private colormap or\n\
close other applications before starting " PKGNAME ".";

#define MAXBASE         9
static float rainbowR[] = { 1.0, 1.0, 1.0, 1.0, 0.6, 0.0, 0.0, 0.0, 0.0};
static float rainbowG[] = { 1.0, 0.0, 0.6, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0};
static float rainbowB[] = { 1.0, 0.0, 0.0, 0.0, 0.3, 1.0, 0.8, 0.3, 0.0};

COLOR *GetColorInfo()
{
    return &gColor;
}

void ColorAllocationWarning()
{
    if (!colorWarningMsg) return;
    
    PostWarningDialog(NULL, colorMsgString);
}

unsigned long GetMyColor(char *s)
{
    int n;
    
    for (n=0; n<gColor.ncols; n++) {
        if (strcmp(s, col_name[n])==0) {
            return gColor.cols[n];
        }
    }
    
    return gColor.black;
}

static void AllocReadOnlyColors(Display *dpy, Colormap cmap)
{
    int n, nerr=0, m, mmax, ncols=0, ngreys=0, nfalse=0;
    double f;
    XColor color;
    unsigned long pixel;
    string grey_name;

    if (gp->privateColors) {
        if (XParseColor(dpy, cmap, "black", &color)) {
            if (XAllocColor(dpy, cmap, &color)) {
                gColor.black = color.pixel;
            }
        }
        if (XParseColor(dpy, cmap, "white", &color)) {
            if (XAllocColor(dpy, cmap, &color)) {
                gColor.white = color.pixel;
            }
        }
    }
    
    /* Try allocate all gColor.ncols col_name[] colours */
    for (n=0; n<gColor.ncols; n++) {
        if (!XParseColor(dpy, cmap, col_name[n], &color)) {
            fprintf(stderr,  "%s: Color %s not in database.\n",
                    PKGNAME, col_name[n]);
            nerr++;
            pixel = gColor.black;
        } else {
            if (!XAllocColor(dpy, cmap, &color)) {
                /* fprintf(stderr, "%s: Couldn't allocate %s.\n",
                        PKGNAME, col_name[n]); */
                nerr++;
                pixel = gColor.black;
            } else {
                pixel = color.pixel;
                ncols++;
            } 
        }
        gColor.cols[n] = pixel;
        gColor.c_r[n] = color.red;
        gColor.c_g[n] = color.green;
        gColor.c_b[n] = color.blue;
    }
    
    /* Try allocate all gColor.ngreys grey scales, if there are more than 100
       calculate the pixel values, otherwise try the "greyXXX" names */
    for (n=0; n<gColor.ngreys; n++) {
        if (gColor.ngreys > 100) {
            color.red = (int)(65535.*(double)n/(double)(gColor.ngreys-1));
            color.green = color.red;
            color.blue  = color.red;
            color.flags = DoRed | DoGreen | DoBlue;
        } else {
            sprintf(grey_name, "grey%d", (100*n)/(gColor.ngreys-1));
            if (!XParseColor(dpy, cmap, grey_name, &color)) {
                fprintf(stderr, "%s: Color %s not in database.\n",
                        PKGNAME, col_name[n]);
                nerr++;
                continue;
            }
        }
        if (!XAllocColor(dpy, cmap, &color)) {
            /* fprintf(stderr, "%s: Couldn't allocate grey %d.\n",
                       PKGNAME, n); */
            nerr++;
        } else {
            gColor.greys[ngreys] = color.pixel;
            gColor.g_r[ngreys] = color.red;
            gColor.g_g[ngreys] = color.green;
            gColor.g_b[ngreys] = color.blue;
            ngreys++;
        }
    }
    
    /* Try allocate the false color ramp */
    for (n=0; n<MAXBASE-1; n++) {
        mmax = false_magn - 1;
        if (n == MAXBASE-2) mmax++;
        for (m=0; m<=mmax; m++) {
            f = (double)m/(double)false_magn;
            color.red   = (int)((rainbowR[n] +
                                 f*(rainbowR[n+1]-rainbowR[n]))*65535.0);
            color.green = (int)((rainbowG[n] +
                                 f*(rainbowG[n+1]-rainbowG[n]))*65535.0);
            color.blue  = (int)((rainbowB[n] +
                                 f*(rainbowB[n+1]-rainbowB[n]))*65535.0);
            color.flags = DoRed | DoGreen | DoBlue;
            if (!XAllocColor(dpy, cmap, &color)) {
                /* fprintf(stderr, "%s: Couldn't allocate false color %d.\n",
                        PKGNAME, n*false_magn + m + 1); */
                nerr++;
            } else {
                gColor.false[nfalse] = color.pixel;
                gColor.f_r[nfalse] = color.red;
                gColor.f_g[nfalse] = color.green;
                gColor.f_b[nfalse] = color.blue;
                nfalse++;
            }
        }
    }
    
    /* One or more colours couldn't be allocated, make a note of it and
       report it with a warning box, at a later stage when all initial
       X code has been passed. */
    if (nerr) {
        colorWarningMsg = 1;
        sprintf(colorMsgString, colorMsgFormat,
                ncols, gColor.ncols,
                ngreys, gColor.ngreys,
                nfalse, gColor.nfalse);
    }

    /* Set the new values of the number of colors */
    gColor.ncols = ncols;
    gColor.ngreys = ngreys;
    gColor.nfalse = nfalse;
}

void CreateColors(Display *dpy, int scr)
{
    int depth;
    XVisualInfo xv_info;
    
    gColor.swapped = 0;
    
    depth = DefaultDepth(dpy, scr);
    
    gColor.ncols  = MAXCOLORS;
    if (XMatchVisualInfo(dpy, scr, depth, TrueColor, &xv_info)) {
        gColor.ngreys = 2*MAXSHADES;
        false_magn    = 2*FALSE_MAGN;
        gColor.nfalse = (MAXBASE-1)*false_magn + 1;
    } else {
        gColor.ngreys = MAXSHADES;
        false_magn    = FALSE_MAGN;
        gColor.nfalse = (MAXBASE-1)*false_magn + 1;
    }

    gColor.cols  = (unsigned long *)XtMalloc(gColor.ncols*sizeof(unsigned long));
    gColor.greys = (unsigned long *)XtMalloc(gColor.ngreys*sizeof(unsigned long));
    gColor.false = (unsigned long *)XtMalloc(gColor.nfalse*sizeof(unsigned long));
    
    gColor.c_r = (unsigned short *)XtMalloc(gColor.ncols*sizeof(unsigned short));
    gColor.c_g = (unsigned short *)XtMalloc(gColor.ncols*sizeof(unsigned short));
    gColor.c_b = (unsigned short *)XtMalloc(gColor.ncols*sizeof(unsigned short));
    gColor.g_r = (unsigned short *)XtMalloc(gColor.ngreys*sizeof(unsigned short));
    gColor.g_g = (unsigned short *)XtMalloc(gColor.ngreys*sizeof(unsigned short));
    gColor.g_b = (unsigned short *)XtMalloc(gColor.ngreys*sizeof(unsigned short));
    gColor.f_r = (unsigned short *)XtMalloc(gColor.nfalse*sizeof(unsigned short));
    gColor.f_g = (unsigned short *)XtMalloc(gColor.nfalse*sizeof(unsigned short));
    gColor.f_b = (unsigned short *)XtMalloc(gColor.nfalse*sizeof(unsigned short));

    gColor.white = WhitePixel(dpy, scr);
    gColor.black = BlackPixel(dpy, scr);
    
    gColor.cmap = XDefaultColormap(dpy, scr);
    if (gp->privateColors)
        gColor.cmap = XCopyColormapAndFree(dpy, gColor.cmap);

    AllocReadOnlyColors(dpy, gColor.cmap);
    
    if (gp->privateColors)
        XtVaSetValues(gp->top, XmNcolormap, gColor.cmap, NULL);
}

void CreateGCs(Display *dpy, int scr)
{
    int n;
    XGCValues gcv;

    GC GetGC(unsigned long, XGCValues *);
    
    gp->gcLine = XDefaultGC(dpy, scr);

    if (gColor.ncols > Blue) 
        gcv.foreground = gColor.cols[Blue];
    else
        gcv.foreground = gColor.black;
    
    for (n=0; n<5; n++)
        gp->gcFrame[n] = GetGC(GCForeground, &gcv);

    gcv.foreground = gColor.black;
    gp->gcGrey = GetGC(GCForeground, &gcv);

    gcv.line_width = 2;
    gcv.line_style = LineSolid;
    if (gColor.ncols > Blue) 
        gcv.foreground = gColor.cols[Blue];
    else
        gcv.foreground = gColor.black;
    gp->gcFrame[5] = GetGC(GCLineWidth | GCLineStyle | GCForeground, &gcv);

    gcv.foreground = gColor.white ^ gColor.black;
    gcv.line_style = LineOnOffDash;
    gcv.function   = GXxor;
    gp->gcErase = GetGC(GCForeground | GCBackground | GCFunction | GCLineStyle,
                        &gcv);

    if (gColor.ncols > Red) 
        gcv.foreground = gColor.cols[Red];
    else
        gcv.foreground = gColor.black;
    gp->gcGauss = GetGC(GCForeground, &gcv);

    if (gColor.ncols > Magenta) 
        gcv.foreground = gColor.cols[Magenta];
    else
        gcv.foreground = gColor.black;
    gp->gcSec = GetGC(GCForeground, &gcv);

    if (gColor.ncols > Coral) 
        gcv.foreground = gColor.cols[Coral];
    else
        gcv.foreground = gColor.black;
    gp->gcGaussI = GetGC(GCForeground, &gcv);
    gp->gcRms = gp->gcGaussI;

    if (gColor.ncols > Cyan) 
        gcv.foreground = gColor.cols[Cyan];
    else
        gcv.foreground = gColor.black;
    gcv.line_width = 2;
    gcv.line_style = LineSolid;
    gp->gcPoly = GetGC(GCForeground | GCLineStyle | GCLineWidth, &gcv);

    if (gColor.ncols > Cyan) 
        gcv.foreground = gColor.cols[Cyan];
    else
        gcv.foreground = gColor.black;
    gcv.line_width = 1;
    gcv.line_style = LineSolid;
    gp->gcTag = GetGC(GCForeground | GCLineStyle | GCLineWidth, &gcv);

    if (gColor.ncols > Green) 
        gcv.foreground = gColor.cols[Green];
    else
        gcv.foreground = gColor.black;
    gp->gcBox = GetGC(GCForeground, &gcv);

    if (gColor.ncols > Red) 
        gcv.foreground = gColor.cols[Red];
    else
        gcv.foreground = gColor.black;
    gcv.line_width = 2;
    gcv.line_style = LineOnOffDash;
    gp->gcMom = GetGC(GCForeground | GCLineStyle | GCLineWidth, &gcv);

    if (gColor.ncols > Blue) 
        gcv.foreground = gColor.cols[Blue];
    else
        gcv.foreground = gColor.black;
    gp->gcStd = GetGC(GCForeground, &gcv);

    gcv.foreground = gColor.white;
    gp->gcClear = GetGC(GCForeground, &gcv);
}

void SwapForegroundAndBackground()
{
    int n;
    Display *dpy = XtDisplay(gp->graph);
    unsigned long old_fg = gColor.black, old_bg = gColor.white;
    unsigned long new_fg = old_bg, new_bg = old_fg;
    
    void draw_main();
    
    gColor.black = old_bg;
    gColor.white = old_fg;
    
    if (gColor.swapped)
        gColor.swapped = 0;
    else
        gColor.swapped = 1;
    
    XSetForeground(dpy, gp->gcLine, new_fg);
    XSetForeground(dpy, gp->gcClear, new_bg);
    XSetForeground(dpy, gp->gcGrey, new_fg);
    if (gColor.ncols <= 0) {
        XSetForeground(dpy, gp->gcMom, new_fg);
        XSetForeground(dpy, gp->gcGauss, new_fg);
    }
    if (gColor.ncols <= 1) {
        XSetForeground(dpy, gp->gcGaussI, new_fg);
        XSetForeground(dpy, gp->gcRms, new_fg);
    }
    if (gColor.ncols <= 2) {
        XSetForeground(dpy, gp->gcStd, new_fg);
        for (n=0; n<6; n++)
            XSetForeground(dpy, gp->gcFrame[n], new_fg);
    }
    if (gColor.ncols <= 3) {
        XSetForeground(dpy, gp->gcPoly, new_fg);
    }
    if (gColor.ncols <= 4) {
        XSetForeground(dpy, gp->gcSec, new_fg);
    }
    if (gColor.ncols <= 5) {
        XSetForeground(dpy, gp->gcBox, new_fg);
    }
    XtVaSetValues(gp->graph, XmNbackground, new_bg, NULL);
    
    draw_main();
}
